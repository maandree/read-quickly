.PHONY:

CONFIGFILE = config.mk
include $(CONFIGFILE)

all: read-quickly

read-quickly: read-quickly.o
	$(CC) -o $@ $@.o $(LDFLAGS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

install: read-quickly
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1"
	cp -- read-quickly "$(DESTDIR)$(PREFIX)/bin/"
	cp -- read-quickly.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/read-quickly"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/read-quickly.1"

clean:
	-rm -rf -- read-quickly *.o

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
