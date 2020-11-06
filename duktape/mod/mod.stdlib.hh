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

#include "../duktape.hh"
#include <iostream>
#include <streambuf>
#include <sstream>

namespace duktape { namespace detail {

  template <typename=void>
  struct stdlib {

    #if(0 && JSDOC)
    /**
     * Exits the script interpreter with a specified exit code.
     *
     * @param {number} status_code
     */
    exit = function(status_code) {};
    #endif
    static int exit_js(api& stack)
    { throw exit_exception((stack.top() <= 0) ? 0 : stack.to<int>(-1)); return 0; }

    #if(0 && JSDOC)
    /**
     * Includes a JS file and returns the result of
     * the last statement.
     * Note that `include()` is NOT recursion protected.
     *
     * @param {string} path
     * @return {any}
     */
    include = function(path) {};
    #endif
    static int include_file(api& stack)
    {
      std::string path = stack.get_string(0);
      std::ifstream is;
      is.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
      std::string code((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
      if(!is) return stack.throw_exception(std::string("Failed to read include file '") + path + "'");
      is.close();
      stack.top(0);
      stack.require_stack(3);
      stack.push_string(std::move(code));
      stack.push_string(path);
      try {
        stack.eval_raw(0, 0, DUK_COMPILE_EVAL | DUK_COMPILE_SHEBANG);
      } catch(const exit_exception& e) {
        stack.top(0);
        stack.gc();
        throw;
      }
      return 1;
    }

    /**
     * Define stdlib functions in the standard locations.
     *
     * @param duktape::engine& js
     */
    static void define_in(duktape::engine& js)
    {
      js.define("exit", exit_js);
      js.define("include", include_file);
    }
  };

}}

namespace duktape { namespace mod {
  using stdlib = detail::stdlib<void>;
}}

#endif
