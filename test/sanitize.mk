#---------------------------------------------------------------------------------------------------
# Sanitizing
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+= --no-print-directory
CLANG_FORMAT=$(firstword $(shell which clang-format-16) $(shell which clang-format-15) $(shell which clang-format-14) $(shell which clang-format) clang-format-is-not-installed)
CLANG_TIDY=$(firstword $(shell which clang-tidy-16) $(shell which clang-tidy-15) $(shell which clang-tidy-14) $(shell which clang-tidy) clang-tidy-is-not-installed)

wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
SANITIZE_SOURCES=$(call wildcardr,./cli,*.cc) $(call wildcardr,./duktape,*.hh) $(call wildcardr,./duktape,*.cc)
SANITIZE_TESTS_SOURCES=$(call wildcardr,./test,*.hh) $(call wildcardr,./test,*.cc)

#---------------------------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------------------------

.PHONY: sanitize
sanitize: format-code

.PHONY: format-code
format-code: ./.clang-format
	@echo "[note] Formatting not enabled yet - format not yet quite what I like to have."
#	@$(CLANG_FORMAT) -i -style=file $(SANITIZE_SOURCES)
#	@$(CLANG_FORMAT) -i -style=file $(SANITIZE_TESTS_SOURCES)
