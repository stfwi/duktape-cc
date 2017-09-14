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
FLAGSCXX=-std=c++11 -W -Wall -Wextra -pedantic
FLAGSCXX+=-Iduktape
DUKOPTS+=-std=c++11 -fstrict-aliasing -fdata-sections -ffunction-sections -Os -DDUK_USE_CPP_EXCEPTIONS
DUKTAPE_VERSION=2.1.0
DUKTAPE_ARCHIVE=duktape/duktape-releases/duktape-$(DUKTAPE_VERSION).tar.xz
GIT_COMMIT_VERSION:=$(shell git log --pretty=format:%h -1)

#---------------------------------------------------------------------------------------------------

ifdef DEBUG
  FLAGSCXX+=-Os -g -fno-omit-frame-pointer -fdata-sections -ffunction-sections
  FLAGSLD+=-Os -g -fno-omit-frame-pointer -Wl,--gc-sections
#  FLAGSCXX+=-fsanitize=address
#  FLAGSLD+=-fsanitize=address
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

# Auto pick windows, other platforms are compatible
ifeq ($(wildcard /bin/*),)
OS=win
endif

ifeq ($(OS),win)
BINARY_EXTENSION=.exe
LDSTATIC+=-static -Os -s -static-libgcc
FLAGSCXX+=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D_WIN32_IE=0x0900
BINARY=djs$(BINARY_EXTENSION)
LIBS+=-ladvapi32 -lshell32
else
BINARY_EXTENSION=.elf
BINARY=djs
LIBS+=-lrt
ifdef STATIC
  LDSTATIC+=-static -Os -s -static-libgcc
endif
endif

ifeq ($(WITH_EXPERIMENTAL),1)
  FLAGSCXX+=-DWITH_EXPERIMENTAL
endif

# Test selection
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
.PHONY: clean all binary documentation dev test test-binaries test-clean-all test-clean help static-code-analysis

all: binary test documentation

test-clean:
	@echo "[note] Cleaning test logs"
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

test-clean-all:
	@echo "[note] Cleaning all test files"
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)
	@rm -f $(TEST_BINARIES) $(TEST_SCRIPT_BINARY)

clean:
	@echo "[note] Cleaning"
	@rm -f cli/*.o duktape/*.o *.o cli/$(BINARY) cli/$(DEVBINARY) cli/$(EXAMPLEBINARY) cli/*.elf cli/*.exe
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)
	@rm -f $(TEST_BINARIES) $(TEST_SCRIPT_BINARY)

#---------------------------------------------------------------------------------------------------
# Duktape base compilation
#---------------------------------------------------------------------------------------------------

duktape/duktape.o: duktape/duktape.c duktape/duk_config.h duktape/duktape.h
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(DUKOPTS)

#---------------------------------------------------------------------------------------------------
# CLI example app
binary: cli/$(BINARY)

cli/$(BINARY): duktape/duktape.o cli/main.o
	@echo "[ld  ] $^  $@"
	@$(CXX) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi
ifneq ($(OS),win)
	@echo -n "[info] binary is "; file $@
endif

cli/main.o: cli/main.cc $(HEADER_DEPS) $(TEST_BINARIES_SOURCES)
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I. -DPROGRAM_VERSION='"""$(GIT_COMMIT_VERSION)"""'

#---------------------------------------------------------------------------------------------------
# Developer testing ground
#---------------------------------------------------------------------------------------------------
dev: cli/$(DEVBINARY)
	@echo "[note] Running development binary ..."
	@cd ./cli; ./$(DEVBINARY) dev.js

cli/$(DEVBINARY): cli/dev.o duktape/duktape.o
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(LDSTATIC) $(FLAGSLD) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

cli/dev.o: cli/dev.cc $(HEADER_DEPS) $(TEST_BINARIES_SOURCES)
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I.
	@echo "[note] Development testing binary is $@"

#---------------------------------------------------------------------------------------------------
# Integration example
#---------------------------------------------------------------------------------------------------
example: cli/$(EXAMPLEBINARY)
	@echo "[note] Running example ..."
	@cd ./cli; ./$(EXAMPLEBINARY) arg1 arg2 arg2 etc

cli/$(EXAMPLEBINARY): cli/example.o duktape/duktape.o
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(LDSTATIC) $(FLAGSLD) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

cli/example.o: cli/example.cc $(HEADER_DEPS) $(TEST_BINARIES_SOURCES)
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I.
	@echo "[note] Example binary is $@"

#---------------------------------------------------------------------------------------------------
# Documentation
#---------------------------------------------------------------------------------------------------
documentation: doc/stdmods.js

# JS documentation (searching the mods for JSDOC preprocessor tags and collecting the contents.
doc/stdmods.js: $(STDMOD_SOURCES) cli/$(BINARY) doc/mkjsdoc.js
	@cli/$(BINARY) doc/mkjsdoc.js > $@

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
	-@$(CXX) -o $@ $< duktape/duktape.o $(FLAGSCXX) $(OPTS) $(FLAGSLD) $(LIBS) || echo "[fail] $@"
	-@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

test/0%/test.log: test/0%/test$(BINARY_EXTENSION)
ifneq ($(OS),win)
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
	-@$(CXX) -o $@ $< duktape/duktape.o $(FLAGSCXX) $(OPTS) $(FLAGSLD) $(LIBS) || echo "[fail] $@"
	-@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

test/1%/test.log: test/1%/test.js $(TEST_SCRIPT_BINARY)
	-@rm -f $@
	-@cd $(dir $<); ../../$(TEST_SCRIPT_BINARY) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $<" || echo "[fail] $@"
ifneq ($(TEST),)
	-@[ -f $@ ] && cat $@
endif

#---------------------------------------------------------------------------------------------------
# Static code analysis
#---------------------------------------------------------------------------------------------------
static-code-analysis:
ifneq ($(OS),win)
	@echo "[sca ] cppcheck ..."
	@cppcheck --std=c++11 --force --platform=unix64 --template=gcc \
	 	  --suppress=missingIncludeSystem --enable=warning \
		  --enable=information --enable=performance --inconclusive \
		  -Iduktape \
		 duktape/*.hh duktape/mod/*.hh cli/*.cc \
		 2>&1 | grep -v "^Checking"
endif

#---------------------------------------------------------------------------------------------------
# help
#---------------------------------------------------------------------------------------------------

help:
	@echo "Usage: make [ clean all binary test documentation"
	@echo "              test-clean test-clean-all ]"
	@echo ""
	@echo " - clean:          Clean binaries, temporary files and tests."
	@echo " - all:            Build CLI binary, documentation, test and examples."
	@echo " - binary:         Build CLI binary"
	@echo " - documentation:  Generate documentation with JSDoc"
	@echo " - test:           Build test binaries, run all tests that have changed."
	@echo " - test-binaries:  Build test binaries."
	@echo " - test-clean:     Clean test log files to re-run tests."
	@echo " - test-clean-all: Clean also test binaries to force rebuild."
	@echo ""

# EOF
#---------------------------------------------------------------------------------------------------
