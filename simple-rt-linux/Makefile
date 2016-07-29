TARGET = simple-rt
LDFLAGS = -lm -lpthread
CC = gcc
CFLAGS = -g -Wall

CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
