# SimpleRT: Reverse tethering utility for Android
# Copyright (C) 2016 Konstantin Menyaev
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

TARGET = simple-rt
LDFLAGS = -lm -lpthread
CC = gcc
CFLAGS = -g -std=c99 -D_DEFAULT_SOURCE -Wall -Iinclude

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
ifeq ($(UNAME),MINGW64_NT-6.1)
PLATFORM = win
endif
ifeq ($(PLATFORM),none)
$(error unknown platform $(UNAME))
endif

CFLAGS += -DPLATFORM="\"$(PLATFORM)\""

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
