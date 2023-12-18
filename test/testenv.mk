#---------------------------------------------------------------------------------------------------
# Test config and selection
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+= --no-print-directory
MICROTEST_ROOT=test

TEST_CLI_ARGS=
TEST_SELECTION:=$(sort $(wildcard test/*$(TEST)*/))
TEST_SCRIPT_ARGS=
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
# Tests
#---------------------------------------------------------------------------------------------------
.PHONY: test test-clean test-results

test-clean:
	@rm -rf $(BUILDDIR)/test

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
test-results: $(TEST_SCRIPT_BINARY) $(TEST_BINARIES) $(TEST_BINARIES_AUXILIARY) $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

# Test binaries (compile test No < 1000)
$(BUILDDIR)/test/%/test$(BINARY_EXTENSION): test/%/test.cc test/testenv.hh $(MICROTEST_ROOT)/microtest.hh $(BUILDDIR)/duktape/duktape.o $(HEADER_DEPS)
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
	@cd $(dir $@); ../../../$(TEST_SCRIPT_BINARY) $(TEST_SCRIPT_ARGS) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $@" || echo "[fail] $@"
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

# Coverage main target (the one we invoke)
coverage:
	@$(MAKE) clean
	@$(MAKE) --jobs=1 coverage-runs WITH_COVERAGE=1
	@$(MAKE) coverage-files
	@$(MAKE) coverage-summary

# Selection of tests where c++ coverage makes sense
coverage-runs: $(TEST_BINARIES_AUXILIARY) $(TEST_BINARIES_RESULTS) $(TEST_SCRIPT_RESULTS)

# All gcov files from the executed tests
coverage-files: $(patsubst %/test.log,%/test.gcov,$(TEST_BINARIES_RESULTS))

# Analysis of one test using gcov/lcov
$(BUILDDIR)/test/%/test.gcov: $(BUILDDIR)/test/%/test.log
	@echo "[gcov] $*"
	@cd $(dir $<) && gcov --source-prefix "$(CURDIR)" *.gcno > gcov.log 2>&1
	@cd $(dir $<) && gcov -m *.cc >> gcov.log 2>&1
	@[ ! -z "$(shell which lcov)" ] || echo "[warn] lcov is not installed."
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
.PHONY: show-vars

show-vars:
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
