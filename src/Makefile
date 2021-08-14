#@@ the following note was preserved from original, everything afterwards is new
#
# Make file for building Rogue
# Michael Toy, AI Design, May 1984
# :bk=0
#
# NOTE: If new files are added or old files removed from these
#	object lists, you must modify the file objs as well.
#
#	To disbale stack overflow checking (thus getting a smidge more
#	speed.  Don't load in csav.o
#
TARGET:=rogue
CFLAGS:=-Wall -Wextra -std=c11 -pedantic
LDLIBS:=

# Defines from original Rogue code turned on by default here:
CFLAGS+=-DMINROG

# New defines: ROGUE_*
ifdef ROGUE_RELEASE
CFLAGS+=-s -O2
else
# -Og is enough to enable _FORTIFY_SOURCE=2
# To test it: gcc -E -Og -dM -x c /dev/null | grep FORTIFY
CFLAGS+=-Og -g3
endif
ifdef ROGUE_DEBUG
CFLAGS+=-DROGUE_DEBUG
endif
ifdef ROGUE_DEMO
CFLAGS+=-DROGUE_DEMO
endif
UNICODE=3
# Valid values for ROGUE_CHARSET:
# 1 - ASCII, requires ncurses. Looks like Rogue UNIX, but with color
# 2 - CP437, requires ncurses, configured terminal and CP437 font. Good luck!
# 3 - Unicode (as defined above), requires ncursesw and Unicode font. The default.
# See curses_common.h
ROGUE_CHARSET:=$(UNICODE)
# Debian and others can use 'ncurses{,w}{5,6}-config' instead of 'pkg-config ncurses{,w}'
ifeq ($(ROGUE_CHARSET),$(UNICODE))
CFLAGS+=$(shell pkg-config ncursesw --cflags)
LDLIBS+=$(shell pkg-config ncursesw --libs)
else
CFLAGS+=-DROGUE_CHARSET=$(ROGUE_CHARSET)
CFLAGS+=$(shell pkg-config ncurses --cflags)
LDLIBS+=$(shell pkg-config ncurses --libs)
endif

export CFLAGS

OBJS=armor.o new_leve.o command.o mach_dep.o rip.o save.o \
	chase.o curses.o daemon.o daemons.o env.o extern.o fakedos.o fight.o \
	init.o io.o list.o maze.o misc.o monsters.o move.o pack.o passages.o \
	potions.o rings.o rooms.o scrolls.o sticks.o strings.o things.o \
	weapons.o wizard.o slime.o load.o protect.o

ASMS=zoom.o dos.o begin.o fio.o sbrk.o csav.o

default: $(TARGET)

all: clean default sdl

sdl: splash $(TARGET)-sdl

splash: splash/splash

splash/splash:
	$(MAKE) -C splash splash

splash/load_sdl.o:
	$(MAKE) -C splash load_sdl.o

main-sdl.o: CFLAGS+=-DROGUE_SPLASH
main-sdl.o:
	$(CC) $(CFLAGS) -c -o $@ main.c

$(TARGET)-sdl: LDLIBS+=$(shell pkg-config sdl2 --libs)
$(TARGET)-sdl: OBJS+=main-sdl.o splash/load_sdl.o
$(TARGET)-sdl: $(OBJS) main-sdl.o splash/load_sdl.o
	$(CC) $(OBJS) $(LDLIBS) -o $@

$(TARGET): OBJS+=main.o
$(TARGET): $(OBJS) main.o
	$(CC) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) $(TARGET)-sdl *.o
	$(MAKE) -C splash $@

.PHONY: default all sdl splash clean
