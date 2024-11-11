#---------------------------------------------------------------------------------------------------
# Duktape c++ wrapper testing, CLI binary and development binary build.
# Requires GNU make version >= v4.2 and basic UNIX tools (for Windows
# simply install GIT globally, so that tools like rm.exe are in the PATH;
# Linux/BSD: no action needed).
#
# For clang++ builds specify `make <target> CXX=clang++`
#
#---------------------------------------------------------------------------------------------------
# Optional tool chain prefix path, sometimes also referred to as CROSSCOMPILE
TOOLCHAIN=

CXX=$(TOOLCHAIN)g++
LD=$(CXX)
CXX_STD=c++17
FLAGSCXX=-std=$(CXX_STD) -W -Wall -Wextra -pedantic -Werror
FLAGSCXX+=-Iduktape
DUKOPTS+=-std=$(CXX_STD) -fstrict-aliasing -fdata-sections -ffunction-sections -Os
GIT_COMMIT_VERSION:=$(shell git log --pretty=format:%h -1 2>/dev/null || echo 0000000)
HOST_CXX=$(CXX)
PROGRAM_NAME=djs
BUILD_DIRECTORY=build
#---------------------------------------------------------------------------------------------------

ifdef DEBUG
 # Note: "debug" is intentinoally with full optimization. It's for symbols, not for stepping through code.
 FLAGSCXX+=-Os -g -fno-omit-frame-pointer -fdata-sections -ffunction-sections
 FLAGSLD+=-Os -g -fno-omit-frame-pointer
 DUKOPTS+=-g -Os
else
 STRIP=$(TOOLCHAIN)strip
 FLAGSCXX+=-Os -fomit-frame-pointer -fdata-sections -ffunction-sections
 FLAGSLD+=-Os
endif

# make command line overrides
FLAGSCXX+=$(FLAGS)
FLAGSCXX+=$(CXXFLAGS)
FLAGSLD+=$(LDFLAGS)

# Pick windows, other platforms are compatible.
ifeq ($(OS),Windows_NT)
 BUILDDIR:=./$(BUILD_DIRECTORY)
 BINARY_EXTENSION=.exe
 LDSTATIC+=-static -static-libstdc++ -static-libgcc
 FLAGSCXX+=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D_WIN32_IE=0x0900
 BINARY=$(PROGRAM_NAME)$(BINARY_EXTENSION)
 LIBS+=-lm -ladvapi32 -lshell32 -lpthread -lws2_32 -lsetupapi
 RC=windres
 RC_OBJ=$(BUILDDIR)/cli/win32/mainrc.o
else
 BUILDDIR:=./$(BUILD_DIRECTORY)
 BINARY_EXTENSION=.elf
 BINARY=$(PROGRAM_NAME)
 INSTALLDIR=/usr/local/bin
 DESTDIR=$(INSTALLDIR)
 LIBS+=-lm -lrt
 ifdef STATIC
  LDSTATIC+=-static -static-libstdc++ -static-libgcc
 endif
endif

-include env.mk
ifeq ($(WITH_EXPERIMENTAL),1)
 FLAGSCXX+=-DWITH_EXPERIMENTAL
endif
ifeq ($(WITH_DEFAULT_STRICT_INCLUDE),1)
 FLAGSCXX+=-DWITH_DEFAULT_STRICT_INCLUDE
endif
ifneq ($(WITHOUT_APP_ATTACHMENT),1)
 FLAGSCXX+=-DWITH_APP_ATTACHMENT
endif
ifeq ($(WITH_MINIMAL_MODS),1)
 FLAGSCXX+=-DWITHOUT_SYSTEM_EXEC -DWITHOUT_SYSTEM_HASH -DWITHOUT_CONVERSION -DWITHOUT_SERIALPORT -DWITHOUT_MMAP
endif

#---------------------------------------------------------------------------------------------------
# Build dependencies and development compiler flags from file.
#---------------------------------------------------------------------------------------------------
wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
HEADER_DEPS=duktape/duktape.hh $(STDMOD_SOURCES) $(sort $(call wildcardr, duktape/mod, *.hh))
OPTS+=$(shell cat $(dir $<)/compiler.flags 2>/dev/null || /bin/true)

#---------------------------------------------------------------------------------------------------
# Standard make targets
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+=--no-print-directory --output-sync=target
.PHONY: default all binary dist clean mrproper install
#---------------------------------------------------------------------------------------------------

default: binary

dist: clean | binary
	@mkdir -p dist/doc
	@cp -f $(BUILDDIR)/cli/$(BINARY) dist/
	@cp -f doc/stdmods.js dist/doc/
	@cp -f doc/js-basics/overview.md dist/doc/

all:
	@$(MAKE) -j binary
	@$(MAKE) -j examples
	@$(MAKE) -j test

clean:
	@rm -rf ./build ./dist
	@rm -f *.gcno *.gcda
	@$(MAKE) -C doc/examples/basic-integration clean
	@$(MAKE) -C doc/examples/native-class-wrapping clean

mrproper: clean

#---------------------------------------------------------------------------------------------------
# Duktape base compilation
#---------------------------------------------------------------------------------------------------

$(BUILDDIR)/duktape/duktape.o: duktape/duktape.c duktape/duk_config.h duktape/duktape.h
	@echo "[c++ ] $< $@"
	@mkdir -p $(BUILDDIR)/duktape
	@$(CXX) -c -o $@ $< $(DUKOPTS) $(TESTOPTS)

#---------------------------------------------------------------------------------------------------
# CLI default application
#---------------------------------------------------------------------------------------------------

binary: $(BUILDDIR)/cli/$(BINARY)

install: binary
	@if [ -z "$(DESTDIR)" ]; then echo "[fail] No DESTDIR specified for installing."; /bin/false; fi
	@if [ ! -d "$(DESTDIR)" ]; then echo "[fail] DESTDIR for installing does not exist."; /bin/false; fi
	@echo "[inst] $(BUILDDIR)/cli/$(BINARY) -> $(DESTDIR)/$(BINARY)"
	@cp -f $(BUILDDIR)/cli/$(BINARY) "$(DESTDIR)/$(BINARY)"
	@[ "$(whoami)"!="root" ] || chown root:root "$(DESTDIR)/$(BINARY)"
	@[ "$(whoami)"!="root" ] || chmod 755 "$(DESTDIR)/$(BINARY)"

$(BUILDDIR)/cli/$(BINARY): $(BUILDDIR)/duktape/duktape.o $(BUILDDIR)/cli/main.o $(RC_OBJ)
	@echo "[ld  ] $^ $@"
	@$(LD) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) --strip-all --discard-locals --discard-all $@ ; fi
 ifneq ($(WITHOUT_APP_ATTACHMENT),1)
	@mkdir -p $(BUILDDIR)/duktape/mod/ext/app_attachment
	@cp -f duktape/mod/ext/app_attachment/* $(BUILDDIR)/duktape/mod/ext/app_attachment/
	@cd $(BUILDDIR)/duktape/mod/ext/app_attachment; make -s patch-binary TARGET_BINARY=../../../../cli/$(BINARY)
 endif

$(BUILDDIR)/cli/main.o: cli/main.cc $(HEADER_DEPS)
	@echo "[c++ ] $<  $@"
	@mkdir -p $(BUILDDIR)/cli
	@$(CXX) -c -o $@ $< $(FLAGSCXX) -I. -DPROGRAM_VERSION='"""$(GIT_COMMIT_VERSION)"""' -DPROGRAM_NAME='"""$(PROGRAM_NAME)"""' $(OPTS)

# win32 resources.
ifeq ($(OS),Windows_NT)
 $(BUILDDIR)/%.o: %.rc
	@echo "[rc  ] $< $@"
	@mkdir -p $(dir $@)
	@$(RC) -i $< -o $@
 $(BUILDDIR)/%.ico: %.png
	@mkdir -p $(dir $@)
	@magick convert $< -define icon:auto-resize="256,128,96,64,48,32,16" $@
endif

#---------------------------------------------------------------------------------------------------
# Examples
#---------------------------------------------------------------------------------------------------
.PHONY: examples

examples:
	@echo "[note] Building/running examples for error checking ..."
	@mkdir -p $(BUILDDIR)/doc/examples
	@$(MAKE) -C doc/examples/basic-integration >$(BUILDDIR)/doc/examples/basic-integration.log 2>&1
	@$(MAKE) -C doc/examples/basic-integration clean
	@$(MAKE) -C doc/examples/native-class-wrapping >$(BUILDDIR)/doc/examples/native-class-wrapping.log 2>&1
	@$(MAKE) -C doc/examples/native-class-wrapping clean

#---------------------------------------------------------------------------------------------------
# Documentation
#---------------------------------------------------------------------------------------------------
.PHONY: documentation
documentation: $(BUILDDIR)/cli/$(BINARY) | doc/src/documentation.djs doc/src/readme.src.md
	$(BUILDDIR)/cli/$(BINARY) -s doc/src/documentation.djs --stdmods -o doc/stdmods.js
	$(BUILDDIR)/cli/$(BINARY) -s doc/src/documentation.djs --readme  -o readme.md

#---------------------------------------------------------------------------------------------------
# Help
#---------------------------------------------------------------------------------------------------
.PHONY: help

help:
	@echo "Usage: make [ clean all binary test dist examples documentation ]"
	@echo ""
	@echo " - all:            Build CLI binary, documentation, test and examples."
	@echo " - binary:         Build CLI binary"
	@echo " - clean:          Clean binaries, temporary files and tests."
	@echo " - dist:           Binary distribution package files."
	@echo " - test:           Build test binaries, run all tests that have changed."
	@echo " - examples:       Build and run the integration examples."
	@echo ""
	@echo " - documentation:  Generate documentation with JSDoc (!affects revisioned files!)"
	@echo ""

#---
include test/testenv.mk
include test/sanitize.mk

# EOF
