#---------------------------------------------------------------------------------------------------
# Duktape c++ wrapper, example application makefile
#---------------------------------------------------------------------------------------------------
# Optional tool chain prefix path, sometimes also referred to as CROSSCOMPILE
TOOLCHAIN=
CXX=$(TOOLCHAIN)g++
LD=$(CXX)
FLAGSCXX=-std=c++17 -W -Wall -Wextra -pedantic
FLAGSCXX+=-I../../
DUKOPTS+=-std=c++17 -fstrict-aliasing -fdata-sections -ffunction-sections -Os -DDUK_USE_CPP_EXCEPTIONS
FLAGSCXX+=-Os -g -fno-omit-frame-pointer -fdata-sections -ffunction-sections
FLAGSLD+=-Os -g -fno-omit-frame-pointer -Wl,--gc-sections
LIBS+=-lm
DUKOPTS+=-g -O0
FLAGSCXX+=$(FLAGS)
FLAGSCXX+=$(CXXFLAGS)
FLAGSLD+=$(LDFLAGS)
ifeq ($(wildcard /bin/*),)
OS=win
endif
ifeq ($(OS),win)
BINARY_EXTENSION=.exe
LDSTATIC+=-static -Os -s -static-libgcc
FLAGSCXX+=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D_WIN32_IE=0x0900
LIBS+=-ladvapi32 -lshell32
else
BINARY_EXTENSION=.elf
LIBS+=-lrt
ifdef STATIC
  LDSTATIC+=-static -Os -s -static-libgcc
endif
endif
ifeq ($(WITH_DEFAULT_STRICT_INCLUDE),1)
  FLAGSCXX+=-DWITH_DEFAULT_STRICT_INCLUDE
endif
STDMOD_SOURCES:=$(sort $(call wildcardr, ../../duktape/mod, *.hh))
HEADER_DEPS=../../duktape/duktape.hh $(STDMOD_SOURCES)
BINARY:=example$(BINARY_EXTENSION)

#---------------------------------------------------------------------------------------------------
# make targets
#---------------------------------------------------------------------------------------------------
.PHONY: clean all binary run

all: binary | run

clean:
	@echo "[note] Cleaning"
	@rm -f $(BINARY) *.o *.elf *.exe

binary: $(BINARY)

run: binary
	@echo "[run ] $< ..."
	@echo "----------------------------------------------------"
	@./$(BINARY)
	@echo "----------------------------------------------------"

duktape.o: ../../duktape/duktape.c ../../duktape/duk_config.h ../../duktape/duktape.h
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(DUKOPTS)

example.o: example.cc $(HEADER_DEPS)
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I.

$(BINARY): duktape.o example.o
	@echo "[ld  ] $^  $@"
	@$(CXX) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)
