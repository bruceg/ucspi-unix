PACKAGE = ucspi-unix
VERSION = 0.35

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

PROGS = unixserver unixclient
SOURCES = unixserver.c unixclient.c env.c utoa.c
SCRIPTS = unixcat

all: $(PROGS)

unixclient: unixclient.o env.o utoa.o
	$(LD) $(LDFLAGS) -o $@ unixclient.o env.o utoa.o $(LIBS)

unixserver: unixserver.o env.o utoa.o
	$(LD) $(LDFLAGS) -o $@ unixserver.o env.o utoa.o $(LIBS)

unixserver.o: unixserver.c
unixclient.o: unixclient.c
env.o: env.c
utoa.o: utoa.c

install:
	$(install) -d $(bindir)
	$(install) -m 755 $(PROGS) $(SCRIPTS) $(bindir)

clean:
	$(RM) *.o $(PROGS)
