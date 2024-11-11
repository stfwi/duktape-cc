/**
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * @version 1.3
 * @platform linux, bsd, windows
 * @standard c++11*,c++14,c++17,c++20
 * @note MSVC c++11 deprecated
 *
 * -----------------------------------------------------------------------------
 *
 * Micro-test class template, used for unit testing where tests are separately
 * compiled main files containing test cases.
 *
 * -----------------------------------------------------------------------------
 * Example:
 *
 * #include <microtest.hh>
 * #include <sstream>
 * #include <vector>
 * #include <deque>
 * #include <list>
 *
 * using namespace std;
 * using namespace sw;
 *
 * // Auxiliary function that throws a runtime error
 * bool throws_exception(bool e)
 * {
 *   if(e) throw std::runtime_error("Bad something");
 *   return false;
 * }
 *
 * // Auxiliary forward iterable container --> string
 * // Not in the microtest harness, as overrides may
 * // collide with existing to_string() implementations.
 * template <typename ContainerType>
 * string to_string(ContainerType c)
 * {
 *   if(c.empty()) return "[]";
 *   stringstream s;
 *   auto it = c.begin();
 *   s << "[ " << *it;
 *   for(++it; it != c.end(); ++it) { s << ", " << *it; }
 *   s << " ]";
 *   return s.str();
 * }
 *
 * //
 * // Example test main function (for `int main()` see below).
 * //
 * void test(const vector<string>& args)
 * {
 *   // Optional, only here for convenience.
 *   using namespace std;
 *   using namespace sw::utest;
 *
 *   // Comments / informational test log output.
 *   char a = 'a';
 *   string s = "test";
 *   test_info("This is ", a, " ", s, "."); // variadic template based information
 *   test_note("This is " << a << " " << s << "."); // ostream based information
 *   test_info("Test args are:", to_string(args));
 *
 *   // Basic checks
 *   test_expect(1 == 0);  // Will fail
 *   test_expect(1 == 1);  // Will pass
 *   test_expect_eq(0, 0); // Check equal, will pass
 *   test_expect_eq(0, 1); // Check equal, will fail
 *   test_expect_ne(1, 0); // Check not equal, will pass
 *   test_expect_ne(1, 1); // Check not equal, will fail
 *
 *   // Exception
 *   test_expect( throws_exception(true) );  // Will fail due to exception
 *   test_expect_except( throws_exception(true) ); // Will pass, expected exception
 *   test_expect_noexcept( throws_exception(false) ); // Will pass, no exception expected
 *
 *   // Random string
 *   test_info("Random string: ", test_random<string>(10) );
 *
 *   // Random arithmetic
 *   test_info("Random char: ",   int(test_random<char>()) );
 *   test_info("Random uint8: ",  int(test_random<std::uint8_t>(100)) );
 *   test_info("Random double: ", test_random<double>() ); // 0 to 1
 *   test_info("Random double: ", test_random<double>(10) ); // 0 to 10
 *   test_info("Random double: ", test_random<double>(-1, 1) ); // -1 to 1
 *
 *   // Random forward iterable container with push_back()
 *   test_info("Random vector<double>: ", to_string(test_random<std::vector<double>>(5, 0, 1)));
 *   test_info("Random deque<int>: ", to_string(test_random<std::deque<int>>(5, -100, 100)));
 *   test_info("Random list<unsigned>: ", to_string(test_random<std::list<unsigned>>(5, 1, 20)));
 *
 *   // Test state information
 *   test_info("Number of checks already done: ", test::num_checks() );
 *   test_info("Number of fails already had: ", test::num_fails() );
 *   test_info("Number of passes already had: ", test::num_passed() );
 *   test_info("Number of warnings up to now: ", test::num_warnings() );
 *
 *   // Optionally reset the example, as we have intentionally generated some FAILs.
 *   // -> test_reset();
 *
 *   // Test summary in main, no return value or statement required here.
 *   return;
 * }
 *
 * ---------------------------------------------------------------------------
 * The output of this is:
 *
 * [info] [@test/example/test.cc:119] compiler: gcc ..., std=c++..., platform: ..., scm=...
 * [note] [@test/example/test.cc:52] This is a test.
 * [note] [@test/example/test.cc:53] This is a test.
 * [note] [@test/example/test.cc:54] Test args are: []
 * [fail] [@test/example/test.cc:57] 1 == 0
 * [pass] [@test/example/test.cc:58] 1 == 1
 * [pass] [@test/example/test.cc:59] 0 == 0   (=0)
 * [fail] [@test/example/test.cc:60] 0 == 1   (0 != 1)
 * [pass] [@test/example/test.cc:61] 1 != 0   (1 != 0)
 * [fail] [@test/example/test.cc:62] 1 != 1   (both =1)
 * [fail] [@test/example/test.cc:65] throws_exception(true) | Unexpected exception: Bad something
 * [pass] [@test/example/test.cc:66] throws_exception(true) | Expected exception: Bad something
 * [pass] [@test/example/test.cc:67] throws_exception(false)
 * [note] [@test/example/test.cc:70] Random string: AE$JPJ^{^v
 * [note] [@test/example/test.cc:73] Random char: -72
 * [note] [@test/example/test.cc:74] Random uint8: 30
 * [note] [@test/example/test.cc:75] Random double: 0.155995
 * [note] [@test/example/test.cc:76] Random double: 8.50354
 * [note] [@test/example/test.cc:77] Random double: 0.908896
 * [note] [@test/example/test.cc:80] Random vector<double>: [ 0.853178, 0.249431, 0.95963, 0.331717, 0.569931 ]
 * [note] [@test/example/test.cc:81] Random deque<int>: [ -27, 26, -97, 65, 48 ]
 * [note] [@test/example/test.cc:82] Random list<unsigned>: [ 12, 12, 16, 14, 20 ]
 * [note] [@test/example/test.cc:85] Number of checks already done: 9
 * [note] [@test/example/test.cc:86] Number of fails already had: 4
 * [note] [@test/example/test.cc:87] Number of passes already had: 5
 * [note] [@test/example/test.cc:88] Number of warnings up to now: 0
 * [FAIL] 4 of 9 checks failed, 0 warnings.        <-- summary
 *
 * -----------------------------------------------------------------------------
 * +++ BSD license header +++
 * Copyright (c) 2012-2023, Stefan Wilhelm (stfwi, <cerbero s@atwilly s.de>)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met: (1) Redistributions
 * of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer. (2) Redistributions in binary form must reproduce
 * the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 * (3) Neither the name of atwillys.de nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * AND CONTRIBUTORS "AS IS" AND VAR EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR VAR DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON VAR THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN VAR
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * -----------------------------------------------------------------------------
 */
#ifndef SW_MICROTEST_HH
#define SW_MICROTEST_HH

#include <sstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <limits>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <atomic>
#include <mutex>
#include <random>
#if defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
  #include <windows.h>
  #ifdef _MSC_VER
    #include <io.h>
  #else
    #include <sys\stat.h>
  #endif
  #ifndef __WINDOWS__
    #define __WINDOWS__
  #endif
  #ifdef min
    #undef min
  #endif
  #ifdef max
    #undef max
  #endif
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <time.h>
#endif

//------------------------------------------------------------------------------------------
// Compiler switches
//------------------------------------------------------------------------------------------

// Use ANSI colors for TTY output (fail red, warn yellow, pass green, etc.)
#ifdef WITH_ANSI_COLORS
  #define MICROTEST_UTEST_ANSI_COLORS (true)
#else
  #define MICROTEST_UTEST_ANSI_COLORS (false)
#endif

// Don't log passes, only infos, warnings, and fails.
#ifdef MICROTEST_WITHOUT_PASS_LOGS
  #define MICROTEST_UTEST_OMIT_PASS_LOGS (true)
#else
  #define MICROTEST_UTEST_OMIT_PASS_LOGS (false)
#endif

// Enable generating temp files/directories (auto deleted).
// @experimental
#ifndef WITH_MICROTEST_TMPFILE
  #ifdef MICROTEST_UTEST_TMPDIR
    #undef MICROTEST_UTEST_TMPDIR
  #endif
#else
  #ifndef MICROTEST_UTEST_TMPDIR
    #define MICROTEST_UTEST_TMPDIR ""
  #endif
#endif

// Further compile time settings (meanings see below in the file):
// - #define WITH_MICROTEST_MAIN
// - #define WITH_MICROTEST_GENERATORS
// - #define WITHOUT_MICROTEST_RANDOM

//------------------------------------------------------------------------------------------
// The ugly macro part (needed to reflect the code line)
// Macros are lower case with the hope that some day these can be actual function templates (->static reflection).
//------------------------------------------------------------------------------------------

/**
 * Register a failed check with message (arguments implicitly printed space separated).
 * @tparam typename ...Args
 * @return void
 */
#define test_fail(...) (::sw::utest::test::fail(__FILE__, __LINE__, __VA_ARGS__))

/**
 * Register a passed check with message (arguments implicitly printed space separated).
 * @tparam typename ...Args
 * @return void
 */
#define test_pass(...) (::sw::utest::test::pass(__FILE__, __LINE__, __VA_ARGS__))

/**
 * Register a check warning (arguments implicitly printed space separated).
 * @tparam typename ...Args
 * @return void
 */
#define test_warn(...) ::sw::utest::test::warning(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Print information without registering a check (arguments implicitly printed space separated).
 * @tparam typename ...Args
 * @return void
 */
#define test_info(...) ::sw::utest::test::comment(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Print information without registering a check (arguments implicitly printed space separated).
 * Alias of `test_info(...)`.
 * @tparam typename ...Args
 * @return void
 */
#define test_comment(...) ::sw::utest::test::comment(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Print information without registering a check (argument stream like).
 * e.g.: test_note("Check loop " << long(current_iteration) << " ...");
 * @tparam typename ...Args
 * @return void
 */
#define test_note(...) { std::stringstream ss_ss; ss_ss << __VA_ARGS__; ::sw::utest::test::comment(__FILE__, __LINE__, ss_ss.str()); }

/**
 * Initialize the test run, print build context information, and
 * conditionally enable ANSI coloring if allowed (WITH_ANSI_COLORS
 * defined and the output stream STDOUT is a TTY/console).
 * @see `WITH_MICROTEST_MAIN`
 */
#define test_initialize() { \
  ::sw::utest::test::ansi_colors((MICROTEST_UTEST_ANSI_COLORS) && ::sw::utest::test::istty()); \
  ::sw::utest::test::buildinfo(__FILE__, __LINE__); \
}

/**
 * Print build context information.
 * @see `WITH_MICROTEST_MAIN`
 * @return void
 */
#define test_buildinfo() (::sw::utest::test::buildinfo(__FILE__, __LINE__))

/**
 * Prints the summary of all test (statistics since the last `test_reset()`
 * or the beginning). Returns a suitable exit code for the `int main(){}`,
 * indicating test success (exit code 0) or fail (exit code >0).
 * @see `WITH_MICROTEST_MAIN`
 * @return int
 */
#define test_summary() (::sw::utest::test::summary())

/**
 * Reset the test statistics (number of fails, passes, and warnings).
 * @return void
 */
#define test_reset() {::sw::utest::test::reset(__FILE__, __LINE__);}

/**
 * Checks (register pass/fail) and print the file+line, as well as the
 * expression formulating the check. Does not catch exceptions.
 * Legacy alias of `test_expect_nocatch()`.
 * @param BoolExpr...
 * @return bool
 */
#define test_expect_cond(...) ::sw::utest::test::commit(bool(__VA_ARGS__), __FILE__, __LINE__, #__VA_ARGS__)

/**
 * Checks (register pass/fail) without printing the line
 * (use for highly iterated/nested check sequences).
 * Does not catch exceptions (test run aborted).
 * @see ::sw::utest::test::omit_pass_log(bool enable)
 * @param BoolExpr...
 * @return bool
 */
#define test_expect_cond_silent(...) ::sw::utest::test::commit(bool(__VA_ARGS__));

/**
 * Checks (register pass/fail) and print the file+line, as well as the
 * expression formulating the check. Does not catch exceptions (abort).
 * @param BoolExpr...
 * @return bool
 */
#define test_expect_nocatch(...) ::sw::utest::test::commit(bool(__VA_ARGS__), __FILE__, __LINE__, #__VA_ARGS__)

/**
 * Checks (register pass/fail) and print the file+line, as well as the
 * expression formulating the check. Fails on exception.
 * @param BoolExpr...
 * @return bool
 */
#define test_expect(...) { \
  try { \
    (void)test_expect_cond(__VA_ARGS__); \
  } catch(const std::exception& e) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception: ") + e.what() )); \
  } catch(...) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception: ") )); \
  } \
}

/**
 * Checks (register pass/fail) without printing the line
 * (use for highly iterated/nested check sequences).
 * @param BoolExpr...
 * @return bool
 */
#define test_expect_silent(...) { \
  try { \
    if(__VA_ARGS__) ::sw::utest::test::pass(); \
    else ::sw::utest::test::fail(__FILE__, __LINE__, #__VA_ARGS__); \
  } catch(const std::exception& e) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception: ") + e.what() )); \
  } catch(...) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception: ") )); \
  } \
}

/**
 * Registers a passed check if `A==B`, a failed check on `!(A==B)` (operator==()
 * match), and prints the file+line, the expression, and the value information
 * accordingly.
 * @tparam T1
 * @tparam T2
 * @param T1&& A
 * @param T1&& B
 * @return bool
 */
#define test_expect_eq(A, B) ::sw::utest::test::check_eq(A, B, __FILE__, __LINE__, #A, #B)

/**
 * Registers a passed check if `A!=B`, a failed check on `!(A!=B)` (operator!=()
 * match), and prints the file+line, the expression, and the value information
 * accordingly.
 * @tparam T1
 * @tparam T2
 * @param T1&& A
 * @param T1&& B
 * @return bool
 */
#define test_expect_ne(A, B) ::sw::utest::test::check_ne(A, B, __FILE__, __LINE__, #A, #B)

/**
 * Registers a passed check if the given expression throws,
 * otherwise a failed check is registered. The result value
 * of the expression itself is ignored.
 * @param BoolExpr...
 * @return void
 */
#define test_expect_except(...) { \
  try { \
    (__VA_ARGS__); \
    (::sw::utest::test::fail(__FILE__, __LINE__, #__VA_ARGS__, " | Exception was expected" )); \
  } catch(const std::exception& e) { \
    (::sw::utest::test::pass(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Expected exception: ") + e.what() )); \
  } catch(...) { \
    (::sw::utest::test::pass(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Expected exception") )); \
  } \
}

/**
 * Registers a failed check if the given expression throws,
 * otherwise a passed check is registered. The result value
 * of the expression itself is ignored.
 * @param BoolExpr...
 * @return void
 */
#define test_expect_noexcept(...) { \
  try { \
    ;(__VA_ARGS__); \
    (::sw::utest::test::pass(__FILE__, __LINE__, #__VA_ARGS__)); \
  } catch(const std::exception& e) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception: ") + e.what() )); \
  } catch(...) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #__VA_ARGS__ " | Unexpected exception") )); \
  } \
}

/**
 * Registers a pass if `ARG` is in the tolerance `TOL` around
 * zero: -TOL <= ARG <= TOL.
 * @deprecated since v1.2
 * @tparam T1
 * @tparam T2
 * @param T&& ARG
 * @param T&& TOL
 * @return void
 */
#define test_expect_diff_in_tolerance(ARG, TOL) { \
  try { \
    const auto r = std::abs(ARG); \
    if((r < (TOL))  && (r > (-(TOL)))) { \
      ::sw::utest::test::pass(__FILE__, __LINE__, std::string("In tolerance abs(" #ARG "), ") + std::to_string(r) + " < " + std::to_string(TOL) ); \
    } else { \
      ::sw::utest::test::fail(__FILE__, __LINE__, std::string("In tolerance abs(" #ARG "), ") + std::to_string(r) + " >=" + std::to_string(TOL) ); \
    } \
  } catch(const std::exception& e) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( #ARG "-> Unexpected exception: ") + e.what()) ); \
  } catch(...) { \
    (::sw::utest::test::fail(__FILE__, __LINE__, std::string( #ARG " | Unexpected exception") )); \
  } \
} \

/**
 * Random value or container generation (uniform distribution).
 * - random<double>()         : double between 0..1
 * - random<long double>(10)  : long double between 0..10
 * - random<float>(-1, 1)     : float between -1 and 1
 * - random<int>(100)         : integer between 0 and 100
 * - random<int>()            : integer between INT_MIN and INT_MAX
 * - random<string>(10)       : std::string with 10 characters, values ASCII, between space (32) and '~' (126).
 * - random<vector<double>>(5,0,1): double vector with 5 entries and a value range between 0 and 1.
 * - random<deque<int>>(5,0,1):  deque with 5 entries, int type, and a value range between 0 and 1.
 */
#ifndef WITHOUT_MICROTEST_RANDOM
  #define test_random ::sw::utest::random
#endif

//------------------------------------------------------------------------------------------
// Detail
//------------------------------------------------------------------------------------------

/**
 * Auxiliary functions.
 */
namespace sw { namespace utest {

  /**
   * Floating point rounding to a desired
   * accuracy, e.g. `round(doubleval, 1e-3)`.
   * @tparam typename FloatingPoint
   * @tparam typename AccuracyType
   * @param FloatingPoint floating_point_value
   * @param AccuracyType accuracy
   * @return FloatingPoint
   */
  template <typename FloatingPoint, typename AccuracyType>
  static inline FloatingPoint round(const FloatingPoint floating_point_value, AccuracyType accuracy)
  {
    static_assert(std::is_floating_point<FloatingPoint>::value, "Rounding only for floating point values");
    static_assert(std::is_floating_point<AccuracyType>::value, "Accuracy type must be floating point, to prevent interpreting it as 'number of digits'.");
    if(accuracy <= AccuracyType(0)) return floating_point_value;
    return std::round(floating_point_value / FloatingPoint(accuracy)) * FloatingPoint(accuracy);
  }

  /**
   * Floating point to_string() with number of precision digits.
   * @tparam typename FloatingPoint
   * @param const FloatingPoint val
   * @param size_t precision_digits
   * @return std::string
   */
  template <typename FloatingPoint>
  static inline std::string to_string(const FloatingPoint val, size_t precision_digits)
  {
    auto ss = std::stringstream(); // --> std::format(). For compat still iostream.
    ss.precision(precision_digits);
    ss << val;
    return ss.str();
  }

}}

/**
 * Build information.
 */
namespace sw { namespace utest {

  namespace detail {

    template <typename=void>
    struct buildinfo
    {
      /**
       * Print build information
       */
      static std::string info() noexcept
      {
        std::stringstream ss;
        ss << "compiler: " << compiler() << ", std=" << compilation_standard() << ", platform: " << platform();
        #ifdef SCM_COMMIT
          // Compiler CLI using e.g. in Makefile: -DSCM_COMMIT='"""$(SCM_COMMIT)"""'
          // ... where e.g. ...
          // SCM_COMMIT:=$(shell git log --pretty=format:%h -1 2>/dev/null || echo 0000000)
          ss << ", scm=" << (SCM_COMMIT);
        #endif
        return ss.str();
      }

      /**
       * Returns the c++ standard, e.g. "c++11"
       * @return const char*
       */
      static constexpr const char* compilation_standard() noexcept
      {
        #if (__cplusplus >= 202600L)
        return "c++26";
        #elif (__cplusplus >= 202300L)
        return "c++23";
        #elif (__cplusplus >= 202000L)
        return "c++20";
        #elif (__cplusplus >= 201700L)
        return "c++17";
        #elif (__cplusplus >= 201400L)
        return "c++14";
        #elif (__cplusplus >= 201100L)
        return "c++11";
        #else
        return "c++98";
        #endif
      }

      /**
       * Returns the compiler, if identified
       * @return constexpr const char*
       */
      static constexpr const char* compiler() noexcept
      {
        #define str(x) #x
        #define s(x) str(x)
        #if defined (__GNUC__)
        #define comp "gcc (" s(__GNUC__) "." s(__GNUC_MINOR__) "." s(__GNUC_PATCHLEVEL__) ")"
        #elif defined (__clang__)
        #define comp "clang (" s(__clang_major__) "." s(__clang_minor__) "." s(__clang_patchlevel__) ")"
        #elif defined (_MSC_VER)
        #define comp "visual-studio (" s(_MSC_FULL_VER) ")"
        #elif defined (__MINGW32__)
        #define comp "mingw32 (" s(__MINGW32_MAJOR_VERSION) "." s(__MINGW32_MINOR_VERSION) ")"
        #elif defined(__MINGW64__)
        #define comp "mingw64 (" s(__MINGW64_MAJOR_VERSION) "." s(__MINGW64_MINOR_VERSION) ")"
        #elif defined (__INTEL_COMPILER)
        #define comp "intel (" s(__INTEL_COMPILER) ")"
        #else
        #define comp "unknown compiler"
        #endif
        return comp;
        #undef str
        #undef s
        #undef comp
      }

      /**
       * Returns the compiler, if identified
       * @return constexpr const char*
       */
      static constexpr const char* platform() noexcept
      {
        #if defined(linux) || defined(__linux) || defined(__linux__)
        return "linux";
        #elif defined(__NetBSD__)
        return "netbsd";
        #elif defined(__FreeBSD__)
        return "freebsd";
        #elif defined(__OpenBSD__)
        return "openbsd";
        #elif defined(__DragonFly__)
        return "fragonfly";
        #elif defined(__MACOSX__) || (defined(__APPLE__) && defined(__MACH__))
        return "macosx";
        #elif defined(_BSD_SOURCE) || defined(_SYSTYPE_BSD)
        return "bsd";
        #elif defined(WIN32) || defined(_WIN32) || defined(__TOS_WIN__) || defined(_MSC_VER)
        return "windows";
        #elif defined(unix) || defined(__unix) || defined(__unix__)
        return "unix"; // "unix compatible"
        #else
        return "(unknown)"
        #endif
      }

      /**
       * True if compiled on windows
       * @return
       */
      static constexpr bool is_windows() noexcept
      {
        #if defined _MSC_VER || defined __MINGW32__ || defined __MINGW64__
        return true;
        #else
        return false;
        #endif
      }
    };
  }

  using buildinfo = detail::buildinfo<>;

}}

/**
 * Main `test` registration and logging, static.
 */
namespace sw { namespace utest {

  namespace detail {

    template <typename=void>
    class microtest
    {
    public:

      /**
       * Returns the number of failed checks.
       * @return unsigned long
       */
      static unsigned long num_fails() noexcept
      { return num_fails_; }

      /**
       * Returns the number of warnings.
       * @return unsigned long
       */
      static unsigned long num_warnings() noexcept
      { return num_warns_; }

      /**
       * Returns the number of warnings.
       * @return unsigned long
       */
      static unsigned long num_passed() noexcept
      { return num_checks_-num_fails_; }

      /**
       * Returns the number of checks
       * @return unsigned long
       */
      static unsigned long num_checks() noexcept
      { return num_checks_; }

      /**
       * Set the output stream for the testing
       * @param std::ostream& os
       */
      static void stream(std::ostream& os) noexcept
      { os_ = &os; }

      /**
       * Returns true if ANSI color printing is allowed
       * for TTY STDOUT.
       * @return bool
       */
      static bool ansi_colors() noexcept
      { return ansi_colors_; }

      /**
       * Sets if ANSI color printing is allowed
       * for TTY STDOUT.
       * @param bool enable
       */
      static void ansi_colors(bool enable) noexcept
      {
        ansi_colors_ = enable;
        #ifdef __WINDOWS__
        {
          #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
            #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
          #endif
          const HANDLE hout = ::GetStdHandle(STD_OUTPUT_HANDLE);
          DWORD conmode = 0;
          if(hout && ::GetConsoleMode(hout, &conmode)) {::SetConsoleMode(hout, conmode|ENABLE_VIRTUAL_TERMINAL_PROCESSING);}
        }
        #endif
      }

      /**
       * Register a succeeded expectation, increases test counter, prints message.
       * @param const std::string& file
       * @param int line
       * @param typename ...Args args
       */
      template <typename ...Args>
      static bool pass(const std::string& file, int line, Args&& ...args)
      { num_checks_++; if(!omit_passes_) osout(osout_pass, file, line, std::forward<Args>(args)...); return true; }

      /**
       * Register pass without logging
       * @return bool
       */
      static bool pass() noexcept
      { num_checks_++; return true; }

      /**
       * Register a failed expectation, increase test counter and fail counter, prints message
       * @param const std::string& file
       * @param int line
       * @param typename ...Args args
       */
      template <typename ...Args>
      static bool fail(const std::string& file, int line, Args&& ...args)
      { num_checks_++; num_fails_++; osout(osout_fail, file, line, std::forward<Args>(args)...); return false; }

      /**
       * Register fail without logging
       * @return bool
       */
      static bool fail() noexcept
      { num_checks_++; num_fails_++; return false; }

      /**
       * Register a check result, optionally tests are
       * not logged using the `silent` flag.
       * Note that `file` is not `nullptr` checked and
       * expected to be always `__FILE__`.
       * @tparam typename ...Args
       * @param bool passed
       * @param const char* file
       * @param int line
       * @param bool silent
       * @param Args ...args
       * @return bool
       */
      template <typename ...Args>
      static bool commit(bool passed, const char* file, int line, Args&& ...args)
      {
        if(omit_passes_) {
          return (passed) ? pass() : fail(file, line, std::forward<Args>(args)...);
        } else {
          return (passed) ? pass(file, line, std::forward<Args>(args)...) : fail(file, line, std::forward<Args>(args)...);
        }
      }

      /**
       * Register a check result without logging.
       * @param bool passed
       * @return bool
       */
      static bool commit(bool passed) noexcept
      { return (passed) ? pass() : fail(); }

      /**
       * Passes if operator==() yields true, fails otherwise.
       * Note: Expects `a_code` and `b_code` to be guaranteed non-`nullptr`.
       */
      template<typename T1, typename T2>
      static bool check_eq(const T1& a, const T2& b, const char* file, int line, const char* a_code, const char* b_code)
      {
        using namespace std;
        if(a == b) {
          return pass(file, line, std::string(a_code), " == ", std::string(b_code), "   (=", a, ")");
        } else {
          return fail(file, line, std::string(a_code), " == ", std::string(b_code), "   (", a, " != ",  b, ")");
        }
      }

      /**
       * Passes if operator!=() yields true, fails otherwise.
       * Note: Expects `a_code` and `b_code` to be guaranteed non-`nullptr`.
       */
      template<typename T1, typename T2>
      static bool check_ne(const T1& a, const T2& b, const char* file, int line, const char* a_code, const char* b_code)
      {
        using namespace std;
        if(a != b) {
          return pass(file, line, std::string(a_code), " != ", std::string(b_code), "   (", a, " != ", b, ")");
        } else {
          return fail(file, line, std::string(a_code), " != ", std::string(b_code), "   (both =", a, ")");
        }
      }

      /**
       * Print a comment
       * @param const std::string& file
       * @param int line
       * @param typename ...Args args
       */
      template <typename ...Args>
      static void comment(const std::string& file, int line, Args&& ...args) noexcept
      { osout(osout_note, file, line, std::forward<Args>(args)...); }

      /**
       * Print a warning
       * @param const std::string& file
       * @param int line
       * @param typename ...Args args
       */
      template <typename ...Args>
      static void warning(const std::string& file, int line, Args&& ...args) noexcept
      { num_warns_++; osout(osout_warn, file, line, std::forward<Args>(args)...); }

      /**
       * Print summary, return 0 on pass, 1 .. 99 on fail.
       * @return int
       */
      static int summary() noexcept
      {
        std::lock_guard<std::mutex> lck(iolock_);
        if(!num_fails_) {
          if(!num_checks_) {
            *os_ << (ansi_colors() ? "\033[0;33m[DONE]\033[0m" : "[DONE]") << " No checks" << std::endl;
          } else if(num_warns_) {
            *os_ << (ansi_colors() ? "\033[0;33m[PASS]\033[0m" : "[PASS]") << " All " << num_checks_ << " checks passed, " << num_warns_ << " warnings." << std::endl;
          } else {
            *os_ << (ansi_colors() ? "\033[0;32m[PASS]\033[0m" : "[PASS]") << " All " << num_checks_ << " checks passed, " << num_warns_ << " warnings." << std::endl;
          }
        } else {
          *os_ << (ansi_colors() ? "\033[0;31m[FAIL]\033[0m" : "[FAIL]") << " " << num_fails_ << " of " << num_checks_ << " checks failed, " << num_warns_ << " warnings." << std::endl;
        }
        unsigned long n = num_fails_;
        return n > 99 ? 99 : n;
      }

      /**
       * Print build information
       */
      static void buildinfo(const char* file, int line) noexcept
      { osout(osout_info, file, line, detail::buildinfo<>::info()); }

      /**
       * Switch logging of "[pass] ...." on/off. Useful for bulk tests
       * where only the fails and comments shall be printed. Note that
       * the passes are still counted, only not written to the output.
       * @param bool switch_off
       */
      static void omit_pass_log(bool switch_off) noexcept
      { omit_passes_ = switch_off; }

      /**
       * Returns if logging of "[pass] ...." is off.
       * @return bool
       */
      static bool omit_pass_log() noexcept
      { return omit_passes_; }

      /**
       * Resets the test statistics
       */
      static void reset() noexcept
      { num_checks_ = 0; num_fails_ = 0; num_warns_ = 0; }

      /**
       * Resets the test statistics
       */
      static void reset(const char* file, int line) noexcept
      { reset(); osout(osout_note, file, line, "Test statistics reset."); }

      /**
       * Returns true if the standard output is bound to a console.
       * @return bool
       */
      static bool istty() noexcept
      {
        #ifndef _MSC_VER
        return bool(::isatty(STDOUT_FILENO));
        #else
        return bool(_isatty(1));
        #endif
      }

    private:

      enum {osout_pass=0, osout_fail, osout_warn, osout_note, osout_info };

      template <typename ...Args>
      static void osout(unsigned what, std::string file, int line, Args ...args) noexcept
      {
        static const char* caption_colors[5] = { "\033[0;32m", "\033[0;31m", "\033[0;33m", "\033[0;37m", "\033[0;34m" };
        static const char* color_reset = "\033[0m";
        what = what > static_cast<unsigned>(osout_info) ? static_cast<unsigned>(osout_fail) : what;
        const char *color_tag_s="", *color_tag_e="", *color_file_s="", *color_file_e="", *color_end="";
        if(ansi_colors()) {
          color_tag_s = caption_colors[what];
          color_tag_e = color_reset;
          color_file_s = "\033[0;36m";
          color_file_e = color_reset;
          color_end = color_reset;
        }

        try {
          if(!os_) return;
          std::string msg;
          {
            std::stringstream ss;
            push_stream(ss, std::forward<Args>(args)...);
            msg = ss.str();
          }
          static const char* captions[5] = { "pass", "fail", "warn", "note", "info" };
          std::lock_guard<std::mutex> lck(iolock_);
          *os_ << color_tag_s << "[" << captions[what] << "]" << color_tag_e << " ";
          if(!file.empty()) *os_ << color_file_s << "[@" << file << ":" << line << "]" << color_file_e << " ";
          for(auto it = msg.begin(); it != msg.end(); ++it) {
            if(*it == '\n') { *os_ << std::endl << "          "; } else { *os_ << *it; }
          }
          *os_<< color_end << std::endl;
        } catch(...) {
          std::cerr << "[fatal  ] Testing frame could not write to the defined output stream, "
                      "aborting." << std::endl;
          ::abort();
        }
      }

      template <typename T, typename ...Args>
      static void push_stream(std::ostream& os, T&& v, Args ...args)
      { os << v; push_stream(os, std::forward<Args>(args)...); }

      template <typename T>
      static void push_stream(std::ostream& os, T&& v)
      { os << v; }

      static std::atomic<unsigned long> num_checks_;
      static std::atomic<unsigned long> num_fails_;
      static std::atomic<unsigned long> num_warns_;
      static std::ostream* os_;
      static std::mutex iolock_;
      static bool ansi_colors_;
      static bool omit_passes_;
    };

    template <typename T> std::atomic<unsigned long> microtest<T>::num_checks_(0);
    template <typename T> std::atomic<unsigned long> microtest<T>::num_fails_(0);
    template <typename T> std::atomic<unsigned long> microtest<T>::num_warns_(0);
    template <typename T> std::ostream* microtest<T>::os_ = &std::cout;
    template <typename T> std::mutex microtest<T>::iolock_;
    template <typename T> bool microtest<T>::ansi_colors_(!!(MICROTEST_UTEST_ANSI_COLORS));
    template <typename T> bool microtest<T>::omit_passes_(!!(MICROTEST_UTEST_OMIT_PASS_LOGS));
  }

  typedef detail::microtest<> test;

}}


/***
 * Random value and container generation.
 * Can be omitted using `WITHOUT_MICROTEST_RANDOM`.
 */
namespace sw { namespace utest {
  #ifndef WITHOUT_MICROTEST_RANDOM

    namespace random_generators {

      /**
       * Statically initialised RND device.
       */
      template <typename=void> struct rnddev { static std::random_device uni; };
      template <typename T> std::random_device rnddev<T>::uni;

      /**
       * Random for floating point types, uniform distribution, single value request.
       * @param T& r
       * @param T min
       * @param T max
       */
      template <typename R, typename A1, typename A2>
      typename std::enable_if<
        std::is_floating_point<typename std::decay<R>::type>::value &&
        std::is_convertible<typename std::decay<A1>::type,R>::value &&
        std::is_convertible<typename std::decay<A2>::type,R>::value,
        void
      >
      ::type rnd(R& r, A1 min, A2 max)
      { r = std::uniform_real_distribution<typename std::decay<R>::type>(static_cast<R>(min), static_cast<R>(max))(rnddev<>::uni); }

      /**
       * Random for floating point types, uniform distribution, single value request.
       * @param T& r
       * @param T max
       */
      template <typename R, typename A1>
      typename std::enable_if<
        std::is_floating_point<typename std::decay<R>::type>::value &&
        std::is_convertible<typename std::decay<A1>::type,R>::value,
        void
      >
      ::type rnd(R& r, A1 max)
      { rnd(r, 0, max); }

      /**
       * Random for floating point types, uniform distribution, single value request.
       * @param T& r
       */
      template <typename R>
      typename std::enable_if<
        std::is_floating_point<typename std::decay<R>::type>::value,
        void
      >
      ::type rnd(R& r)
      { rnd(r, 0, 1); }

      /**
       * Random for integral types, uniform distribution, min to max, single value request.
       * @param T& r
       * @param T min
       * @param T max
       */
      template <typename R, typename A1, typename A2>
      typename std::enable_if<
        std::is_integral<typename std::decay<R>::type>::value &&
        std::is_convertible<typename std::decay<A1>::type,R>::value &&
        std::is_convertible<typename std::decay<A2>::type,R>::value &&
        !std::is_same<typename std::decay<R>::type, char>::value &&
        !std::is_same<typename std::decay<R>::type, unsigned char>::value,
        void
      >
      ::type rnd(R& r, A1 min, A2 max)
      { r = std::uniform_int_distribution<typename std::decay<R>::type>(static_cast<R>(min), static_cast<R>(max))(rnddev<>::uni); }

      /**
       * Random for integral types, uniform distribution, 0 to max, single value request.
       * @param T& r
       * @param T max
       */
      template <typename R, typename A1>
      typename std::enable_if<
        std::is_integral<typename std::decay<R>::type>::value &&
        std::is_convertible<typename std::decay<A1>::type,R>::value &&
        !std::is_same<typename std::decay<R>::type, char>::value &&
        !std::is_same<typename std::decay<R>::type, unsigned char>::value,
        void
      >
      ::type rnd(R& r, A1 max)
      { rnd(r, 0, max); }

      /**
       * Random for integral types, uniform distribution, over the complete
       * range of the type, single value request.
       * @param T& r
       */
      template <typename R>
      typename std::enable_if<
        std::is_integral<typename std::decay<R>::type>::value &&
        !std::is_same<typename std::decay<R>::type, char>::value &&
        !std::is_same<typename std::decay<R>::type, unsigned char>::value,
        void
      >
      ::type rnd(R& r)
      { rnd(r, std::numeric_limits<R>::min(), std::numeric_limits<R>::max()); }

      /**
       * Random for byte types, uniform distribution, min to max, single value request.
       * @param T& r
       * @param T min
       * @param T max
       */
      template <typename R, typename A1, typename A2>
      typename std::enable_if<
        std::is_integral<typename std::decay<A1>::type>::value &&
        std::is_integral<typename std::decay<A2>::type>::value && (
          std::is_same<typename std::decay<R>::type, char>::value ||
          std::is_same<typename std::decay<R>::type, unsigned char>::value
        ),
        void
      >
      ::type rnd(R& r, A1 min, A2 max)
      { auto rr = 0; rnd(rr, int(min), int(max)); r = R(rr); }

      /**
       * Random for byte types, uniform distribution, 0 to max, single value request.
       * @param T& r
       * @param T max
       */
      template <typename R, typename A1>
      typename std::enable_if<
        std::is_integral<typename std::decay<A1>::type>::value && (
          std::is_same<typename std::decay<R>::type, char>::value ||
          std::is_same<typename std::decay<R>::type, unsigned char>::value
        ),
        void
      >
      ::type rnd(R& r, A1 max)
      { auto rr = 0; rnd(rr, 0, max); r = R(rr); }

      /**
       * Random for byte types, uniform distribution, over the complete type value
       * range, single value request.
       * @param T& r
       */
      template <typename R>
      typename std::enable_if<
        std::is_same<typename std::decay<R>::type, char>::value ||
        std::is_same<typename std::decay<R>::type, unsigned char>::value,
        void
      >
      ::type rnd(R& r)
      { auto rr = 0; rnd(rr, int(std::numeric_limits<R>::min()), int(std::numeric_limits<R>::max())); r = R(rr); }

      /**
       * Random string, fixed length, uniform dist, values: space (32) to '~' (126)
       * @param std::basic_string<typename R>& r
       * @param int length
       */
      template <typename R>
      void rnd(std::basic_string<R>& r, typename std::basic_string<R>::size_type length)
      {
        if(length < 1) { r.clear(); return; }
        typedef std::basic_string<R> str_t;
        str_t s(length, typename str_t::value_type());
        std::uniform_int_distribution<int> d(int(' '), int('~'));
        for(auto& e:s) e = char(d(rnddev<>::uni));
        r.swap(s);
      }

      /**
       * Random arithmetic container, fixed length, uniform distribution, from --> to
       * Note: No Concept yet in std=c++11, the filter used here applies to objects that
       * meet the requirements: They must have have allocator, iterator, integral size_type,
       * arithmetic value_type; arguments 'min' and 'max' are convertible to value_type, argument
       * 'size' is convertible to size_type, and the object can be constructed with
       * Class(size_type, value_type) for reservation of the required memory space.
       *
       * Means it fits pretty much to STL containers, but not only. Hence, this filter may
       * fail if the object does not have the methods clear() and swap(...).
       */
      template <typename R, typename Sz, typename A1, typename A2>
      typename std::enable_if<
        std::is_object<R>::value &&
        std::is_object<typename R::iterator>::value &&
        std::is_object<typename R::allocator_type>::value &&
        std::is_integral<typename R::size_type>::value &&
        std::is_constructible<R, typename R::size_type, typename R::value_type>::value &&
        std::is_arithmetic<typename std::decay<typename R::value_type>::type>::value &&
        std::is_convertible<typename std::decay<Sz>::type, typename R::size_type>::value &&
        std::is_convertible<typename std::decay<A1>::type, typename R::value_type>::value &&
        std::is_convertible<typename std::decay<A2>::type, typename R::value_type>::value,
        void
      >
      ::type rnd(R& r, Sz sz, A1 min, A2 max)
      {
        if(sz < 1) { r.clear(); return; }
        R container(static_cast<typename R::size_type>(sz), typename R::value_type());
        for(auto& e: container) rnd(e, min, max);
        r.swap(container);
      }

      /**
       * Random arithmetic container, given length, uniform distribution,
       * from MIN to MAX of the value type.
       */
      template <typename R, typename Sz>
      typename std::enable_if<
        std::is_object<R>::value &&
        std::is_object<typename R::iterator>::value &&
        std::is_object<typename R::allocator_type>::value &&
        std::is_integral<typename R::size_type>::value &&
        std::is_constructible<R, typename R::size_type, typename R::value_type>::value &&
        std::is_arithmetic<typename std::decay<typename R::value_type>::type>::value &&
        std::is_convertible<typename std::decay<Sz>::type, typename R::size_type>::value &&
        !std::is_same<typename std::decay<R>::type, std::string>::value &&
        !std::is_same<typename std::decay<R>::type, std::wstring>::value,
        void
      >
      ::type rnd(R& r, Sz sz)
      {
        if(sz < 1) { r.clear(); return; }
        R container(static_cast<typename R::size_type>(sz), typename R::value_type());
        for(auto& e: container) rnd(e);
        r.swap(container);
      }

    }

    /**
     * Random value generation. Relay function template.
     */
    template <typename R, typename ...Args>
    static inline R random(Args&& ...args)
    { auto r = R(); random_generators::rnd(r, std::forward<Args>(args)...); return r; }

  #endif
}}

/**
 * Auxiliary generators.
 */
#ifdef WITH_MICROTEST_GENERATORS
  #include <vector>
  #include <string>
  #include <array>

  namespace sw { namespace utest {

    /**
     * Generates a sequential value test vector, with
     * a given element type T, a size N, and with the
     * first value being `start_value`.
     * @tparam typename T
     * @tparam size_t N
     * @return std::vector<T>
     */
    template <typename T, size_t N>
    std::vector<T> sequence_vector(T start_value)
    {
      static_assert(N>0, "An empty container cannot hold a sequence.");
      auto v = std::vector<T>();
      v.reserve(N);
      for(size_t i=0; i<N; ++i) v.push_back(start_value + T(i));
      return v;
    }

    /**
     * Generates a sequential value test array, with
     * a given element type T, a size N, and with the
     * first value being `start_value`.
     * @tparam typename T
     * @tparam size_t N
     * @return std::array<T,N>
     */
    template <typename T, size_t N>
    std::array<T,N> sequence_array(T start_value)
    {
      static_assert(N>0, "An empty container cannot hold a sequence.");
      std::array<T,N> a;
      for(size_t i=0; i<N; ++i) a[i] = start_value + T(i);
      return a;
    }

  }}
#endif

/**
 * Optional `main()` function. Initialized the test environment,
 * invokes `void test(const std::vector<std::string>& args);`,
 * prints the summary, and returns nonzero on fails.
 */
#ifdef WITH_MICROTEST_MAIN
  #include <vector>
  #include <string>
  auto testenv_argv = std::vector<std::string>();
  auto testenv_envv = std::vector<std::string>();
  void test(const std::vector<std::string>& args);
  int main(int argc, char* argv[], char* envv[])
  {
    test_initialize();
    for(size_t i=1; (i<size_t(argc)) && (argv[i]); ++i) { testenv_argv.push_back(argv[i]); }
    for(size_t i=0; envv[i]; ++i) { testenv_envv.push_back(envv[i]); }
    test(testenv_argv);
    return test_summary();
  }
#endif

/**
 * Temporary file/directory creation and deletion.
 * Filesystem auxiliary functionality. Opt-in, compatible down to c++11, uses only basic (presumable always available) system functions.
 * @experimental To be evaluated if encompassing this with a minimal system compat layer is worth having it in this file.
 */
#ifdef WITH_MICROTEST_TMPFILE
  namespace sw { namespace utest {
    namespace detail {

      template <bool IsDirectory>
      class tmp_file
      {
      public:

        ~tmp_file() noexcept { try { clear(); } catch(...) {;} } // Deletes the file. Intentionally non-virtual.
        tmp_file(const tmp_file&) = delete;
        tmp_file(tmp_file&&) = default;
        tmp_file& operator=(const tmp_file&) = delete;
        tmp_file& operator=(tmp_file&&) = default;
        explicit tmp_file() : path_(mktmpfile(std::string())), removed_() {}
        explicit tmp_file(std::string suffix_name) : path_(mktmpfile(suffix_name)), removed_() {}

      public:

        /**
         * Returns the file path as string, empty string if
         * not assigned or could not be created.
         * @return std::string
         */
        inline const std::string& path() const noexcept
        { return path_; }

        /**
         * Clear file path, delete file
         */
        inline void clear() noexcept
        {
          if(removed_ || path_.empty()) return;
          if/*constexpr*/(!IsDirectory) {
            unlink(path_);
          } else {
            rmdir(path_);
          }
          removed_ = true;
        }

      private:

        static std::string temp_root_dir()
        {
          #ifdef __WINDOWS__
            // compat down to c++11 -> winapi
            auto buf = std::string(8192, '\0');
            const auto sz = ::GetTempPathA(buf.size()-1, &buf[0]); // string.data() not c++11
            if((sz<=0) || (sz>=buf.size()) || (buf.front()=='\0')) throw std::runtime_error("Failed to query temporary directory");
            buf.resize(sz);
            if(buf.back() !='\\') buf.push_back('\\');
            return buf;
          #else
            return std::string("/tmp/");
          #endif
        }

        static bool isdir(const std::string& path)
        {
          #ifdef _MSC_VER
            const auto attr = ::GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
          #else
            struct stat st;
            return (::stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
          #endif
        }

        static bool isfile(const std::string& path)
        {
          #ifdef _MSC_VER
            const auto attr = ::GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES) && (!(attr & FILE_ATTRIBUTE_DIRECTORY));
          #else
            struct stat st;
            return (::stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode);
          #endif
        }

        static bool exists(const std::string& path)
        {
          #ifdef _MSC_VER
            const auto attr = ::GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES);
          #else
            struct stat st;
            return (::stat(path.c_str(), &st) == 0);
          #endif
        }

        static void rmdir(const std::string& path)
        {
          #ifdef _MSC_VER
            (void)::RemoveDirectoryA(path.c_str());
          #else
            (void)::rmdir(path.c_str());
          #endif
        }

        static bool mkdir(const std::string& path)
        {
          #ifdef _MSC_VER
            return (::CreateDirectoryA(path.c_str(), nullptr));
          #elif defined(__WINDOWS__)
            return (::mkdir(path.c_str()) == 0);
          #else
            return (::mkdir(path.c_str(), 0700) == 0);
          #endif
        }

        static bool mkfile(const std::string& path)
        {
          std::ofstream of(path.c_str(), std::ios::out|std::ios::binary|std::ios::app|std::ios::ate);
          of.flush();
          return of.good();
        }

        static void unlink(const std::string& path)
        {
          #ifdef _MSC_VER
            (void)::DeleteFileA(path.c_str());
          #else
            (void)::unlink(path.c_str());
          #endif
        }

      private:

        static std::string mktmpfile(const std::string& name)
        {
          static bool has_seed = false;
          if(!has_seed) { has_seed=true; ::srand(time(0)); }
          auto tmpbase = std::string(MICROTEST_UTEST_TMPDIR);
          if(tmpbase.empty()) tmpbase = temp_root_dir();
          if(!isdir(tmpbase)) throw std::runtime_error(std::string("Temporary directory (OS query) missing: ") + tmpbase);
          tmpbase += "utest-";
          const char* fnrnd = "abcdefghijklmnopqrstuvwxyz123456";
          for(int chk=0; chk<100; chk++) {
            std::string f;
            for(int i=0; i<3; ++i) {
              const int r = ::rand();
              f += fnrnd[ (r>> 0) & 31 ];
              f += fnrnd[ (r>> 5) & 31 ];
              f += fnrnd[ (r>>10) & 31 ];
              f += fnrnd[ (r>>15) & 31 ];
            }
            if(name.empty()) {
              f = tmpbase + f + ".tmp";
            } else {
              f = tmpbase + f + ("-") + name;
            }
            if(!exists(f)) {
              if(!IsDirectory) {
                if(mkfile(f)) return f;
              } else {
                if(mkdir(f)) return f;
              }
            }
          }
          throw std::runtime_error("Temporary file/dir creation failed.");
        }

      private:

        const std::string path_;
        bool removed_;
      };
    }

    #define test_make_tmpfile()   (::sw::utest::detail::tmp_file<false>())
    #define test_make_tmpdir()    (::sw::utest::detail::tmp_file<true>())
  }}
#endif

#endif
