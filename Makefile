#!/usr/bin/make -f
CFLAGS = -std=c11 -msse4.2 -Wall -I. -Isrc -include src/hfsinspect-Prefix.pch
CFLAGS += -g -O0

SOURCEDIRS := src src/crc32c src/hfs src/hfs/Apple src/hfs/btree src/hfs/hfsplus src/logging src/misc src/volumes
SOURCES := $(foreach dir,$(SOURCEDIRS),$(wildcard $(dir)/*.c))
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all
all: hfsinspect

hfsinspect: $(OBJECTS)
	$(LINK.c) -o $@ $^

.PHONY: clean
clean:
	$(RM) hfsinspect $(OBJECTS)
	