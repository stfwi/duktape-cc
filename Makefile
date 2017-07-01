#---------------------------------------------------------------------------------------------------
# Duktape c++ wrapper testing, CLI binary and development binary build.
# Requires GNU make version >= v4.0 and basic UNIX tools (for Windows
# simply install GIT globally, so that tools like rm.exe are in the PATH;
# Linux/BSD: no action needed).
#---------------------------------------------------------------------------------------------------

CXX=g++
LD=$(CXX)
CXXFLAGS=-std=c++11 -W -Wall -Wextra -pedantic
CXXFLAGS+=-Iduktape
DUKOPTS+=-std=c++11 -fstrict-aliasing -fdata-sections -ffunction-sections -Os -DDUK_USE_CPP_EXCEPTIONS
DUKTAPE_VERSION=2.1.0
DUKTAPE_ARCHIVE=duktape/duktape-releases/duktape-$(DUKTAPE_VERSION).tar.xz

#---------------------------------------------------------------------------------------------------

ifdef DEBUG
  CXXFLAGS+=-Os -g -fno-omit-frame-pointer -fdata-sections -ffunction-sections
  LDFLAGS+=-Os -g -fno-omit-frame-pointer -Wl,--gc-sections
#  CXXFLAGS+=-fsanitize=address
#  LDFLAGS+=-fsanitize=address
  LIBS+=-lm
  DUKOPTS+=-g
else
  STRIP=strip
  CXXFLAGS+=-Os -fomit-frame-pointer -fdata-sections -ffunction-sections
  LDFLAGS+=-Os -Wl,--gc-sections
  LIBS+=-lm
endif

CXXFLAGS+=$(FLAGS)

# Auto pick windows, other platforms are compatible
ifeq ($(wildcard /bin/*),)
OS=win
endif

ifeq ($(OS),win)
BINARY_EXTENSION=.exe
LDSTATIC+=-static -Os -s -static-libgcc
BINARY=js$(BINARY_EXTENSION)
LIBS+=-ladvapi32
else
BINARY_EXTENSION=.elf
BINARY=js
LIBS+=-lrt
ifdef STATIC
  LDSTATIC+=-static -Os -s -static-libgcc
endif
endif

DEVBINARY=dev$(BINARY_EXTENSION)
EXAMPLEBINARY=example$(BINARY_EXTENSION)
wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
TEST_RESULT_EXTENSION=.log
TEST_BINARIES:=$(foreach F, $(filter %/ , $(sort $(wildcard test/*/))), $Ftest$(BINARY_EXTENSION))
TEST_RESULTS:=$(patsubst %$(BINARY_EXTENSION),%$(TEST_RESULT_EXTENSION),$(TEST_BINARIES))
TEST_SOURCES:=$(patsubst %$(BINARY_EXTENSION),%.cc,$(TEST_BINARIES))
STDMOD_SOURCES:=$(sort $(call wildcardr, duktape/mod, *.hh))
HEADER_DEPS=duktape/duktape.hh $(STDMOD_SOURCES)
MAIN_TESTJS:=$(wildcard main.js)

#---------------------------------------------------------------------------------------------------
# make targets
#---------------------------------------------------------------------------------------------------
.PHONY: clean all binary bininfo run tests test test-run documentation jsdoc dev mrproper

binary: cli/$(BINARY)
all: clean binary bininfo test documentation jsdoc dev

ifeq ($(OS),win)
run: cli/$(BINARY)	; cli/$(BINARY) $(MAIN_TESTJS)
bininfo:
clean:			; @rm -f cli/$(BINARY) cli/$(DEVBINARY) cli/$(EXAMPLEBINARY) cli/*.o duktape/*.o *.o; rm -f $(TEST_BINARIES) $(TEST_RESULTS)
jsdoc:
else
run: cli/$(BINARY)	; ./cli/$(BINARY) $(MAIN_TESTJS)
bininfo: cli/$(BINARY)  ; @echo -n "[info] "; file cli/$(BINARY); echo -n "[info] "; ls -lh cli/$(BINARY)
clean:			; @rm -f cli/$(BINARY) cli/$(DEVBINARY) cli/*.o duktape/*.o *.o; rm -f $(TEST_BINARIES) $(TEST_RESULTS)
jsdoc: doc/stdmods.js
endif

documentation: jsdoc
mrproper: clean		; @rm -rf duktape/duktape-releases; rm -f duktape/*.h duktape/*.c

#---------------------------------------------------------------------------------------------------
# CLI example app

cli/$(BINARY): cli/main.o duktape/duktape.o
	@echo "[ld  ] $^  $@"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(LDSTATIC) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi
	@echo "[note] binary is $@"

cli/main.o: cli/main.cc $(HEADER_DEPS) $(TEST_SOURCES)
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(OPTS) -I.

#---------------------------------------------------------------------------------------------------
# Duktape base compilation
#---------------------------------------------------------------------------------------------------

duktape/duktape.o: duktape/duktape.c duktape/duk_config.h duktape/duktape.h
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(DUKOPTS)

#---------------------------------------------------------------------------------------------------
# Retrieving Duktape base sources from duktape.org or github
#---------------------------------------------------------------------------------------------------

ifeq ($(GET_DUKTAPE_RELEASE),y)
duktape/duk_config.h duktape/duktape.h duktape/duktape.c:
ifeq ($(GET_DUKTAPE_VIA_WGET),y)
	@if [ ! -f $(DUKTAPE_ARCHIVE) ]; then \
	  mkdir -p duktape/duktape-releases; \
	  echo "[wget] Downloading ($(DUKTAPE_ARCHIVE)) from duktape.org ..."; \
	  cd duktape/duktape-releases; \
	  wget -q http://duktape.org/$(notdir $(DUKTAPE_ARCHIVE)); \
	fi
else
	@if [ ! -f $(DUKTAPE_ARCHIVE) ]; then \
	  echo "[git ] Getting ($(DUKTAPE_ARCHIVE)) from github.com/svaarala/duktape-releases.git ..."; \
	  cd duktape; git clone --depth=1 https://github.com/svaarala/duktape-releases.git; \
	fi
endif
	@echo "[tar ] Extracting ..."
	@tar -xJf $(DUKTAPE_ARCHIVE) duktape-$(DUKTAPE_VERSION)/src/duk_config.h --to-command=cat > duktape/duk_config.h
	@tar -xJf $(DUKTAPE_ARCHIVE) duktape-$(DUKTAPE_VERSION)/src/duktape.h --to-command=cat > duktape/duktape.h
	@tar -xJf $(DUKTAPE_ARCHIVE) duktape-$(DUKTAPE_VERSION)/src/duktape.c --to-command=cat > duktape/duktape.c

duktape/duktape.hh: duktape/duktape.h duktape/duk_config.h
endif

#---------------------------------------------------------------------------------------------------
# Developer testing ground
#---------------------------------------------------------------------------------------------------
dev: cli/$(DEVBINARY)
	@echo "[note] Running development binary ..."
	@cd ./cli; ./$(DEVBINARY) dev.js

cli/$(DEVBINARY): cli/dev.o duktape/duktape.o
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(LDSTATIC) $(LDFLAGS) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

cli/dev.o: cli/dev.cc $(HEADER_DEPS) $(TEST_SOURCES)
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(OPTS) -I.
	@echo "[note] Development testing binary is $@"

#---------------------------------------------------------------------------------------------------
# Integration example
#---------------------------------------------------------------------------------------------------
example: cli/$(EXAMPLEBINARY)
	@echo "[note] Running example ..."
	@cd ./cli; ./$(EXAMPLEBINARY) arg1 arg2 arg2 etc

cli/$(EXAMPLEBINARY): cli/example.o duktape/duktape.o
	@echo "[ld  ] $^ $@"
	@$(CXX) -o $@ $^ $(LDSTATIC) $(LDFLAGS) $(LIBS)
	@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

cli/example.o: cli/example.cc $(HEADER_DEPS) $(TEST_SOURCES)
	@echo "[c++ ] $< $@"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(OPTS) -I.
	@echo "[note] Example binary is $@"

#---------------------------------------------------------------------------------------------------
# Tests
#---------------------------------------------------------------------------------------------------
test/%/test$(BINARY_EXTENSION): test/%/test.cc test/%/test.js duktape/duktape.o $(HEADER_DEPS)
	@echo "[c++ ] $@"
	-@$(CXX) -o $@ $< duktape/duktape.o $(CXXFLAGS) $(OPTS) $(LDFLAGS) $(LIBS) || echo "[fail] $@"
	-@if [ ! -z "$(STRIP)" ]; then $(STRIP) $@; fi

test/%/test$(TEST_RESULT_EXTENSION): test/%/test$(BINARY_EXTENSION) test/%/test.cc test/%/test.js
ifneq ($(OS),win)
	-@rm -f $@
	-@cd $(dir $<); ./$(notdir $<) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $<" || echo "[fail] $@"
else
	-@rm -f $@
	-@cd "$(dir $<)"; echo "" | "./$(notdir $<)" >$(notdir $@) && echo "[pass] $<" || echo "[fail] $@"
endif

tests: $(TEST_BINARIES)

test-run: $(TEST_BINARIES) $(TEST_RESULTS)

test: test-run

#---------------------------------------------------------------------------------------------------
# JS documentation (searching the mods for JSDOC preprocessor tags and collecting the contents.
#---------------------------------------------------------------------------------------------------

ifneq ($(OS),win)
doc/stdmods.js: $(STDMOD_SOURCES)
	@find duktape/mod/ -name '*.hh' -exec cat {} \; \
	| awk '/#if\(0 && JSDOC\)/,/#endif/' \
	| sed -e 's/^[\t ]\+#if.*$$//' -e 's/^[\t ]\+#endif.*$$//' -e 's/^  //' -e 's/[\t ]\+$$//' \
	> $@
endif

# EOF
#---------------------------------------------------------------------------------------------------
