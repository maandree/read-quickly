PREFIX    = /usr
MANPREFIX = $(PREFIX)/share/man

CPPFLAGS  = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700
CFLAGS    = -std=c99 -O2 -Wall -pedantic
LDFLAGS   = -s

COMMAND   = rq
