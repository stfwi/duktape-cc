#---------------------------------------------------------------------------------------------------
# Application attachment patch auxiliary binary.
#---------------------------------------------------------------------------------------------------
# Optional tool chain prefix path, sometimes also referred to as CROSSCOMPILE
TOOLCHAIN=
HOST_CXX=$(TOOLCHAIN)g++
MAKEFLAGS+=--no-print-directory --output-sync=target
# Set in the command line
TARGET_BINARY=

# Auto pick windows, other platforms are compatible
ifeq ($(OS),Windows_NT)
 PATCH_AUX_BINARY:=app_attachment_bin.exe
else
 PATCH_AUX_BINARY:=app_attachment_bin
endif

.PHONY: patch-binary clean
#---------------------------------------------------------------------------------------------------

patch-binary: $(PATCH_AUX_BINARY) | $(TARGET_BINARY)
	@./$(PATCH_AUX_BINARY) $(TARGET_BINARY)

clean:
	@rm -f $(PATCH_AUX_BINARY) *.exe *.elf

$(PATCH_AUX_BINARY): app_attachment.cc
	@echo "[c++ ] $<  $@"
	@$(HOST_CXX) -o $@ $< -std=c++14 -DAPP_ATTACHMENT_PATCHING_BINARY
