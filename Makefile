CFLAGS += -msse4.2 -Isrc -include src/hfsinspect-Prefix.pch
CFLAGS += -g -O0 -Wall #debug

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

all: hfsinspect
everything: hfsinspect docs
clean: clean-hfsinspect
distclean: clean-test clean-docs
	$(RM) -r build

$(OBJDIR)/%.o : %.c
	@echo Compiling $<
	@mkdir -p `dirname $@`
	@$(COMPILE.c) $< -o $@



test: all
	@echo "Running tests."
	@./$(BINARYPATH) --help
	@cp images/test.img.gz images/test.img.1.gz
	@gunzip -qNf images/test.img.1.gz
	@./$(BINARYPATH) -d images/test.img -v

clean-test:
	$(RM) "images/test.img" "images/MBR.dmg"



hfsinspect: $(OBJECTS)
	@echo "Building hfsinspect."
	@mkdir -p `dirname $(BINARYPATH)`
	@$(LINK.c) -o $(BINARYPATH) $^ $(LIBS)

clean-hfsinspect:
	$(RM) -r "$(BUILDDIR)"



install: hfsinspect
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
