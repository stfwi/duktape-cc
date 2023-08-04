#---------------------------------------------------------------------------------------------------
# Sanitizing
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+= --no-print-directory
CLANG_CXX=$(firstword $(shell which clang++-16) $(shell which clang++-15) $(shell which clang++-14) $(shell which clang++) clang++-is-not-installed)
CLANG_FORMAT=$(firstword $(shell which clang-format-16) $(shell which clang-format-15) $(shell which clang-format-14) $(shell which clang-format) clang-format-is-not-installed)
CLANG_TIDY=$(firstword $(shell which clang-tidy-16) $(shell which clang-tidy-15) $(shell which clang-tidy-14) $(shell which clang-tidy) clang-tidy-is-not-installed)
SCAN_BUILD=$(firstword $(shell which scan-build-16) $(shell which scan-build-15) $(shell which scan-build-14) $(shell which scan-build) scan-build-is-not-installed)
CLANG_TIDY_CONFIG=$(abspath ./.clang-tidy)

wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
SANITIZE_SOURCES=$(call wildcardr,./cli,*.cc) $(call wildcardr,./duktape,*.hh) $(call wildcardr,./duktape,*.cc)
SANITIZE_TESTS_SOURCES=$(call wildcardr,./test,*.hh) $(call wildcardr,./test,*.cc)

#---------------------------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------------------------

.PHONY: sanitize
sanitize: code-format code-analysis

.PHONY: code-format code-analysis

#---------------------------------------------------------------------------------------------------
# Code formatting
#---------------------------------------------------------------------------------------------------

format-code: ./.clang-format
	@echo "[note] Formatting not enabled yet - format not yet quite what I like to have."
#	@$(CLANG_FORMAT) -i -style=file $(SANITIZE_SOURCES)
#	@$(CLANG_FORMAT) -i -style=file $(SANITIZE_TESTS_SOURCES)

#---------------------------------------------------------------------------------------------------
# Code analysis
#---------------------------------------------------------------------------------------------------

code-analysis:
	@echo "[note] Running clang-tidy on c++ files ..."
	@rm -rf build
	@mkdir -p build
	@$(CLANG_TIDY) --config-file=$(CLANG_TIDY_CONFIG) -p=build ./cli/main.cc -- $(CXXFLAGS) -I. $(INCLUDES) >build/clang-tidy--cli.log 2>&1
	@$(CLANG_TIDY) --config-file=$(CLANG_TIDY_CONFIG) -p=build ./doc/examples/basic-integration/example.cc -- $(CXXFLAGS) -I. $(INCLUDES) >build/clang-tidy--basic-integration.log 2>&1
	@$(CLANG_TIDY) --config-file=$(CLANG_TIDY_CONFIG) -p=build ./doc/examples/native-class-wrapping/example.cc -- $(CXXFLAGS) -I. $(INCLUDES) >build/clang-tidy--native-class-wrapping.log 2>&1
	@echo "[note] Running scan-build ..."
	@mkdir -p build/scanbuild
	@$(SCAN_BUILD) -v -o build/scanbuild \
		--exclude duktape/duktape.c --exclude duktape/duktape.h --exclude duktape/duk_config.h \
		-enable-checker core.DivideZero \
		-enable-checker core.UndefinedBinaryOperatorResult \
		-enable-checker core.uninitialized.ArraySubscript \
		-enable-checker core.uninitialized.Assign \
		-enable-checker core.uninitialized.Branch \
		-enable-checker core.uninitialized.CapturedBlockVariable \
		-enable-checker core.uninitialized.UndefReturn \
		-enable-checker core.CallAndMessage \
		-enable-checker core.UndefinedBinaryOperatorResult \
		-enable-checker core.VLASize \
		-enable-checker deadcode.DeadStores \
		-enable-checker security.FloatLoopCounter \
		-enable-checker security.insecureAPI.UncheckedReturn \
		-enable-checker security.insecureAPI.getpw \
		-enable-checker security.insecureAPI.gets \
		-enable-checker security.insecureAPI.mkstemp \
		-enable-checker security.insecureAPI.mktemp \
		-enable-checker security.insecureAPI.rand \
		-enable-checker security.insecureAPI.strcpy \
		-enable-checker security.insecureAPI.vfork \
		-enable-checker unix.API \
		-enable-checker unix.Malloc \
		-enable-checker unix.cstring.BadSizeArg \
		-enable-checker unix.cstring.NullArg \
		make binary CXX=$(CLANG_CXX) DEBUG=1 CXXFLAGS="-DCODE_ANALYSIS=1"
