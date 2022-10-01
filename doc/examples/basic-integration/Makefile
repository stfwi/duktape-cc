#---------------------------------------------------------------------------------------------------
# Duktape c++ wrapper, example application makefile
#---------------------------------------------------------------------------------------------------
# Optional tool chain prefix path, sometimes also referred to as CROSSCOMPILE
TOOLCHAIN=
CXX=$(TOOLCHAIN)g++
LD=$(CXX)
FLAGSCXX=-std=c++17 -W -Wall -Wextra -pedantic -I../../../ -Os -fno-omit-frame-pointer -fdata-sections -ffunction-sections
DUKOPTS+=-std=c++17 -fstrict-aliasing -fdata-sections -ffunction-sections -Os -DDUK_USE_CPP_EXCEPTIONS
FLAGSLD+=-Os -fno-omit-frame-pointer -Wl,--gc-sections

ifeq ($(OS),Windows_NT)
 BINARY_EXTENSION=.exe
 LDSTATIC+=-static -Os -s -static-libgcc
 FLAGSCXX+=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D_WIN32_IE=0x0900
 LIBS+=-lm -ladvapi32 -lshell32
else
 BINARY_EXTENSION=.elf
 LIBS+=-lm -lrt
endif

BINARY:=example$(BINARY_EXTENSION)

#---------------------------------------------------------------------------------------------------
# make targets
#---------------------------------------------------------------------------------------------------
.PHONY: clean all binary run

all: binary | run

binary: $(BINARY)

clean:
	@rm -f $(BINARY) *.o *.elf *.exe

run: binary
	@echo "[run ] $< ..."
	@echo "----------------------------------------------------"
	@./$(BINARY)
	@echo "----------------------------------------------------"

$(BINARY): duktape.o example.o
	@echo "[ld  ] $^  $@"
	@$(LD) -o $@ $^ $(FLAGSLD) $(LDSTATIC) $(LIBS)

duktape.o: ../../../duktape/duktape.c ../../../duktape/duk_config.h ../../../duktape/duktape.h
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(DUKOPTS)

example.o: example.cc
	@echo "[c++ ] $<  $@"
	@$(CXX) -c -o $@ $< $(FLAGSCXX) $(OPTS) -I.
