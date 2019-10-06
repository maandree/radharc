.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	cg-base.o\
	radharc.o

HDR =\
	arg.h\
	cg-base.h

all: radharc
$(OBJ): $(@:.o=.c) $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

radharc: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	-rm -f -- radharc *.o

.SUFFIXES:
.SUFFIXES: .c .o

.PHONY: all check install uninstall clean
