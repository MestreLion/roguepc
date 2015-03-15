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
CFLAGS:=-Wall -Wextra -D_FORTIFY_SOURCE=2 -O0 -std=c99 -pedantic
LDLIBS:=
UNICODE=3
ROGUE_CHARSET:=$(UNICODE)

ifdef ROGUE_DEBUG
CFLAGS+=-DROGUE_DEBUG -g3
endif
ifdef ROGUE_DEMO
CFLAGS+=-DROGUE_DEMO
endif
ifeq ($(ROGUE_CHARSET),$(UNICODE))
CFLAGS+=$(shell pkg-config ncursesw --cflags-only-I)
LDLIBS+=$(shell pkg-config ncursesw --libs)
else
CFLAGS+=-DROGUE_CHARSET=$(ROGUE_CHARSET)
CFLAGS+=$(shell pkg-config ncurses --cflags-only-I)
LDLIBS+=$(shell pkg-config ncurses --libs)
endif


OBJS=armor.o new_leve.o command.o mach_dep.o main.o rip.o save.o \
	chase.o curses.o daemon.o daemons.o env.o extern.o fakedos.o fight.o \
	init.o io.o list.o maze.o misc.o monsters.o move.o pack.o passages.o \
	potions.o rings.o rooms.o scrolls.o sticks.o strings.o things.o \
	weapons.o wizard.o slime.o load.o protect.o

ASMS=zoom.o dos.o begin.o fio.o sbrk.o csav.o

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean
