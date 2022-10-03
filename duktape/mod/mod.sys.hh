/**
 * @file duktape/mod/mod.sys.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++11
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++11 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional basic system functionality.
 *
 * -----------------------------------------------------------------------------
 * License: http://opensource.org/licenses/MIT
 * Copyright (c) 2014-2022, the authors (see the @authors tag in this file).
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions: The above copyright notice and
 * this permission notice shall be included in all copies or substantial portions
 * of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef DUKTAPE_MOD_BASIC_SYSTEM_HH
#define DUKTAPE_MOD_BASIC_SYSTEM_HH

#if defined(_MSCVER)
  #error "not yet implemented"
#endif

#include "../duktape.hh"
#include "mod.sys.os.hh"
#include <algorithm>
#include <chrono>
#include <thread>
#include <cmath>
#include <unistd.h>
#if defined(OS_WINDOWS)
  #include <windows.h>
  #include <Lmcons.h>
#else
  #include <sys/utsname.h>
  #include <pwd.h>
  #include <grp.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <sys/types.h>
  #include <unistd.h>
  #ifdef OS_LINUX
    #include <linux/kd.h>
  #endif
#endif


namespace duktape { namespace detail {

  //
  // JS Date <--> c++ time_t like value with keeping sub-seconds.
  //
  // @sw: Has to be checked how accurate the double values
  //      (also stored in the Date object as underlaying
  //      value) actually are when the numbers get bigger.
  //      When Duktape changes the type we should do this,
  //      too.
  //
  namespace {
    template<typename time_type=double>
    struct basic_unix_timestamp {
      time_type t;
      inline basic_unix_timestamp() : t(0) {}
      inline basic_unix_timestamp(time_type ts) : t(ts) {}
      inline basic_unix_timestamp(struct ::timespec ts)
        : t(time_type(ts.tv_sec) + time_type(ts.tv_nsec) * time_type(1e-9)) {}
    };
  }
  using unix_timestamp = basic_unix_timestamp<>;

  template <> struct conv<unix_timestamp>
  {
    using type = unix_timestamp;

    static constexpr int nret() noexcept
    { return 1; }

    static constexpr const char* cc_name() noexcept
    { return "unix_timestamp"; }

    static constexpr const char* ecma_name() noexcept
    { return "Date"; }

    static bool is(duk_context* ctx, int index) noexcept
    { return duktape::api(ctx).is_date(index); }

    static type get(duk_context* ctx, int index) noexcept
    { return to(ctx, index); }

    static type req(duk_context* ctx, int index) noexcept
    { return to(ctx, index); }

    static type to(duk_context* ctx, int index) noexcept
    {
      duktape::api stack(ctx);
      if(!stack.is_date(index)) return type(0);
      return type(stack.to_number(index) / 1000);
    }

    static void push(duk_context* ctx, type ts) noexcept
    {
      duktape::api stack(ctx);
      stack.require_stack(2);
      if(!stack.get_global_string("Date")) return;
      stack.push(ts.t * 1000);
      stack.pnew(1);
    }
  };

  #if defined(__linux__) || defined(__MINGW32__) || defined(__MINGW64__)
    template <> struct conv<struct ::timespec>
    {
      typedef struct ::timespec type;

      static constexpr int nret() noexcept
      { return 1; }

      static constexpr const char* cc_name() noexcept
      { return "timespec"; }

      static constexpr const char* ecma_name() noexcept
      { return "Date"; }

      static bool is(duk_context* ctx, int index) noexcept
      { return duktape::api(ctx).is_date(index); }

      static type get(duk_context* ctx, int index) noexcept
      { return to(ctx, index); }

      static type req(duk_context* ctx, int index) noexcept
      { return to(ctx, index); }

      static type to(duk_context* ctx, int index) noexcept
      {
        duktape::api stack(ctx);
        if(!stack.is_date(index)) return type{0,0};
        type ts{0,0};
        uint64_t v = (uint64_t) stack.to_number(index); // duktape will coerce the \x00Value property.
        ts.tv_sec = v / 1000;
        ts.tv_nsec = (v % 1000) * 1000000;
        return ts;
      }

      static void push(duk_context* ctx, type val) noexcept
      {
        duktape::api stack(ctx);
        stack.require_stack(2);
        if(!stack.get_global_string("Date")) return;
        stack.push( ((double) val.tv_sec * 1000) + ((double) (val.tv_nsec / 1000000)) );
        stack.pnew(1);
      }
    };
  #endif

}}

namespace duktape { namespace detail { namespace system {

  template <typename=void>
  int getpid(duktape::api& stack)
  {
    #ifndef OS_WINDOWS
    pid_t val = ::getpid();
    if(val < 0) return 0;
    stack.push(val);
    return 1;
    #else
    stack.push(_getpid());
    return 1;
    #endif
  }

  template <typename=void>
  int getuid(duktape::api& stack)
  {
    #ifndef OS_WINDOWS
    uid_t val = ::getuid();
    static_assert(!std::numeric_limits<uid_t>::is_signed, "Additional uid_t value check needed.");
    // if(std::numeric_limits<uid_t>::is_signed) if(val < 0) return 0;
    stack.push(val);
    return 1;
    #else
    (void) stack;
    return 0; // not applicable
    #endif
  }

  template <typename=void>
  int getgid(duktape::api& stack)
  {
    #ifndef OS_WINDOWS
    gid_t val = ::getgid();
    static_assert(!std::numeric_limits<uid_t>::is_signed, "Additional gid_t value check needed.");
    // if(std::numeric_limits<gid_t>::is_signed && val < 0) return 0;
    stack.push(val);
    return 1;
    #else
    (void) stack;
    return 0; // not applicable
    #endif
  }

  template <typename=void>
  int getuser(duktape::api& stack)
  {
    #ifndef OS_WINDOWS
    uid_t uid;
    if(stack.is_undefined(0)) {
      uid = ::getuid();
    } else if(stack.is<uid_t>(0)) {
      uid = stack.to<uid_t>(0);
    } else {
      return 0;
    }
    char name[256];
    struct ::passwd pw, *ppw;
    if((::getpwuid_r(uid, &pw, name, sizeof(name), &ppw) == 0) && pw.pw_name) {
      stack.push((const char*)pw.pw_name);
      return 1;
    }
    return 0;
    #else
    char nam[UNLEN+1];
    DWORD namsz = UNLEN+1;
    ::GetUserNameA(nam, &namsz);
    nam[UNLEN] = '\0';
    stack.push(nam);
    return 1;
    #endif
  }

  template <typename=void>
  int getgroup(duktape::api& stack)
  {
    #ifndef OS_WINDOWS
    gid_t gid = 0;
    if(stack.is_undefined(0)) {
      gid = ::getgid();
    } else if(stack.is<uid_t>(0)) {
      gid = stack.to<uid_t>(0);
    } else {
      return 0;
    }
    char name[256];
    struct ::group gr, *pgr;
    if((::getgrgid_r(gid, &gr, name, sizeof(name), &pgr) == 0) && gr.gr_name) {
      stack.push((const char*)gr.gr_name);
      return 1;
    }
    return 0;
    #else
    (void) stack;
    return 0;
    #endif
  }

  template <typename=void>
  std::string application_path()
  {
    char app_path[PATH_MAX];
    memset(app_path, 0, sizeof(app_path));
    #if !defined(_WIN32) && !defined(_MSC_VER)
      char lnk_path[PATH_MAX];
      memset(lnk_path, 0, sizeof(lnk_path));
      #if defined (__linux__)
      ::strncpy(lnk_path, "/proc/self/exe", sizeof(lnk_path)-1);
      #elif defined (__NetBSD__)
      ::strncpy(lnk_path, "/proc/curproc/exe", sizeof(lnk_path)-1);
      #elif defined (__FreeBSD__) || defined (__OpenBSD__)
      int ic[4];
      ic[0] = CTL_KERN; ic[1] = KERN_PROC; ic[2] = KERN_PROC_PATHNAME; ic[3] = -1;
      size_t sz = sizeof(lnk_path)-1;
      if(sysctl(ic, 4, lnk_path, &sz, nullptr, 0)) lnk_path[0] = '\0';
      #elif defined (__DragonFly__)
      ::strncpy(lnk_path, "/proc/curproc/file", sizeof(lnk_path)-1);
      #elif defined (__APPLE__) && __MACH__
      uint32_t sz = sizeof(lnk_path);
      if(_NSGetExecutablePath(lnk_path, &sz)) lnk_path[0] = '\0';
      #endif
      if(!::realpath(lnk_path, app_path)) return ""; // error
    #else
      if(::GetModuleFileNameA(nullptr, app_path, sizeof(app_path)) <= 0) return ""; // error
    #endif
    app_path[sizeof(app_path)-1] = '\0';
    return std::string(app_path);
  }

  template <typename=void>
  int app_path(duktape::api& stack)
  {
    std::string path = application_path();
    if(path.empty()) return 0;
    stack.push(path);
    return 1;
  }

  template <typename=void>
  int getuname(duktape::api& stack)
  {
    stack.push_object();
    #if defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__)
    struct ::utsname un;
    if(::uname(&un) != 0) return 0;
    stack.set("sysname", (const char*)un.sysname);
    stack.set("release", (const char*)un.release);
    stack.set("machine", (const char*)un.machine);
    stack.set("version", (const char*)un.version);
    #elif defined(OS_WINDOWS)
      stack.set("sysname", "windows");
    #else
      stack.set("sysname", "unknown");
    #endif
    return 1;
  }

  template <typename=void>
  int sleep_seconds(duktape::api& stack)
  {
    double t = stack.to<double>(0);
    if(std::isnan(t) || t < 0) {
      stack.push(false);
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(static_cast<unsigned long>(t*1e6)));
      stack.push(true);
    }
    return 1;
  }

  template <typename=void>
  int clock_seconds(duktape::api& stack)
  {
    char c = 'm';
    {
      std::string s = stack.get<std::string>(0);
      if(!s.empty()) c = s[0];
    }
    double t = std::numeric_limits<double>::quiet_NaN();
    #ifdef __linux__
    // Separate case because we have explicit boot time here
    switch(c) {
      case 'r':
      case 'b': {
        static ::timespec ts;
        if(::clock_gettime(c=='b' ? CLOCK_BOOTTIME : CLOCK_REALTIME, &ts) == 0) {
          t = double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
        }
        break;
      }
      default: {
        static double t0= std::numeric_limits<double>::quiet_NaN();
        static ::timespec ts;
        if(::clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
          t = double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
          if(std::isnan(t0)) t0 = t;
          t -= t0;
        }
        break;
      }
    }
    #else
    using namespace std::chrono;
    switch(c) {
      case 'r': {
        t = double(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count()) * 1e-6;
        break;
      }
      default: {
        static double t0 = std::numeric_limits<double>::quiet_NaN();
        t = double(duration_cast<std::chrono::microseconds>(steady_clock::now().time_since_epoch()).count()) * 1e-6;
        if(std::isnan(t0)) t0 = t;
        t -= t0;
        break;
      }
    }
    #endif
    stack.push(t);
    return 1;
  }

  template <typename=void>
  int isatty_by_name(duktape::api& stack)
  {
    enum { ckin, ckout, ckerr } check;
    bool r = false;
    {
      std::string s = stack.get<std::string>(0);
      if(s.find_first_of("Ii") != s.npos) {
        check = ckin;
      } else if(s.find_first_of("Oo") != s.npos) {
        check = ckout;
      } else if(s.find_first_of("Ee") != s.npos) {
        check = ckerr;
      } else {
        return 0; // undefined
      }
    }
    #ifndef OS_WINDOWS
    switch(check) {
      case ckin : r = ::isatty(STDIN_FILENO ) != 0; break;
      case ckout: r = ::isatty(STDOUT_FILENO) != 0; break;
      case ckerr: r = ::isatty(STDERR_FILENO) != 0; break;
      default: return 0;
    }
    #else
    {
      switch(check) {
        case ckin: {
          DWORD mode;
          r = ::GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
          break;
        }
        case ckout: {
          CONSOLE_SCREEN_BUFFER_INFO sbi;
          r = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi) != 0;
          break;
        }
        case ckerr: {
          CONSOLE_SCREEN_BUFFER_INFO sbi;
          r = GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &sbi) != 0;
          break;
        }
        default:
          return 0;
      }
    }
    #endif
    stack.push(r);
    return 1;
  }

  template <typename=void>
  int mundane_beep(duktape::api& stack)
  {
    using namespace std;
    const auto frequency = std::min(std::max(stack.get<int>(0), 80), 12000);
    const auto duration  = std::min(int(stack.get<double>(1) * 1000), 1000);
    if(duration < 10) return 0;
    #if defined(OS_WINDOWS)
      ::Beep(DWORD(frequency), DWORD(duration));
      stack.top(0);
      stack.push(true);
      return 1;
    #elif defined(__linux__)
      stack.top(0);
      stack.push(false);
      const auto tick_rate = 1193180;
      if(tick_rate <= frequency) return 1;
      const auto periodck = static_cast<unsigned long>(tick_rate) / static_cast<unsigned long>(frequency);
      const auto ioarg = ((static_cast<unsigned long>(duration)<<16ul) & 0xffff0000ul) | ((static_cast<unsigned long>(periodck)<< 0ul) & 0x0000fffful);
      const auto fd = ::open("/dev/console", O_WRONLY);
      if(fd < 0) return 1;
      const auto iok = ::ioctl(STDOUT_FILENO, KDMKTONE, ioarg) >= 0;
      ::close(fd);
      if(!iok) return 1;
      stack.top(0);
      stack.push(true);
      return 1;
    #else
      return 0;
    #endif
  }

}}}

namespace duktape { namespace mod { namespace system {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace ::duktape::detail::system;

    #if(0 && JSDOC)
    /**
     * Operating system functionality object.
     * @var {object}
     */
    var sys = {};
    #endif

    #if(0 && JSDOC)
    /**
     * Returns the ID of the current process or `undefined` on error.
     *
     * @return {number|undefined}
     */
    sys.pid = function() {};
    #endif
    js.define("sys.pid", getpid<>, 0);

    #if(0 && JSDOC)
    /**
     * Returns the ID of the current user or `undefined` on error.
     *
     * @return {number|undefined}
     */
    sys.uid = function() {};
    #endif
    js.define("sys.uid", getuid<>, 0);

    #if(0 && JSDOC)
    /**
     * Returns the ID of the current group or `undefined` on error.
     *
     * @return {number|undefined}
     */
    sys.gid = function() {};
    #endif
    js.define("sys.gid", getgid<>, 0);

    #if(0 && JSDOC)
    /**
     * Returns the login name of a user or `undefined` on error.
     * If the user ID is not specified (called without arguments), the ID of the current user is
     * used.
     *
     * @param {number} [uid]
     * @return {string|undefined}
     */
    sys.user = function(uid) {};
    #endif
    js.define("sys.user", getuser<>, 1);

    #if(0 && JSDOC)
    /**
     * Returns the group name of a group ID or `undefined` on error.
     * If the group ID is not specified (called without arguments), the group ID of the
     * current user is used.
     *
     * @param {number} [gid]
     * @return {string|undefined}
     */
    sys.group = function(gid) {};
    #endif
    js.define("sys.group", getgroup<>, 1);

    #if(0 && JSDOC)
    /**
     * Returns a plain object containing information about the operating system
     * running on, `undefined` on error. The object looks like:
     *
     * {
     *   sysname: String, // e.g. "Linux"
     *   release: String, // e.g. ""3.16.0-4-amd64"
     *   machine: String, // e.g. "x86_64"
     *   version: String, // e.g. "#1 SMP Debian 3.16.7-ckt20-1+deb8u3 (2016-01-17)"
     * }
     *
     * @return {object|undefined}
     */
    sys.uname = function() {};
    #endif
    js.define("sys.uname", getuname<>, 0);

    #if(0 && JSDOC)
    /**
     * Makes the thread sleep for the given time in seconds (with sub seconds).
     * Note that this function blocks the complete thread until the time has
     * expired or sleeping is interrupted externally.
     *
     * @param {number} seconds
     * @return {boolean}
     */
    sys.sleep = function(seconds) {};
    #endif
    js.define("sys.sleep", sleep_seconds<>, 1);

    #if(0 && JSDOC)
    /**
     * Returns the time in seconds (with sub seconds) of a selected
     * time/clock source:
     *
     *  - "r": Real time clock (value same as Date object, maybe
     *         higher resolution)
     *  - "b": Boot time (if available, otherwise equal to "m" source)
     *
     *  - "m": Monotonic time, starts at zero when the function
     *         is first called.
     *
     * Returns NaN on error or when a source is not supported on the
     * current platform.
     *
     * @param {string} clock_source
     * @return {number} seconds
     */
    sys.clock = function(clock_source) {};
    #endif
    js.define("sys.clock", clock_seconds<>, 1);

    #if(0 && JSDOC)
    /**
     * Returns true if the descriptor given as string
     * is an interactive TTY or false otherwise, e.g.
     * when connected to a pipe / file source. Valid
     * descriptors are:
     *
     *  - "stdin"  or "i": STDIN  (standard input read with confirm, prompt etc)
     *  - "stdout" or "o": STDOUT (standard output fed by print())
     *  - "stderr" or "e": STDERR (standard error output, e.g. fed by alert())
     *
     * The function returns undefined if not implemented on the
     * platform or if the descriptor name is incorrect.
     *
     * @param {string} descriptorName
     * @return {boolean}
     */
    sys.isatty = function(descriptorName) {};
    #endif
    js.define("sys.isatty", isatty_by_name, 1);

    #if(0 && JSDOC)
    /**
     * Returns path ("realpath") of the executable where the ECMA script is
     * called from (or undefined on error or if not allowed).
     *
     * @return {string|undefined}
     */
    sys.executable = function() {};
    #endif
    js.define("sys.executable", app_path, 0);

    #if(0 && JSDOC)
    /**
     * Mundane auditive beeper signal with a given frequency in Hz and duration
     * in seconds. Only applied if the hardware and system supports beeping,
     * otherwise no action. Returns true if the system calls were successful.
     *
     * @param {number} frequency
     * @param {number} duration
     * @return {boolean}
     */
    sys.beep = function() {};
    #endif
    js.define("sys.beep", mundane_beep, 2);

  }

}}}

#endif
