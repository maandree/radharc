.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	cg-base.o\
	radharc.o

HDR =\
	cg-base.h

all: radharc
$(OBJ): $(@:.o=.c) $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

radharc: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

install: radharc
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	cp radharc -- "$(DESTDIR)$(PREFIX)/bin"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/radharc"

clean:
	-rm -f -- radharc *.o

.SUFFIXES:
.SUFFIXES: .c .o

.PHONY: all check install uninstall clean
