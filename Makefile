PACKAGE = ucspi-unix
VERSION = 0.10

CC = gcc
CFLAGS = -O -g -W -Wall

LD = $(CC)
LDFLAGS = -g
LIBS =

prefix = /usr
install_prefix =
bindir = $(install_prefix)$(prefix)/bin

install = /usr/bin/install

distdir = $(PACKAGE)-$(VERSION)

progs = unixserver unixclient
sources = unixserver.c unixclient.c
scripts = unixcat
docs = ANNOUNCEMENT COPYING NEWS

all: $(progs)

unixclient: unixclient.o
	$(LD) $(LDFLAGS) -o $@ unixclient.o $(LIBS)

unixserver: unixserver.o
	$(LD) $(LDFLAGS) -o $@ unixserver.o $(LIBS)

install:
	$(install) -d $(bindir)
	$(install) -m 755 $(progs) $(scripts) $(bindir)

distdir:
	rm -rf $(distdir)
	mkdir $(distdir)
	cp Makefile $(sources) $(scripts) $(docs) $(distdir)
	sed  -e "s/%VERSION%/$(VERSION)/g" $(PACKAGE).spec >$(distdir)/$(PACKAGE)-$(VERSION).spec

dist: distdir
	tar -czf $(distdir).tar.gz $(distdir)
	rm -rf $(distdir)

clean:
	$(RM) *.o $(progs)
