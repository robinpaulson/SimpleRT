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

prefix = /usr/local
exec_prefix = $(prefix)
sbindir = $(exec_prefix)/sbin
libexecdir = $(exec_prefix)/libexec

CC ?= gcc
LDFLAGS = -lm -lpthread -lresolv
CFLAGS = -g -std=c11 -D_DEFAULT_SOURCE -DIFACE_UP_SH_PATH=$(libexecdir)/iface_up.sh -Wall -pedantic -Iinclude

CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

.PHONY: default all clean install uninstall

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

CFLAGS += -DPLATFORM="\"$(PLATFORM)\""

OBJ = obj
SOURCES = src
UNIX_SRC = src/unix_common
PLATFORM_SRC = $(SOURCES)/$(PLATFORM)
OBJECTS = $(patsubst %.c, $(OBJ)/%.o, $(wildcard $(SOURCES)/*.c $(UNIX_SRC)/*.c $(PLATFORM_SRC)/*.c))
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

install: simple-rt
	mkdir -p $(DESTDIR)$(sbindir)
	mkdir -p $(DESTDIR)$(libexecdir)/simple-rt
	cp $< $(DESTDIR)$(sbindir)
	cp iface_up.sh $(DESTDIR)$(libexecdir)/simple-rt/iface_up.sh
	echo "SimpleRT installed, make sure $(DESTDIR)$(sbindir)/ is in your PATH-variable."

uninstall:
	rm -rf $(DESTDIR)$(libexecdir)/simple-rt
	rm -f $(DESTDIR)$(sbindir)/simple-rt
