##########################################################
#
# $Id:$
#
# $Log:$
#
#

CC=emcc
CFLAGS=-O -DNORMALUNIX -DLINUX $(shell sdl2-config --cflags) -s USE_SDL_MIXER=2
LDFLAGS=
LIBS=-lm $(shell sdl2-config --libs) -lSDL2_mixer

O=linux

all:	 $(O)/sndserver

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*

# Target
$(O)/sndserver: \
	$(O)/soundsrv.o \
	$(O)/sounds.o \
	$(O)/wadread.o \
	$(O)/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) \
	$(O)/soundsrv.o \
	$(O)/sounds.o \
	$(O)/wadread.o \
	$(O)/linux.o -o $(O)/sndserver $(LIBS)
	@echo make complete.

# Rule
$(O)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


