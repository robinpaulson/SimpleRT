TARGET = simple-rt
LDFLAGS = -lm -lpthread
CC = gcc
CFLAGS = -g -Wall -Iinclude

CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

.PHONY: default all clean

default: $(TARGET)
all: default

PLATFORM := none
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
PLATFORM = linux
endif
ifeq ($(UNAME),Darwin)
PLATFORM = osx
endif
ifeq ($(PLATFORM),none)
$(error unknown platform $(UNAME))
endif

OBJ = obj
SOURCES = src
PLATFORM_SRC = $(SOURCES)/$(PLATFORM)
OBJECTS += $(patsubst %.c, $(OBJ)/%.o, $(wildcard $(SOURCES)/*.c $(PLATFORM_SRC)/*.c))
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
