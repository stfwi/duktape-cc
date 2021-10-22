#---------------------------------------------------------------------------------------------------
# Duktape c++ wrapper testing, CLI binary and development binary build.
# Requires GNU make version >= v4.0 and basic UNIX tools (for Windows
# simply install GIT globally, so that tools like rm.exe are in the PATH;
# Linux/BSD: no action needed).
#---------------------------------------------------------------------------------------------------
# Optional tool chain prefix path, sometimes also referred to as CROSSCOMPILE
TOOLCHAIN=

CXX=$(TOOLCHAIN)g++
LD=$(CXX)
CXX_STD=c++17
FLAGSCXX=-std=$(CXX_STD) -W -Wall -Wextra -pedantic
FLAGSCXX+=-Iduktape
DUKOPTS+=-std=$(CXX_STD) -fstrict-aliasing -fdata-sections -ffunction-sections -Os -DDUK_USE_CPP_EXCEPTIONS
GIT_COMMIT_VERSION:=$(shell git log --pretty=format:%h -1 || echo 0000001)
HOST_CXX=$(CXX)
PROGRAM_NAME=djs

#---------------------------------------------------------------------------------------------------

ifdef DEBUG
 FLAGSCXX+=-Os -g -fno-omit-frame-pointer -fdata-sections -ffunction-sections
 FLAGSLD+=-Os -g -fno-omit-frame-pointer -Wl,--gc-sections
 LIBS+=-lm
 DUKOPTS+=-g -O0
else
 STRIP=$(TOOLCHAIN)strip
 FLAGSCXX+=-Os -fomit-frame-pointer -fdata-sections -ffunction-sections
 FLAGSLD+=-Os -Wl,--gc-sections
 LIBS+=-lm
endif

# make command line overrides
FLAGSCXX+=$(FLAGS)
FLAGSCXX+=$(CXXFLAGS)
FLAGSLD+=$(LDFLAGS)

# Pick windows, other platforms are compatible.
ifeq ($(OS),Windows_NT)
 BINARY_EXTENSION=.exe
 LDSTATIC+=-static -Os -s -static-libgcc
 FLAGSCXX+=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D_WIN32_IE=0x0900
 BINARY=$(PROGRAM_NAME)$(BINARY_EXTENSION)
 LIBS+=-ladvapi32 -lshell32 -lws2_32 -lsetupapi
 RC=windres
 RC_OBJ=cli/win32/mainrc.o
else
 BINARY_EXTENSION=.elf
 BINARY=$(PROGRAM_NAME)
 LIBS+=-lrt
 ifdef STATIC
  LDSTATIC+=-static -Os -s -static-libgcc
 endif
endif

ifeq ($(WITH_EXPERIMENTAL),1)
 FLAGSCXX+=-DWITH_EXPERIMENTAL
endif
ifeq ($(WITH_DEFAULT_STRICT_INCLUDE),1)
 FLAGSCXX+=-DWITH_DEFAULT_STRICT_INCLUDE
endif
ifneq ($(WITHOUT_APP_ATTACHMENT),1)
 FLAGSCXX+=-DCONFIG_WITH_APP_ATTACHMENT
endif
ifneq ($(WITHOUT_SOCKET),1)
 FLAGSCXX+=-DCONFIG_WITH_SOCKET
endif

#---------------------------------------------------------------------------------------------------
# Test selection
#---------------------------------------------------------------------------------------------------
wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
DEVBINARY=dev$(BINARY_EXTENSION)
EXAMPLEBINARY=example$(BINARY_EXTENSION)
TEST_BINARIES:=$(foreach F, $(filter %/ , $(sort $(wildcard test/0*/))), $Ftest$(BINARY_EXTENSION))
TEST_BINARIES_RESULTS:=$(patsubst %$(BINARY_EXTENSION),%.log,$(TEST_BINARIES))
TEST_BINARIES_SOURCES:=$(patsubst %$(BINARY_EXTENSION),%.cc,$(TEST_BINARIES))
TEST_SCRIPT_SOURCES:=$(foreach F, $(filter %/ , $(sort $(wildcard test/1*/))), $Ftest.js)
TEST_SCRIPT_RESULTS:=$(patsubst %.js,%.log,$(TEST_SCRIPT_SOURCES))
TEST_SCRIPT_BINARY:=test/1000-script-test-binary/test$(BINARY_EXTENSION)
TEST_SCRIPT_BINARY_SOURCE:=$(patsubst %$(BINARY_EXTENSION),%.cc,$(TEST_SCRIPT_BINARY))

ifneq ($(TEST),)
  TESTDIRS=$(wildcard test/*$(TEST)*/)
  TEST_BINARIES:=$(foreach f,$(TESTDIRS),$(filter $(f)%,$(TEST_BINARIES)))
  TEST_BINARIES_RESULTS:=$(foreach f,$(TESTDIRS),$(filter $(f)%,$(TEST_BINARIES_RESULTS)))
  TEST_BINARIES_SOURCES:=$(foreach f,$(TESTDIRS),$(filter $(f)%,$(TEST_BINARIES_SOURCES)))
  TEST_SCRIPT_SOURCES:=$(foreach f,$(TESTDIRS),$(filter $(f)%,$(TEST_SCRIPT_SOURCES)))
  TEST_SCRIPT_RESULTS:=$(foreach f,$(TESTDIRS),$(filter $(f)%,$(TEST_SCRIPT_RESULTS)))
endif

STDMOD_SOURCES:=$(sort $(call wildcardr, duktape/mod, *.hh))
HEADER_DEPS=duktape/duktape.hh $(STDMOD_SOURCES)
MAIN_TESTJS:=$(wildcard main.js)

#---------------------------------------------------------------------------------------------------
# make targets
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+=--no-print-directory --output-sync=target
.PHONY: clean all binary patched-binary documentation documentation-clean dev run test test-binaries test-clean-all test-clean mrproper help
#---------------------------------------------------------------------------------------------------

all: binary | test

clean:
	@echo "[note] Cleaning"
	@rm -f cli/*.o duktape/*.o *.o  cli/win32/*.o cli/$(BINARY) cli/$(DEVBINARY) cli/$(EXAMPLEBINARY) cli/devlib-$(BINARY) cli/*.elf cli/*.exe cli/lib/*.elf cli/lib/*.exe cli/*.tmp
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)
	@rm -f $(TEST_BINARIES) $(TEST_SCRIPT_BINARY)
	@rm -f $(call wildcardr, test, *.elf) $(call wildcardr, test, *.exe)
	@cd duktape/mod/ext/app_attachment; make -s clean

mrproper: clean test-clean test-clean-all

test-clean:
	@echo "[note] Cleaning test logs"
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

test-clean-all:
	@echo "[note] Cleaning all test files"
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)
	@rm -f $(TEST_BINARIES) $(TEST_SCRIPT_BINARY)

#---------------------------------------------------------------------------------------------------
# Duktape base compilation
#---------------------------------------------------------------------------------------------------

duktape/duktape.o: duktape/duktape.c duktape/duk_config.h duktape/duktape.h
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(DUKOPTS)

#---------------------------------------------------------------------------------------------------
# CLI example app
#---------------------------------------------------------------------------------------------------
run: binary cli/dev.js
	@echo "[ js ] cli/dev.js"
	@cd ./cli; ./$(BINARY) dev.js

binary: cli/$(BINARY)

cli/$(BINARY): duktape/duktape.o cli/main.o $(RC_OBJ)
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) --strip-all --discard-locals --discard-all $@ ; fi
ifneq ($(WITHOUT_APP_ATTACHMENT),1)
	@cd duktape/mod/ext/app_attachment; make -s patch-binary TARGET_BINARY=../../../../cli/$(BINARY)
endif

cli/main.o: cli/main.cc $(HEADER_DEPS) $(TEST_BINARIES_SOURCES)
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I. -DPROGRAM_VERSION='"""$(GIT_COMMIT_VERSION)"""' -DPROGRAM_NAME='"""$(PROGRAM_NAME)"""'

# win32 resources.
ifeq ($(OS),Windows_NT)
%.o: %.rc
	@echo "[rc  ] $< $@"
	@$(RC) -i $< -o $@
%.ico: %.png
	@magick convert $< -define icon:auto-resize="256,128,96,64,48,32,16" $@
endif

#---------------------------------------------------------------------------------------------------
# Developer testing grounds
#---------------------------------------------------------------------------------------------------

dev: cli/$(DEVBINARY)
	@echo "[note] Running development binary ..."
	@cd ./cli; ./$(DEVBINARY) dev.js

cli/$(DEVBINARY): cli/dev.cc duktape/duktape.o $(HEADER_DEPS) $(TEST_BINARIES_SOURCES) $(RC_OBJ)
	@echo "[c++ ] $< $@"
	@$(CXX) -o $@ $< duktape/duktape.o $(FLAGSCXX) $(OPTS) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS)

#---------------------------------------------------------------------------------------------------
# Integration example
#---------------------------------------------------------------------------------------------------
example: examples/basic-integration/$(EXAMPLEBINARY)
	@echo "[note] Running basic integration example ..."
	@cd ./examples/basic-integration/; ./$(EXAMPLEBINARY) arg1 arg2 arg2 etc

examples/basic-integration/$(EXAMPLEBINARY): examples/basic-integration/example.o duktape/duktape.o
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(LDSTATIC) $(FLAGSLD) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

examples/basic-integration/example.o: examples/basic-integration/example.cc $(HEADER_DEPS) $(TEST_BINARIES_SOURCES)
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I.
	@echo "[note] Example binary is $@"

#---------------------------------------------------------------------------------------------------
# Documentation
#---------------------------------------------------------------------------------------------------
documentation-clean:
	@rm -f doc/stdmods.js doc/src/function-list.md

documentation: documentation-clean doc/stdmods.js doc/src/function-list.md readme.md

# JS documentation (searching the mods for JSDOC preprocessor tags and collecting the contents.
doc/stdmods.js: $(STDMOD_SOURCES) cli/$(BINARY) doc/src/mkjsdoc.js
	@cli/$(BINARY) doc/src/mkjsdoc.js > $@

doc/src/function-list.md: doc/stdmods.js cli/$(BINARY)
	@cat "$<" | cli/$(BINARY) doc/src/mkfnlist.js > "$@"

readme.md: doc/src/readme.src.md doc/src/function-list.md doc/src/mkreadme.js
	@cli/$(BINARY) doc/src/mkreadme.js > $@

#---------------------------------------------------------------------------------------------------
# Tests
#---------------------------------------------------------------------------------------------------
test: binary $(TEST_BINARIES) $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_BINARY) $(TEST_SCRIPT_RESULTS)
test-binaries: binary $(TEST_BINARIES)

#
# Test binaries (compile, run, test No < 1000)
#
test/%/test$(BINARY_EXTENSION): test/%/test.cc test/testenv.hh test/microtest.hh duktape/duktape.o $(HEADER_DEPS)
	@echo "[c++ ] $@"
	-@$(CXX) -o $@ $< duktape/duktape.o $(FLAGSCXX) $(OPTS) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS) || echo "[fail] $@"
	-@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

test/0%/test.log: test/0%/test$(BINARY_EXTENSION)
ifneq ($(OS),Windows_NT)
	-@rm -f $@
	-@cd $(dir $<); ./$(notdir $<) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $<" || echo "[fail] $@"
else
	-@rm -f $@
	-@cd "$(dir $<)"; echo "" | "./$(notdir $<)" >$(notdir $@) && echo "[pass] $<" || echo "[fail] $@"
endif
ifneq ($(TEST),)
	-@[ -f $@ ] && cat $@
endif

#
# Test scripts running with the test script binary (No > 1000)
#
$(TEST_SCRIPT_BINARY): $(TEST_SCRIPT_BINARY_SOURCE) duktape/duktape.o $(HEADER_DEPS) test/testenv.hh test/microtest.hh
	@echo "[c++ ] $@"
	-@$(CXX) -o $@ $< duktape/duktape.o $(FLAGSCXX) $(OPTS) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS) || echo "[fail] $@"
	-@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

test/1%/test.log: test/1%/test.js $(TEST_SCRIPT_BINARY)
	-@rm -f $@
	-@cd $(dir $<); ../../$(TEST_SCRIPT_BINARY) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $<" || echo "[fail] $@"
ifneq ($(TEST),)
	-@[ -f $@ ] && cat $@
endif

#---------------------------------------------------------------------------------------------------
# help
#---------------------------------------------------------------------------------------------------

help:
	@echo "Usage: make [ clean all binary test documentation"
	@echo "              test-clean test-clean-all ]"
	@echo ""
	@echo " - all:            Build CLI binary, documentation, test and examples."
	@echo " - binary:         Build CLI binary"
	@echo " - clean:          Clean binaries, temporary files and tests."
	@echo " - documentation:  Generate documentation with JSDoc"
	@echo " - test:           Build test binaries, run all tests that have changed."
	@echo " - test-binaries:  Build test binaries."
	@echo " - test-clean:     Clean test log files to re-run tests."
	@echo " - test-clean-all: Clean also test binaries to force rebuild."
	@echo ""

# EOF
#---------------------------------------------------------------------------------------------------
