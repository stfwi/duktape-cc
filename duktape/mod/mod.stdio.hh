/**
 * @file duktape/mod/mod.stdio.hh
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
 * Optional I/O functions for stdin, stdout and stderr.
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
#ifndef DUKTAPE_STDIO_HH
#define DUKTAPE_STDIO_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "../duktape.hh"
#include <iostream>
#include <streambuf>
#include <sstream>
// </editor-fold>

// <editor-fold desc="duktape::stdio" defaultstate="collapsed">
namespace duktape { namespace detail {

template <typename=void>
struct stdio
{
  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  static void define_in(duktape::engine& js)
  {
    js.define("print", print);
    js.define("alert", alert);
    js.define("console.log", console_log);
    js.define("console.read", console_read);
    js.define("console.write", console_write);
    js.define("confirm", confirm);
    js.define("prompt", prompt);
  }

public:

  /**
   * print(a,b,c, ...). Writes to the `out_stream` set in this class (default: std::cout).
   * Buffers are printed as binary stream.
   *
   * @param duktape::api& stack
   * @return int
   */
  static int print(duktape::api& stack)
  { return print_to(stack, out_stream); }

  /**
   * alert(a,b,c, ...). Writes to the `err_stream` set in this class (default: std::cerr).
   * Buffers are printed as binary stream.
   *
   * @param duktape::api& stack
   * @return int
   */
  static int alert(duktape::api& stack)
  { return print_to(stack, err_stream); }

  /**
   * Returns a first character entered as string. First program argument
   * is a text that is printed before reading the input stream (without newline,
   * "?" or ":"). The stream read from is defined as `in_stream` of this class,
   * default is std::cin).
   *
   * @param duktape::api& stack
   * @return int
   */
  static int confirm(duktape::api& stack)
  {
    if(!in_stream) return 0;
    if(!!out_stream) {
      if(stack.top() > 0) {
        (*out_stream) << stack.to<std::string>(0);
      } else {
        (*out_stream) << "[press ENTER to continue ...]";
      }
    }
    std::string s;
    std::getline(*in_stream, s);
    if(s.length() > 0) s = s[0];
    stack.push(s);
    return 1;
  }

  /**
   * Returns a line entered via the input stream (default std::cin).
   *
   * @param duktape::api& stack
   * @return int
   */
  static int prompt(duktape::api& stack)
  {
    if(!in_stream) return 0;
    if(!!out_stream) {
      if(stack.top() > 0) {
        (*out_stream) << stack.to<std::string>(0);
      }
    }
    std::string s;
    std::getline(*in_stream, s);
    stack.push(s);
    return 1;
  }

  /**
   * Writes a line to the log stream (automatically appends a newline).
   * The default log stream is stderr, but it can be redefined in (class variable).
   *
   * @param duktape::api& stack
   * @return int
   */
  static int console_log(duktape::api& stack)
  { return print_to(stack, log_stream); }

  /**
   * Write to STDOUT without any conversion, whitespaces between arguments given,
   * and without newline at the end.
   *
   * @param duktape::api& stack
   * @return int
   */
  static int console_write(duktape::api& stack)
  {
    if(!out_stream) return 0;
    int nargs = stack.top();
    for(int i=0; i<nargs; ++i) {
      if(stack.is_buffer(i)) {
        const char *buf = NULL;
        duk_size_t sz = 0;
        if((buf = (const char *) stack.get_buffer(0, sz)) && (sz > 0)) {
          (*out_stream).write(buf, sz);
        }
      } else {
        (*out_stream) << stack.to<std::string>(i);
      }
    }
    (*out_stream).flush();
    return 0;
  }

  /**
   * Read from stdin until EOF.
   *
   * @param duktape::api& stack
   * @return int
   */
  static int console_read(duktape::api& stack)
  {
    if(!in_stream) return 0;
    int nargs = stack.top();
    if((nargs > 0) && stack.is_callable(0)) {
      // Filtered string from lines
      std::stringstream ss;
      while((*in_stream).good()) {
        std::string s;
        std::getline((*in_stream), s);
        stack.dup(0);
        stack.push_string(s);
        stack.call(1);
        bool use = stack.get<bool>(-1);
        stack.pop();
        if(use) ss << s << std::endl;
      }
      stack.push(ss.str());
      return 1;
    } else {
      std::string s((std::istreambuf_iterator<char>(*in_stream)),
                   std::istreambuf_iterator<char>());

      if((nargs > 0) && stack.is<bool>(0) && stack.get<bool>(0)) {
        // Binary without filter function
        char* buf = (char*) stack.push_buffer(s.length(), false);
        if(!buf) {
          throw script_error("Failed to read binary data from console (buffer allocation failed)");
        } else {
          std::copy(s.begin(), s.end(), buf);
        }
      } else {
        // String without filter function
        stack.push_string(s);
      }
      return 1;
    }
  }

public:

  static std::ostream* out_stream;
  static std::ostream* err_stream;
  static std::ostream* log_stream;
  static std::istream* in_stream;

protected:

  static int print_to(duktape::api& stack, std::ostream* stream)
  {
    if(!stream) return 0;
    int nargs = stack.top();
    if((nargs == 1) && stack.is_buffer(0)) {
      const char *buf = NULL;
      duk_size_t sz = 0;
      if((buf = (const char *) stack.get_buffer(0, sz)) && sz > 0) {
        (*stream).write(buf, sz);
        (*stream).flush();
      }
    } else if(nargs > 0) {
      (*stream) << stack.to<std::string>(0);
      for(int i=1; i<nargs; i++) {
        std::string sss = stack.to<std::string>(i);
        (*stream) << " " << sss;
      }
      (*stream) << std::endl;
      (*stream).flush();
    }
    return 0;
  }
};

template <typename T>
std::ostream* stdio<T>::out_stream = &std::cout;

template <typename T>
std::ostream* stdio<T>::err_stream = &std::cerr;

template <typename T>
std::ostream* stdio<T>::log_stream = &std::cerr;

template <typename T>
std::istream* stdio<T>::in_stream = &std::cin;

}}

namespace duktape { namespace mod {
  using stdio = detail::stdio<void>;
}}

// </editor-fold>

#endif
