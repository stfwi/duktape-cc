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
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional basic system functionality.
 *
 * -----------------------------------------------------------------------------
 * License: http://opensource.org/licenses/MIT
 * Copyright (c) 2014-2017, the authors (see the @authors tag in this file).
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

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#if defined(_MSCVER)
  #error "not yet implemented"
#endif

#include "../duktape.hh"
#include <algorithm>
#include <chrono>
#include <thread>
#include <cmath>
#include <unistd.h>
#if defined(__MINGW32__) || defined(__MINGW64__)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
  #include <windows.h>
  #include <Lmcons.h>
#else
  #include <sys/utsname.h>
  #include <pwd.h>
  #include <grp.h>
#endif
// </editor-fold>

namespace duktape { namespace detail {

// <editor-fold desc="conv<unix_timestamp>" defaultstate="collapsed">
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
// </editor-fold>

// <editor-fold desc="conv<struct ::timespec>" defaultstate="collapsed">
#if defined(__linux) || defined(__linux__) || defined(__MINGW32__) || defined(__MINGW64__)
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
// </editor-fold>

}}

namespace duktape { namespace detail { namespace system {

  // <editor-fold desc="process/user/group information" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Returns the ID of the current process or `undefined` on error.
   *
   * @return Number|undefined
   */
  sys.pid = function() {};
  #endif
  template <typename=void>
  int getpid(duktape::api& stack)
  {
    #ifndef WINDOWS
    pid_t val = ::getpid();
    if(val < 0) return 0;
    stack.push(val);
    return 1;
    #else
    stack.push(_getpid());
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns the ID of the current user or `undefined` on error.
   *
   * @return Number|undefined
   */
  sys.uid = function() {};
  #endif
  template <typename=void>
  int getuid(duktape::api& stack)
  {
    #ifndef WINDOWS
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

  #if(0 && JSDOC)
  /**
   * Returns the ID of the current group or `undefined` on error.
   *
   * @return Number|undefined
   */
  sys.gid = function() {};
  #endif
  template <typename=void>
  int getgid(duktape::api& stack)
  {
    #ifndef WINDOWS
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

  #if(0 && JSDOC)
  /**
   * Returns the login name of a user or `undefined` on error.
   * If the user ID is not specified (called without arguments), the ID of the current user is
   * used.
   *
   * @param undefined|Number uid
   * @return String|undefined
   */
  sys.user = function(uid) {};
  #endif
  template <typename=void>
  int getuser(duktape::api& stack)
  {
    #ifndef WINDOWS
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

  #if(0 && JSDOC)
  /**
   * Returns the group name of a group ID or `undefined` on error.
   * If the group ID is not specified (called without arguments), the group ID of the
   * current user is used.
   *
   * @param undefined|Number gid
   * @return String|undefined
   */
  sys.group = function(gid) {};
  #endif
  template <typename=void>
  int getgroup(duktape::api& stack)
  {
    #ifndef WINDOWS
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

  // </editor-fold>

  // <editor-fold desc="platform information" defaultstate="collapsed">
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
   * @return Object|undefined
   */
  sys.uname = function() {};
  #endif
  template <typename=void>
  int getuname(duktape::api& stack)
  {
    stack.push_object();
    #if defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux) /*note:linux implies unix*/
    struct ::utsname un;
    if(::uname(&un) != 0) return 0;
    stack.set("sysname", (const char*)un.sysname);
    stack.set("release", (const char*)un.release);
    stack.set("machine", (const char*)un.machine);
    stack.set("version", (const char*)un.version);
    #elif defined(WIN32) || defined(_WIN32) || defined(__TOS_WIN__) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
      stack.set("sysname", "windows");
    #else
      stack.set("sysname", "unknown");
    #endif
    return 1;
  }
  // </editor-fold>

  // <editor-fold desc="misc" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Makes the thread sleep for the given time in seconds (with sub seconds).
   * Note that this function blocks the complete thread until the time has
   * expired or sleeping is interrupted externally.
   *
   * @param Number seconds
   * @return bool
   */
  sys.sleep = function(seconds) {};
  #endif
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
  // </editor-fold>

}}}

namespace duktape { namespace mod { namespace system {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace ::duktape::detail::system;
    js.define("sys.pid", getpid<>, 0);
    js.define("sys.uid", getuid<>, 0);
    js.define("sys.gid", getgid<>, 0);
    js.define("sys.user", getuser<>, 1);
    js.define("sys.group", getgroup<>, 1);
    js.define("sys.uname", getuname<>, 0);
    js.define("sys.sleep", sleep_seconds<>, 1);
  }
  // </editor-fold>

}}}

#endif
