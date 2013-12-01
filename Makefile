CFLAGS += -std=c11 -msse4.2 -Isrc -include src/hfsinspect-Prefix.pch
CFLAGS += -g -O0 -Wall

SOURCEDIRS := src src/crc32c src/hfs src/hfs/Apple src/hfs/btree src/hfs/hfsplus src/logging src/misc src/volumes
SOURCES := $(foreach dir,$(SOURCEDIRS),$(wildcard $(dir)/*.c))
OBJECTS = $(SOURCES:.c=.o)

UNAME := $(shell uname)
include Makefile.$(UNAME)

.PHONY: all
all: hfsinspect

hfsinspect: $(OBJECTS)
	$(LINK.c) -o "$(BINARY)" $^ $(LIBS)

.PHONY: clean
clean: $(BINDIR)
	$(RM) "$(BINARY)" $(OBJECTS)

.PHONY: distclean
distclean: clean
	$(RM) "images/test.img" "images/MBR.dmg"
	
.PHONY: test
test: hfsinspect
	./$(BINARY) --help
	gunzip -dq images/test.img.gz
	./$(BINARY) -d images/test.img -v
	