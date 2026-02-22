.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

all: read-quickly

read-quickly: read-quickly.o
	$(CC) -o $@ $@.o $(LDFLAGS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

install: read-quickly
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin/"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- read-quickly "$(DESTDIR)$(PREFIX)/bin/"
	cp -- read-quickly.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/read-quickly"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/read-quickly.1"

clean:
	-rm -f -- *.o *.su read-quickly

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
