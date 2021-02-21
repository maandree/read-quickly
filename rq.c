/* See LICENSE file for copyright and license details. */
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>



/**
 * The default word rate.
 */
#ifndef DEFAULT_RATE
# define DEFAULT_RATE  120  /* 2 hz */
#endif

/**
 * Delta-value of rate increment and rate decrement.
 */
#ifndef RATE_DELTA
# define RATE_DELTA  10  /* 10/min */
#endif



/**
 * The a word.
 */
struct word {
	/**
	 * The word.
	 */
	const char *word;

	/**
	 * Should reverse video be applied?
	 */
	int reverse_video;
};

/**
 * The name of the process.
 */
static const char *argv0;

/**
 * Have the terminal been resized?
 */
static volatile sig_atomic_t caught_sigwinch = 1;

/**
 * Has the timer expired?
 */
static volatile sig_atomic_t caught_sigalrm = 0;

/**
 * The width of the terminal.
 */
static size_t width = 80;

/**
 * The height of the terminal.
 */
static size_t height = 30;

/**
 * The number of words.
 */
static size_t word_count = 0;

/**
 * All loaded words. Refer to `word_count`
 * for the number of contained words.
 */
static struct word *words;



/**
 * Signal handler for SIGWINCH.
 * Invoked when the terminal resizes.
 */
static void
sigwinch(int signo)
{
	caught_sigwinch = 1;
	(void) signo;
}


/**
 * Signal handler for SIGALRM.
 * Invoked when the timer expires.
 */
static void
sigalrm(int signo)
{
	caught_sigalrm = 1;
	(void) signo;
}


/**
 * Get the size of the terminal.
 */
static void
get_terminal_size(void)
{
	struct winsize winsize;

	if (!caught_sigwinch)
		return;

	caught_sigwinch = 0;

	while (ioctl(STDOUT_FILENO, (unsigned long)TIOCGWINSZ, &winsize) < 0)
		if (errno != EINTR)
			return;

	height = winsize.ws_row;
	width = winsize.ws_col;
}


/**
 * Get the selected word rate by reading
 * the environment variable RQ_RATE.
 * 
 * @return  The rate in words per minute.
 */
static long
get_word_rate(void)
{
	char *s, *e;
	long r;

	errno = 0;
	s = getenv("RQ_RATE");
	if (!s || !*s || !isdigit(*s))
		return DEFAULT_RATE;

	r = strtol(s, &e, 10);
	if (r <= 0)
		return DEFAULT_RATE;
	while (*e == ' ')
		e++;

	if      (!*e)                      r *= 1;
	else if (!strcasecmp(e, "wpm"))    r *= 1;
	else if (!strcasecmp(e, "w/m"))    r *= 1;
	else if (!strcasecmp(e, "/m"))     r *= 1;
	else if (!strcasecmp(e, "wpmin"))  r *= 1;
	else if (!strcasecmp(e, "w/min"))  r *= 1;
	else if (!strcasecmp(e, "/min"))   r *= 1;
	else if (!strcasecmp(e, "wps"))    r *= 60;
	else if (!strcasecmp(e, "w/s"))    r *= 60;
	else if (!strcasecmp(e, "/s"))     r *= 60;
	else if (!strcasecmp(e, "wpsec"))  r *= 60;
	else if (!strcasecmp(e, "w/sec"))  r *= 60;
	else if (!strcasecmp(e, "/sec"))   r *= 60;
	else if (!strcasecmp(e, "hz"))     r *= 60;
	else
		return DEFAULT_RATE;

	return r;
}


/**
 * Count the number of character in a string.
 * 
 * Possible improvement:
 *   Figure out how many columns the terminal is
 *   likely to used to display the each character,
 *   and sum it.
 * 
 * @param   s  The string.
 * @return     The number of characters in `s`.
 */
static size_t
display_len(const char *s)
{
	size_t r = 0;
	for (; *s; s++)
		r += (((int)*s & 0xC0) != 0x80);
	return r;
}


/**
 * Load the file and do some preparsing.
 * 
 * @param   fd  The file descriptor to the file, -1 to clean up instead.
 * @return      0 on success, -1 on error.
 */
static int
load_file(int fd)
{
	static char *buffer = NULL;
	size_t ptr = 0;
	size_t size = 0;
	void *new;
	int saved_errno;
	char *s;
	char *end;
	size_t i;
	ssize_t n;

	if (fd == -1)
		return free(buffer), 0;

	/* Load file. */
	for (;;) {
		if (ptr == size) {
			size = size ? (size << 1) : (8 << 10);
			new = realloc(buffer, size);
			if (!new)
				goto fail;
			buffer = new;
		}
		n = read(fd, buffer + ptr, size - ptr);
		if (n < 0) {
			if (errno != EINTR)
				goto fail;
			continue;
		} else if (n == 0) {
			break;
		}
		ptr += (size_t)n;
	}
	if (buffer == NULL)
		return 0;
	if (ptr == size) {
		new = realloc(buffer, size += 2);
		if (!new)
			goto fail;
		buffer = new;
	}
	buffer[ptr++] = '\0';
	buffer[ptr++] = '\0';

	/* Split words. */
	size = 0;
	for (s = buffer; *s; s = end + 1) {
		if (word_count == size) {
			size = size ? (size << 1) : 512;
			new = realloc(words, size * sizeof(*words));
			if (!new)
				goto fail;
			words = new;
		}
		while (isspace(*s))
			s++;
		end = strpbrk(s, " \f\n\r\t\v");
		if (end == NULL)
			end = strchr(s, '\0');
		*end = '\0';
		words[word_count].word = s;
		words[word_count].reverse_video = 0;
		word_count++;
	}

	/* Figure out which words should have reverse video. */
	for (i = 1; i < word_count; i++)
		if (!strcmp(words[i].word, words[i - 1].word))
			words[i].reverse_video = words[i - 1].reverse_video ^ 1;

	return 0;

fail:
	saved_errno = errno;
	free(buffer), buffer = NULL;
	errno = saved_errno;
	return -1;
}


/**
 * Display a file word by word.
 * 
 * @param   ttyfd  File descriptor for reading from the terminal.
 * @param   rate   The number of words per minute to display.
 * @return         0 on success, -1 on error.
 */
static int
display_file(int ttyfd, long rate)
{
#define SET_RATE \
	(interval.it_value.tv_usec = 60000000L / rate, \
	 interval.it_value.tv_sec = interval.it_value.tv_usec / 1000000L, \
	 interval.it_value.tv_usec %= 1000000L)

	ssize_t n;
	int timer_set = 1;
	char c;
	size_t i;
	struct itimerval interval;

	memset(&interval, 0, sizeof(interval));

	SET_RATE;
	for (i = 0; i < word_count; i++) {
		if (setitimer(ITIMER_REAL, &interval, NULL))
			goto fail;
	rewait:
		n = read(ttyfd, &c, sizeof(c));
		if (n < 0) {
			if (errno != EINTR)
				goto fail;
			c = 0;
		} else if (n == 0) {
			break;
		}
		switch (c) {
		case '+': /* plus */
		case '-': /* hyphen */
			rate += c == '+' ? RATE_DELTA : -RATE_DELTA;
			rate = rate <= 0 ? 1 : rate;
			SET_RATE;
			goto rewait;
		case 'p': /* P */
			if (timer_set)
				memset(&interval, 0, sizeof(interval));
			else
				SET_RATE;
			if (setitimer(ITIMER_REAL, &interval, NULL))
				goto fail;
			timer_set ^= 1;
			goto rewait;
		case 'q': /* Q */
			goto done;
		case 'B': /* down */
		case 'C': /* right */
			break;
		case 'A': /* up */
		case 'D': /* left */
			i = i < 2 ? 0 : i - 2;
			break;
		case 0:
			if (!caught_sigalrm)
				goto rewait;
			caught_sigalrm = 0;
			break;
		default:
			goto rewait;
		}

		get_terminal_size();
		if (fprintf(stdout, "\033[H\033[2J\033[%zu;%zuH%s%s%s",
		            (height + 1) / 2,
		            (width - display_len(words[i].word)) / 2 + 1,
		            words[i].reverse_video ? "\033[7m" : "",
		            words[i].word,
		            words[i].reverse_video ? "\033[27m" : "") < 0)
			goto fail;
		if (fflush(stdout))
			goto fail;
	}

	if (setitimer(ITIMER_REAL, &interval, NULL))
		goto fail;
	(void) read(ttyfd, &c, sizeof(c));

done:
	return 0;

fail:
	return -1;
}


int
main(int argc, char *argv[])
{
	long rate = get_word_rate();
	char *file = NULL;
	int fd = -1, ttyfd = -1, tty_configured = 0;
	struct termios stty, saved_stty;
	struct stat _attr;
	struct sigaction sa;

	/* Parse arguments. */
	argv0 = argv ? (argc--, *argv++) : "rq";
	if (argc && argv[0][0] == '-') {
		if (argv[0][1] == '-' && !argv[0][2]) {
			argc--;
			argv++;
		} else if (argv[0][1]) {
			goto usage;
		}
	}
	if (argc > 1)
		goto usage;

	/* Check that we have a stdout. */
	if (fstat(STDOUT_FILENO, &_attr))
		if (errno == EBADF)
			goto fail;

	/* Open file. */
	if (!file || !strcmp(file, "-")) {
		fd = STDIN_FILENO;
	} else {
		fd = open(file, O_RDONLY);
		if (fd < 0)
			goto fail;
	}

	/* Load file. */
	if (load_file(fd))
		goto fail;

	/* We do not need the file anymore. */
	close(fd);
	fd = -1;

	/* Get a readable file descriptor for the controlling terminal. */
	ttyfd = open("/dev/tty", O_RDONLY);
	if (ttyfd < 0)
		goto fail;

	/* Configure terminal. */
	if (fprintf(stdout, "\033[?1049h\033[?25l") < 0)
		goto fail;
	if (fflush(stdout))
		goto fail;
	if (tcgetattr(ttyfd, &stty))
		goto fail;
	saved_stty = stty;
	stty.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);
	if (tcsetattr(ttyfd, TCSAFLUSH, &stty))
		goto fail;
	tty_configured = 1;

	/* Display file. */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigalrm;
	sigaction(SIGALRM, &sa, NULL);
	sa.sa_handler = sigwinch;
	sigaction(SIGWINCH, &sa, NULL);
	if (display_file(ttyfd, rate))
		goto fail;

	/* Restore terminal configurations. */
	tcsetattr(ttyfd, TCSAFLUSH, &saved_stty);
	fprintf(stdout, "\033[?25h\033[?1049l");
	fflush(stdout);
	tty_configured = 0;

	free(words);
	load_file(-1);
	close(ttyfd);
	return 0;

fail:
	perror(argv0);
	free(words);
	load_file(-1);
	if (tty_configured) {
		tcsetattr(ttyfd, TCSAFLUSH, &saved_stty);
		fprintf(stdout, "\033[?25h\033[?1049l");
		fflush(stdout);
	}
	if (fd >= 0)
		close(fd);
	if (ttyfd >= 0)
		close(ttyfd);
	return 1;

usage:
	fprintf(stderr, "usage: %s [file].\n", argv0);
	return 1;
}
