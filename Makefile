.PHONY:

CONFIGFILE = config.mk
include $(CONFIGFILE)

all: rq

rq: rq.o
	$(CC) -o $@ $@.o $(LDFLAGS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

install: rq
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1"
	cp -- rq "$(DESTDIR)$(PREFIX)/bin/$(COMMAND)"
	cp -- rq.1 "$(DESTDIR)$(MANPREFIX)/man1/$(COMMAND).1"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/$(COMMAND)"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/$(COMMAND).1"

clean:
	-rm -rf -- rq *.o

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
