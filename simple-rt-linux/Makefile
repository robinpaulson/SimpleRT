TARGET = simple-rt
LDFLAGS = -lm -lpthread
CC = gcc
CFLAGS = -g -Wall -Iinclude

CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

.PHONY: default all clean

default: $(TARGET)
all: default

OBJ = obj
SOURCES = src
PLATFORM = $(SOURCES)/osx
OBJECTS += $(patsubst %.c, $(OBJ)/%.o, $(wildcard $(SOURCES)/*.c $(PLATFORM)/*.c))
HEADERS = $(wildcard include/*.h)

$(OBJ)/%.o: %.c $(HEADERS)
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	-rm -rf $(OBJ)
	-rm -f $(TARGET)
