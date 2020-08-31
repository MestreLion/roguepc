TARGET:=splash
CFLAGS:=-Wall -Wextra -std=c11 -pedantic
LDLIBS:=

OBJS=splash.o

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean
