CFLAGS += -std=c1x -Isrc -include src/hfsinspect-Prefix.pch -Wall -msse4.2
CFLAGS += -g -O0 #debug

OS := $(shell uname -s)
MACHINE := $(shell uname -m)
include Makefile.$(OS)

BINARYNAME = hfsinspect
BINARYPATH = $(BINDIR)/$(BINARYNAME)
BUILDDIR = build/$(OS)-$(MACHINE)
BINDIR = $(BUILDDIR)
OBJDIR = $(BUILDDIR)/obj

SOURCES := $(shell find src -name *.c)
OBJECTS := $(patsubst src/%, $(OBJDIR)/%, $(SOURCES:.c=.o))

INSTALL = $(shell which install)
PREFIX = /usr/local

vpath %.c src
.PHONY: all test docs clean distclean
.NOTPARALLEL:

all: $(BINARYPATH)
everything: $(BINARYPATH) docs
clean: clean-hfsinspect
distclean: clean-test clean-docs
	$(RM) -r build

$(OBJDIR)/%.o : %.c
	@echo Compiling $<
	@mkdir -p `dirname $@`
	@$(COMPILE.c) $< -o $@


test: all
	gunzip --decompress --keep --force images/test.img.gz
	./tools/tests.sh $(BINARYPATH) images/test.img

clean-test:
	$(RM) "images/test.img" "images/MBR.dmg"



$(BINARYPATH): $(OBJECTS)
	@echo "Building hfsinspect."
	@mkdir -p `dirname $(BINARYPATH)`
	@$(LINK.c) -o $(BINARYPATH) $^ $(LIBS)

clean-hfsinspect:
	$(RM) -r "$(BUILDDIR)"



install: $(BINARYPATH)
	@echo "Installing hfsinspect in $(PREFIX)"
	@mkdir -p $(PREFIX)/bin
	@$(INSTALL) $(BINARYPATH) $(PREFIX)/bin
	@echo "Installing manpage in $(PREFIX)"
	@mkdir -p $(PREFIX)/share/man/man1
	@$(INSTALL) src/hfsinspect.1 $(PREFIX)/share/man/man1

uninstall:
	$(RM) $(PREFIX)/bin/$(BINARYNAME)
	$(RM) $(PREFIX)/share/man/man1/hfsinspect.1



docs:
	@echo "Building documentation."
	@doxygen docs/doxygen.config

clean-docs:
	$(RM) -r docs/html docs/doxygen.log
