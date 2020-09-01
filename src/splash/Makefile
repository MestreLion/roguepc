TARGET:=splash
CFLAGS:=-Wall -Wextra -std=c11 -pedantic -Og -g
LDLIBS:=

OBJS=splash.o

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean
