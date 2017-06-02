/**
 * @file duktape/mod/mod.stdlib.hh
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
 * Optional basic standard functions.
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
#ifndef DUKTAPE_STDLIB_HH
#define DUKTAPE_STDLIB_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "../duktape.hh"
#include <iostream>
#include <streambuf>
#include <sstream>
// </editor-fold>

// <editor-fold desc="duktape::stdlib" defaultstate="collapsed">
namespace duktape { namespace detail {

template <typename=void>
struct stdlib {

  /**
   * Throws an exit_exception, which is handled in the wrappers
   * if this file, and eventually thrown out of the invoked
   * (root) engine function. You have to define this function
   * in the engine if you like to allow exit() calls in JS.
   *
   * @param api& stack
   * @return int === 0
   */
  static int exit_js(api& stack)
  { throw exit_exception((stack.top() <= 0) ? 0 : stack.to<int>(-1)); return 0; }

  /**
   * Define stdlib functions in the standard locations.
   *
   * @param duktape::engine& js
   */
  static void define_in(duktape::engine& js)
  {
    js.define("exit", exit_js);
  }
};

}}

namespace duktape { namespace mod {
  using stdlib = detail::stdlib<void>;
}}

// </editor-fold>

#endif
