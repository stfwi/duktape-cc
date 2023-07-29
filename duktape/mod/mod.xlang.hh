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

  namespace {
    /**
     * @from swlib-cc:alg
     *
     * Returns linear regression (poly-fit 1st order) coefficients
     * by fitting the given vectors/forward iterable numeric containers
     * using least error squares. The returned array holds the coefficients
     * in ascending order, so that the fit function evaluates to:
     *
     *  y = coeffs[1] * x + coeffs[0];
     *
     * Input containers are `x` (independent) and `y` (dependent). The
     * function throws a `std::runtime_error` if the containers do not
     * have the same size or are empty. For integral coeff types beware
     * arithmetic size and sign-ness.
     *
     * @tparam CoeffType
     * @tparam ValueContainerType
     * @param const ValueContainerType& x
     * @param const ValueContainerType& y
     * @return std::array<CoeffType, 2u>
     */
    template <
      typename CoeffType,           // Returned fit coefficient type.
      typename ValueContainerType   // Random access container with numeric values, deduced from arguments.
    >
    std::array<CoeffType, 2u> linear_fit(const ValueContainerType& x, const ValueContainerType& y)
    {
      using coeff_type = std::decay_t<CoeffType>;
      using vect_type  = ValueContainerType;
      using value_type = typename std::decay_t<typename vect_type::value_type>;
      static_assert(std::is_arithmetic_v<coeff_type> && !std::is_pointer_v<coeff_type>, "Arithmetic coefficient type is no number.");
      static_assert(std::is_arithmetic_v<value_type> && !std::is_pointer_v<value_type>, "Arithmetic value type is no number.");
      const auto size = x.size();
      if(size != y.size()) throw std::runtime_error("Cannot fit, x and y data do not have the same size.");
      if(size == 0) throw std::runtime_error("Cannot fit, x and y data are empty.");
      // Sums for x, y, square sum for x.
      const auto sx  = std::accumulate(x.begin(), x.end(), coeff_type(0));
      const auto sy  = std::accumulate(y.begin(), y.end(), coeff_type(0));
      const auto sxx = std::accumulate(x.begin(), x.end(), coeff_type(0), [](auto acc, auto x){ return acc + x*x; });
      // Cross element sum xy.
      auto y_it = y.cbegin();
      auto sxy = coeff_type(0);
      for(auto vx: x) { sxy += vx * (*y_it++); }
      // Linear coeffs, see Numerical Algorithms in C.
      const auto n = coeff_type(size);
      const auto c1 = (n * sxy - sx * sy) / (n * sxx - sx * sx);
      const auto c0 = (sy - c1 * sx) / n;
      // Polynomial format coefficient return, ascending order.
      return std::array<coeff_type, 2u>{ c0, c1 };
    }

  }

  template<typename=void>
  int math_linear_fit(duktape::api& stack)
  {
    if((stack.top()!=2) || (!stack.is_array(0)) || (!stack.is_array(1))) return stack.throw_exception("Math.linfit requires numeric arrays x and y.");
    const auto x = stack.get<std::vector<double>>(0);
    const auto y = stack.get<std::vector<double>>(1);
    stack.top(0);
    const auto coeffs = linear_fit<double>(x, y);
    stack.push_object();
    stack.set("offset", coeffs[0]);
    stack.set("slope", coeffs[1]);
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
     * Linear regression fitting of two value arrays (x and y)
     * using least error square 1st order polynomial fitting.
     * Returns an object containing `slope` and `offset` of
     * the best fitting line. The function throws if the arrays
     * do not have numeric values, or do not have the same size,
     * or are empty.
     *
     * @param {array} x_values
     * @param {array} y_values
     * @return {object}
     */
    Math.linfit = function(x_values, y_values) {};
    #endif
    js.define("Math.linfit", math_linear_fit<>, 2);

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
