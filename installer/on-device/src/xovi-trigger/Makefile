CC      ?= cc
CFLAGS  ?= -std=c11 -D_POSIX_C_SOURCE=200112L -Os -Wall -Wextra -Wpedantic -fno-exceptions -fno-stack-protector
LDFLAGS ?= -static -s

TARGET = xovi-trigger
OBJS   = gesture.o main.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

gesture.o: gesture.c gesture.h
	$(CC) $(CFLAGS) -c -o $@ gesture.c

main.o: main.c gesture.h
	$(CC) $(CFLAGS) -c -o $@ main.c

clean:
	rm -f $(OBJS) $(TARGET)
