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
 FLAGSLD+=-Os -g -fno-omit-frame-pointer -Wl,--gc-sections
 DUKOPTS+=-g -Os
else
 STRIP=$(TOOLCHAIN)strip
 FLAGSCXX+=-Os -fomit-frame-pointer -fdata-sections -ffunction-sections
 FLAGSLD+=-Os -Wl,--gc-sections
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
 FLAGSCXX+=-DCONFIG_WITH_APP_ATTACHMENT
endif

#---------------------------------------------------------------------------------------------------
# Test selection
#---------------------------------------------------------------------------------------------------
TEST_SELECTION:=$(sort $(wildcard test/*$(TEST)*/))
TEST_BINARIES_SOURCES:=$(foreach F, $(filter test/0%/ , $(TEST_SELECTION)), $Ftest.cc)
TEST_BINARIES:=$(patsubst %.cc,$(BUILDDIR)/%$(BINARY_EXTENSION),$(TEST_BINARIES_SOURCES))
TEST_BINARIES_RESULTS:=$(patsubst %.cc,$(BUILDDIR)/%.log,$(TEST_BINARIES_SOURCES))
TEST_SCRIPT_SOURCES:=$(foreach F, $(filter test/1%/ , $(TEST_SELECTION)), $Ftest.js)
TEST_SCRIPT_RESULTS:=$(patsubst %.js,$(BUILDDIR)/%.log,$(TEST_SCRIPT_SOURCES))
TEST_BINARIES_AUXILIARY_SOURCES:=$(wildcard $(foreach F, $(filter test/0%/ , $(TEST_SELECTION)), $Ftaux.cc))
TEST_BINARIES_AUXILIARY:=$(patsubst %.cc,$(BUILDDIR)/%$(BINARY_EXTENSION),$(TEST_BINARIES_AUXILIARY_SOURCES))
TEST_SCRIPT_AUXILIARY_SOURCES:=$(wildcard $(foreach F, $(filter test/0%/ , $(TEST_SELECTION)), $Ftaux.js))
TEST_SCRIPT_BINARY_SOURCE:=test/1000-script-test-binary/test.cc
TEST_SCRIPT_BINARY:=$(BUILDDIR)/test/1000-script-test-binary/test$(BINARY_EXTENSION)

# g++ pedantic test run options.
ifneq (,$(findstring g++,$(CXX)))
 # (Careful with formatting, there are no tabs these blocks)
 DUKOPTS+=-Wno-deprecated
 ifeq ($(WITH_SANITIZERS),1)
  TESTOPTS+=-g -ggdb -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
  FLAGSLD+=-static-libstdc++ -static-libasan
	ifeq ($(MORE_SANITIZERS),1)
   TESTOPTS+=-fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak
	endif
 endif
endif

# clang++ pedantic test run options
ifneq (,$(findstring clang++,$(CXX)))
 DUKOPTS+=-Wno-deprecated
 ifeq ($(WITH_SANITIZERS),1)
  TESTOPTS+=-g -fsanitize=address -fsanitize=memory -fsanitize=memory-track-origins -fsanitize=thread
 endif
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
.PHONY: default all binary dist test clean mrproper install
#---------------------------------------------------------------------------------------------------

default: binary

dist: clean | binary
	@mkdir -p dist/doc
	@cp -f $(BUILDDIR)/cli/$(BINARY) dist/
	@cp -f doc/stdmods.js dist/doc/

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
	@$(CXX) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)
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
# Tests
#---------------------------------------------------------------------------------------------------

# Test invocation and summary (we can't use a js script to analyze the tests, so the unix tools
# that are available on linux and windows (with GIT) are the tools of choice).
test: $(TEST_BINARIES_SOURCES) $(TEST_SCRIPT_SOURCES) $(TEST_SCRIPT_BINARY_SOURCE) $(TEST_BINARIES_AUXILIARY)
	@mkdir -p $(BUILDDIR)/test
	@rm -f $(BUILDDIR)/test/*.log
 ifneq ($(TEST),)
	@rm -f $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)
 endif
	@$(MAKE) -j -k test-results | tee $(BUILDDIR)/test/summary.log 2>&1
	@if grep -e '^\[fail\]' -- $(BUILDDIR)/test/summary.log >/dev/null 2>&1; then echo "[FAIL] At least one test failed."; /bin/false; else echo "[PASS] All tests passed."; fi
 ifneq ($(TEST),)
  ifneq ($(TEST_BINARIES_RESULTS)$(TEST_SCRIPT_RESULTS),)
	-@cat $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS) 2>/dev/null
  endif
 endif

# Actual test compilations and runs
.PHONY: test-results
test-results: $(TEST_SCRIPT_BINARY) $(TEST_BINARIES) $(TEST_BINARIES_AUXILIARY) $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

# Test binaries (compile test No < 1000)
$(BUILDDIR)/test/%/test$(BINARY_EXTENSION): test/%/test.cc test/testenv.hh test/microtest.hh $(BUILDDIR)/duktape/duktape.o $(HEADER_DEPS)
	@echo "[c++ ] $@"
	@mkdir -p $(dir $@)
	@cp -rf $(dir $<)/* $(dir $@)/
	@$(CXX) -o $@ $< $(BUILDDIR)/duktape/duktape.o $(FLAGSCXX) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS) $(TESTOPTS) $(OPTS) || echo "[fail] $@"
	@[ -f test.gcno ] && mv test.gcno $(dir $@) || /bin/true

# Test auxiliary binaries (compile test No < 1000)
$(BUILDDIR)/test/%/taux$(BINARY_EXTENSION): test/%/taux.cc $(BUILDDIR)/duktape/duktape.o $(HEADER_DEPS) $(TEST_SCRIPT_AUXILIARY_SOURCES)
	@echo "[c++ ] $@"
	@mkdir -p $(dir $@)
	@cp -rf $(dir $<)/* $(dir $@)/
	@$(CXX) -o $@ $< $(BUILDDIR)/duktape/duktape.o $(FLAGSCXX) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS) $(TESTOPTS) $(OPTS) || echo "[fail] $@"
	@[ -f taux.gcno ] && mv taux.gcno $(dir $@) || /bin/true

# Test binaries (run, test No < 1000)
$(BUILDDIR)/test/0%/test.log: $(BUILDDIR)/test/0%/test$(BINARY_EXTENSION)
	@mkdir -p $(dir $@)
	@rm -f $@
 ifneq ($(OS),Windows_NT)
	@cd $(dir $<) && ./$(notdir $<) $(ARGS) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $@" || echo "[fail] $@"
	@[ -f test.gcda ] && mv test.gcda $(dir $@) || /bin/true
 else
	@cd $(dir $<) && echo "" | "./$(notdir $<)" $(ARGS) >$(notdir $@) && echo "[pass] $@" || echo "[fail] $@"
 endif

# Test scripts runner binary (No > 1000)
$(TEST_SCRIPT_BINARY): $(TEST_SCRIPT_BINARY_SOURCE) $(BUILDDIR)/duktape/duktape.o $(HEADER_DEPS) test/testenv.hh test/microtest.hh
	@echo "[c++ ] $@"
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $< $(BUILDDIR)/duktape/duktape.o $(FLAGSCXX) -I. $(FLAGSLD) $(LDSTATIC) $(LIBS) $(TESTOPTS) $(OPTS) || echo "[fail] $@"
	@[ -f test.gcno ] && mv test.gcno $(dir $@) || /bin/true

# Test scripts runs (No > 1000)
$(BUILDDIR)/test/1%/test.log: test/1%/test.js $(TEST_SCRIPT_BINARY)
	@mkdir -p $(dir $@)
	@rm -f $@
	@cp -f $(dir $<)/* $(dir $@)/
	@cd $(dir $@); ../../../$(TEST_SCRIPT_BINARY) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $@" || echo "[fail] $@"
 ifneq ($(OS),Windows_NT)
	@[ -f test.gcda ] && mv test.gcda $(dir $@) || /bin/true
	@[ ! -f test.gcno ] && cp $(patsubst %$(BINARY_EXTENSION),%.gcno,$(TEST_SCRIPT_BINARY)) >/dev/null 2>&1 $(dir $@) || /bin/true
 endif
#---------------------------------------------------------------------------------------------------
# Coverage (only available with gcov/lcov using linux g++)
#---------------------------------------------------------------------------------------------------
.PHONY: coverage coverage-runs coverage-files coverage-summary
LCOV_DATA_ROOT=$(BUILDDIR)/coverage

# Coverage compile flags for g++
ifneq (,$(WITH_COVERAGE))
 ifneq (Linux,$(shell uname))
 	$(error Coverage is supported under Linux with g++ and gcov/lcov)
 endif
 .NOTPARALLEL:
 TESTOPTS+=--coverage -ftest-coverage -fprofile-arcs -fprofile-abs-path -O0 -fprofile-filter-files='main\.cc;duktape/.*\.hh'
endif

# Coverate main target (the one we invoke)
coverage:
	@$(MAKE) clean
	@$(MAKE) --jobs=1 coverage-runs WITH_COVERAGE=1
	@$(MAKE) coverage-files
	@$(MAKE) coverage-summary

# Selection of tests where c++ coverage makes sense
coverage-runs: $(TEST_BINARIES_AUXILIARY) $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

# All gcov files from the executed tests
coverage-files: $(patsubst %/test.log,%/test.gcov,$(TEST_BINARIES_RESULTS))  $(patsubst %/test.log,%/test.gcov,$(TEST_SCRIPT_RESULTS))

# Analysis of one test using gcov/lcov
$(BUILDDIR)/test/%/test.gcov: $(BUILDDIR)/test/%/test.log
	@echo "[gcov] $*"
	@cd $(dir $<) && gcov --source-prefix "$(CURDIR)" *.gcno > gcov.log 2>&1
	@cd $(dir $<) && gcov -m *.cc >> gcov.log 2>&1
	@cd $(dir $<) && lcov -c -d . -o lcov.info >> lcov.log 2>&1
	@cd $(dir $<) && lcov -r lcov.info "/usr*" -o lcov.info >> lcov.log 2>&1
	@cd $(dir $<) && lcov -r lcov.info "*microtest/include*" -o lcov.info >> lcov.log 2>&1
	@mkdir -p $(BUILDDIR)/coverage/data
	@mv $(dir $<)/lcov.info $(BUILDDIR)/coverage/data/$*.info

# Combined coverage all executed tests
coverage-summary:
	@echo "[lcov] Summary"
	@cd $(LCOV_DATA_ROOT) && genhtml data/*.info --output-directory ./html >> lcov.log 2>&1

#---------------------------------------------------------------------------------------------------
# Dump environment
#---------------------------------------------------------------------------------------------------
.PHONY: vars

vars:
	@echo "BINARY=$(BINARY)"
	@echo "TEST_SELECTION=$(TEST_SELECTION)"
	@echo "TEST_BINARIES_SOURCES='$(TEST_BINARIES_SOURCES)'"
	@echo "TEST_BINARIES='$(TEST_BINARIES)'"
	@echo "TEST_BINARIES_RESULTS='$(TEST_BINARIES_RESULTS)'"
	@echo "TEST_SCRIPT_SOURCES='$(TEST_SCRIPT_SOURCES)'"
	@echo "TEST_SCRIPT_RESULTS='$(TEST_SCRIPT_RESULTS)'"
	@echo "TEST_SCRIPT_BINARY_SOURCE='$(TEST_SCRIPT_BINARY_SOURCE)'"
	@echo "TEST_SCRIPT_BINARY='$(TEST_SCRIPT_BINARY)'"
	@echo "TEST_BINARIES_AUXILIARY_SOURCES='$(TEST_BINARIES_AUXILIARY_SOURCES)'"
	@echo "TEST_BINARIES_AUXILIARY='$(TEST_BINARIES_AUXILIARY)'"
	@echo "TEST_SCRIPT_AUXILIARY_SOURCES='$(TEST_SCRIPT_AUXILIARY_SOURCES)'"

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

# EOF
#---------------------------------------------------------------------------------------------------
