/**
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * @version 1.1
 * @ccflags
 * @ldflags
 * @platform linux, bsd
 * @standard >= c++11
 *
 * -----------------------------------------------------------------------------
 *
 * Micro-test class template, used for unit testing where tests are separately
 * compiled main files containing test cases.
 *
 * -----------------------------------------------------------------------------
 * Example:
 *
 * #include <sw/microtest.hh>
 * #include <iostream>
 * #include <sstream>
 * #include <vector>
 * #include <deque>
 * #include <forward_list>
 * #include <list>
 *
 * using namespace std;
 * using namespace sw;
 *
 * // Auxiliary: function that throws a runtime error
 * void throws(bool e)
 * { if(e) throw std::runtime_error("Bad something"); }
 *
 * // Auxiliary: forward iterable container --> string
 * template <typename ContainerType>
 * string to_string(ContainerType c)
 * {
 *   if(c.empty()) return "[]"; stringstream s; auto it = c.begin(); s << "[ " << *it;
 *   for(++it; it != c.end(); ++it) s << ", " << *it; s << " ]"; return s.str();
 * }
 *
 * // Example main
 * int main(int argc, char** argv)
 * {
 *   // Comments
 *   char a = 'a';
 *   string s = "test";
 *   test_note("This is ", a, " ", s, ".");
 *
 *   // Checks --> expect
 *   test_expect(argc >= 1 && argv && argv[0]);
 *   test_expect_except( throws(true) );
 *   test_expect_noexcept( throws(false) );
 *   test_expect(1 == 0);
 *
 *   // Test state information
 *   test_note("Number of checks already done: ", utest::test::num_checks );
 *   test_note("Number of fails already had: ", utest::test::num_fails );
 *
 *   // Random string
 *   test_note("Random string: ", utest::random<string>(10) );
 *
 *   // Random arithmetic
 *   test_note("Random char: ",   (int) utest::random<char>() );
 *   test_note("Random uint8: ",  (int) utest::random<std::uint8_t>(100) );
 *   test_note("Random double: ", utest::random<double>() ); // 0 to 1
 *   test_note("Random double: ", utest::random<double>(10) ); // 0 to 10
 *   test_note("Random double: ", utest::random<double>(-1, 1) ); // -1 to 1
 *
 *   // Random forward iterable container with push_back()
 *   test_note("Random vector<double>: ", to_string(utest::random<std::vector<double>>(5, 0, 1)));
 *   test_note("Random deque<int>: ", to_string(utest::random<std::deque<int>>(5, -100, 100)));
 *   test_note("Random list<unsigned>: ", to_string(utest::random<std::list<unsigned>>(5, 1, 20)));
 *
 *   // Test summary
 *   return utest::test::summary();
 * }
 *
 * -----------------------------------------------------------------------------
 * +++ BSD license header +++
 * Copyright (c) 2012-2014, Stefan Wilhelm (stfwi, <cerbero s@atwilly s.de>)
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
#ifndef SW_UTEST_HH
#define SW_UTEST_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
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
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>
#if defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
  #include <windows.h>
  #include <sys\stat.h>
  #ifndef __WINDOWS__
    #define __WINDOWS__
  #endif
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <time.h>
#endif
// </editor-fold>

namespace sw { namespace utest {

// <editor-fold desc="std common code" defaultstate="collapsed">
#ifdef __MSC_VER
// fix of the century joke
namespace std { template <typename T> bool isnan(T d) { return !!_isnan(d); } }
#endif

namespace { namespace templates {

  // <editor-fold desc="buildinfo" defaultstate="collapsed">
  template <typename T=void>
  struct buildinfo
  {

    /**
     * Print build information
     */
    static std::string info() noexcept
    {
      std::stringstream ss;
      ss << "compiler: " << compiler() << ", std=" << compilation_standard()
         << ", platform: " << platform();
      return ss.str();
    }

    /**
     * Returns the c++ standard, e.g. "c++11"
     * @return constexpr const char*
     */
    static constexpr const char* compilation_standard() noexcept
    {
      #if (__cplusplus >= 201400L)
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
  // </editor-fold>

  // <editor-fold desc="tmpfile" defaultstate="collapsed">
  template <typename T=void>
  class tmp_file
  {
  public:

    /**
     * Defined name suffix.
     * @param std::string name
     */
    tmp_file(std::string name = "")
  {
      static bool hasseed = false;
      if(!hasseed) { hasseed=true; ::srand(time(0)); }
      std::string tmpbase;
      #ifdef UTEST_TMPDIR
      tmpbase = UTEST_TMPDIR;
      #elif defined __MSC_VER
      tmpbase = "c:\\tmp\\"; _mkdir("c:\\tmp");
      #elif defined(__MINGW32__) || defined(__MINGW64__)
      tmpbase = "c:\\tmp\\"; ::mkdir("c:\\tmp");
      #else
      tmpbase += "/tmp/";
      #endif
      {
        struct stat st;
        if((::stat(tmpbase.c_str(), &st) != 0) || !S_ISDIR(st.st_mode)) {
          throw std::runtime_error(std::string("Temporary directory missing: ") + tmpbase);
        }
      }
      tmpbase += "utest-";
      const char* fnrnd = "abcdefghijklmnopqrstuvwxyz123456";
      for(int chk=0; chk<100; chk++) {
        std::string f;
        for(int i=0; i<3; ++i) {
          int r = ::rand();
          f += fnrnd[ (r>> 0) & 31 ];
          f += fnrnd[ (r>> 5) & 31 ];
          f += fnrnd[ (r>>10) & 31 ];
        }
        if(name.empty()) {
          f = tmpbase + f + ".tmp";
        } else {
          f = tmpbase + f + ("-") + name;
        }
        if(::access(f.c_str(), 0) != 0) {
          std::ofstream of(f.c_str(), std::ios::app|std::ios::ate);
          if(of.good()) { file_ = f; break; }
        }
      }
    }

    /**
     * Deletes the file
     */
    ~tmp_file()
    { try { clear(); } catch(...) {;} }

    /**
     * Clear file path, delete file
     */
    inline void clear() noexcept
    {
      if(file_.empty()) return;
      #ifdef OS_WIN
      _unlink(file_.c_str());
      #else
      ::unlink(file_.c_str());
      #endif
      std::string().swap(file_);
    }

    inline bool empty() const noexcept
    { return file_.empty(); }

    /**
     * Returns the file path, empty string if not assigned or could not be created.
     * @return std::string
     */
    inline std::string path() const noexcept
    { return file_; }

    /**
     * Returns the path of the file (alias of `path()` ).
     * @return std::string
     */
    operator std::string() const noexcept
    { return file_; }

    /**
     * Returns if the file is not successfully created (yet or error). Alias of empty().
     * @return bool
     */
    bool operator !() const noexcept
    { return file_.empty(); }

  private:
    tmp_file(const tmp_file&) {}
    std::string file_;
  };
  // </editor-fold>

  template <typename=void>
  struct auxfn
  {
    static std::string srtrim(std::string s)
    { while(!s.empty() && std::isspace(s.back())) s.pop_back(); return s; }
  };

}}

using buildinfo = templates::buildinfo<>;
using tmp_file = templates::tmp_file<>;
// </editor-fold>

// <editor-fold desc="test_ macros" defaultstate="collapsed">

#define test_fail(...) (::sw::utest::test::fail   (__FILE__, __LINE__, __VA_ARGS__))

#define test_pass(...) (::sw::utest::test::pass(__FILE__, __LINE__, __VA_ARGS__))

#define test_reset() (::sw::utest::test::reset(__FILE__, __LINE__));

#define test_note(ARG) { \
  std::stringstream ss_ss; ss_ss << ARG; \
  (::sw::utest::test::comment(__FILE__, __LINE__, ss_ss.str())); \
}

#define test_comment(X) test_note(X)

#define test_buildinfo() (::sw::utest::test::buildinfo(__FILE__, __LINE__))

#define test_expect_cond(...) ( \
  (__VA_ARGS__) ? \
  (::sw::utest::test::pass(__FILE__, __LINE__, #__VA_ARGS__)) : \
  (::sw::utest::test::fail(__FILE__, __LINE__, #__VA_ARGS__)) \
)

#define test_expect_nocatch(...) ( \
  (__VA_ARGS__) ? \
  (::sw::utest::test::pass(__FILE__, __LINE__, #__VA_ARGS__)) : \
  (::sw::utest::test::fail(__FILE__, __LINE__, #__VA_ARGS__)) \
)

#define test_expect(...) try { \
  test_expect_nocatch(__VA_ARGS__); \
} catch(const std::exception& e) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Unexpected exception: ") + ::sw::utest::templates::auxfn<>::srtrim(std::string(e.what())) )); \
} catch(...) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Unexpected exception: ") )); \
}

#define test_expect_except(...) try { \
  (__VA_ARGS__); \
  (::sw::utest::test::fail(__FILE__, __LINE__, #__VA_ARGS__, " | Exception was expected" )); \
} catch(const std::exception& e) { \
  (::sw::utest::test::pass(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Expected exception: ") + ::sw::utest::templates::auxfn<>::srtrim(std::string(e.what())) )); \
} catch(...) { \
  (::sw::utest::test::pass(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Expected exception") )); \
}

#define test_expect_noexcept(...) try { \
  ;(__VA_ARGS__); \
  (::sw::utest::test::pass(__FILE__, __LINE__, #__VA_ARGS__)); \
} catch(const std::exception& e) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Unexpected exception: ") + ::sw::utest::templates::auxfn<>::srtrim(std::string(e.what())) )); \
} catch(...) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
    #__VA_ARGS__ " | Unexpected exception") )); \
}

#define test_expect_in_tolerance(ARG, TOL) try { \
  double r = (ARG); \
  if(r < (ARG)+(TOL) && r > (ARG)-(TOL)) { \
    ::sw::utest::test::pass(__FILE__, __LINE__, std::string(#ARG)); \
  } else { \
    ::sw::utest::test::fail(__FILE__, __LINE__, std::string(#ARG)); \
  } \
} catch(const std::exception& e) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
      #ARG "-> Unexpected exception: ") + ::sw::utest::templates::auxfn<>::srtrim(std::string(e.what()))) ); \
} catch(...) { \
  (::sw::utest::test::fail(__FILE__, __LINE__, std::string( \
    #ARG " | Unexpected exception") )); \
}

// </editor-fold>

// <editor-fold desc="wrappers for platform dependencies" defaultstate="collapsed">
/**
 * isnan forward for platform trouble prevention
 * @param typename T n
 * @return bool
 */
template <typename T>
static bool isnan(T n)
{
  #ifndef __MSC_VER
  return std::isnan(n);
  #else
  return _isnan(n);
  #endif
}

template <typename T>
static T round(T v, T dim)
{ return std::round((double)v / dim) * dim; }

// </editor-fold>

// <editor-fold desc="random" defaultstate="collapsed">
namespace random_generators {

  /**
   * Statically initialised RND device.
   */
  template <typename=void> struct rnddev
  { static std::default_random_engine rd; };
  template <typename T> std::default_random_engine rnddev<T>::rd(
    static_cast<long unsigned int>(::std::chrono::high_resolution_clock::now().time_since_epoch().count())
  );

  /**
   * Float / int distribution selector
   */
  template <typename T, typename=void>
  struct distrbution;

  template <typename T>
  struct distrbution<T, typename std::enable_if<std::is_integral<T>::value>::type>
  { using type = std::uniform_int_distribution<T> ; };

  template <typename T>
  struct distrbution<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
  { using type = std::uniform_real_distribution<T>; };

  /**
   * Random for arithmetic types, uniform distribution, single value request.
   * @param T& r
   * @param T min
   * @param T max
   */
  template <typename R, typename A1, typename A2>
  typename std::enable_if<
    std::is_arithmetic<typename std::decay<R>::type>::value &&
    std::is_convertible<typename std::decay<A1>::type,R>::value &&
    std::is_convertible<typename std::decay<A2>::type,R>::value
  >
  ::type rnd(R& r, A1 min, A2 max)
  {
    typename distrbution<R>::type d(static_cast<R>(min), static_cast<R>(max));
    r = d(rnddev<>::rd);
  }

  /**
   * Random for arithmetic types, uniform distribution, single value request.
   * @param T& r
   * @param T max
   */
  template <typename R, typename A1>
  typename std::enable_if<
    std::is_arithmetic<typename std::decay<R>::type>::value &&
    std::is_convertible<typename std::decay<A1>::type,R>::value
  >
  ::type rnd(R& r, A1 max)
  {
    typename distrbution<R>::type d(static_cast<R>(0), static_cast<R>(max));
    r = d(rnddev<>::rd);
  }

  /**
   * Random for arithmetic types, uniform distribution, single value request.
   * @param T& r
   * @param T max
   */
  template <typename R>
  typename std::enable_if<
    std::is_arithmetic<typename std::decay<R>::type>::value
  >
  ::type rnd(R& r)
  {
    if(std::is_floating_point<R>::value) {
      typename distrbution<R>::type d(0.0, 1.0);
      r = d(rnddev<>::rd);
    } else {
      typename distrbution<R>::type d(std::numeric_limits<R>::min(), std::numeric_limits<R>::max());
      r = d(rnddev<>::rd);
    }
  }

  /**
   * Random string, fixed length, uniform dist, values: space (32) to '~' (126)
   * @param std::basic_string<typename R>& r
   * @param int length
   */
  template <typename R>
  void rnd(std::basic_string<R>& r, typename std::basic_string<R>::size_type length)
  {
    if(length < 1) { r.clear(); return; }
    using str_t = std::basic_string<R>;
    str_t s(length, typename str_t::value_type());
    std::uniform_int_distribution<typename str_t::value_type> d(
      typename str_t::value_type(' '),
      typename str_t::value_type('~')
    );
    for(auto& e : s) e = d(rnddev<>::rd);
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
   *
   * @param std::basic_string<typename R>& r
   * @param int length
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
    std::is_convertible<typename std::decay<A2>::type, typename R::value_type>::value
  >
  ::type rnd(R& r, Sz sz, A1 min, A2 max)
  {
    if(sz < 1) { r.clear(); return; }
    R container(static_cast<typename R::size_type>(sz), typename R::value_type());
    typename distrbution<typename R::value_type>::type d(
      static_cast<typename R::value_type>(min),
      static_cast<typename R::value_type>(max)
    );
    for(auto& e : container) e = d(rnddev<>::rd);
    r.swap(container);
  }

} // /utest::random_generators

/**
 * Random value generation
 */
template <typename R, typename ...Args>
static R random(Args ...args)
{ R r; random_generators::rnd(r, std::forward<Args>(args)...); return std::forward<R>(r); }
// </editor-fold>

// <editor-fold desc="class utest" defaultstate="collapsed">
namespace detail {

  template <typename=void>
  class utest
  {
  public:

    // <editor-fold desc="class var getters / setters" defaultstate="collapsed">
    /**
     * Returns the number of failed checks
     * @return unsigned long
     */
    static unsigned long num_fails() noexcept
    { return num_fails_; }

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
    { osout = &os; }
    // </editor-fold>

    // <editor-fold desc="pass, fail, comment, summary, buildinfo" defaultstate="collapsed">
    /**
     * Register a succeeded expectation, increases test counter, prints message.
     * @param std::string file
     * @param int line
     * @param typename ...Args args
     */
    template <typename ...Args>
    static bool pass(std::string file, int line, Args ...args) noexcept
    { num_checks_++; osout("pass", file, line, args...); return true; }

    /**
     * Register pass without logging
     * @return bool
     */
    static bool pass() noexcept
    { num_checks_++; return true; }

    /**
     * Register a failed expectation, increase test counter and fail counter, prints message
     * @param std::string file
     * @param int line
     * @param typename ...Args args
     */
    template <typename ...Args>
    static bool fail(std::string file, int line, Args ...args) noexcept
    { num_checks_++; num_fails_++; osout("fail", file, line, args...); return false; }

    /**
     * Register a failed expectation, increase test counter and fail counter, prints message
     * @param std::string file
     * @param int line
     * @param typename ...Args args
     */
    template <typename ...Args>
    static bool warn(std::string file, int line, Args ...args) noexcept
    { num_warnings_++; osout("warn", file, line, args...); return false; }

    /**
     * Register fail without logging
     * @return bool
     */
    static bool fail() noexcept
    { num_checks_++; num_fails_++; return false; }

    /**
     * Print a comment
     * @param std::string file
     * @param int line
     * @param typename ...Args args
     */
    template <typename ...Args>
    static void comment(std::string file, int line, Args ...args) noexcept
    { osout("note", file, line, args...); }

    /**
     * Resets errors, warnings and passes (counters).
     */
    template <typename=void>
    static void reset(std::string file, int line) noexcept
    {
      num_checks_ = 0;
      num_fails_ = 0;
      num_warnings_ = 0;
      comment(file, line, "Test counters reset.");
    }

    /**
     * Print summary, return 0 on pass, 1 .. 99 on fail.
     * @return int
     */
    static int summary() noexcept
    {
      std::lock_guard<std::mutex> lck(iolock_);
      if(!num_fails_) {
        if(!num_checks_) {
          *os_ << "[PASS] No checks" << std::endl;
        } else {
          *os_ << "[PASS] All " << num_checks_ << " checks passed," << num_warnings_ << (num_warnings_ == 1 ? " warning." : " warnings.") << std::endl;
        }
      } else {
        *os_ << "[FAIL] " << num_fails_ << " of " << num_checks_ << " checks failed, " << num_warnings_ << (num_warnings_ == 1 ? " warning." : " warnings.") << std::endl;
      }
      unsigned long n = num_fails_;
      return n > 99 ? 99 : n;
    }

    /**
     * Summary alias
     * @return int
     */
    static int done() noexcept
    { return summary(); }

    /**
     * Print build information
     */
    static void buildinfo(const char* file, int line)
    { osout("info", file, line, templates::buildinfo<>::info()); }

    // </editor-fold>

  private:

    // <editor-fold desc="stream output" defaultstate="collapsed">
    template <typename ...Args>
    static void osout(const char* what, std::string file,
            int line, Args ...args) noexcept
    {
      try {
        if(!os_) return;
        std::string msg;
        {
          std::stringstream ss;
          push_stream(ss, std::forward<Args>(args)...);
          msg = ss.str();
          while(!msg.empty() && std::isspace(msg.back())) msg.pop_back();
        }
        std::lock_guard<std::mutex> lck(iolock_);
        *os_ << "[" << what << "] ";
        if(!file.empty()) *os_ << "[@" << file << ":" << line << "] ";
        for(auto it = msg.begin(); it != msg.end(); ++it) {
          if(*it == '\n') { *os_ << std::endl << "          "; } else { *os_ << *it; }
        }
        *os_ << std::endl;
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
    // </editor-fold>

    // <editor-fold desc="variables" defaultstate="collapsed">
    static std::atomic<unsigned long> num_checks_;
    static std::atomic<unsigned long> num_fails_;
    static std::atomic<unsigned long> num_warnings_;
    static std::ostream* os_;
    static std::mutex iolock_;
    // </editor-fold>

  };

  // <editor-fold desc="statics init" defaultstate="collapsed">
  template <typename T> std::atomic<unsigned long> utest<T>::num_checks_(0);
  template <typename T> std::atomic<unsigned long> utest<T>::num_fails_(0);
  template <typename T> std::atomic<unsigned long> utest<T>::num_warnings_(0);
  template <typename T> std::ostream* utest<T>::os_ = &std::cout;
  template <typename T> std::mutex utest<T>::iolock_;
  // </editor-fold>

}

using test = detail::utest<>;
// </editor-fold>

// <editor-fold desc="tmpdir" defaultstate="collapsed">
#ifndef WITHOUT_TMPDIR
  namespace detail {

    template <typename=void>
    class tmpdir
    {
    public:

      ~tmpdir() noexcept
      { remove(); }

      /**
       * Creates a temporary directory (only if not existing yet) and returns the path of it.
       * @return const char*
       */
      static const std::string& path() noexcept
      {
        if(instance_.path_.empty()) {
          bool failed = false;
          std::string dir, subdir;
          #if (defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))
          {
            char bf[4096];
            ::memset(bf, 0, sizeof(bf));
            DWORD r = ::GetTempPathA(sizeof(bf), bf);
            if(!r || r > sizeof(bf)) bf[0] = '\0';
            bf[r] = bf[sizeof(bf)-1] = '\0';
            if(r) while(--r > 0 && bf[r] == '\\') bf[r] = '\0';
            dir = bf;
          }
          {
            std::string s("000");
            std::default_random_engine rd(static_cast<long unsigned int>(::std::chrono::high_resolution_clock::now().time_since_epoch().count()));
            std::uniform_int_distribution<char> d('a','z');
            for(auto& e : s) e = d(rd);
            subdir = std::string("\\utest-test-") + std::to_string(::time(nullptr)) + "-" + s;
          }
          auto filestat = [](const char* p, struct ::stat* st) { return ::stat(p, st); };
          auto makedir = [](const char* p) { return ::mkdir(p); };
          #else
          dir = "/tmp";
          subdir = std::string("/utest-") + std::to_string(::time(nullptr)) + "-" + std::to_string(::clock() % 100);
          auto makedir = [](const char* p) { return ::mkdir(p, 0755); };
          auto filestat = [](const char* p, struct ::stat* st) { return ::lstat(p, st); };
          #endif
          struct ::stat st;
          if(!dir.length() || (filestat(dir.c_str(), &st)) || (!S_ISDIR(st.st_mode))) {
            utest<>::fail(__FILE__, __LINE__, "Failed to get temporary test directory");
            failed = true;
          }
          dir += subdir;
          std::string().swap(subdir);
          if(!dir.length() || (!filestat((dir).c_str(), &st))) {
            utest<>::fail(__FILE__, __LINE__, std::string("Temporary test directory already existing: '") + dir + "'");
            failed = true;
          } else if((makedir(dir.c_str()) != 0) || (filestat(dir.c_str(), &st)) || (!S_ISDIR(st.st_mode))) {
            utest<>::fail(__FILE__, __LINE__, std::string("Failed to create test directory '") + dir + "'");
            failed = true;
          }
          if(failed) {
            utest<>::summary();
            ::exit(1);
          } else {
            utest<>::comment("microtest.hh", __LINE__, std::string("Test temporary directory created: '") + dir + "'");
          }
          instance_.path_ = dir;
        }
        return instance_.path_;
      }

      /**
       * Removes the generated temporary path (if existing, and owned by the current user).
       */
      static void remove() noexcept
      {
        #if (defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))
        struct ::stat st;
        if(instance_.path_.size() && (!::stat(instance_.path_.c_str(), &st)) && S_ISDIR(st.st_mode)) {
          std::string cmd = "rmdir /S /Q \"";
          cmd += instance_.path_;
          cmd += "\" >NUL 2>&1";
          int r = ::system(cmd.c_str());
          if(r != 0) {
            ::chdir(instance_.path_.c_str());
            ::chdir("..");
            r = ::system(cmd.c_str());
          }
          if(::stat(instance_.path_.c_str(), &st) != 0) {
            utest<>::comment("microtest.hh", __LINE__, std::string("Test temporary directory removed: '") + instance_.path_ + "'");
          } else {
            utest<>::comment("microtest.hh", __LINE__, std::string("Could not remove test temporary directory: '") + instance_.path_ + "'");
          }
        }
        #else
        struct ::stat st;
        if(instance_.path_.size() && (!::stat(instance_.path_.c_str(), &st)) && S_ISDIR(st.st_mode)
            && (st.st_uid == ::getuid())) {
          std::string cmd = "rm -rf -- '";
          cmd += instance_.path_;
          cmd += "' >/dev/null 2>&1";
          int r=::system(cmd.c_str()); (void)r;
          utest<>::comment("microtest.hh", __LINE__, std::string("Test temporary directory removed: '") + instance_.path_ + "'");
        }
        #endif
      }

    private:
      std::string path_;
      static tmpdir instance_;
    };

    template <typename T> tmpdir<T> tmpdir<T>::instance_ = tmpdir<T>();
  }

  using tmpdir = detail::tmpdir<>;

  #define test_tmpdir()        (::sw::utest::tmpdir::path().c_str())
  #define test_tmpdir_remove() { ::sw::utest::tmpdir::remove(); }
#endif
// </editor-fold>

}}
#endif
