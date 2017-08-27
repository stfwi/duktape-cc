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

  #if(0 && JSDOC)
  /**
   * print(a,b,c, ...). Writes data stringifyed to STDOUT.
   *
   * @param {...*} args
   */
  print = function(args) {}
  #endif
  static int print(duktape::api& stack)
  { return print_to(stack, out_stream); }

  #if(0 && JSDOC)
  /**
   * alert(a,b,c, ...). Writes data stringifyed to STDERR.
   *
   * @param {...*} args
   */
  alert = function(args) {}
  #endif
  static int alert(duktape::api& stack)
  { return print_to(stack, err_stream); }

  #if(0 && JSDOC)
  /**
   * Returns a first character entered as string. First function argument
   * is a text that is printed before reading the input stream (without newline,
   * "?" or ":"). The input is read from STDIN.
   *
   * @param {string} text
   * @returns {string}
   */
  confirm = function(text) {}
  #endif
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

  #if(0 && JSDOC)
  /**
   * Returns a line entered via the input stream STDIN.
   *
   * @returns {string}
   */
  prompt = function() {};
  #endif
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

  #if(0 && JSDOC)
  /**
   * Console object known from various JS implementations.
   */
  console = {};
  #endif

  #if(0 && JSDOC)
  /**
   * Writes a line to the log stream (automatically appends a newline).
   * The default log stream is STDERR.
   *
   * @param {...*} args
   */
  console.log = function(args) {};
  #endif
  static int console_log(duktape::api& stack)
  { return print_to(stack, log_stream); }

  #if(0 && JSDOC)
  /**
   * Write to STDOUT without any conversion, whitespaces between the given arguments,
   * and without newline at the end.
   *
   * @param {...*} args
   */
  console.write = function(args) {};
  #endif
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

  #if(0 && JSDOC)
  /**
   * Read from STDIN until EOF. Using the argument `arg` it is possible
   * to use further functionality:
   *
   *  - By default (if `arg` is undefined or not given) the function
   *    returns a string when all data are read.
   *
   *  - If `arg` is boolean and `true`, then the input is read into
   *    a buffer variable, not a string. The function returns when
   *    all data are read.
   *
   * - If `arg` is a function, then a string is read line by line,
   *   and each line passed to this callback function. If the function
   *   returns `true`, then the line is added to the output. The
   *   function may also preprocess line and return a String. In this
   *   case the returned string is added to the output.
   *   Otherwise, if `arg` is not `true` and no string, the line is
   *   skipped.
   *
   * @param {function|boolean} arg
   * @returns {string|buffer}
   */
  console.read = function(arg) {};
  #endif
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
        if(stack.is_string(-1)) {
          ss << stack.get_string(-1) << std::endl;
        } else if(stack.get<bool>(-1)) {
          ss << s << std::endl;
        }
        stack.top(1);
      }
      stack.top(0);
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
