# Lovingly crafted by hand.

# Some defaults.  Feel free to override them.
CFLAGS += -g -O0 -Wall
PREFIX = /usr/local

# ------------ Systems and Platforms ------------

# System-specific CFLAGS
sys_CFLAGS =

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

# Linux needs some love.
ifeq ($(OS), Linux)
sys_CFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_ISOC11_SOURCE
LIBS += -lm $(shell pkg-config --libs libbsd-overlay uuid)
endif

# Our GCC options.
ifeq ($(CC), gcc)
sys_CFLAGS += -fstack-protector -Wno-multichar -Wno-unknown-pragmas
endif

# Our clang options.
ifeq ($(CC), clang)
sys_CFLAGS += -fstack-protector-all -fstrict-enums -ftrapv
# Clang 3.5.1
# sys_CFLAGS += -fsanitize=undefined -fsanitize=address -fsanitize=local-bounds -fsanitize-memory-track-origins
# Warnings
sys_CFLAGS += -Wpedantic -Wno-four-char-constants
endif

# General options
ifeq ($(USEGC), 1)
sys_CFLAGS += -DGC_ENABLED
LIBS += -lgc
endif

# ------------ hfsinspect ------------

# Required CFLAGS
bin_CFLAGS = -std=c1x -msse4.2 -include src/cdefs.h -Isrc -Isrc/vendor -Isrc/vendor/crc-32c

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
ALLFILES := $(SRCFILES) $(HDRFILES) $(AUXFILES)

INSTALL = install
RM = rm -f


# ------------ Definitions ------------

# The superset we're going to use.
ALL_CFLAGS = $(CFLAGS) $(bin_CFLAGS) $(sys_CFLAGS)
ALL_LDFLAGS = $(LDFLAGS) 

# ------------ Actions ------------

.PHONY: all everything clean distclean pretty depend docs install uninstall test clean-hfsinspect clean-test clean-docs

all: depend $(BINARYPATH)
#	 @echo "Compiled and linked with: $(CC)/$(ALL_CFLAGS)/$(ALL_LDFLAGS)/$(LIBS)/"
	
everything: all docs

clean: clean-hfsinspect

distclean: clean-hfsinspect clean-test clean-docs
	@echo "Removing all build artifacts."
	@$(RM) -r build

pretty:
	find src -iname '*.[hc]' -and \! -path '*vendor*' -and \! -path '*Apple*' | uncrustify -c uncrustify.cfg -F- --replace --no-backup --mtime -lC

depend: .depend

.depend: $(SRCFILES)
	@echo "Building dependancy graph"
	@rm -f ./.depend
	@$(CC) $(ALL_CFLAGS) -MM $^>>./.depend;

-include .depend

$(OBJDIR)/%.o : src/%.c
	@echo Compiling $<
	@mkdir -p `dirname $@`
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(BINARYPATH): $(OBJFILES)
	@echo "Building hfsinspect."
	@mkdir -p `dirname $(BINARYPATH)`
	$(CC) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $(BINARYPATH) $^ $(LIBS)
	@echo "=> $(BINARYPATH)"

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
	@$(RM) -r "$(BUILDDIR)" ./.depend

clean-docs:
	@echo "Cleaning documentation."
	@$(RM) -r docs/html docs/doxygen.log

dist.tgz: $(ALLFILES)
	tar -czf dist.tgz $^
