# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

PREFIX = /usr
BIN = /bin
DATA = /share
BINDIR = $(PREFIX)$(BIN)
DATADIR = $(PREFIX)$(DATA)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
MANDIR = $(DATADIR)/man
MAN1DIR = $(MANDIR)/man1
LICENSEDIR = $(DATADIR)/licenses

PKGNAME = rq
COMMAND = rq

#OPTIMISE = -O2
#WARN = -Wall -Wextra -pedantic
OPTIMISE = -Og -g
WARN = -Wall -Wextra -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs \
       -Wtrampolines -Wfloat-equal -Wshadow -Wmissing-prototypes -Wmissing-declarations \
       -Wredundant-decls -Wnested-externs -Winline -Wno-variadic-macros -Wsign-conversion \
       -Wswitch-default -Wconversion -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align \
       -Wstrict-overflow -Wdeclaration-after-statement -Wundef -Wbad-function-cast \
       -Wcast-qual -Wwrite-strings -Wlogical-op -Waggregate-return -Wstrict-prototypes \
       -Wold-style-definition -Wpacked -Wvector-operation-performance \
       -Wunsuffixed-float-constants -Wsuggest-attribute=const -Wsuggest-attribute=noreturn \
       -Wsuggest-attribute=pure -Wsuggest-attribute=format -Wnormalized=nfkc -pedantic
FLAGS = -std=c99 $(WARN) $(OPTIMISE)



.PHONY: default
default: base info

.PHONY: all
all: base doc

.PHONY: bas
base: cmd

.PHONY: command
cmd: bin/rq

bin/rq: obj/rq.o
	@mkdir -p bin
	${CC} ${FLAGS} -o $@ $^ ${LDFLAGS}

obj/%.o: src/%.c
	mkdir -p obj
	${CC} ${FLAGS} -c -o $@ ${CPPFLAGS} ${CFLAGS} $<

.PHONY: doc
doc: info pdf dvi ps

.PHONY: info
info: bin/rq.info
bin/%.info: doc/info/%.texinfo
	@mkdir -p bin
	$(MAKEINFO) $<
	mv $*.info $@

.PHONY: pdf
pdf: bin/rq.pdf
bin/%.pdf: doc/info/%.texinfo
	@! test -d obj/pdf || rm -rf obj/pdf
	@mkdir -p bin obj/pdf
	cd obj/pdf && texi2pdf ../../"$<" < /dev/null
	mv obj/pdf/$*.pdf $@

.PHONY: dvi
dvi: bin/rq.dvi
bin/%.dvi: doc/info/%.texinfo
	@! test -d obj/dvi || rm -rf obj/dvi
	@mkdir -p bin obj/dvi
	cd obj/dvi && $(TEXI2DVI) ../../"$<" < /dev/null
	mv obj/dvi/$*.dvi $@

.PHONY: ps
ps: bin/rq.ps
bin/%.ps: doc/info/%.texinfo
	@! test -d obj/ps || rm -rf obj/ps
	@mkdir -p bin obj/ps
	cd obj/ps && texi2pdf --ps ../../"$<" < /dev/null
	mv obj/ps/$*.ps $@



.PHONY: install
install: install-base install-info install-man

.PHONY: install-all
install-all: install-base install-doc

.PHONY: install-base
install-base: install-cmd install-copyright

.PHONY: install-cmd
install-cmd: bin/rq
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"

.PHONY: install-copyright
install-copyright: install-license

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 LICENSE -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"

.PHONY: install-doc
install-doc: install-info install-pdf install-dvi install-ps install-man

.PHONY: install-info
install-info: bin/rq.info
	install -dm755 -- "$(DESTDIR)$(INFODIR)"
	install -m644 $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: bin/rq.pdf
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- "$<" "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-dvi
install-dvi: bin/rq.dvi
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- "$<" "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"

.PHONY: install-ps
install-ps: bin/rq.ps
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- "$<" "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-man
install-man: doc/man/rq.1
	install -dm755 -- "$(DESTDIR)$(MAN1DIR)"
	install -m644 "$<" -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"



.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	-rm -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
	-rm -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"



.PHONY: clean
clean:
	-rm -r bin obj

