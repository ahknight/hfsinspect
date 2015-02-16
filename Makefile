# Lovingly crafted by hand.

# Some defaults.  Feel free to override them.
CFLAGS += -g -O0 -Wall
PREFIX = /usr/local

# ------------ Systems and Platforms ------------

# System-specific CFLAGS
sys_CFLAGS =

# Compiler-specific options
cc_CFLAGS =

# Get the OS and machine type.
OS := $(shell uname -s)
MACHINE := $(shell uname -m)

# Pick a compiler, preferring clang, if the user hasn't expressed a preference.
ifeq ($(CC), cc)
	CLANG_PATH = $(shell which clang)
	GCC_PATH = $(shell which gcc)
	ifdef CLANG_PATH
		CC = clang
	else
		ifdef GCC_PATH
			CC = gcc
		endif
	endif
endif

CC_name := $(shell basename $(CC))

# Linux needs some love.
ifeq ($(OS), Linux)
sys_CFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_ISOC11_SOURCE
LIBS += -lm $(shell pkg-config --libs libbsd-overlay uuid)
endif

# Our GCC options.
ifeq ($(CC_name), gcc)
sys_CFLAGS += -fstack-protector
# Warnings
sys_CFLAGS += -Wall -Wextra -Wno-multichar -Wno-unknown-pragmas
cc_CFLAGS = -include $(PCHFILENAME)
endif

# Our clang options.
ifeq ($(CC_name), clang)
sys_CFLAGS += -fstack-protector-all -fstrict-enums -ftrapv
# Warnings
sys_CFLAGS += -Weverything -Wno-four-char-constants
cc_CFLAGS = -include-pch $(PCHFILE)
endif

# General options
ifeq ($(GC), 1)
sys_CFLAGS += -DGC_ENABLED
LIBS += -lgc
endif

ifneq ($(DEBUG), 1)
sys_CFLAGS += -DNDEBUG
endif

# ------------ hfsinspect ------------

PRODUCTNAME = hfsinspect
BUILDDIR = build/$(OS)-$(MACHINE)
BINARYPATH = $(BUILDDIR)/$(PRODUCTNAME)
OBJDIR = $(BUILDDIR)/obj

AUXFILES := Makefile README.md
PROJDIRS := src/hfsinspect src/hfs src/hfsplus src/logging src/vendor/memdmp src/vendor/crc-32c/crc32c src/volumes
SRCFILES := $(shell find $(PROJDIRS) -type f -name *.c)
HDRFILES := $(shell find $(PROJDIRS) -type f -name *.h)
OBJFILES := $(patsubst src/%.c, $(OBJDIR)/%.o, $(SRCFILES))
DEPFILES := $(patsubst src/%.c, $(OBJDIR)/%.d, $(SRCFILES))
PCHFILENAME := cdefs.h
PCHFILE  := $(OBJDIR)/$(PCHFILENAME).pch
ALLFILES := $(SRCFILES) $(HDRFILES) $(AUXFILES)

# Required CFLAGS
bin_CFLAGS = -std=c1x -msse4.2 -Isrc -Isrc/vendor -Isrc/vendor/crc-32c

INSTALL = install
RM = rm -f


# ------------ Definitions ------------

# The superset we're going to use.
ALL_CFLAGS = $(CFLAGS) $(bin_CFLAGS) $(sys_CFLAGS)
ALL_LDFLAGS = $(LDFLAGS) 

# ------------ Actions ------------

.PHONY: all everything clean distclean pretty docs install uninstall test clean-hfsinspect clean-test clean-docs

all: $(PRODUCTNAME)

$(PRODUCTNAME): $(PCHFILE) $(BINARYPATH)
#	 @echo "Compiled and linked with: $(CC)/$(ALL_CFLAGS)/$(ALL_LDFLAGS)/$(LIBS)/"

$(BINARYPATH): $(OBJFILES)
	@echo Building hfsinspect
	@mkdir -p `dirname $(BINARYPATH)`
	@$(CC) -o $(BINARYPATH) $^ $(ALL_CFLAGS) $(ALL_LDFLAGS) $(LIBS)
	@echo "=> $(BINARYPATH)"

$(OBJDIR)/%.o: src/%.c $(PCHFILE)
	@echo Compiling $<
	@mkdir -p `dirname $@`
	@$(CC) -o $@ -c $< $(cc_CFLAGS) $(ALL_CFLAGS)

$(OBJDIR)/%.h.pch: src/%.h
	@echo Precompiling $<
	@mkdir -p `dirname $@`
	@$(CC) -o $@ -x c-header $< $(ALL_CFLAGS)

$(OBJDIR)/%.d: src/%.c
	@echo Generating dependancies $<
	@mkdir -p `dirname $@`
	@$(CC) -M -c $< -MF $@

everything: all docs

clean: clean-hfsinspect

distclean: clean-hfsinspect clean-test clean-docs
	@echo "Removing all build artifacts."
	@$(RM) -r build

pretty:
	find src -iname '*.[hc]' -and \! -path '*vendor*' -and \! -path '*Apple*' | uncrustify -c uncrustify.cfg -F- --replace --no-backup --mtime -lC

docs:
	@echo "Building documentation."
	@doxygen docs/doxygen.config >/dev/null 2>&1

install: $(BINARYPATH)
	@echo "Installing hfsinspect in $(PREFIX)"
	@mkdir -p $(PREFIX)/bin
	@$(INSTALL) $(BINARYPATH) $(PREFIX)/bin
	@echo "Installing manpage in $(PREFIX)"
	@mkdir -p $(PREFIX)/share/man/man1
	@$(INSTALL) docs/hfsinspect.1 $(PREFIX)/share/man/man1

uninstall:
	$(RM) $(PREFIX)/bin/$(PRODUCTNAME)
	$(RM) $(PREFIX)/share/man/man1/hfsinspect.1

test: all
	gunzip < images/test.img.gz > images/test.img
	./tools/tests.sh $(BINARYPATH) images/test.img

clean-test:
	@echo "Cleaning test images."
	@$(RM) "images/test.img" "images/MBR.dmg"

clean-hfsinspect:
	@echo "Cleaning hfsinspect."
	@$(RM) -r $(BUILDDIR)

clean-docs:
	@echo "Cleaning documentation."
	@$(RM) -r docs/html docs/doxygen.log

dist.tgz: $(ALLFILES)
	tar -czf dist.tgz $^
