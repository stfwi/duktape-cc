/**
 * @file duktape/mod/mod.xlang.hh
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
 * Optional cross-language convenience extensions.
 *
 *    WARNING, THIS EXTENSION ADDS METHODS TO THE PROTOTYPES OF
 *    BASIC DATA TYPES. THIS MAY HAVE UNWANTED SIDE EFFECTS IN
 *    FUTURE VERSIONS OF JS.
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
#ifndef DUKTAPE_CROSS_LANGUAGE_EXTENSIONS_HH
#define DUKTAPE_CROSS_LANGUAGE_EXTENSIONS_HH

#include "../duktape.hh"
#include <algorithm>
#include <string>


namespace duktape { namespace detail { namespace xlang {

  /**
   * Numeric min-max clamping.
   */
  template<typename=void>
  int number_limit(duktape::api& stack)
  {
    if((stack.top()!=2) || (!stack.is_number(0)) || (!stack.is_number(1))) return stack.throw_exception("Number.limit requires numeric arguments min and max.");
    const auto min = stack.get<double>(0);
    const auto max = stack.get<double>(1);
    stack.top(0);
    stack.push_this();
    if(!stack.is_number(0)) return stack.throw_exception("Number.limit() called on non-number data.");
    const auto val = stack.get<double>(0);
    stack.push(std::max(min, std::min(max, val)));
    return 1;
  }

}}}

namespace duktape { namespace mod { namespace xlang {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace ::duktape::detail::xlang;

    #if(0 && JSDOC)
    /**
     * Returns the limited ("clamped") value of the number.
     * Minimum and maximum value have to be specified.
     *
     * @param {number} min
     * @param {number} max
     */
    Number.prototype.limit = function(min, max) {};
    #endif
    js.define("Number.prototype.limit", number_limit<>, 2);

    #if(0 && JSDOC)
    /**
     * Returns the limited ("clamped") value of the number.
     * Minimum and maximum value have to be specified. Alias
     * of `Number.prototype.limit`.
     * @see Number.prototype.limit
     * @param {number} min
     * @param {number} max
     */
    Number.prototype.clamp = function(min, max) {};
    #endif
    js.define("Number.prototype.clamp", number_limit<>, 2);
  }

}}}

#endif
