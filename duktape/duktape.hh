/**
 * @file duktape.hh
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
#ifndef DUKTAPE_HH
#define DUKTAPE_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include <limits.h>
#if (!defined(UINT_MAX)) || (UINT_MAX < 0xffffffffUL)
#error "This interface requires at least 32 bit integer size for type int."
#endif
#if (!defined(__cplusplus)) || (__cplusplus < 201103L)
#error "You have to compile with at least std=c++11"
#endif
#include <cstdint>
#undef DUK_SIZE_MAX_COMPUTED
#define DUK_SIZE_MAX_COMPUTED
#include "duktape.h"
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <utility>
#include <memory>
#include <functional>
#include <type_traits>
#include <mutex>

#ifdef WITH_DUKTAPE_HH_ASSERT
#include <cassert>
#define dukcc_assert(X) assert(X)
#else
#define dukcc_assert(X)
#endif
// </editor-fold>

// <editor-fold desc="forwards" defaultstate="collapsed">
namespace duktape {
  namespace detail {
    template <typename R=void> class basic_api;
    template <typename R=void> class basic_stack_guard;
    template <typename MutexType=std::recursive_timed_mutex> class basic_engine;
    template <typename T> struct conv;
  }

  using api = detail::basic_api<>;
  using engine = detail::basic_engine<>;
  using stack_guard = detail::basic_stack_guard<>;
}
// </editor-fold>

// <editor-fold desc="exceptions" defaultstate="collapsed">
namespace duktape {

  namespace detail {
    template <typename=void>
    class basic_engine_error : public std::runtime_error
    {
      public:
        explicit basic_engine_error(const std::string& msg) : std::runtime_error(msg)  { ; }
    };

    template <typename=void>
    class basic_script_error : public std::runtime_error
    {
      public:
        explicit basic_script_error(const std::string&& msg) : std::runtime_error(std::move(msg)), callstack_()
        {}

        explicit basic_script_error(std::string&& msg, std::string&& callstack)
                  : std::runtime_error(std::move(msg)), callstack_(std::move(callstack)) {}

        const std::string& callstack() const noexcept
        { return callstack_; }

      private:
        std::string callstack_;
    };

    template <typename=void>
    class basic_exit_exception
    {
      public:
        explicit basic_exit_exception() : code_(0) { ; }
        explicit basic_exit_exception(int code) : code_(code) { ; }
        const char* what() const noexcept { return "exit"; }
        int exit_code() const noexcept { return code_; }
      private:
        int code_;
    };
  }

  /**
   * Thrown on (fatal) errors related to C/C++ issues, allocation, bugs/wrong usage, etc.
   */
  using engine_error = detail::basic_engine_error<>;

  /**
   * Thrown on ECMA runtime related errors.
   */
  using script_error = detail::basic_script_error<>;

  /**
   * Thown to indicate that the engine shall exit.
   * Not derived from std::exception and only interpreted from wrapper functions
   * for cleanup/finalisation purposes.
   */
  using exit_exception = detail::basic_exit_exception<>;
}
// </editor-fold>

// <editor-fold desc="auxiliary functions" defaultstate="collapsed">
namespace duktape { namespace detail { namespace {

template <typename=void>
struct aux
{
  static bool split_selector(std::string name, std::string& base, std::string& tail)
  {
    auto p = name.find_last_of('.');
    if(p == name.npos) {
      if(name.empty()) {
        // invalid
        base.clear();
        tail.clear();
        return false;
      } else {
        // no "."
        base.clear();
        tail.swap(name);
        return true;
      }
    } else {
      if((!p) || (p == name.length()-1)) {
        // invalid
        base.clear();
        tail.swap(name);
        return false;
      } else {
        // ok
        tail = name.substr(p+1);
        base.swap(name);
        base.resize(p);
        return true;
      }
    }
  }
};

}}}
// </editor-fold>

// <editor-fold desc="api" defaultstate="collapsed">
namespace duktape { namespace detail {

  /**
   * Class to reflect the duktape C API with minor interface modifications to ease
   * the usage in modern C++ IDE and code readability.
   *
   *  - The method names correspond to the API function names without the prefix `duk_`.
   *
   *  - The duktape context pointer is stored in a protected variable and applied
   *    to all calls. It can be passed to the constructor.
   *
   *  - Typedefs are set for duktape types, enumerations for exported macros.
   *
   *  - Boolean return values are `bool`
   *
   *  - String functions are generally translated to std::string, only `lstring` functions
   *    are used (not the c-string API functions). On duktape conversion or allocation errors,
   *    the returned strings are empty.
   *
   *  - duk_int_t / duk_uint_t are `int` / `unsigned` (as at least 32 bit int size is checked
   *    in this file). These are the types that duktape also chooses if `unsigned int` is at
   *    least 32 bits.
   *
   *  - sprintf / vsprintf like API functions are omitted.
   *
   *  - Function duk_throw is method `throw_exception`
   *  - Function duk_enum is method `enumerator`
   *
   * Note: The pointer passed to the constructor is NOT seen to be owned by this class,
   *       hence, the destructor will NOT free any memory related to this context.
   */
  template <typename>
  class basic_api
  {
    // <editor-fold desc="types" defaultstate="collapsed">

  public:

    using context_t = duk_context*;
    using index_t = duk_idx_t;
    using size_t = duk_size_t;
    using codepoint_t = duk_codepoint_t;
    using array_index_t = duk_uarridx_t;
    using thread_state_t = duk_thread_state;

    typedef enum {
      compile_default = 0,
      compile_eval = DUK_COMPILE_EVAL,
      compile_function = DUK_COMPILE_FUNCTION,
      compile_strict = DUK_COMPILE_STRICT
    } compile_flags_t;

    typedef enum {
      err_ok = 0,
      err_ecma = DUK_ERR_ERROR,
      err_ecma_eval = DUK_ERR_EVAL_ERROR,
      err_ecma_range = DUK_ERR_RANGE_ERROR,
      err_ecma_reference = DUK_ERR_REFERENCE_ERROR,
      err_ecma_syntax = DUK_ERR_SYNTAX_ERROR,
      err_ecma_type = DUK_ERR_TYPE_ERROR,
      err_ecma_uri = DUK_ERR_URI_ERROR
    } error_code_t;

    typedef enum {
      ret_ok = 0,
      ret_ecma = DUK_RET_ERROR,
      ret_ecma_eval = DUK_RET_EVAL_ERROR,
      ret_ecma_range = DUK_RET_RANGE_ERROR,
      ret_ecma_reference = DUK_RET_REFERENCE_ERROR,
      ret_ecma_syntax = DUK_RET_SYNTAX_ERROR,
      ret_ecma_type = DUK_RET_TYPE_ERROR,
      ret_ecma_uri = DUK_RET_URI_ERROR
    } return_code_t;

    typedef enum {
      /**
       * Safe call was successful
       */
      safe_call_success = DUK_EXEC_SUCCESS,

      /**
       * Safe call failed
       */
      safe_call_error = DUK_EXEC_ERROR
    } safe_call_code_t;

    typedef int enumerator_flags;
    /**
     * Enumerate also non-enumerable properties (by default only enumerable properties are
     * enumerated)
     */
    static constexpr enumerator_flags enum_include_nonenumerable = DUK_ENUM_INCLUDE_NONENUMERABLE;

    /**
     * Enumerate also internal properties (by default internal properties are not enumerated)
     */
    static constexpr enumerator_flags enum_include_hidden = DUK_ENUM_INCLUDE_HIDDEN;

    /**
     * Enumerate only an object's "own" properties (by default also inherited properties are
     * enumerated)
     */
    static constexpr enumerator_flags enum_own_properties_only = DUK_ENUM_OWN_PROPERTIES_ONLY;

    /**
     * Enumerate only array indices, i.e. property names of the form "0", "1", "2", etc.
     */
    static constexpr enumerator_flags enum_array_indices_only = DUK_ENUM_ARRAY_INDICES_ONLY;

    /**
     * guarantee that array indices are sorted by their numeric value, only use with
     * enum_array_indices_only; this is quite slow
     */
    static constexpr enumerator_flags enum_sort_array_indices = DUK_ENUM_SORT_ARRAY_INDICES;

    // </editor-fold>

    // <editor-fold desc="c'tor d'tor" defaultstate="collapsed">

  public:

    basic_api() noexcept : ctx_(0)
    { ; }

    basic_api(const basic_api& o) noexcept : ctx_(o.ctx_)
    { ; }

    basic_api(duk_context* ct) noexcept : ctx_(ct)
    { ; }

    template <typename T>
    basic_api(const basic_engine<T>& en) : ctx_(en.ctx())
    { ; }

    ~basic_api() noexcept
    { ; }

    // </editor-fold>

    // <editor-fold desc="methods" defaultstate="collapsed">

  public:

    context_t ctx() const noexcept
    { return ctx_; }

    context_t ctx(context_t ctx) noexcept
    { ctx_ = ctx; return ctx_; }

    // </editor-fold>

    // <editor-fold desc="api methods" defaultstate="collapsed">

  public:

    void compile(compile_flags_t flags) const
    { duk_compile(ctx_, flags); }

    void compile_file(compile_flags_t flags, const std::string& path) const
    { duk_compile_lstring_filename(ctx_, flags, path.data(), static_cast<size_t>(path.length())); }

    void compile_string(compile_flags_t flags, const std::string& src) const
    { duk_compile_lstring(ctx_, flags, src.data(), static_cast<size_t>(src.length())); }

    void eval() const
    { duk_eval(ctx_); }

    void eval_string(const std::string& src) const
    { duk_eval_lstring(ctx_, src.data(), static_cast<size_t>(src.length())); }

    void eval_string_noresult(const std::string& src) const
    { duk_eval_lstring_noresult(ctx_, src.data(), static_cast<size_t>(src.length())); }

    void call(int nargs) const
    { duk_call(ctx_,  nargs); }

    void call_method(int nargs) const
    { duk_call_method(ctx_, nargs); }

    void call_prop(index_t obj_index, int nargs) const
    { duk_call_prop(ctx_, obj_index, nargs); }

    int pcompile(compile_flags_t flags) const
    { return duk_pcompile(ctx_, flags); }

    int pcompile_string(compile_flags_t flags, const std::string& src) const
    { return duk_pcompile_lstring(ctx_, flags, src.data(), (size_t) src.length()); }

    int pcompile_file(compile_flags_t flags, const std::string& src) const
    { return duk_pcompile_lstring_filename(ctx_, flags, src.data(), (size_t) src.length()); }

    int peval() const
    { return duk_peval(ctx_); }

    int peval_string(const std::string& src) const
    { return duk_peval_lstring(ctx_, src.data(), (size_t) src.length()); }

    int peval_string_noresult(const std::string& src) const
    { return duk_peval_lstring_noresult(ctx_, src.data(), (size_t) src.length()); }

    int pcall(int nargs) const
    { return duk_pcall(ctx_, nargs); }

    int pcall_method(int nargs) const
    { return duk_pcall_method(ctx_, nargs); }

    int pcall_prop(index_t obj_index, int nargs) const
    { return duk_pcall_prop(ctx_, obj_index, nargs); }

    int safe_call(duk_safe_call_function func, void *udata, int nargs, int nrets) const
    { return duk_safe_call(ctx_, func, udata, nargs, nrets); }

    bool check_type(index_t index, int type) const
    { return duk_check_type(ctx_, index, type) != 0; }

    bool check_type_mask(index_t index, unsigned mask) const
    { return duk_check_type_mask(ctx_, index, mask) != 0; }

    index_t get_top() const
    { return duk_get_top(ctx_); }

    void set_top(index_t index) const
    { duk_set_top(ctx_, index); }

    index_t get_top_index() const
    { return duk_get_top_index(ctx_); }

    void dup(index_t from_index) const
    { duk_dup(ctx_, from_index); }

    void dup_top() const
    { duk_dup_top(ctx_); }

    void copy(index_t from_index, index_t to_index) const
    { duk_copy(ctx_, from_index, to_index); }

    void remove(index_t index) const
    { duk_remove(ctx_, index); }

    void insert(index_t to_index) const
    { duk_insert(ctx_, to_index); }

    void replace(index_t to_index) const
    { duk_replace(ctx_, to_index); }

    void swap(index_t index1, index_t index2) const
    { duk_swap(ctx_, index1, index2); }

    void swap_top(index_t index) const
    { duk_swap_top(ctx_, index); }

    bool check_stack(index_t extra) const
    { return duk_check_stack(ctx_, extra) != 0; }

    bool check_stack_top(index_t top) const
    { return duk_check_stack_top(ctx_, top) != 0; }

    bool pnew(int nargs) const
    { return duk_pnew(ctx_, nargs) == 0; }

    bool next(index_t enum_index, bool get_value) const
    { return duk_next(ctx_, enum_index, (duk_bool_t) get_value) != 0; }

    void compact(index_t obj_index) const
    { duk_compact(ctx_, obj_index); }

    bool del_prop(index_t obj_index) const
    { return duk_del_prop(ctx_, obj_index) != 0; }

    bool del_prop_index(index_t obj_index, array_index_t arr_index) const
    { return duk_del_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool del_prop_string(index_t obj_index, const char* key) const
    { return key && (duk_del_prop_string(ctx_, obj_index, key) != 0); }

    bool del_prop_string(index_t obj_index, const std::string& key) const
    { return duk_del_prop_lstring(ctx_, obj_index, key.c_str(), key.length()) != 0; }

    bool get_prop(index_t obj_index) const
    { return duk_get_prop(ctx_, obj_index) != 0; }

    bool get_prop_index(index_t obj_index, array_index_t arr_index) const
    { return duk_get_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool get_prop_string(index_t obj_index, const char* key) const
    { return key && (duk_get_prop_string(ctx_, obj_index, key) != 0); }

    bool get_prop_string(index_t obj_index, const std::string& key) const
    { return duk_get_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    void get_prop_desc(index_t obj_index, unsigned flags) const
    { duk_get_prop_desc(ctx_, obj_index, duk_uint_t(flags)); }

    void get_prototype(index_t index) const
    { duk_get_prototype(ctx_, index); }

    bool get_global_string(const char* key) const
    { return key && (duk_get_global_string(ctx_, key) != 0); }

    bool get_global_string(const std::string& key) const
    { return duk_get_global_lstring(ctx_, key.c_str(), key.size()) != 0; }

    bool has_prop(index_t obj_index) const
    { return duk_has_prop(ctx_, obj_index) != 0; }

    bool has_prop_index(index_t obj_index, array_index_t arr_index) const
    { return duk_has_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool has_prop_string(index_t obj_index, const char* key) const
    { return key && (duk_has_prop_string(ctx_, obj_index, key) != 0); }

    bool has_prop_string(index_t obj_index, const std::string& key) const
    { return duk_has_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    void put_function_list(index_t obj_index, const duk_function_list_entry *funcs) const
    { duk_put_function_list(ctx_, obj_index, funcs); }

    bool put_global_string(const char* key)
    { return duk_put_global_string(ctx_, key); }

    bool put_global_string(const std::string& key)
    { return duk_put_global_lstring(ctx_, key.c_str(), key.size()); }

    void put_number_list(index_t obj_index, const duk_number_list_entry *numbers) const
    { duk_put_number_list(ctx_, obj_index, numbers); }

    bool put_prop(index_t obj_index) const
    { return duk_put_prop(ctx_, obj_index) != 0; }

    bool put_prop_index(index_t obj_index, array_index_t arr_index) const
    { return duk_put_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool put_prop_string(index_t obj_index, const char* key) const
    { return key && (duk_put_prop_string(ctx_, obj_index, key) != 0); }

    bool put_prop_string(index_t obj_index, const std::string& key) const
    { return duk_put_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    index_t normalize_index(index_t index) const
    { return duk_normalize_index(ctx_, index); }

    bool get_boolean(index_t index) const
    { return duk_get_boolean(ctx_, index) != 0; }

    // @sw: concider kicking this function in favour of the template
    const void* get_buffer(index_t index, size_t& out_size) const
    { return (const void*) duk_get_buffer(ctx_, index, &out_size); }

    // @sw: concider kicking this function in favour of the template
    const void* get_buffer_data(index_t index, size_t& out_size) const
    { return (const void*) duk_get_buffer_data(ctx_, index, &out_size); }

    template <typename ContainerType,  bool NoCoercing=false>
    ContainerType get_buffer(index_t index) const
    {
      ContainerType data;
      duk_size_t size = 0;
      const char* buffer = nullptr;
      if(NoCoercing) {
        buffer = reinterpret_cast<const char*>(duk_get_buffer_data(ctx_, index, &size));
      } else {
        buffer = reinterpret_cast<const char*>(duk_get_buffer(ctx_, index, &size));
      }
      if(size) {
        data.resize(size);
        std::copy(&(buffer[0]), &(buffer[size]), data.begin());
      }
      return data;
    }

    ::duk_c_function get_c_function(index_t index) const
    { return duk_get_c_function(ctx_, index); }

    duk_context* get_context(index_t index) const
    { return duk_get_context(ctx_, index); }

    int get_int(index_t index) const
    { return duk_get_int(ctx_,  index); }

    size_t get_length(index_t index) const
    { return duk_get_length(ctx_, index); }

    void get_memory_functions(duk_memory_functions *out_funcs) const
    { duk_get_memory_functions(ctx_, out_funcs); }

    double get_now() const
    { return duk_get_now(ctx_); }

    double get_number(index_t index) const
    { return duk_get_number(ctx_, index); }

    void* get_pointer(index_t index) const
    { return duk_get_pointer(ctx_, index); }

    std::string get_string(index_t index)  const
    { size_t l; const char* s = duk_get_lstring(ctx_, index, &l); return (s && (l>0))?s:""; }

    unsigned get_uint(index_t index) const
    { return duk_get_uint(ctx_, index); }

    int get_type(index_t index) const
    { return duk_get_type(ctx_, index); }

    unsigned get_type_mask(index_t index) const
    { return duk_get_type_mask(ctx_, index); }

    bool is_instanceof(index_t obj, index_t proto) const
    { return duk_instanceof(ctx_, obj, proto) != 0; }

    bool is_array(index_t index) const
    { return duk_is_array(ctx_, index) != 0; }

    bool is_boolean(index_t index) const
    { return duk_is_boolean(ctx_, index) != 0; }

    bool is_bound_function(index_t index) const
    { return duk_is_bound_function(ctx_, index) != 0; }

    bool is_buffer(index_t index) const
    { return duk_is_buffer(ctx_, index) != 0; }

    bool is_buffer_data(index_t index) const
    { return duk_is_buffer_data(ctx_, index) != 0; }

    bool is_c_function(index_t index) const
    { return duk_is_c_function(ctx_, index) != 0; }

    bool is_callable(index_t index) const
    { return duk_is_callable(ctx_, index) != 0; }

    bool is_constructor_call() const
    { return duk_is_constructor_call(ctx_) != 0; }

    bool is_dynamic_buffer(index_t index) const
    { return duk_is_dynamic_buffer(ctx_, index) != 0; }

    bool is_ecmascript_function(index_t index) const
    { return duk_is_ecmascript_function(ctx_, index) != 0; }

    bool is_error(index_t index) const
    { return duk_is_error(ctx_, index) != 0; }

    bool is_eval_error(index_t index) const
    { return duk_is_eval_error(ctx_, index) != 0; }

    bool duk_is_fixed_buffer(index_t index) const
    { return duk_is_fixed_buffer(ctx_, index) != 0; }

    bool is_function(index_t index) const
    { return duk_is_function(ctx_, index) != 0; }

    bool is_lightfunc(index_t index) const
    { return duk_is_lightfunc(ctx_, index) != 0; }

    bool is_nan(index_t index) const
    { return duk_is_nan(ctx_, index) != 0; }

    bool is_null(index_t index) const
    { return duk_is_null(ctx_, index) != 0; }

    bool is_null_or_undefined(index_t index) const
    { return duk_is_null_or_undefined(ctx_, index) != 0; }

    bool is_number(index_t index) const
    { return duk_is_number(ctx_, index) != 0; }

    bool is_object(index_t index) const
    { return duk_is_object(ctx_, index) != 0; }

    bool is_object_coercible(index_t index) const
    { return duk_is_object_coercible(ctx_, index) != 0; }

    bool is_pointer(index_t index) const
    { return duk_is_pointer(ctx_, index) != 0; }

    bool is_primitive(index_t index) const
    { return duk_is_primitive(ctx_, index) != 0; }

    bool is_range_error(index_t index) const
    { return duk_is_range_error(ctx_, index) != 0; }

    bool is_reference_error(index_t index) const
    { return duk_is_reference_error(ctx_, index) != 0; }

    bool is_strict_call() const
    { return duk_is_strict_call(ctx_) != 0; }

    bool is_string(index_t index) const
    { return duk_is_string(ctx_, index) != 0; }

    bool is_symbol(index_t index) const
    { return duk_is_symbol(ctx_, index) != 0; }

    bool is_syntax_error(index_t index) const
    { return duk_is_syntax_error(ctx_, index) != 0; }

    bool is_thread(index_t index) const
    { return duk_is_thread(ctx_, index) != 0; }

    bool is_type_error(index_t index) const
    { return duk_is_type_error(ctx_, index) != 0; }

    bool is_undefined(index_t index) const
    { return duk_is_undefined(ctx_, index) != 0; }

    bool is_uri_error(index_t index) const
    { return duk_is_uri_error(ctx_, index) != 0; }

    bool is_valid_index(index_t index) const
    { return duk_is_valid_index(ctx_, index) != 0; }

    index_t push_array() const
    { return duk_push_array(ctx_); }

    index_t push_bare_object() const
    { return duk_push_bare_object(ctx_); }

    void push_boolean(bool val) const
    { duk_push_boolean(ctx_, (duk_bool_t) val); }

    void* push_buffer(size_t size, bool dynamic) const
    { return duk_push_buffer(ctx_, size, (duk_bool_t) dynamic); }

    void push_buffer_object(index_t buffer_index, size_t byte_offset, size_t byte_length, unsigned flags) const
    { return duk_push_buffer_object(ctx_, buffer_index, duk_size_t(byte_offset), duk_size_t(byte_length), duk_uint_t(flags)); }

    index_t push_c_function(::duk_c_function func, int nargs) const
    { return duk_push_c_function(ctx_, func, nargs); }

    void push_context_dump() const
    { duk_push_context_dump(ctx_); }

    void push_current_function() const
    { duk_push_current_function(ctx_); }

    void push_current_thread() const
    { duk_push_current_thread(ctx_); }

    void* push_dynamic_buffer(size_t size) const
    { return duk_push_dynamic_buffer(ctx_, size); }

    void push_false() const
    { duk_push_false(ctx_); }

    void* push_fixed_buffer(size_t size) const
    { return duk_push_fixed_buffer(ctx_, size); }

    void push_global_object() const
    { duk_push_global_object(ctx_); }

    void push_global_stash() const
    { duk_push_global_stash(ctx_); }

    void push_heap_stash() const
    { duk_push_heap_stash(ctx_); }

    void push_int(int val) const
    { duk_push_int(ctx_, val); }

    void push_string(const char* s) const
    { duk_push_string(ctx_, s ? s : ""); }

    void push_string(const std::string& s) const
    { duk_push_lstring(ctx_, s.data(), (size_t) s.length()); }

    void push_string(std::string&& s) const
    { duk_push_lstring(ctx_, s.data(), (size_t) s.length()); }

    void push_nan() const
    { duk_push_nan(ctx_); }

    void push_null() const
    { duk_push_null(ctx_); }

    void push_number(double val) const
    { duk_push_number(ctx_, val); }

    index_t push_object() const
    { return duk_push_object(ctx_); }

    void push_pointer(void *p) const
    { duk_push_pointer(ctx_, p); }

    void push_this() const
    { duk_push_this(ctx_); }

    index_t push_thread() const
    { return duk_push_thread(ctx_); }

    index_t push_thread_new_globalenv() const
    { return duk_push_thread_new_globalenv(ctx_); }

    void push_thread_stash(duk_context *target_ctx) const
    { duk_push_thread_stash(ctx_, target_ctx); }

    void push_true() const
    { duk_push_true(ctx_); }

    void push_uint(unsigned val) const
    { duk_push_uint(ctx_, val); }

    void push_undefined() const
    { duk_push_undefined(ctx_); }

    void pop() const
    { duk_pop(ctx_); }

    void pop(index_t count) const
    { if(count > 0) duk_pop_n(ctx_, count); }

    bool require_boolean(index_t index) const
    { return duk_require_boolean(ctx_, index); }

    void* require_buffer(index_t index, size_t *out_size) const
    { return duk_require_buffer(ctx_, index, out_size); }

    void* require_buffer_data(index_t index, size_t *out_size) const
    { return duk_require_buffer_data(ctx_, index, out_size); }

    ::duk_c_function require_c_function(index_t index) const
    { return duk_require_c_function(ctx_, index); }

    void require_callable(index_t index) const
    { return duk_require_callable(ctx_, index); }

    int require_int(index_t index) const
    { return duk_require_int(ctx_, index); }

    std::string require_string(index_t index) const
    { size_t l; const char* s = duk_require_lstring(ctx_, index, &l); return (s && (l>0))?s:""; }

    index_t require_normalize_index(index_t index) const
    { return duk_require_normalize_index(ctx_, index); }

    void require_null(index_t index) const
    { duk_require_null(ctx_, index); }

    double require_number(index_t index) const
    { return duk_require_number(ctx_, index); }

    void require_object_coercible(index_t index) const
    { duk_require_object_coercible(ctx_, index); }

    void* require_pointer(index_t index) const
    { return duk_require_pointer(ctx_, index); }

    unsigned require_uint(index_t index) const
    { return duk_require_uint(ctx_, index); }

    void require_undefined(index_t index) const
    { duk_require_undefined(ctx_, index); }

    duk_context* require_context(index_t index) const
    { return duk_require_context(ctx_, index); }

    void require_function(index_t index) const
    { duk_require_function(ctx_, index); }

    void require_stack_top(index_t top) const
    { duk_require_stack_top(ctx_, top); }

    void require_stack(index_t extra) const
    { duk_require_stack(ctx_, extra); }

    index_t require_top_index() const
    { return duk_require_top_index(ctx_); }

    void require_type_mask(index_t index, unsigned mask) const
    { duk_require_type_mask(ctx_, index, mask); }

    void require_valid_index(index_t index) const
    { duk_require_valid_index(ctx_, index); }

    bool to_boolean(index_t index) const
    { return duk_to_boolean(ctx_, index) != 0; }

    void* to_buffer(index_t index, size_t *out_size) const
    { return duk_to_buffer(ctx_, index, out_size); }

    void* to_dynamic_buffer(index_t index, size_t *out_size) const
    { return duk_to_dynamic_buffer(ctx_, index, out_size); }

    void* to_fixed_buffer(index_t index, size_t *out_size) const
    { return duk_to_fixed_buffer(ctx_, index, out_size); }

    int to_int(index_t index) const
    { return duk_to_int(ctx_, index); }

    int32_t to_int32(index_t index) const
    { return duk_to_int32(ctx_, index); }

    void to_null(index_t index) const
    { duk_to_null(ctx_, index); }

    double to_number(index_t index) const
    { return duk_to_number(ctx_, index); }

    void to_object(index_t index) const
    { duk_to_object(ctx_, index); }

    const void* to_pointer(index_t index) const
    { return (const void*) duk_to_pointer(ctx_, index); }

    void to_primitive(index_t index, int hint) const
    { duk_to_primitive(ctx_, index, hint); }

    unsigned to_uint(index_t index) const
    { return duk_to_uint(ctx_, index); }

    uint16_t to_uint16(index_t index) const
    { return duk_to_uint16(ctx_, index); }

    uint32_t to_uint32(index_t index) const
    { return duk_to_uint32(ctx_, index); }

    void to_undefined(index_t index) const
    { duk_to_undefined(ctx_, index); }

    std::string to_string(index_t index) const
    { size_t l; const char* s = duk_to_lstring(ctx_, index, &l); return (s && (l>0))?s:""; }

    std::string safe_to_string(index_t index) const
    { size_t l; const char *s = duk_safe_to_lstring(ctx_, index, &l); return std::string(s, s+l); }

    bool samevalue(index_t index1, index_t index2)
    { return !!duk_samevalue(ctx_, index1, index2); }

    void concat(index_t count) const
    { duk_concat(ctx_, count); }

    void join(index_t count) const
    { duk_join(ctx_, count); }

    void substring(index_t index, size_t start_char_offset, size_t end_char_offset) const
    { duk_substring(ctx_, index, start_char_offset, end_char_offset); }

    void trim(index_t index) const
    { duk_trim(ctx_, index); }

    codepoint_t char_code_at(index_t index, size_t char_offset) const
    { return duk_char_code_at(ctx_, index, char_offset); }

    void json_decode(index_t index) const
    { duk_json_decode(ctx_, index); }

    std::string json_encode(index_t index) const
    { const char* s = duk_json_encode(ctx_, index); return s ? s : ""; }

    std::string base64_encode(index_t index) const
    { const char* s = duk_base64_encode(ctx_, index); return s ? s : ""; }

    void base64_decode(index_t index) const
    { duk_base64_decode(ctx_, index); }

    void hex_decode(index_t index) const
    { duk_hex_decode(ctx_, index); }

    std::string hex_encode(index_t index) const
    { const char* s = duk_hex_encode(ctx_, index); return s ? s : ""; }

    std::string buffer_to_string(index_t index) const
    { const char* s = duk_buffer_to_string(ctx_, index); return s ? s : ""; }

    void map_string(index_t index, duk_map_char_function callback, void *udata) const
    { duk_map_string(ctx_, index, callback, udata); }

    void decode_string(index_t index, duk_decode_char_function callback, void *udata) const
    { duk_decode_string(ctx_, index, callback, udata); }

    void* resize_buffer(index_t index, size_t new_size) const
    { return duk_resize_buffer(ctx_, index, new_size); }

    void* alloc(size_t size) const
    { return duk_alloc(ctx_, size); }

    void* alloc_raw(size_t size) const
    { return duk_alloc_raw(ctx_, size); }

    void* realloc(void *ptr, size_t size) const
    { return duk_realloc(ctx_, ptr, size); }

    void* realloc_raw(void *ptr, size_t size) const
    { return duk_realloc_raw(ctx_, ptr, size); }

    void free(void *ptr) const
    { duk_free(ctx_, ptr); }

    void free_raw(void *ptr) const
    { duk_free_raw(ctx_, ptr); }

    void enumerator(index_t obj_index, enumerator_flags enum_flags)  const
    { duk_enum(ctx_, obj_index, (duk_uint_t) enum_flags); }

    bool equals(index_t index1, index_t index2) const
    { return duk_equals(ctx_, index1, index2) != 0; }

    bool strict_equals(index_t index1, index_t index2) const
    { return duk_strict_equals(ctx_, index1, index2); }

    void gc() const  // unsigned flags: not defined yet
    { duk_gc(ctx_, 0); }

    int throw_exception() const
    { ::duk_throw_raw(ctx_); return 0; }

    int throw_exception(std::string msg) const
    { error(error_code_t::err_ecma, msg); return 0; }

    int error(error_code_t err_code, const std::string& msg, std::string file="(native c++)", int line=0) const
    { duk_error_raw(ctx_, (duk_errcode_t) err_code, file.c_str(), (duk_int_t) line, msg.c_str()); return 0; }

    error_code_t get_error_code(index_t index) const
    { return error_code_t(duk_get_error_code(ctx_, index));  }

    void push_external_buffer()
    { duk_push_external_buffer(ctx_); }

    void push_external_buffer(void* data, size_t size)
    { duk_push_external_buffer(ctx_); duk_config_buffer(ctx_, -1, data, duk_size_t(size)); }

    void config_buffer(index_t index, void* data, size_t size)
    { duk_config_buffer(ctx_, index, data, duk_size_t(size)); }

    std::string dump_context() const
    { stack_guard sg(ctx_); push_context_dump(); return safe_to_string(-1); }

    void def_prop(index_t index, unsigned flags) const
    { duk_def_prop(ctx_, index, (duk_uint_t)flags); }

    void dump_function() const
    { duk_dump_function(ctx_); }

    void load_function() const
    { duk_load_function(ctx_); }

    void duk_get_finalizer(index_t index) const
    { duk_get_finalizer(ctx_, index); }

    void set_finalizer(index_t index) const
    { duk_set_finalizer(ctx_, index); }

    void set_global_object() const
    { duk_set_global_object(ctx_); }

    void set_length(index_t index, size_t length) const
    { duk_set_length(ctx_, index, length); }

    void set_prototype(index_t index) const
    { duk_set_prototype(ctx_, index); }

    void* steal_buffer(index_t index, size_t *size)
    { return *duk_steal_buffer(ctx_, index, size); }

    thread_state_t suspend() const
    { thread_state_t state; duk_suspend(ctx_, &state); return state; }

    void resume(const thread_state_t& state) const
    { duk_resume(ctx_, &state); }

    void inspect_callstack_entry(int level) const
    { duk_inspect_callstack_entry(ctx_, duk_int_t(level)); }

    void inspect_value(index_t index) const
    { duk_inspect_value(ctx_, index); }

    static void xcopy_top(context_t to_ctx, context_t from_ctx, size_t count)
    { duk_xcopy_top(to_ctx, from_ctx, duk_idx_t(count)); }

    static void xmove_top(context_t to_ctx, context_t from_ctx, size_t count)
    { duk_xmove_top(to_ctx, from_ctx, duk_idx_t(count)); }

    /**
     * Push a (raw) buffer with defined size and in turn push a
     * buffer object of type ArrayBuffer linked to the raw buffer.
     * Returns a pointer to the allocated buffer. If the allocation
     * failed, then pushing the buffer view object is omitted.
     * @param size_t size
     * @param bool dynamic
     * @return void*
     */
    void* push_array_buffer(size_t size, bool dynamic) const
    {
      void* p = duk_push_buffer(ctx_, size, (duk_bool_t) dynamic);
      if(p) duk_push_buffer_object(ctx_, -1, 0, size, DUK_BUFOBJ_ARRAYBUFFER);
      return p;
    }

    /**
     * Get a value from an object or a default if not existing.
     */
    template <typename T>
    T get_optional_property(index_t object_index, const char* property_name, T default_value=T()) const
    {
      if(!has_prop_string(object_index, property_name)) return default_value;
      if(get_prop_string(object_index, property_name)) default_value = get<T>(-1);
      pop();
      return default_value;
    }

    /**
     * The method returns the absolute index of a given **valid** index (absolute or relative).
     * @param index_t index
     * @return index_t
     */
    index_t absindex(index_t index) const noexcept
    { return (index >= 0) ? (index) : (top() + index); }

    /**
     * Raw evaluation with flags. src_buffer=nullptr, src_length=0 --> get from current stack.
     * Returns 0 on ok, !0 on fail.
     * @param const char* src_buffer
     * @param size_t src_length
     * @param unsigned flags
     * @return int
     */
    int eval_raw(const char* src_buffer, size_t src_length, unsigned flags)
    { return duk_eval_raw(ctx_, src_buffer, (duk_size_t) src_length, (duk_uint_t) flags); }

    /**
     * Returns the type name of a ECMA value on the stack.
     * @param int index
     * @return const char*
     */
    const char* get_typename(int index) const
    {
      switch(get_type(index)) {
        case DUK_TYPE_UNDEFINED: return "undefined";
        case DUK_TYPE_NUMBER:    return "Number";
        case DUK_TYPE_STRING:    return "String";
        case DUK_TYPE_BOOLEAN:   return "Boolean";
        case DUK_TYPE_OBJECT:    return "Object";
        case DUK_TYPE_NULL:      return "null";
        case DUK_TYPE_BUFFER:    return "Buffer";
        case DUK_TYPE_POINTER:   return "Pointer";
        case DUK_TYPE_LIGHTFUNC: return "Function pointer";
      }
      return "(unknown type!)";
    }

    /**
     * Alias of `get_top()`.
     * @return index_t
     */
    index_t top() const
    { return get_top(); }

    /**
     * Returns true of a value on the stack corresponds to the c++ type specified
     * with typename T, false otherwise. Note that `const char*` is not valid, use
     * `std::string` instead. Uses functions `duk_is_*`.
     * @param index_t index
     * @return typename T
     */
    template <typename T>
    bool is(index_t index) const
    { return conv<T>::is(ctx_, index); }

    /**
     * Get a value from the stack with implicit coersion.
     * Uses functions `duk_to_*`.
     * @param index_t index
     * @return typename T
     */
    template <typename T>
    T to(index_t index) const
    {
      if(conv<T>::is(ctx_, index)) {
        return conv<T>::get(ctx_, index);
      } else {
        conv<T>::to(ctx_, index);
        return conv<T>::get(ctx_, index);
      }
    }

    /**
     * Get a value from the stack withOUT implicit conversion / coersion.
     * Uses functions `duk_get_*`.
     * @param index_t index
     * @return typename T
     */
    template <typename T>
    T get(index_t index) const
    { return conv<T>::get(ctx_, index); }

    /**
     * Get a value from the stack without implicit conversion / coersion, throwing
     * an error in the ECMA runtime on type mismatch. Uses functions `duk_require_*`.
     *
     * @param index_t index
     * @return typename T
     */
    template <typename T>
    T req(index_t index) const
    { return conv<T>::req(ctx_, index); }

    /**
     * Push decl and overloads
     */
    template <typename ...Args>
    void push(Args...) const ;

    /**
     * Push nothing
     */
    void push() const
    { }

    /**
     * Push a number or pointer on the stack
     * @param typename T val
     * @return void
     */
    template <
      typename T,
      typename std::enable_if<!std::is_void<T>::value && (std::is_fundamental<T>::value || std::is_pointer<T>::value), int>::type=0
    >
    void push(T val) const
    { return conv<T>::push(ctx_, val); }

    /**
     * Push an object rvalue on the stack
     * @param typename T&& val
     * @return void
     */
    template <
      typename T,
      typename std::enable_if<std::is_class<T>::value && std::is_rvalue_reference<T>::value, int>::type=0
    >
    void push(T&& val) const
    { return conv<T>::push(ctx_, std::move(val)); }

    /**
     * Push an object by const reference on the stack
     * @param const typename T& val
     * @return void
     */
    template <
      typename T,
      typename std::enable_if<std::is_class<T>::value && !std::is_rvalue_reference<T>::value, int>::type=0
    >
    void push(const T& val) const
    { return conv<T>::push(ctx_, val); }

    /**
     * Push multiple arguments
     * @param typename ...Args
     */
    template <typename T, typename ...Args>
    void push(T val, Args ...args) const
    {
      check_stack((sizeof...(Args))+1);
      push_variadic(val, args...);
    }

    /**
     * Alias of set_top(index_t).
     * @param index_t index
     */
    void top(index_t index) const
    { set_top(index); }

    /**
     * Recursively selects sub objects of the global object defined by a dot
     * separated path (e.g. "system.debug.whatever").
     * Returns boolean success. If the method fails, then the stack will be
     * unchanged. Otherwise the selected property will be on top of the stack.
     *
     * @param std::string name
     * @return bool
     */
    bool select(std::string name) const
    {
      if(name.empty()) return false;
      index_t initial_top = top();
      push_global_object();
      std::string s;
      bool err = false;
      for(auto c : name) {
        if(c == '.') {
          if(s.empty() || !has_prop_string(-1, s.c_str())) { err = true; break; }
          get_prop_string(-1, s);
          s.clear();
        } else if((!::isalnum(c)) && (c != '_')) {
          err = true; break;
        } else {
          s += c;
        }
      }
      if(!err && !s.empty()) {
        if(!has_prop_string(-1, s.c_str())) {
          err = true;
        } else {
          get_prop_string(-1, s);
          name.clear();
        }
      }
      if(err) top(initial_top);
      return !err;
    }

    /**
     * Set a property of an object, non forced, no special define attributes.
     */
    template <typename T>
    bool set(std::string&& key, T&& value)
    {
      if(!is_object(-1)) return false;
      push(std::move(key));
      push(std::move(value));
      put_prop(-3);
      return true;
    }

    bool is_date(index_t index)
    {
      index = absindex(index);
      if(!is_object(index)) return false;
      check_stack(1);
      get_global_string("Date");
      bool r = is_instanceof(index, -1);
      pop();
      return r;
    }

    bool is_regex(index_t index)
    {
      index = absindex(index);
      if(!is_object(index)) return false;
      check_stack(1);
      get_global_string("RegExp");
      bool r = is_instanceof(index, -1);
      pop();
      return r;
    }

    bool is_false(index_t index)
    { return is_boolean(index) && !get_boolean(index); }

    bool is_true(index_t index)
    { return is_boolean(index) && get_boolean(index); }

    engine& parent_engine()
    {
      stack_guard sg(*this);
      push_heap_stash();
      get_prop_string(-1, "_engine_");
      if(!is_pointer(-1)) {
        throw engine_error("Duktape stack has no duktape::engine assigned.");
      }
      return *reinterpret_cast<engine*>(get_pointer(-1));
    }

  private:

    template <typename T>
    void push_variadic(T val) const
    { push<T>(val); }

    template <typename T, typename ...Args>
    void push_variadic(T val, Args ...args) const
    { push<T>(val); push_variadic(args...); }

    // </editor-fold>

    // <editor-fold desc="instance variables" defaultstate="collapsed">

  protected:

    duk_context* ctx_;

    // </editor-fold>

  };
}}
// </editor-fold>

// <editor-fold desc="stack_guard" defaultstate="collapsed">
namespace duktape { namespace detail {
  /**
   * Saves the current top index of the stack using `duk_get_top()`
   * during construction and restores this stack top during destruction
   * (using `duk_set_top()`) IF the top index is greater than the
   * initial stack top.
   */
  template <typename>
  class basic_stack_guard
  {
  public:

    basic_stack_guard() : ctx_(0), initial_top_(0), gc_(false)
    { ; }

    basic_stack_guard(const basic_stack_guard& g, bool collect_garbage=false)
            : ctx_(g.ctx_), initial_top_(g.initial_top_), gc_(collect_garbage)
    { ; }

    basic_stack_guard(duk_context* ctx, bool collect_garbage=false)
            : ctx_(ctx), initial_top_(-1), gc_(collect_garbage)
    { if(ctx_) initial_top_ = duk_get_top(ctx_); }

    template <typename T>
    basic_stack_guard(const basic_api<T>& o, bool collect_garbage=false)
            : ctx_(o.ctx()), gc_(collect_garbage)
    { if(ctx_) initial_top_ = duk_get_top(ctx_); }

    ~basic_stack_guard() noexcept
    {
      if(!ctx_ || initial_top_ < 0) return;
      duk_idx_t top = duk_get_top(ctx_);
      if(top <= initial_top_) return;
      duk_set_top(ctx_, initial_top_);
    }

  public:

    duk_context* ctx() const noexcept
    { return ctx_; }

    duk_idx_t initial_top() const noexcept
    { return initial_top_; }

    void initial_top(duk_idx_t index) noexcept
    { initial_top_ = index; }

  private:

    duk_context* ctx_;
    duk_idx_t initial_top_;
    bool gc_;
  };
}}
// </editor-fold>

// <editor-fold desc="conversion" defaultstate="collapsed">
namespace duktape { namespace detail {

  /**
   * This is only a long long list of conversion traits for the
   * common c++ types.
   */
  template <typename T>
  struct conv { using type = void; };

  // <editor-fold desc="conv<void>" defaultstate="collapsed">
  template <> struct conv<void>
  {
    using type = void;

    static constexpr const char* cc_name() noexcept
    { return "void"; }

    static constexpr const char* ecma_name() noexcept
    { return "undefined"; }

    static constexpr int nret() noexcept // c++ functions returning void return 0 results in the ECMA space.
    { return 0; }

    static bool is(duk_context* ctx, int index)  // void returns ignore any type, hence true
    { (void)ctx; (void)index; return true; }

    static void to(duk_context* ctx, int index)
    { (void)ctx; (void)index; return void(); }

    static void get(duk_context* ctx, int index)
    { (void)ctx; (void)index; return void(); }

    static void req(duk_context* ctx, int index)
    { (void)ctx; (void)index; return void(); }

    static void push(duk_context* ctx)
    { (void)ctx; }
  };
  // </editor-fold>

  // <editor-fold desc="conv<NUMERIC>" defaultstate="collapsed">
  // @note: The c++ community will kill me for using macros,
  //        but writing it all out or nesting further template
  //        structures is eventually less readable than this.
  #define decl_conv_tmp(TYPE, GET, REQ, TO, PUSH, IS, NAME) \
  template <> struct conv<TYPE> { \
    using type = TYPE; \
    static constexpr const char* cc_name() noexcept { return NAME; } \
    static constexpr const char* ecma_name() noexcept { return "Number"; } \
    static constexpr int nret() noexcept { return 1; } \
    static bool is(duk_context* ctx, int index)  { return IS(ctx, index); } \
    static type get(duk_context* ctx, int index)  { return GET(ctx, index); } \
    static type req(duk_context* ctx, int index)  { return REQ(ctx, index); } \
    static type to(duk_context* ctx, int index)  { return TO(ctx, index); } \
    static void push(duk_context* ctx, type val)  { PUSH(ctx, val); } \
  }
  decl_conv_tmp(char, duk_get_int, duk_require_int, duk_to_int, duk_push_int, duk_is_number, "char");
  decl_conv_tmp(signed char, duk_get_int, duk_require_int, duk_to_int, duk_push_int, duk_is_number, "signed char");
  decl_conv_tmp(short, duk_get_int, duk_require_int, duk_to_int, duk_push_int, duk_is_number, "short");
  decl_conv_tmp(int, duk_get_int, duk_require_int, duk_to_int, duk_push_int, duk_is_number, "int");
  decl_conv_tmp(long, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "long");
  decl_conv_tmp(long long, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "long long");
  decl_conv_tmp(unsigned char, duk_get_uint, duk_require_uint, duk_to_uint, duk_push_uint, duk_is_number, "unsigned char");
  decl_conv_tmp(unsigned short, duk_get_uint, duk_require_uint, duk_to_uint, duk_push_uint, duk_is_number, "unsigned short");
  decl_conv_tmp(unsigned int, duk_get_uint, duk_require_uint, duk_to_uint, duk_push_uint, duk_is_number, "unsigned int");
  decl_conv_tmp(unsigned long, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "unsigned long");
  decl_conv_tmp(unsigned long long, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "unsigned long long");
  decl_conv_tmp(float, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "float");
  decl_conv_tmp(double, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "double");
  decl_conv_tmp(long double, duk_get_number, duk_require_number, duk_to_number, duk_push_number, duk_is_number, "long double");
  #undef decl_conv_tmp
  // </editor-fold>

  // <editor-fold desc="conv<bool>" defaultstate="collapsed">
  template <> struct conv<bool>
  {
    using type = bool;

    static constexpr const char* cc_name() noexcept
    { return "bool"; }

    static constexpr const char* ecma_name() noexcept
    { return "Boolean"; }

    static constexpr int nret() noexcept
    { return 1; }

    static bool is(duk_context* ctx, int index)
    { return api(ctx).is_boolean(index); }

    static type get(duk_context* ctx, int index)
    { return api(ctx).get_boolean(index); }

    static type req(duk_context* ctx, int index)
    { return api(ctx).require_boolean(index); }

    static type to(duk_context* ctx, int index)
    { return api(ctx).to_boolean(index); }

    static void push(duk_context* ctx, const type& val)
    { return api(ctx).push_boolean(val); }

    static void push(duk_context* ctx, type&& val)
    { return api(ctx).push_boolean(std::move(val)); }
  };
  // </editor-fold>

  // <editor-fold desc="conv<std::string>" defaultstate="collapsed">
  // @note: Intentionally wstring, u16/u32string omitted.
  template <> struct conv<std::string>
  {
    using type = std::string;

    static constexpr const char* cc_name() noexcept
    { return "string"; }

    static constexpr const char* ecma_name() noexcept
    { return "String"; }

    static constexpr int nret() noexcept
    { return 1; }

    static bool is(duk_context* ctx, int index)
    { return api(ctx).is_string(index); }

    static type get(duk_context* ctx, int index)
    { return api(ctx).get_string(index); }

    static type req(duk_context* ctx, int index)
    { return api(ctx).require_string(index); }

    static type to(duk_context* ctx, int index)
    { return api(ctx).to_string(index); }

    static void push(duk_context* ctx, const type& val)
    { return api(ctx).push_string(val); }

    static void push(duk_context* ctx, type&& val)
    { return api(ctx).push_string(std::move(val)); }
  };
  // </editor-fold>

  // <editor-fold desc="conv<const char*>" defaultstate="collapsed">
  template <> struct conv<const char*>
  {
    using type = const char*;

    static constexpr int nret() noexcept
    { return 1; }

    static constexpr const char* cc_name() noexcept
    { return "const char*"; }

    static constexpr const char* ecma_name() noexcept
    { return "String"; }

    static bool is(duk_context* ctx, int index)
    { return api(ctx).is_string(index); }

    static void push(duk_context* ctx, type val)
    { return api(ctx).push_string(val); }
  };

  template <> struct conv<char*> : public conv<const char*>
  {
    using type = const char*;

    static void push(duk_context* ctx, type val)
    { return api(ctx).push_string(reinterpret_cast<const char*>(val)); }
  };
  // </editor-fold>

  // <editor-fold desc="conv<std::vector<T>>" defaultstate="collapsed">
  template <typename T> struct conv<std::vector<T>>
  {
    using type = std::vector<T>;

    static constexpr int nret() noexcept
    { return 1; }

    static constexpr const char* cc_name() noexcept
    { return (std::string("vector<") + conv<T>::cc_name() + ">").c_str(); }

    static constexpr const char* ecma_name() noexcept
    { return "Array"; }

    static bool is(duk_context* ctx, int index)
    { return api(ctx).is_array(index); }

    static type to(duk_context* ctx, int index)
    { return get_array<false>(ctx, index); }

    static type get(duk_context* ctx, int index)
    { return get_array<true>(ctx, index); }

    static type req(duk_context* ctx, int index)
    { return get_array<true>(ctx, index); }

    static void push(duk_context* ctx, const type& val)
    {
      api stack(ctx);
      auto array_index = stack.push_array();
      auto i = api::index_t(0);
      if(!stack.check_stack(4)) {
        stack.throw_exception("Not enough stack space (to push an array).");
        return;
      }
      for(const auto& e : val) {
        stack.push(e);
        if(!stack.put_prop_index(array_index, i)) {
          return; // @see duk_put_prop(): Either returns 1 or throws an error.
        }
        ++i;
      }
    }

  private:

    template <bool Strict>
    static type get_array(duk_context* ctx, int index)
    {
      api stack(ctx);
      if(!stack.check_stack(4)) {
        stack.throw_exception("Not enough stack space (to get an array).");
        return type();
      } else if(!stack.is_array(index)) {
        stack.throw_exception("Property is no array.");
        return type();
      }
      type ret;
      const api::index_t size = stack.get_length(index);
      for(auto i = api::index_t(0); i < size; ++i) {
        if(!stack.get_prop_index(index, i)) return type();
        ret.push_back(!Strict ? stack.to<T>(-1) : stack.get<T>(-1));
        if(Strict && !stack.is<T>(-1)) { stack.pop(); return type(); }
        stack.pop();
      }
      return ret;
    }

  };
  // </editor-fold>

  // <editor-fold desc="ecma_typename (for debugging use)" defaultstate="collapsed">
  /**
   * Javascript type name query. Note: This is a runtime type query and expensive.
   * Use it only for debugging or in exceptional situations.
   *
   * @param duk_context* ctx
   * @return int index
   */
  template <typename=void>
  const char* ecma_typename(duk_context* ctx, int index)
  {
    api stack(ctx);
    if(index < 0 || index > stack.get_top_index()) {
      return "(invalid stack index)";
    } else if(stack.is_undefined(index)) {
      return "undefined";
    } else if(stack.is_nan(index)) {
      return "NaN";
    } else if(stack.is_boolean(index)) {
      return stack.get_boolean(index) ? "true" : "false";
    } else if(stack.is_null(index)) {
      return "null";
    } else if(stack.is_string(index)) {
      return "String";
    } else if(stack.is_number(index)) {
      return "Number";
    } else if(stack.is_c_function(index)) {
      return "Function (native)";
    } else if(stack.is_function(index)) {
      return "Function";
    } else if(stack.is_array(index)) {
      return "Array";
    } else if(stack.is_object(index) || stack.is_null(index)) {
      return "Object";
    } else if(stack.is_dynamic_buffer(index)) {
      return "Buffer (dynamic)";
    } else if(stack.is_buffer(index)) {
      return "Buffer";
    } else if(stack.is_pointer(index)) {
      return "Pointer";
    } else if(stack.is_thread(index)) {
      return "Thread";
    }
    return "(unrecognised script type)";
  }
  // </editor-fold>

}}
// </editor-fold>

// <editor-fold desc="function_proxy" defaultstate="collapsed">
namespace duktape { namespace detail {
  using native_function_type = int(*)(api&);
}}

namespace duktape { namespace detail { namespace {

  template<unsigned...> struct indices{};
  template<unsigned N, unsigned... Is> struct indices_gen : indices_gen<N-1, N-1, Is...>{};
  template<unsigned... Is> struct indices_gen<0, Is...> : indices<Is...>{};

  template<typename R, unsigned I>
  R convert_arg(duk_context* ctx)
  { return std::forward<R>(conv<R>::to(ctx, I)); }

  template<typename R, typename... Args, unsigned... Is, typename std::enable_if<!std::is_void<R>::value>::type* = nullptr >
  int fn_wrap(R(*fn)(Args...), duk_context* ctx, indices<Is...>)
  { conv<R>::push(ctx, fn( std::move(convert_arg<Args, Is>(ctx))... )); return 1; }

  template<typename R, typename... Args, unsigned... Is, typename std::enable_if<std::is_void<R>::value>::type* = nullptr >
  int fn_wrap(R(*fn)(Args...), duk_context* ctx, indices<Is...>)
  { fn( std::move(convert_arg<Args, Is>(ctx))... ); (void)ctx; return 0; }

  template<typename R, typename... Args, typename std::enable_if<sizeof...(Args) != 1>::type* = nullptr>
  int function_wrap(R(*fn)(Args...), duk_context* ctx)
  { return fn_wrap(fn, ctx, indices_gen<sizeof...(Args)>()); }

  template<typename R, typename Arg, typename std::enable_if<!std::is_same<std::remove_reference<api>::type,Arg>::value>::type* = nullptr >
  int function_wrap(R(*fn)(Arg), duk_context* ctx)
  { return fn_wrap(fn, ctx, indices_gen<1>()); }

  template<typename R, typename ApiType, typename std::enable_if<std::is_same<api, ApiType>::value>::type* = nullptr>
  int function_wrap(int(*fn)(ApiType), duk_context* ctx)
  { return fn(api(ctx)) ? 1 : 0; }

  template<typename R, typename ApiType, typename std::enable_if<std::is_same<api&, ApiType>::value>::type* = nullptr>
  int function_wrap(int(*fn)(ApiType), duk_context* ctx)
  { api stack(ctx); return fn(stack) ? 1 : 0; }

  /**
   * Template struct definition.
   */
  template <typename R, typename ...Args>
  struct function_proxy;

  /**
   * Function proxy main template.
   */
  template <typename R, typename ...Args>
  struct function_proxy
  {
    using function_type = R(*)(Args...);

    static int func(duk_context *ctx)
    {
      constexpr bool is_byref_api = std::is_same<function_type, native_function_type>::value;
      api stack(ctx);
      stack.push_current_function();
      stack.get_prop_string(-1, "\xff_fp");
      function_type fn = reinterpret_cast<function_type>(stack.get_pointer(-1));
      stack.pop(2);
      if((!is_byref_api) && (stack.top() != sizeof...(Args))) {
        // Note: Duktape should have filled the args with undefined.
        stack.throw_exception("Invalid number of arguments.");
        return 0;
      } else if(!fn) {
        stack.throw_exception("Invalid function definition (bug in JS subsystem!).");
        return 0;
      } else if((!is_byref_api) && stack.is_constructor_call()) {
        stack.throw_exception("The called function cannot be a constructor.");
        return 0;
      } else {
        if(!is_byref_api) {
          // For native c++ wrapped functions the conversion has to be
          // more strict because there is no possibility to check the
          // original arguments passed from JS.
          for(auto i = stack.top()-1; i >= 0; --i) {
            if(stack.is_undefined(i) || stack.is_null(i)) {
              stack.throw_exception(std::string("Argument undefined/null passed to native function or missing arguments."));
              return 0;
            }
          }
        }
        try {
          return function_wrap<R, Args...>(fn, ctx);
        } catch(const engine_error& e) {
          throw; // no cleanup, something on the heap might be wrong.
        } catch(const exit_exception&) {
          stack.gc(); // sweep through to trigger possible finalisers, then rethrow to next higher frame.
          throw;
        } catch(const script_error& e) {
          if(e.callstack().empty()) {
            // Script error was thrown from the c++ code itself
            stack.throw_exception(e.what());
          } else {
            // Script error was caught by call() or eval() invoked in the function,
            // and the forwarded Error object is still on stack top --> rethrow.
            stack.throw_exception();
          }
          return 0;
        } catch(const std::exception& e) {
          stack.throw_exception(e.what());
          return 0;
        }
      }
    }
  };

}}}
// </editor-fold>

// <editor-fold desc="engine" defaultstate="collapsed">
namespace duktape { namespace detail {

  /**
   * This class represents a "duktape" engine with its own heap.
   * Engines are independent from another.
   *
   * The allocated heap is freed during destruction.
   */
  template <typename MutexType>
  class basic_engine
  {
  public:

    // <editor-fold desc="types" defaultstate="collapsed">
    using api_type = ::duktape::api;
    using stack_guard_type = ::duktape::stack_guard;
    using lock_guard_type = std::lock_guard<MutexType>;

    /**
     * define(....) : Defines are done using these flags (DUK_DEFPROP_HAVE_... will be
     * automatically added). The default is defining sealed, frozen - means not writable,
     * not configurable, but enumerable.
     */
    struct defflags
    {
      using type = unsigned;
      static constexpr type restricted = 0;
      static constexpr type writable = (DUK_DEFPROP_WRITABLE);
      static constexpr type enumerable = (DUK_DEFPROP_ENUMERABLE);
      static constexpr type configurable = (DUK_DEFPROP_CONFIGURABLE);
      static constexpr type defaults = (DUK_DEFPROP_ENUMERABLE);
    };
    // </editor-fold>

  public:

    // <editor-fold desc="c'tors/d'tor" defaultstate="collapsed">
    /**
     * c' tor
     */
    explicit basic_engine() : stack_(), define_flags_(defflags::defaults), mutex_()
    { clear(); }

    /**
     * d' tor
     */
    virtual ~basic_engine()
    { if(ctx()) ::duk_destroy_heap(ctx()); }

    basic_engine(const basic_engine&) = delete;
    basic_engine(basic_engine&&) = delete;
    basic_engine& operator=(const basic_engine&) = delete;

    // </editor-fold>

  public:

    // <editor-fold desc="getters/setters" defaultstate="collapsed">
    /**
     * Returns the `duk_context*` of this engine.
     * @return api_type&
     */
    api_type& stack() noexcept
    { return stack_; }

    /**
     * Returns the `duk_context*` of this engine.
     * @return typename api_type::context_t
     */
    typename api_type::context_t ctx() const noexcept
    { return stack_.ctx(); }

    /**
     * Returns the flags used for define(). These are used to set how mutable
     * the defined functions, objects etc are.
     * @return define_flags_t
     */
    typename defflags::type define_flags() const noexcept
    { return define_flags_; }

    /**
     * Sets the flags used for define(). These are used to set how mutable
     * the defined functions, objects etc are.
     * @param define_flags_t flags
     */
    void define_flags(typename defflags::type flags) noexcept
    { define_flags_ = flags; }

    // </editor-fold>

  public:

    // <editor-fold desc="standard methods" defaultstate="collapsed">
    /**
     * Resets the engine to a new, empty heap.
     */
    void clear()
    {
      lock_guard_type lck(mutex_);
      define_flags_ = defflags::defaults;
      if(ctx()) ::duk_destroy_heap(ctx());
      stack().ctx(::duk_create_heap(0, 0, 0, 0, 0));
      if(!ctx()) throw engine_error("Failed to create context");
      stack().push_heap_stash();
      stack().push_pointer(this);
      stack().put_prop_string(-2, "_engine_");
      stack().top(0);
    }
    // </editor-fold>

    // <editor-fold desc="include/eval" defaultstate="collapsed">
    /**
     * Includes a file, throws a `duktape::script_error` on fail.
     * @param const char* path
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false>
    ReturnType include(const char* path)
    { return include<ReturnType, Strict>(std::string(path)); }

    /**
     * Includes a file, throws a `duktape::script_error` on fail.
     * @param std::string path
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false>
    ReturnType include(std::string path)
    {
      std::ifstream is;
      is.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
      std::string code((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
      if(!is) throw script_error(std::string("Failed to read include file '") + path + "'");
      is.close();
      return eval<ReturnType, Strict>(std::move(code), path);
    }

    /**
     * Includes a file, throws a `duktape::script_error` on fail. Optionally a file name
     * can be specified for tracing purposes in the JS engine.
     * @param const char* code
     * @param const char* file=nullptr
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false>
    ReturnType eval(const char* code, const char* file=nullptr)
    { return eval<ReturnType,Strict>(std::string(code), std::string(file ? file : "(eval)")); }

    /**
     * Evaluate code given as string, throws a `duktape::script_error` on fail. Optionally a
     * file name can be specified for tracing purposes in the JS engine.
     * @param std::string code
     * @param std::string file="(eval)"
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false>
    ReturnType eval(std::string&& code, std::string file="(eval)")
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx(), true);
      stack().require_stack(2);
      stack().push_string(std::move(code));
      stack().push_string(file);
      bool ok;
      try {
        ok = (stack().eval_raw(0, 0, DUK_COMPILE_EVAL | DUK_COMPILE_SAFE | DUK_COMPILE_SHEBANG) == 0);
      } catch(const exit_exception&) {
        stack().top(0);
        stack().gc();
        throw;
      }
      if(!ok) {
        if(stack().top() > 0) {
          // The stack top index is the error
          stack().swap_top(sg.initial_top());
          sg.initial_top(sg.initial_top()+1);
          stack().top(sg.initial_top());
          stack().dup_top(); // Ensure that safe_to_string() does not modify the original (Error) object.
          std::string msg = stack().safe_to_string(-1);
          stack().pop();
          std::string callstack;
          stack().get_prop_string(-1, "stack");
          if(!stack().is_undefined(-1)) callstack = stack().to_string(-1);
          stack().pop();
          throw script_error(std::move(msg), std::move(callstack));
        } else {
          throw script_error(std::string("Unspecified exception evaluating code."));
        }
      } else if(std::is_void<ReturnType>::value) {
        return ReturnType();
      } else if(!Strict) {
        return conv<ReturnType>::to(ctx(), -1);
      } else {
        if(!conv<ReturnType>::is(ctx(), -1)) {
          throw script_error(
            std::string("Included '") + file + "' with expected return type '" +
            conv<ReturnType>::ecma_name() + "' (--> '" + conv<ReturnType>::cc_name() + "'), " +
            " but '" + stack().get_typename(-1) + "' was returned."
          );
        } else {
          return conv<ReturnType>::get(ctx(), -1);
        }
      }
    }
    // </editor-fold>

    // <editor-fold desc="call" defaultstate="collapsed">
    /**
     * Call a function, fetch the (strict) return value.
     * @param std::string funct
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false, typename ...Args>
    ReturnType call(const char* funct, Args ...args)
    { if(!funct) throw engine_error("BUG: engine::call(nullptr, ...)");
      return call<ReturnType, Strict, Args...>(std::string(funct), args...);
    }

    /**
     * Call a function, fetch the (strict) return value.
     * @param std::string funct
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool Strict=false, typename ...Args>
    ReturnType call(std::string funct, Args ...args)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx(), true);
      stack().require_stack(6);
      if(!stack().select(funct)) {
        throw script_error(std::string("'") + funct + "' not defined");
      } else if(!stack().is_callable(-1)) {
        throw script_error(std::string("'") + funct + "' is not callable");
      }
      stack().push(args...);
      bool ok = false;
      try {
        ok = (stack().pcall(sizeof...(Args)) == 0);
      } catch(const exit_exception& e) {
        stack().gc(); // to invoke already possible finalisations before next call stack frame.
        throw e;
      }
      if(!ok) {
        if(stack().top() > 0) {
          // The stack top index is the error
          stack().swap_top(sg.initial_top());
          sg.initial_top(sg.initial_top()+1);
          stack().top(sg.initial_top());
          stack().dup_top();
          std::string msg = stack().safe_to_string(-1);
          stack().pop();
          std::string callstack;
          stack().get_prop_string(-1, "stack");
          if(!stack().is_undefined(-1)) callstack = stack().to_string(-1);
          stack().pop();
          throw script_error(std::move(msg), std::move(callstack));
        } else {
          throw script_error(std::string("Unspecified exception calling function '") + funct + "");
        }
      } else if(std::is_void<ReturnType>::value) {
        return ReturnType();
      } else if(!Strict) {
        return conv<ReturnType>::to(ctx(), -1);
      } else if(!conv<ReturnType>::is(ctx(), -1)) {
        stack().top(0);
        throw script_error(
          std::string("Called '") + funct + "' with expected return type '" +
          conv<ReturnType>::ecma_name() + "' (--> '" + conv<ReturnType>::cc_name() + "'), " +
          " but '" + stack().get_typename(-1) + "' was returned."
        );
      } else {
        return conv<ReturnType>::get(ctx(), -1);
      }
    }
    // </editor-fold>

    // <editor-fold desc="define/undef" defaultstate="collapsed">

    /**
     * Remove an object or value (forced). Resolves dot separated names
     * (e.g. "system.debug" will remove "debug" from object "system").
     *
     * @param std::string name
     * @return void
     */
    void undef(std::string name)
    {
      if(name.empty()) return;
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      std::string base, tail;
      if(!aux<>::split_selector(name, base, tail)) {
        throw engine_error(std::string("Invalid selector: '") + name + "'");
      } else if(!base.empty()) {
        if(!stack().select(base)) return; // No need to undefine that, parent object not defined.
      } else {
        stack().push_global_object();
      }
      if(!stack().has_prop_string(-1, tail)) {
        return; // No need to undefine that, not defined after all.
      }
      // unlock
      stack().push(tail);
      stack().def_prop(-2, DUK_DEFPROP_FORCE
        | DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_WRITABLE
        | DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_CONFIGURABLE
      );
      stack().del_prop_string(-1, tail);
      stack().gc();
    }

    /**
     * Define an empty object.
     *
     * @param std::string name
     * @return void
     */
    void define(std::string name)
    { stack_guard_type sg(ctx()); define_r(name); }

    /**
     * Defines a (Duktape convention) C function in the global object of the engine. Canonical
     * names separated with '.' are allowed. Means empty parent objects will be created implicitly
     * if needed.
     *
     * @param const char* name
     * @param ::duk_c_function fn
     * @param int nargs
     * @return void
     */
    void define(const char* name, ::duk_c_function fn, int nargs=-1)
    { define(std::string(name), fn, nargs);  }

    /**
     * Defines a (Duktape convention) C function in the global object of the engine. Canonical
     * names separated with '.' are allowed. Means empty parent objects will be created implicitly
     * if needed.
     *
     * @param std::string name
     * @param ::duk_c_function fn
     * @param int nargs
     * @return void
     */
    void define(std::string name, ::duk_c_function fn, int nargs=-1)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      name = define_base(name);
      stack().push_string(name);
      stack().push_c_function(fn, nargs >= 0 ? nargs : DUK_VARARGS);
      stack().def_prop(-3, convert_define_flags(define_flags_));
    }

    /**
     * Defines a wrapped Duktape c++ function in the global object of the engine.
     * Canonical names separated with '.' are allowed. Means empty parent objects
     * will be created implicitly if needed.
     *
     * These functions look like:
     *
     *   int myfunction(duktape::api& stack) { ... }
     *
     * @param const char* name
     * @param native_function_type fn
     * @param int nargs
     * @return void
     */
    void define(const char* name, native_function_type fn, int nargs=-1)
    { define(std::string(name), fn, nargs); }

    /**
     * Defines a wrapped Duktape c++ function in the global object of the engine.
     * Canonical names separated with '.' are allowed. Means empty parent objects
     * will be created implicitly if needed.
     *
     * These functions look like:
     *
     *   int myfunction(duktape::api& stack) { ... }
     *
     * @param std::string name
     * @param native_function_type fn
     * @param int nargs
     * @return void
     */
    void define(std::string name, native_function_type fn, int nargs=-1)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      name = define_base(name);
      stack().push_string(name);
      stack().push_c_function(function_proxy<int, api_type&>::func, nargs >= 0 ? nargs : DUK_VARARGS); ///@sw: replace these template params with auto det
      stack().def_prop(-3, convert_define_flags(define_flags_));
      stack().get_prop_string(-1, name);
      stack().push_string("\xff_fp");
      stack().push_pointer((void*)fn); //@sw: double check / alternative: fn pointer vs data pointers size.
      stack().def_prop(-3, convert_define_flags(define_flags_));
    }

    /**
     * Defines a function in the global object of the engine. Canonical names separated with
     * '.' are allowed. Means empty parent objects will be created implicitly if needed.
     * E.g:
     *
     *  define("console.log", console_functions::console_log);
     *
     * This will also create the "console" object if it does not exist yet.
     *
     * @param std::string name
     * @param R(*)(Args...) fn
     * @return void
     */
    template <typename R, typename ...Args>
    void define(std::string name, R(*fn)(Args...))
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      name = define_base(name);
      stack().push_string(name);
      stack().push_c_function(function_proxy<R, Args...>::func, sizeof...(Args));
      stack().def_prop(-3, convert_define_flags(define_flags_));
      stack().get_prop_string(-1, name);
      stack().push_string("\xff_fp");
      stack().push_pointer((void*)fn); //@sw: double check / alternative: fn pointer vs data pointers size.
      stack().def_prop(-3, convert_define_flags(define_flags_));
    }

    /**
     * Defines a function in the global object of the engine. Canonical names separated with
     * '.' are allowed. Means empty parent objects will be created implicitly if needed.
     * E.g:
     *
     *  define("console.log", console_functions::console_log);
     *
     * This will also create the "console" object if it does not exist yet.
     *
     * @param const char* name
     * @param R(*)(Args...) fn
     * @return void
     */
    template <typename R, typename ...Args>
    void define(const char* name, R(*fn)(Args...))
    { define<R, Args...>(std::string(name), fn); }

    /**
     * Define a primitive value, array or plain object.
     * By default this value is immutable (see `define_flags()` for changing this).
     * The resulting script data type depends on the data type you pass. Strings,
     * and numbers are "String" and "Number" primitives. `std::vector<T>` becomes
     * an array of `T` values, std::map<string,ValueType> becomes a plain object etc.
     * @see the `duktape::detail::conv<T>` (trait) structure overloads.
     *
     * @param std::string name
     * @param T value
     */
    template<typename T, typename std::enable_if<true
      && (!std::is_void<T>::value)
      && ((!std::is_pointer<T>::value) || std::is_same<T,const char*>::value)
      && (std::is_class<conv<T>>::value)
      && (!std::is_same<typename conv<T>::type,void>::value)
    >::type* = nullptr>
    void define(std::string name, T value)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      name = define_base(name);
      stack().push_string(name);
      conv<T>::push(stack().ctx(), value);
      stack().def_prop(-3, convert_define_flags(define_flags_));
    }

    // </editor-fold>

  private:

    // <editor-fold desc="private auxiliary methods/functions" defaultstate="collapsed">
    /**
     * Recursively defines empty parent objects of the given (canonical) name
     * and returns the object key (which is the last part of the given name).
     *
     * @param std::string name
     * @return std::string name
     */
    std::string define_base(std::string name)
    {
      std::string base, tail;
      if(!aux<>::split_selector(name, base, tail)) {
        throw engine_error(std::string("Invalid selector: '") + name + "'");
      }
      define_r(base);
      return tail;
    }

    /**
     * Defines empty objects recursively from a canonical name.
     *
     * @param std::string name
     */
    void define_r(std::string name)
    {
      stack().push_global_object();
      if(name.empty()) return;
      std::string s;
      bool err = false;
      for(auto c : name) {
        if(c == '.') {
          if(s.empty()) {
            err = true;
            break;
          } else {
            if(!stack().has_prop_string(-1, s.c_str())) {
              stack().push_string(s);
              stack().push_object();
              stack().def_prop(-3, convert_define_flags(define_flags_));
              stack().get_prop_string(-1, s);
            } else {
              stack().get_prop_string(-1, s);
            }
            s.clear();
          }
        } else if((!::isalnum(c)) && (c != '_')) {
          err = true;
          break;
        } else {
          s += c;
        }
      }
      if(err) {
        throw engine_error(std::string("Invalid name: '") + s + "'");
      } else if(!s.empty()) {
        if(!stack().has_prop_string(-1, s.c_str())) {
          stack().push_string(s);
          stack().push_object();
          stack().def_prop(-3, convert_define_flags(define_flags_));
          stack().get_prop_string(-1, s);
        } else {
          stack().get_prop_string(-1, s);
        }
      }
    }

    /**
     * Handles/converts the flags needed for def_prop() from the simplified
     * `defflags::*` constants.
     *
     * @param typename defflags::type
     * @return unsigned
     */
    static unsigned convert_define_flags(typename defflags::type flags) noexcept
    {
      return (((unsigned)0) | DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE
        | DUK_DEFPROP_HAVE_WRITABLE | ((flags & defflags::writable) ? DUK_DEFPROP_WRITABLE:0)
        | DUK_DEFPROP_HAVE_CONFIGURABLE | ((flags & defflags::configurable) ? DUK_DEFPROP_CONFIGURABLE:0)
        | DUK_DEFPROP_HAVE_ENUMERABLE | ((flags & defflags::enumerable) ? DUK_DEFPROP_ENUMERABLE:0)
      );
    }
    // </editor-fold>

  private:

    // <editor-fold desc="instance variables" defaultstate="collapsed">
    api_type stack_;
    typename defflags::type define_flags_;
    MutexType mutex_;
    // </editor-fold>
  };
}}
// </editor-fold>

#endif
