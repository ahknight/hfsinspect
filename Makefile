CFLAGS += -msse4.2 -Isrc -include src/hfsinspect-Prefix.pch
CFLAGS += -g -O0 -Wall #debug

UNAME := $(shell uname -s)

SOURCEDIRS := src src/crc32c src/hfs src/hfs/Apple src/hfs/btree src/hfs/hfsplus src/logging src/misc src/volumes
SOURCES := $(foreach dir,$(SOURCEDIRS),$(wildcard $(dir)/*.c))
OBJECTS = $(SOURCES:.c=.o)

include Makefile.$(UNAME)
BINPATH = bin/$(UNAME)
BINARY = hfsinspect

.PHONY: all test docs clean distclean
.NOTPARALLEL:

all: hfsinspect
	@echo "make all"

test: all
	@echo "make test"
	./"$(BINPATH)/$(BINARY)" --help
	cp images/test.img.gz images/test.img.1.gz
	gunzip -qNf images/test.img.1.gz
	./"$(BINPATH)/$(BINARY)" -d images/test.img -v

hfsinspect: $(OBJECTS)
	@echo "make hfsinspect"
	mkdir -p "$(BINPATH)"
	$(LINK.c) -o "$(BINPATH)/$(BINARY)" $^ $(LIBS)

docs:
	@echo "make docs"
	doxygen doxygen.config

clean:
	@echo "make clean"
	$(RM) "$(BINPATH)/$(BINARY)" $(OBJECTS)

distclean: clean
	@echo "make distclean"
	$(RM) "images/test.img" "images/MBR.dmg"
	$(RM) -r docs/*
