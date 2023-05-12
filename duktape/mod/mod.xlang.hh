/**
 * @file duktape/mod/mod.xlang.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++14
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

  /**
   * Iterative object operation selection.
   */
  enum class iterate_mode { foreach, any, none, all };

  /**
   * Iterative object/array operations: each/forEach, and
   * predicate queries `any()`, `all()`, `none()`.
   */
  template<iterate_mode Mode>
  int iterate_object(duktape::api& stack)
  {
    if((stack.top()!=1) || (!stack.is_callable(0))) return stack.throw_exception("Require a function to iterate with.");
    stack.push_this();
    const auto numerically_indexed = stack.is_array(1);
    if(numerically_indexed) {
      stack.enumerator(1, stack.enum_array_indices_only|stack.enum_sort_array_indices);
    } else if(stack.is_object(1)) {
      stack.enumerator(1, stack.enum_own_properties_only);
    } else {
      throw duktape::script_error("Functional iteration can only be called on arrays or objects.");
    }
    stack.swap(0,2);              // [enum,this,fn]
    stack.swap(1,2);              // [enum,fn,this]
    while(stack.next(0, true)) {  // [enum,fn,this,key,value]
      stack.dup(1);               // [enum,fn,this,key,value,fn]
      stack.swap(3,5);            // [enum,fn,this,fn,value,key]
      stack.dup(2);               // [enum,fn,this,fn,value,key,this]
      if(numerically_indexed) stack.to_int(5);
      if(stack.pcall(3)) {        // [enum,fn,this,result]
        stack.swap_top(0);
        stack.top(1);
        return stack.throw_exception();
      }
      switch(Mode) {
        case iterate_mode::foreach: {
          break;
        }
        case iterate_mode::any: {
          if(stack.is_true(-1)) { stack.top(0); stack.push(true); return 1; }
          break;
        }
        case iterate_mode::none: {
          if(stack.is_true(-1)) { stack.top(0); stack.push(false); return 1; }
          break;
        }
        case iterate_mode::all: {
          if(!stack.is_true(-1)) { stack.top(0); stack.push(false); return 1; }
          break;
        }
      }
      stack.top(3);
    }
    stack.top(0);
    switch(Mode) {
      case iterate_mode::foreach: { return 0; }
      case iterate_mode::any:     { stack.push(false); return 1; }
      case iterate_mode::none:    { stack.push(true); return 1; }
      case iterate_mode::all:     { stack.push(true); return 1; }
    }
    return 0;
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
    const auto flags = js.define_flags();
    js.define_flags(duktape::engine::defflags::restricted);

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

    #if(0 && JSDOC)
    /**
     * Iterates over an array (ascending indices) or object
     * (own properties only) using a given function. Arguments
     * passed into that function are the value, the key/index,
     * and the referred object.
     * The `forEach()` method does not return a value.
     * Example:
     *
     *    obj.forEach(function(value, key, this_ref){...});
     *    arr.forEach(function(value, index, this_ref){...});
     *
     * @param {function} func
     * @return {undefined}
     */
    Object.prototype.forEach = function(func) {};
    #endif
    js.define("Object.prototype.forEach", iterate_object<iterate_mode::foreach>, 1);

    #if(0 && JSDOC)
    /**
     * Iterates over an array (ascending indices) or object
     * (own properties only) using a given processing function.
     * Arguments passed into that function are the value, the
     * key/index, and the referred object. The `each()` method
     * does not return a value. Example:
     *
     *    obj.each(function(value, key, this_ref){...});
     *    arr.each(function(value, index, this_ref){...});
     *
     * @param {function} func
     * @return {undefined}
     */
    Object.prototype.each = function(func) {};
    #endif
    js.define("Object.prototype.each", iterate_object<iterate_mode::foreach>, 1);

    #if(0 && JSDOC)
    /**
     * Returns true if all elements of the referred (`this`) object or array
     * matches the given predicate function. That is, if the given function
     * returns `true` when being invoked with all of the object/array elements.
     *
     * Example:
     *
     *    if(obj.every(function(value, key  , this_ref){...})) { ... }
     *    if(arr.every(function(value, index, this_ref){...})) { ... }
     *
     * @param {function} predicate
     * @return {boolean}
     */
    Object.prototype.every = function(predicate) {};
    #endif
    js.define("Object.prototype.every", iterate_object<iterate_mode::all>, 1);

    #if(0 && JSDOC)
    /**
     * Alias of `Object.prototype.every()`.
     *
     * @param {function} predicate
     * @return {boolean}
     */
    Object.prototype.all = function(predicate) {};
    #endif
    js.define("Object.prototype.all", iterate_object<iterate_mode::all>, 1);

    #if(0 && JSDOC)
    /**
     * Returns true if at least one element of the referred (`this`)
     * object or array matches the given predicate function. That is,
     * if the given function returns `true` when being invoked with
     * the object/array elements.
     *
     * Example:
     *
     *    if(obj.some(function(value, key  , this_ref){...})) { ... }
     *    if(arr.some(function(value, index, this_ref){...})) { ... }
     *
     * @param {function} predicate
     * @return {boolean}
     */
    Object.prototype.some = function(predicate) {};
    #endif
    js.define("Object.prototype.some", iterate_object<iterate_mode::any>, 1);

    #if(0 && JSDOC)
    /**
     * Alias of `Object.prototype.some(predicate)`.
     *
     * @param {function} predicate
     * @return {boolean}
     */
    Object.prototype.any = function(predicate) {};
    #endif
    js.define("Object.prototype.any", iterate_object<iterate_mode::any>, 1);

    #if(0 && JSDOC)
    /**
     * Returns true if not one single element of the referred (`this`)
     * object or array matches the given predicate function. That is,
     * if the given function does **not** return `true` when being invoked
     * with any of the object/array elements.
     *
     * Example:
     *
     *    if(obj.none(function(value, key  , this_ref){...})) { ... }
     *    if(arr.none(function(value, index, this_ref){...})) { ... }
     *
     * @param {function} predicate
     * @return {boolean}
     */
    Object.prototype.none = function(predicate) {};
    #endif
    js.define("Object.prototype.none", iterate_object<iterate_mode::none>, 1);

    #if(0 && JSDOC)
    /**
     * Returns a string with whitespaces stripped off at
     * the begin and end.
     * @return {string}
     */
    String.prototype.trim = function() {};
    #endif
    js.eval<void>(";Object.defineProperty(String.prototype, 'trim', {value:function(){return this.replace(/^\\s+/,'').replace(/\\s+$/,'')}, configurable:false, writable:false, enumerable:false});"); // Initially not worth doing this with c++.

    js.define_flags(flags);
  }

}}}

#endif
