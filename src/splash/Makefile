TARGET:=splash
CFLAGS:=-Wall -Wextra -std=c11 -pedantic -Og -g
LDLIBS:=

CFLAGS+=$(shell pkg-config sdl2 --cflags)
LDLIBS+=$(shell pkg-config sdl2 --libs)

OBJS=splash.o

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean
