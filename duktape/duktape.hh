/**
 * @file duktape.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.4
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++17 -W -Wall -Wextra -pedantic -fstrict-aliasing
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
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
#ifndef DUKTAPE_HH
#define DUKTAPE_HH

#undef DUK_SIZE_MAX_COMPUTED
#define DUK_SIZE_MAX_COMPUTED
#include <cstdint>
#include "duktape.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <exception>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <climits>
#include <utility>
#include <memory>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <regex>
#include <mutex>

#if (!defined(DUK_USE_CPP_EXCEPTIONS))
  #error "DUK_USE_CPP_EXCEPTIONS is now a fixed setting in duk_config.h, you need to enable it by editing that file (#define DUK_USE_CPP_EXCEPTIONS instead of #undef DUK_USE_CPP_EXCEPTIONS)."
#endif

#if (!defined(UINT_MAX)) || (UINT_MAX < 0xffffffffUL)
  #error "This interface requires at least 32 bit integer size for type int."
#endif
#if (!defined(__cplusplus)) || (__cplusplus < 201103L)
  #error "You have to compile with at least std=c++11"
#endif

#ifndef WITH_DEFAULT_STRICT_INCLUDE
  #define DEFAULT_STRICT_INCLUDE (false)
#elif !defined(DEFAULT_STRICT_INCLUDE)
  #define DEFAULT_STRICT_INCLUDE (true)
#endif

/**
 * Forward declarations required in this file.
 */
namespace duktape {

  namespace detail {
    template <typename R=void> class basic_api;
    template <typename R=void> class basic_stack_guard;
    template <typename MutexType=std::recursive_timed_mutex, bool StrictInclude=bool(DEFAULT_STRICT_INCLUDE)> class basic_engine;
    template <typename T> struct conv;
  }

  using api = detail::basic_api<>;
  using engine = detail::basic_engine<>;
  using stack_guard = detail::basic_stack_guard<>;
}

/**
 * JS related exception types.
 */
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

        explicit basic_script_error(std::string&& msg) : std::runtime_error(std::move(msg)), callstack_()
        { remove_internal_traces(); }

        explicit basic_script_error(std::string&& msg, std::string&& callstack) : std::runtime_error(std::move(msg)), callstack_(std::move(callstack))
        { remove_internal_traces(); }

        const std::string& callstack() const noexcept
        { return callstack_; }

      private:

        inline void remove_internal_traces() noexcept
        {
          // Remove internal C/C++ lines, on error (e.g. nomem) simply not.
          try {
            // c++17 multiline missing ...
            // const std::regex re("^\\s+at\\s+\\[anon\\].*?internal\\s*$", std::regex_constants::icase|std::regex_constants::multiline);
            // stack = std::regex_replace(stack, re, "\n");
            // hence, the hard way ...
            std::stringstream ss(callstack_);
            std::string stack;
            const std::regex re("preventsyield$", std::regex_constants::icase);
            for(std::string line; std::getline(ss, line);) {
              if((line.find("[anon]")!=line.npos) && (line.find(" internal")!=line.npos)) continue;
              line = std::regex_replace(line, re, "");
              stack += line;
              stack += "\n";
            }
            callstack_.swap(stack);
          } catch(...) {
            (void)0; // leave callstack_ stack as it is.
          }
        }

      private:
        std::string callstack_;
    };

    template <typename=void>
    class basic_exit_exception
    {
      public:
        explicit basic_exit_exception() noexcept : code_(0) { ; }
        explicit basic_exit_exception(int code) noexcept : code_(code) { ; }
        basic_exit_exception(const basic_exit_exception&) noexcept = default;
        basic_exit_exception(basic_exit_exception&&) noexcept = default;
        basic_exit_exception& operator=(const basic_exit_exception&) noexcept = default;
        basic_exit_exception& operator=(basic_exit_exception&&) noexcept = default;
        ~basic_exit_exception() noexcept = default;

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

/**
 * Duktape property access wrappers.
 */
namespace duktape { namespace detail {

  namespace {

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
  }

  /**
   * define(....) : Defines are done using these flags (DUK_DEFPROP_HAVE_... will be
   * automatically added). The default is defining sealed, frozen - means not writable,
   * not configurable, but enumerable.
   */
  template <typename T>
  struct basic_defprop_flags
  {
    using type = T;
    static constexpr type restricted = 0;
    static constexpr type writable = (DUK_DEFPROP_WRITABLE);
    static constexpr type enumerable = (DUK_DEFPROP_ENUMERABLE);
    static constexpr type configurable = (DUK_DEFPROP_CONFIGURABLE);
    static constexpr type defaults = (DUK_DEFPROP_ENUMERABLE);

    /**
     * Handles/converts the flags needed for def_prop() from the simplified
     * `defflags::*` constants.
     *
     * @param typename type
     * @return unsigned
     */
    static unsigned convert(type flags) noexcept
    {
      return (((unsigned)0) | DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE
        | DUK_DEFPROP_HAVE_WRITABLE | ((flags & writable) ? DUK_DEFPROP_WRITABLE:0)
        | DUK_DEFPROP_HAVE_CONFIGURABLE | ((flags & configurable) ? DUK_DEFPROP_CONFIGURABLE:0)
        | DUK_DEFPROP_HAVE_ENUMERABLE | ((flags & enumerable) ? DUK_DEFPROP_ENUMERABLE:0)
      );
    }
  };

  using defprop_flags = basic_defprop_flags<unsigned>;

}}

/**
 * Stack Guard, usage like `lock_guard`, ensures that the stack
 * top is reset to its original value when leaving the scope.
 */
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

    basic_stack_guard() = delete;
    basic_stack_guard(const basic_stack_guard&) = delete;
    basic_stack_guard(basic_stack_guard&&) = default;
    basic_stack_guard& operator=(const basic_stack_guard&) = delete;
    basic_stack_guard& operator=(basic_stack_guard&&) = default;

    basic_stack_guard(::duk_context* ctx, bool collect_garbage=false) noexcept
            : ctx_(ctx), initial_top_((!ctx) ? (-1) : (::duk_get_top(ctx))), gc_(collect_garbage)
    {}

    template <typename T>
    basic_stack_guard(const basic_api<T>& o, bool collect_garbage=false) noexcept
            : ctx_(o.ctx()), initial_top_((!o.ctx()) ? (-1) : (::duk_get_top(o.ctx()))), gc_(collect_garbage)
    {}

    ~basic_stack_guard() noexcept
    {
      if(!ctx_ || initial_top_ < 0) return;
      ::duk_idx_t top = ::duk_get_top(ctx_);
      if(top <= initial_top_) return;
      ::duk_set_top(ctx_, initial_top_);
    }

  public:

    ::duk_context* ctx() const noexcept
    { return ctx_; }

    ::duk_idx_t initial_top() const noexcept
    { return initial_top_; }

    void initial_top(::duk_idx_t index) noexcept
    { initial_top_ = index; }

  private:

    ::duk_context* ctx_;
    ::duk_idx_t initial_top_;
    bool gc_;
  };

  template <typename MutexType, bool StrictInclude>
  class basic_engine_lock
  {
  public:

    using engine_type = basic_engine<MutexType, StrictInclude>;
    using lock_guard_type = typename engine_type::lock_guard_type;

  public:

    basic_engine_lock() = delete;
    basic_engine_lock(const basic_engine_lock&) = delete;
    basic_engine_lock(basic_engine_lock&&) noexcept = delete;
    basic_engine_lock& operator=(const basic_engine_lock&) = delete;
    basic_engine_lock& operator=(basic_engine_lock&&) noexcept = delete;
    basic_engine_lock(engine_type& js) noexcept : lock_(js.mutex_) {}
    ~basic_engine_lock() noexcept = default;

  private:

    lock_guard_type lock_;
  };

}}

/**
 * Main Duktape API wrapper. Mostly referred to as variables called `stack`.
 */
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
  public:

    using context_type = ::duk_context*;
    using index_type = ::duk_idx_t;
    using codepoint_type = ::duk_codepoint_t;
    using array_index_type = ::duk_uarridx_t;
    using thread_state_type = ::duk_thread_state;
    using size_type = ::size_t;
    using string = std::string;

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

  public:

    basic_api() noexcept : ctx_(0)
    { ; }

    basic_api(const basic_api& o) noexcept : ctx_(o.ctx_)
    { ; }

    basic_api(basic_api&&) noexcept = default;

    basic_api(::duk_context* ct) noexcept : ctx_(ct)
    { ; }

    template <typename T, bool S>
    basic_api(const basic_engine<T,S>& en) : ctx_(en.ctx())
    { ; }

    ~basic_api() noexcept
    { ; }

    basic_api& operator=(const basic_api&) = delete;
    basic_api& operator=(basic_api&&) = default;

  public:

    context_type ctx() const noexcept
    { return ctx_; }

    context_type ctx(context_type ctx) noexcept
    { ctx_ = ctx; return ctx_; }

  public:

    void compile(compile_flags_t flags) const
    { duk_compile(ctx_, flags); }

    void compile_string(compile_flags_t flags, const string& src) const
    { duk_compile_lstring(ctx_, flags, src.data(), static_cast<::duk_size_t>(src.size())); }

    void eval() const
    { duk_eval(ctx_); }

    void eval_string(const string& src) const
    { duk_eval_lstring(ctx_, src.data(), static_cast<::duk_size_t>(src.size())); }

    void eval_string_noresult(const string& src) const
    { duk_eval_lstring_noresult(ctx_, src.data(), static_cast<::duk_size_t>(src.size())); }

    void call(int nargs) const
    { ::duk_call(ctx_,  nargs); }

    void call_method(int nargs) const
    { ::duk_call_method(ctx_, nargs); }

    void call_prop(index_type obj_index, int nargs) const
    { ::duk_call_prop(ctx_, obj_index, nargs); }

    int pcompile(compile_flags_t flags) const
    { return duk_pcompile(ctx_, flags); }

    int pcompile_string(compile_flags_t flags, const string& src) const
    { return duk_pcompile_lstring(ctx_, flags, src.data(), static_cast<::duk_size_t>(src.length())); }

    int pcompile_file(compile_flags_t flags, const string& src) const
    { return duk_pcompile_lstring_filename(ctx_, flags, src.data(), static_cast<::duk_size_t>(src.length())); }

    int peval() const
    { return duk_peval(ctx_); }

    int peval_string(const string& src) const
    { return duk_peval_lstring(ctx_, src.data(), static_cast<::duk_size_t>(src.length())); }

    int peval_string_noresult(const string& src) const
    { return duk_peval_lstring_noresult(ctx_, src.data(), static_cast<::duk_size_t>(src.length())); }

    int pcall(int nargs) const
    { return ::duk_pcall(ctx_, nargs); }

    int pcall_method(int nargs) const
    { return ::duk_pcall_method(ctx_, nargs); }

    int pcall_prop(index_type obj_index, int nargs) const
    { return ::duk_pcall_prop(ctx_, obj_index, nargs); }

    int safe_call(::duk_safe_call_function func, void *udata, int nargs, int nrets) const
    { return ::duk_safe_call(ctx_, func, udata, nargs, nrets); }

    bool check_type(index_type index, int type) const
    { return ::duk_check_type(ctx_, index, type) != 0; }

    bool check_type_mask(index_type index, unsigned mask) const
    { return ::duk_check_type_mask(ctx_, index, mask) != 0; }

    index_type get_top() const
    { return ::duk_get_top(ctx_); }

    void set_top(index_type index) const
    { ::duk_set_top(ctx_, index); }

    index_type get_top_index() const
    { return ::duk_get_top_index(ctx_); }

    void dup(index_type from_index) const
    { ::duk_dup(ctx_, from_index); }

    void dup_top() const
    { ::duk_dup_top(ctx_); }

    void copy(index_type from_index, index_type to_index) const
    { ::duk_copy(ctx_, from_index, to_index); }

    void remove(index_type index) const
    { ::duk_remove(ctx_, index); }

    void insert(index_type to_index) const
    { ::duk_insert(ctx_, to_index); }

    void replace(index_type to_index) const
    { ::duk_replace(ctx_, to_index); }

    void swap(index_type index1, index_type index2) const
    { ::duk_swap(ctx_, index1, index2); }

    void swap_top(index_type index) const
    { ::duk_swap_top(ctx_, index); }

    bool check_stack(index_type extra) const
    { return ::duk_check_stack(ctx_, extra) != 0; }

    bool check_stack_top(index_type top) const
    { return ::duk_check_stack_top(ctx_, top) != 0; }

    bool pnew(int nargs) const
    { return ::duk_pnew(ctx_, nargs) == 0; }

    bool next(index_type enum_index, bool get_value) const
    { return ::duk_next(ctx_, enum_index, ::duk_bool_t(get_value)) != 0; }

    void compact(index_type obj_index) const
    { ::duk_compact(ctx_, obj_index); }

    bool del_prop(index_type obj_index) const
    { return ::duk_del_prop(ctx_, obj_index) != 0; }

    bool del_prop_index(index_type obj_index, array_index_type arr_index) const
    { return ::duk_del_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool del_prop_string(index_type obj_index, const char* key) const
    { return key && (::duk_del_prop_string(ctx_, obj_index, key) != 0); }

    bool del_prop_string(index_type obj_index, const string& key) const
    { return ::duk_del_prop_lstring(ctx_, obj_index, key.c_str(), key.length()) != 0; }

    bool get_prop(index_type obj_index) const
    { return ::duk_get_prop(ctx_, obj_index) != 0; }

    bool get_prop_index(index_type obj_index, array_index_type arr_index) const
    { return ::duk_get_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool get_prop_string(index_type obj_index, const char* key) const
    { return key && (::duk_get_prop_string(ctx_, obj_index, key) != 0); }

    bool get_prop_string(index_type obj_index, const string& key) const
    { return ::duk_get_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    void get_prop_desc(index_type obj_index, unsigned flags) const
    { ::duk_get_prop_desc(ctx_, obj_index, duk_uint_t(flags)); }

    void get_prototype(index_type index) const
    { ::duk_get_prototype(ctx_, index); }

    bool get_global_string(const char* key) const
    { return key && (::duk_get_global_string(ctx_, key) != 0); }

    bool get_global_string(const string& key) const
    { return ::duk_get_global_lstring(ctx_, key.c_str(), key.size()) != 0; }

    bool has_prop(index_type obj_index) const
    { return ::duk_has_prop(ctx_, obj_index) != 0; }

    bool has_prop_index(index_type obj_index, array_index_type arr_index) const
    { return ::duk_has_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool has_prop_string(index_type obj_index, const char* key) const
    { return key && (::duk_has_prop_string(ctx_, obj_index, key) != 0); }

    bool has_prop_string(index_type obj_index, const string& key) const
    { return ::duk_has_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    void put_function_list(index_type obj_index, const ::duk_function_list_entry *funcs) const
    { ::duk_put_function_list(ctx_, obj_index, funcs); }

    bool put_global_string(const char* key)
    { return ::duk_put_global_string(ctx_, key); }

    bool put_global_string(const string& key)
    { return ::duk_put_global_lstring(ctx_, key.c_str(), key.size()); }

    void put_number_list(index_type obj_index, const duk_number_list_entry *numbers) const
    { ::duk_put_number_list(ctx_, obj_index, numbers); }

    bool put_prop(index_type obj_index) const
    { return ::duk_put_prop(ctx_, obj_index) != 0; }

    bool put_prop_index(index_type obj_index, array_index_type arr_index) const
    { return ::duk_put_prop_index(ctx_, obj_index, arr_index) != 0; }

    bool put_prop_string(index_type obj_index, const char* key) const
    { return key && (::duk_put_prop_string(ctx_, obj_index, key) != 0); }

    bool put_prop_string(index_type obj_index, const string& key) const
    { return ::duk_put_prop_lstring(ctx_, obj_index, key.c_str(), key.size()) != 0; }

    index_type normalize_index(index_type index) const
    { return ::duk_normalize_index(ctx_, index); }

    bool get_boolean(index_type index) const
    { return ::duk_get_boolean(ctx_, index) != 0; }

    const void* get_buffer(index_type index, size_type& out_size) const
    {
      auto sz = ::duk_size_t(0);
      auto rp = static_cast<const void*>(::duk_get_buffer(ctx_, index, &sz));
      out_size = static_cast<size_type>(sz);
      return rp;
    }

    const void* get_buffer_data(index_type index, size_type& out_size) const
    {
      auto sz = ::duk_size_t(0);
      auto rp = static_cast<const void*>(::duk_get_buffer_data(ctx_, index, &sz));
      out_size = static_cast<size_type>(sz);
      return rp;
    }

    ::duk_c_function get_c_function(index_type index) const
    { return ::duk_get_c_function(ctx_, index); }

    duk_context* get_context(index_type index) const
    { return ::duk_get_context(ctx_, index); }

    int get_int(index_type index) const
    { return ::duk_get_int(ctx_,  index); }

    index_type get_length(index_type index) const
    { return static_cast<index_type>(::duk_get_length(ctx_, index)); }

    void get_memory_functions(duk_memory_functions& out_funcs) const
    { ::duk_get_memory_functions(ctx_, &out_funcs); }

    double get_now() const
    { return ::duk_get_now(ctx_); }

    double get_number(index_type index) const
    { return ::duk_get_number(ctx_, index); }

    void* get_pointer(index_type index) const
    { return ::duk_get_pointer(ctx_, index); }

    void* get_pointer(index_type index, void* default_value) const
    { return ::duk_get_pointer_default(ctx_, index, default_value); }

    string get_string(index_type index)  const
    { ::duk_size_t l=0; const char* s = ::duk_get_lstring(ctx_, index, &l); return (s && (l>0))?s:""; }

    unsigned get_uint(index_type index) const
    { return ::duk_get_uint(ctx_, index); }

    int get_type(index_type index) const
    { return ::duk_get_type(ctx_, index); }

    unsigned get_type_mask(index_type index) const
    { return ::duk_get_type_mask(ctx_, index); }

    bool is_instanceof(index_type obj, index_type proto) const
    { return ::duk_instanceof(ctx_, obj, proto) != 0; }

    bool is_array(index_type index) const
    { return ::duk_is_array(ctx_, index) != 0; }

    bool is_boolean(index_type index) const
    { return ::duk_is_boolean(ctx_, index) != 0; }

    bool is_bound_function(index_type index) const
    { return ::duk_is_bound_function(ctx_, index) != 0; }

    bool is_buffer(index_type index) const
    { return ::duk_is_buffer(ctx_, index) != 0; }

    bool is_buffer_data(index_type index) const
    { return ::duk_is_buffer_data(ctx_, index) != 0; }

    bool is_c_function(index_type index) const
    { return ::duk_is_c_function(ctx_, index) != 0; }

    bool is_callable(index_type index) const
    { return duk_is_callable(ctx_, index) != 0; }

    bool is_constructor_call() const
    { return ::duk_is_constructor_call(ctx_) != 0; }

    bool is_dynamic_buffer(index_type index) const
    { return ::duk_is_dynamic_buffer(ctx_, index) != 0; }

    bool is_ecmascript_function(index_type index) const
    { return ::duk_is_ecmascript_function(ctx_, index) != 0; }

    bool is_error(index_type index) const
    { return duk_is_error(ctx_, index) != 0; }

    bool is_eval_error(index_type index) const
    { return duk_is_eval_error(ctx_, index) != 0; }

    bool is_fixed_buffer(index_type index) const
    { return ::duk_is_fixed_buffer(ctx_, index) != 0; }

    bool is_function(index_type index) const
    { return ::duk_is_function(ctx_, index) != 0; }

    bool is_lightfunc(index_type index) const
    { return ::duk_is_lightfunc(ctx_, index) != 0; }

    bool is_nan(index_type index) const
    { return ::duk_is_nan(ctx_, index) != 0; }

    bool is_null(index_type index) const
    { return ::duk_is_null(ctx_, index) != 0; }

    bool is_null_or_undefined(index_type index) const
    { return duk_is_null_or_undefined(ctx_, index) != 0; }

    bool is_number(index_type index) const
    { return ::duk_is_number(ctx_, index) != 0; }

    bool is_object(index_type index) const
    { return ::duk_is_object(ctx_, index) != 0; }

    bool is_object_coercible(index_type index) const
    { return duk_is_object_coercible(ctx_, index) != 0; }

    bool is_pointer(index_type index) const
    { return ::duk_is_pointer(ctx_, index) != 0; }

    bool is_primitive(index_type index) const
    { return duk_is_primitive(ctx_, index) != 0; }

    bool is_range_error(index_type index) const
    { return duk_is_range_error(ctx_, index) != 0; }

    bool is_reference_error(index_type index) const
    { return duk_is_reference_error(ctx_, index) != 0; }

    bool is_strict_call() const
    { return ::duk_is_strict_call(ctx_) != 0; }

    bool is_string(index_type index) const
    { return ::duk_is_string(ctx_, index) != 0; }

    bool is_symbol(index_type index) const
    { return ::duk_is_symbol(ctx_, index) != 0; }

    bool is_syntax_error(index_type index) const
    { return duk_is_syntax_error(ctx_, index) != 0; }

    bool is_thread(index_type index) const
    { return ::duk_is_thread(ctx_, index) != 0; }

    bool is_type_error(index_type index) const
    { return duk_is_type_error(ctx_, index) != 0; }

    bool is_undefined(index_type index) const
    { return ::duk_is_undefined(ctx_, index) != 0; }

    bool is_uri_error(index_type index) const
    { return duk_is_uri_error(ctx_, index) != 0; }

    bool is_valid_index(index_type index) const
    { return ::duk_is_valid_index(ctx_, index) != 0; }

    index_type push_array() const
    { return ::duk_push_array(ctx_); }

    index_type push_bare_object() const
    { return ::duk_push_bare_object(ctx_); }

    void push_boolean(bool val) const
    { ::duk_push_boolean(ctx_, ::duk_bool_t(val)); }

    void* push_buffer(size_type size, bool dynamic) const
    { return duk_push_buffer(ctx_, ::duk_size_t(size), ::duk_bool_t(dynamic)); }

    void push_buffer_object(index_type buffer_index, size_type byte_offset, size_type byte_length, unsigned flags) const
    { return ::duk_push_buffer_object(ctx_, buffer_index, ::duk_size_t(byte_offset), ::duk_size_t(byte_length), ::duk_uint_t(flags)); }

    index_type push_c_function(::duk_c_function func, int nargs=DUK_VARARGS) const
    { return ::duk_push_c_function(ctx_, func, nargs); }

    void push_context_dump() const
    { ::duk_push_context_dump(ctx_); }

    void push_current_function() const
    { ::duk_push_current_function(ctx_); }

    void push_current_thread() const
    { ::duk_push_current_thread(ctx_); }

    void* push_dynamic_buffer(size_type size) const
    { return duk_push_dynamic_buffer(ctx_, ::duk_size_t(size)); }

    void push_false() const
    { ::duk_push_false(ctx_); }

    void* push_fixed_buffer(size_type size) const
    { return duk_push_fixed_buffer(ctx_, ::duk_size_t(size)); }

    void push_global_object() const
    { ::duk_push_global_object(ctx_); }

    void push_global_stash() const
    { ::duk_push_global_stash(ctx_); }

    void push_heap_stash() const
    { ::duk_push_heap_stash(ctx_); }

    void push_int(int val) const
    { ::duk_push_int(ctx_, val); }

    void push_string(const char* s) const
    { ::duk_push_string(ctx_, s ? s : ""); }

    void push_string(const string& s) const
    { ::duk_push_lstring(ctx_, s.data(), ::duk_size_t(s.size())); }

    void push_string(string&& s) const
    { ::duk_push_lstring(ctx_, s.data(), ::duk_size_t(s.size())); }

    void push_nan() const
    { ::duk_push_nan(ctx_); }

    void push_null() const
    { ::duk_push_null(ctx_); }

    void push_number(double val) const
    { ::duk_push_number(ctx_, val); }

    index_type push_object() const
    { return ::duk_push_object(ctx_); }

    template<typename T>
    void push_pointer(T *p) const
    { ::duk_push_pointer(ctx_, reinterpret_cast<void*>(p)); } // NOLINT: C API pointer interface.

    void push_this() const
    { ::duk_push_this(ctx_); }

    index_type push_thread() const
    { return duk_push_thread(ctx_); }

    index_type push_thread_new_globalenv() const
    { return duk_push_thread_new_globalenv(ctx_); }

    void push_thread_stash(duk_context *target_ctx) const
    { ::duk_push_thread_stash(ctx_, target_ctx); }

    void push_true() const
    { ::duk_push_true(ctx_); }

    void push_uint(unsigned val) const
    { ::duk_push_uint(ctx_, val); }

    void push_undefined() const
    { ::duk_push_undefined(ctx_); }

    index_type push_proxy() const
    { return ::duk_push_proxy(ctx_, 0); } // proxy_flags unused, must be 0.

    void pop() const
    { ::duk_pop(ctx_); }

    void pop(index_type count) const
    { if(count > 0) ::duk_pop_n(ctx_, count); }

    void require_stack(index_type extra) const
    { ::duk_require_stack(ctx_, extra); }

    bool to_boolean(index_type index) const
    { return ::duk_to_boolean(ctx_, index) != 0; }

    template<::duk_uint_t Mode=(DUK_BUF_MODE_DONTCARE)>
    void* to_buffer(index_type index, size_type& out_size) const
    {
      auto sz = ::duk_size_t(0);
      auto rp = static_cast<void*>(::duk_to_buffer_raw(ctx_, index, &sz, Mode));
      out_size = static_cast<size_type>(sz);
      return rp;
    }

    void* to_dynamic_buffer(index_type index, size_type& out_size) const
    { return to_buffer<DUK_BUF_MODE_DYNAMIC>(index, out_size); }

    void* to_fixed_buffer(index_type index, size_type& out_size) const
    { return to_buffer<DUK_BUF_MODE_FIXED>(index, out_size); }

    int to_int(index_type index) const
    { return ::duk_to_int(ctx_, index); }

    int32_t to_int32(index_type index) const
    { return ::duk_to_int32(ctx_, index); }

    void to_null(index_type index) const
    { ::duk_to_null(ctx_, index); }

    double to_number(index_type index) const
    { return ::duk_to_number(ctx_, index); }

    void to_object(index_type index) const
    { ::duk_to_object(ctx_, index); }

    const void* to_pointer(index_type index) const
    { return (const void*) ::duk_to_pointer(ctx_, index); }

    void to_primitive(index_type index, int hint) const
    { ::duk_to_primitive(ctx_, index, hint); }

    unsigned to_uint(index_type index) const
    { return ::duk_to_uint(ctx_, index); }

    uint16_t to_uint16(index_type index) const
    { return ::duk_to_uint16(ctx_, index); }

    uint32_t to_uint32(index_type index) const
    { return ::duk_to_uint32(ctx_, index); }

    void to_undefined(index_type index) const
    { ::duk_to_undefined(ctx_, index); }

    string to_string(index_type index) const
    { ::duk_size_t l=0; const char* s = ::duk_to_lstring(ctx_, index, &l); return (s && (l>0))?(s):(""); }

    string safe_to_string(index_type index) const
    { ::duk_size_t l=0; const char *s = ::duk_safe_to_lstring(ctx_, index, &l); return (s && (l>0)) ? (string(s, size_t(0), size_t(l))) : (""); }

    string to_stacktrace(index_type index) const
    { const char *s = ::duk_to_stacktrace(ctx_, index); return string((s)?(s):("Error")); }

    string safe_to_stacktrace(index_type index) const
    { return string(::duk_safe_to_stacktrace(ctx_, index)); }

    bool samevalue(index_type index1, index_type index2)
    { return !!::duk_samevalue(ctx_, index1, index2); }

    void concat(index_type count) const
    { ::duk_concat(ctx_, count); }

    void join(index_type count) const
    { ::duk_join(ctx_, count); }

    void substring(index_type index, size_type start_char_offset, size_type end_char_offset) const
    { ::duk_substring(ctx_, index, ::duk_size_t(start_char_offset), ::duk_size_t(end_char_offset)); }

    void trim(index_type index) const
    { ::duk_trim(ctx_, index); }

    codepoint_type char_code_at(index_type index, size_type char_offset) const
    { return ::duk_char_code_at(ctx_, index, ::duk_size_t(char_offset)); }

    void json_decode(index_type index) const
    { ::duk_json_decode(ctx_, index); }

    string json_encode(index_type index) const
    { const char* s = ::duk_json_encode(ctx_, index); return (s) ? (s) : (""); }

    string base64_encode(index_type index) const
    { const char* s = ::duk_base64_encode(ctx_, index); return (s) ? (s) : (""); }

    void base64_decode(index_type index) const
    { ::duk_base64_decode(ctx_, index); }

    void hex_decode(index_type index) const
    { ::duk_hex_decode(ctx_, index); }

    string hex_encode(index_type index) const
    { const char* s = ::duk_hex_encode(ctx_, index); return (s) ? (s) : (""); }

    string buffer_to_string(index_type index) const
    { const char* s = ::duk_buffer_to_string(ctx_, index); return (s) ? (s) : (""); }

    void map_string(index_type index, ::duk_map_char_function callback, void *udata) const
    { ::duk_map_string(ctx_, index, callback, udata); }

    void decode_string(index_type index, ::duk_decode_char_function callback, void *udata) const
    { ::duk_decode_string(ctx_, index, callback, udata); }

    void* resize_buffer(index_type index, size_type new_size) const
    { return ::duk_resize_buffer(ctx_, index, ::duk_size_t(new_size)); }

    void* alloc(size_type size) const
    { return ::duk_alloc(ctx_, ::duk_size_t(size)); }

    void* alloc_raw(size_type size) const
    { return ::duk_alloc_raw(ctx_, ::duk_size_t(size)); }

    void* realloc(void *ptr, size_type size) const
    { return ::duk_realloc(ctx_, ptr, ::duk_size_t(size)); }

    void* realloc_raw(void *ptr, size_type size) const
    { return ::duk_realloc_raw(ctx_, ptr, ::duk_size_t(size)); }

    void free(void *ptr) const
    { ::duk_free(ctx_, ptr); }

    void free_raw(void *ptr) const
    { ::duk_free_raw(ctx_, ptr); }

    void enumerator(index_type obj_index, enumerator_flags enum_flags)  const
    { ::duk_enum(ctx_, obj_index, ::duk_uint_t(enum_flags)); }

    bool equals(index_type index1, index_type index2) const
    { return ::duk_equals(ctx_, index1, index2) != 0; }

    bool strict_equals(index_type index1, index_type index2) const
    { return ::duk_strict_equals(ctx_, index1, index2); }

    void gc() const  // unsigned flags: not defined yet
    { ::duk_gc(ctx_, 0); }

    int throw_exception() const
    { ::duk_throw_raw(ctx_); return 0; }

    int throw_exception(string msg)
    { return error(error_code_t::err_ecma, msg); }

    int error(error_code_t err_code, const string& msg, string file="(native c++)", int line=0)
    { ::duk_error_raw(ctx_, ::duk_errcode_t(err_code), file.c_str(), ::duk_int_t(line), "%s", msg.c_str()); return 0; }

    error_code_t get_error_code(index_type index) const
    { return error_code_t(::duk_get_error_code(ctx_, index));  }

    void push_external_buffer()
    { duk_push_external_buffer(ctx_); }

    void push_external_buffer(void* data, size_type size)
    { duk_push_external_buffer(ctx_); ::duk_config_buffer(ctx_, -1, data, ::duk_size_t(size)); }

    void config_buffer(index_type index, void* data, size_type size)
    { ::duk_config_buffer(ctx_, index, data, ::duk_size_t(size)); }

    string dump_context() const
    { stack_guard sg(ctx_); push_context_dump(); return safe_to_string(-1); }

    void def_prop(index_type index, unsigned flags) const
    { ::duk_def_prop(ctx_, index, ::duk_uint_t(flags)); }

    void def_prop(index_type index) const
    { ::duk_def_prop(ctx_, index, detail::defprop_flags::convert(detail::defprop_flags::defaults)); }

    void dump_function() const
    { ::duk_dump_function(ctx_); }

    void load_function() const
    { ::duk_load_function(ctx_); }

    void get_finalizer(index_type index) const
    { ::duk_get_finalizer(ctx_, index); }

    void set_finalizer(index_type index) const
    { ::duk_set_finalizer(ctx_, index); }

    void set_global_object() const
    { ::duk_set_global_object(ctx_); }

    void set_length(index_type index, size_type length) const
    { ::duk_set_length(ctx_, index, ::duk_size_t(length)); }

    void set_prototype(index_type index) const
    { ::duk_set_prototype(ctx_, index); }

    void* steal_buffer(index_type index, size_type& out_size)
    {
      auto sz = ::duk_size_t(0);
      auto rp = static_cast<void*>(::duk_steal_buffer(ctx_, index, &sz));
      out_size = static_cast<size_type>(sz);
      return rp;
    }

    thread_state_type suspend() const
    { thread_state_type state; ::duk_suspend(ctx_, &state); return state; }

    void resume(const thread_state_type& state) const
    { ::duk_resume(ctx_, &state); }

    void inspect_callstack_entry(int level) const
    { ::duk_inspect_callstack_entry(ctx_, ::duk_int_t(level)); }

    void inspect_value(index_type index) const
    { ::duk_inspect_value(ctx_, index); }

    void seal(index_type index) const
    { ::duk_seal(ctx_, index); }

    void freeze(index_type index) const
    { ::duk_freeze(ctx_, index); }

    void xcopy_to_thread(basic_api& to, size_type count)
    { ::duk_require_stack(to.ctx_, ::duk_idx_t(count)); ::duk_xcopymove_raw(to.ctx_, ctx_, ::duk_idx_t(count), 1); }

    void xmove_to_thread(basic_api& to, size_type count)
    { ::duk_require_stack(to.ctx_, ::duk_idx_t(count)); ::duk_xcopymove_raw(to.ctx_, ctx_, ::duk_idx_t(count), 0); }

  public:

    /**
     * Get a value from an object or a default if not existing.
     */
    template <typename T>
    T get_prop_string(index_type object_index, const char* property_name, T default_value) const
    {
      if(!has_prop_string(object_index, property_name)) return default_value;
      if(get_prop_string(object_index, property_name)) default_value = get<T>(-1);
      pop();
      return default_value;
    }

    template <typename ContainerType>
    ContainerType buffer(index_type index) const
    {
      ContainerType data;
      ::duk_size_t size = 0;
      const char* buffer = nullptr;
      if(is_buffer(index)) {
        buffer = static_cast<const char*>(::duk_get_buffer(ctx_, index, &size));
      } else if(is_buffer_data(index)) {
        buffer = static_cast<const char*>(::duk_get_buffer_data(ctx_, index, &size));
      }
      if(buffer && size) {
        data.resize(size);
        std::copy(&(buffer[0]), &(buffer[size]), data.begin()); // NOLINT: No std::span available yet.
      }
      return data;
    }

    /**
     * Push a (raw) buffer with defined size and in turn push a
     * buffer object of type ArrayBuffer linked to the raw buffer.
     * Returns a pointer to the allocated buffer. If the allocation
     * failed, then pushing the buffer view object is omitted.
     * @param size_type size
     * @param bool dynamic
     * @return void*
     */
    void* push_array_buffer(size_type size, bool dynamic) const
    {
      void* p = duk_push_buffer(ctx_, ::duk_size_t(size), ::duk_bool_t(dynamic));
      if(p) ::duk_push_buffer_object(ctx_, -1, 0, size, DUK_BUFOBJ_ARRAYBUFFER);
      return p;
    }

    /**
     * Like put_prop_string(obj_index, key), except that the property
     * is not accessible from the ECMA engine. Uses `def_prop()` with
     * not-enumerable,not-writable,not-configurable and prepends "\xff_"
     * to the key.
     *
     * @param index_type obj_index
     * @param string key
     * @return bool
     */
    bool put_prop_string_hidden(index_type obj_index, string key) const
    {
      if(key.empty() || (!is_object(obj_index))) return false;
      require_stack(1);
      push_string(string("\xff_") + key);
      swap(-1,-2);
      def_prop(obj_index, defprop_flags::convert(defprop_flags::restricted));
      return true;
    }

    /**
     * Like get_prop_string(obj_index, key), except that a hidden property
     * (key prepended with "\xff_") is retrieved. Uses `get_prop_string()`.
     *
     * @param index_type obj_index
     * @param const string& key
     * @return bool
     */
    bool get_prop_string_hidden(index_type obj_index, const string& key) const
    { return get_prop_string(obj_index, string("\xff_") + key); }

    /**
     * The method returns the absolute index of a given **valid** index (absolute or relative).
     * @param index_type index
     * @return index_type
     */
    index_type absindex(index_type index) const noexcept
    { return (index >= 0) ? (index) : (top() + index); }

    /**
     * Raw evaluation with flags. src_buffer=nullptr, src_length=0 --> get from current stack.
     * Returns 0 on ok, !0 on fail.
     * @param const char* src_buffer
     * @param size_type src_length
     * @param unsigned flags
     * @return int
     */
    int eval_raw(const char* src_buffer, size_type src_length, unsigned flags)
    { return ::duk_eval_raw(ctx_, src_buffer, ::duk_size_t(src_length), ::duk_uint_t(flags)); }

    /**
     * Returns the type name of a ECMA value on the stack.
     * The returned pointer is guaranteed non-nullptr.
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
        default:                 return "(unknown type!)";
      }
    }

    /**
     * Alias of `get_top()`.
     * @return index_type
     */
    index_type top() const
    { return get_top(); }

    /**
     * Alias of get_top_index()
     */
    index_type top_index() const
    { return get_top_index(); }

    /**
     * Returns true of a value on the stack corresponds to the c++ type specified
     * with typename T, false otherwise. Note that `const char*` is not valid, use
     * `string` instead. Uses functions `duk_is_*`.
     * @param index_type index
     * @return typename T
     */
    template <typename T>
    bool is(index_type index) const
    { return conv<T>::is(ctx_, index); }

    /**
     * Get a value from the stack with implicit coersion.
     * Uses functions `duk_to_*`.
     * @param index_type index
     * @return typename T
     */
    template <typename T>
    T to(index_type index) const
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
     * @param index_type index
     * @return typename T
     */
    template <typename T>
    T get(index_type index) const
    { return conv<T>::get(ctx_, index); }

    /**
     * Get a value from the stack withOUT implicit conversion / coersion.
     * Returns the default value if the index exceeds the stack top.
     * @param index_type index
     * @param T default_value
     * @return typename T
     */
    template <typename T>
    T get(index_type index, const T& default_value) const
    { return (top() <= index) ? (default_value) : (conv<T>::get(ctx_, index)); }

    /**
     * Resets the stack top to 0 and runs the
     * garbage collector.
     */
    void clear()
    { top(0); gc(); }

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
     * Alias of set_top(index_type).
     * @param index_type index
     */
    void top(index_type index) const
    { set_top(index); }

    /**
     * Recursively selects sub objects of the global object defined by a dot
     * separated path (e.g. "system.debug.whatever").
     * Returns boolean success. If the method fails, then the stack will be
     * unchanged. Otherwise the selected property will be on top of the stack.
     *
     * @param string name
     * @return bool
     */
    bool select(string name) const
    {
      if(name.empty()) return false;
      index_type initial_top = top();
      push_global_object();
      string s;
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
    bool set(string&& key, T&& value)
    { return property(std::move(key), std::move(value)); }

    template <typename T>
    bool property(string&& key, T&& value)
    {
      if(!is_object(-1)) return false;
      push(std::move(key));
      push(std::move(value));
      put_prop(-3);
      return true;
    }

    template <typename T>
    T property(string&& key)
    {
      if(!is_object(-1)) return T();
      push(std::move(key));
      get_prop(-2);
      const T value = is_undefined(-1) ? T() : to<T>(-1);
      pop();
      return value;
    }

    bool is_date(index_type index) const
    {
      index = absindex(index);
      if(!is_object(index)) return false;
      check_stack(1);
      get_global_string("Date");
      bool r = is_instanceof(index, -1);
      pop();
      return r;
    }

    bool is_regex(index_type index) const
    {
      index = absindex(index);
      if(!is_object(index)) return false;
      check_stack(1);
      get_global_string("RegExp");
      bool r = is_instanceof(index, -1);
      pop();
      return r;
    }

    bool is_false(index_type index) const
    { return is_boolean(index) && !get_boolean(index); }

    bool is_true(index_type index) const
    { return is_boolean(index) && get_boolean(index); }

    engine& parent_engine() const
    {
      stack_guard sg(*this);
      push_heap_stash();
      get_prop_string(-1, "_engine_");
      if(!is_pointer(-1)) {
        throw engine_error("Duktape stack has no duktape::engine assigned.");
      }
      return *static_cast<engine*>(get_pointer(-1));
    }

    string callstack() const
    {
      // note: Due to performance tests choosing request of
      //       one stack trace and performing a string
      //       analysis to filer and format.
      auto currtop = top();
      ::duk_push_error_object_raw(ctx(), DUK_ERR_ERROR, __FILE__, __LINE__, "Trace");
      get_prop_string(-1, "stack");
      string s = get<string>(-1);
      top(currtop);
      string out;
      s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
      while(!s.empty() && s.back() == '\n') s.pop_back();
      auto p = s.find('\n');
      if(p != s.npos) {
        s = s.substr(p+1); // omit error name
        do {
          p = s.find('\n');
          string line = s.substr(0, p);
          s = s.substr(p+1);
          if(line.find("at [anon] (") != line.npos) continue;
          auto ps = line.rfind(')');
          if(ps == line.npos) continue;
          line.resize(ps);
          ps = line.find(" at ");
          if((ps == line.npos) || (ps == line.size()-1)) continue;
          line = line.substr(ps+4);
          ps = line.rfind("(");
          if((ps == line.npos) || (ps == line.size()-1)) continue;
          line[ps] = '@';
          out += line + "\n";
        } while(p != s.npos);
      }
      while(!out.empty() && ::isspace(out.back())) out.pop_back();
      return out;
    }

    /**
     * Returns a string representation of the current stack
     * in a c++ function. Use e.g. for debugging purposes.
     * @return string
     */
    string dump() const
    {
      string s = "function-stack {\n";
      for(index_type i=0; i<top(); ++i) {
        dup(i);
        string tos = (is_object(-1) && !is_null(-1) && !is_date(-1) && !is_regex(-1)) ? "[object]" : to_string(-1);
        s += string(" [") +  std::to_string(i) +  "] = (" + get_typename(i) + ") " + tos + "\n";
        pop();
      }
      s += "}";
      return s;
    }

    /**
     * Javascript type name query. Guaranteed non-nullptr.
     *
     * @param index_type index
     * @return const char*
     */
    const char* ecma_typename(index_type index) const
    {
      const auto& stack = *this;
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
      } else {
        return "(unrecognised script type)";
      }
    }

  private:

    template <typename T>
    void push_variadic(T val) const
    { push<T>(val); }

    template <typename T, typename ...Args>
    void push_variadic(T val, Args ...args) const
    { push<T>(val); push_variadic(args...); }

  private:

    duk_context* ctx_;
  };
}}

/**
 * JS <--> c++ type conversion functionality.
 */
namespace duktape { namespace detail {

  /**
   * This is only a long long list of conversion traits for the
   * common c++ types.
   */
  template <typename T>
  struct conv { using type = void; };

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

  // @note: The c++ community will kill me for using macros,
  //        but writing it all out or nesting further template
  //        structures is eventually less readable than this.
  #define decl_conv_tmp(TYPE, GET, REQ, TO, PUSH, IS, NAME) \
  template <> struct conv<TYPE> { \
    using type = TYPE; \
    static constexpr const char* cc_name() noexcept { return NAME; } \
    static constexpr const char* ecma_name() noexcept { return "Number"; } \
    static constexpr int nret() noexcept { return 1; } \
    static bool is(::duk_context* ctx, int index) noexcept { return IS(ctx, index); } \
    static type get(::duk_context* ctx, int index) noexcept { return GET(ctx, index); } \
    static type req(::duk_context* ctx, int index) noexcept  { return REQ(ctx, index); } \
    static type to(::duk_context* ctx, int index) noexcept { return TO(ctx, index); } \
    static void push(::duk_context* ctx, type val) noexcept { PUSH(ctx, val); } \
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

    static type to(duk_context* ctx, int index)
    { return api(ctx).to_boolean(index); }

    static void push(duk_context* ctx, const type& val)
    { return api(ctx).push_boolean(val); }

    static void push(duk_context* ctx, type&& val)
    { return api(ctx).push_boolean(val); }
  };

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

    static type to(duk_context* ctx, int index)
    { return api(ctx).to_string(index); }

    static void push(duk_context* ctx, const type& val)
    { return api(ctx).push_string(val); }

    static void push(duk_context* ctx, type&& val)
    { return api(ctx).push_string(val); }
  };

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
    { return api(ctx).push_string(static_cast<const char*>(val)); }
  };

  template <typename T> struct conv<std::vector<T>>
  {
    using type = std::vector<T>;

    static constexpr int nret() noexcept
    { return 1; }

    static inline const char* cc_name() noexcept
    { static const auto name = std::string("vector<") + conv<T>::cc_name() + ">"; return name.c_str(); }

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
      auto i = api::index_type(0);
      if(!stack.check_stack(4)) {
        stack.throw_exception("Not enough stack space (to push an array)");
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

    template <bool StrictReturn>
    static type get_array(duk_context* ctx, int index)
    {
      api stack(ctx);
      if(!stack.check_stack(4)) {
        stack.throw_exception("Not enough stack space (to get an array)");
        return type();
      } else if(!stack.is_array(index)) {
        stack.throw_exception("Property is no array.");
        return type();
      }
      auto ret = type();
      const api::index_type size = stack.get_length(index);
      for(auto i = api::index_type(0); i < size; ++i) {
        if(!stack.get_prop_index(index, i)) return type();
        ret.push_back(!StrictReturn ? stack.to<T>(-1) : stack.get<T>(-1));
        if(StrictReturn && !stack.is<T>(-1)) { stack.pop(); return type(); }
        stack.pop();
      }
      return ret;
    }

  };

}}

/**
 * Type for c++ functions callable from the JS engine, instead of a C `ctx`,
 * a `duktape::api` stack is used as interfacing handle.
 */
namespace duktape { namespace detail {
  using native_function_type = int(*)(api&);
}}

/**
 * Native wrappers to enable exposing c++ functions to the JS engine.
 */
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
      function_type fn = reinterpret_cast<function_type>(stack.get_pointer(-1)); // NOLINT: Only function_type pointers are used.
      stack.pop(2);
      if((!is_byref_api) && (stack.top() != sizeof...(Args))) {
        // Note: Duktape should have filled the args with undefined.
        return stack.throw_exception("Invalid number of arguments.");
      } else if(!fn) {
        return stack.throw_exception("Invalid function definition (bug in JS subsystem!)");
      } else if((!is_byref_api) && stack.is_constructor_call()) {
        return stack.throw_exception("The called function cannot be a constructor.");
      } else {
        if(!is_byref_api) {
          // For native c++ wrapped functions the conversion has to be
          // more strict because there is no possibility to check the
          // original arguments passed from JS.
          for(auto i = stack.top()-1; i >= 0; --i) {
            if(stack.is_undefined(i) || stack.is_null(i)) {
              return stack.throw_exception(std::string("Argument undefined/null passed to native function or missing arguments (argument: ") + std::to_string(i+1) + ").");
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
            return stack.throw_exception(e.what());
          } else {
            // Script error was caught by call() or eval() invoked in the function,
            // and the forwarded Error object is still on stack top --> rethrow.
            return stack.throw_exception();
          }
        } catch(const std::exception& e) {
          return stack.throw_exception(e.what());
        }
      }
    }
  };

}}}

/**
 * Native object interfacing functionality to enable simpler construction
 * and handling of JS object with a native backend.
 */
namespace duktape {

  /**
   * Native c++ object/class/struct wrapper.
   * Facilitates object instantiation, destruction,
   * method and property access via defined
   * wrapper functions/lambdas. Usage:
   *
   * // Assuming js is an `engine` instance defined above.
   * js.define(
   *   // Native object registrar constructor with the JS class name.
   *   // All following called methods return a reference to this object,
   *   // so that they can be chained. Finally `js.define()` is called
   *   // with the complete c++ class wrapping definition.
   *   native_object("MyJsClassName")
   *    // arguments are on the Duktape stack,
   *    // decide here how to construct your c++
   *    // instance and return the instance pointer
   *    // created using the `new` operator.
   *    // When the JS object is destroyed, the
   *    // created native c++ instance will be
   *    // `delete`d automatically.
   *    .constructor([](api& stack){
   *      return new my_cpp_class();
   *    })
   *    // --- define property getters (like `image.size` or `vec3d.x`)
   *    // You get the Duktape stack and the reference to your created
   *    // c++ instance. Push a value on the stack that shall be returned.
   *    .getter("my_property_name", [](api& stack, my_cpp_class& instance){
   *      stack.push(instance.my_prop());
   *    })
   *    // Same for setters, you get the Duktape stack and the reference to your
   *    // created c++ instance. The value to set is the last value on the stack.
   *    // You might want to check the type etc beforehand.
   *    .setter("my_property_name", [](api& stack, my_cpp_class& instance){
   *      if(!stack.is<my_cpp_class::my_property_type>(-1)) {
   *        throw script_error("Wrong type for setting MyJsClassName.my_property_name");
   *      }
   *      my_cpp_class::my_property_type new_value = stack.get<my_cpp_class::my_property_type>(-1);
   *      instance.my_prop(new_value);
   *    })
   *    // Example for calling a method if the c++ instance
   *    // All arguments are on the Duktape stack. For toString()
   *    // we ignore these. The value that you push on the stack
   *    // will be returned. Return `true` if you have something
   *    // to return, `false` if your function return is `void`.
   *    .method("toString", [](api& stack, my_cpp_class& instance) {
   *      stack.push(instance.to_string());
   *      return true;
   *    })
   *  );
   */
  template <typename NativeType>
  class native_object
  {
  public:

    using value_type = NativeType;
    using constructor_type = std::function<value_type*(api& stack)>;
    using method_type = std::function<bool(api& stack, value_type&)>;
    using getter_type = std::function<void(api& stack, value_type&)>; // not using const value_type& here, as the wrapped classes may not be const correct.
    using setter_type = std::function<void(api& stack, value_type&)>;
    using string = std::string;

  public:

    native_object() noexcept = default;
    native_object(const native_object&) = default;
    native_object(native_object&&) noexcept = default;
    native_object& operator=(const native_object&) = default;
    native_object& operator=(native_object&&) noexcept = default;
    ~native_object() noexcept = default;

    explicit native_object(std::string name) noexcept : name_(name),
                                               constructor_([](api&){return new value_type();}), // NOLINT: The raw pointer is passed to Duktape, memory management of these objects is handled there.
                                               methods_(), getters_(), setters_()
    {}

  public:

    /**
     * Assigned to all objects in the JS constructor function.
     * Shall free the native c++ instance when the garbage
     * collector cleans up the JS object.
     */
    static int finalizer_proxy(api::context_type ctx) noexcept
    {
      try {
        api stack(ctx);
        stack.get_prop_string(-1, "\xff_op");
        void* p = stack.get_pointer(-1);
        if(!p) return 0;
        delete static_cast<value_type*>(p); // NOLINT: Onwership passed from Duktape back.
      } catch(...) {
        // we're not allowed to throw, so if the object
        // does not have the right type we currently may
        // get a leak.
        // @todo -> move native objects in a c++ kv container
        //          with pointer as key, maybe wrapped in a
        //          std::any/uniqptr. Allows safe leakless cleanup.
        //          The engine needs to own that container.
      }
      return 0;
    }

    /**
     * Duktape C function proxy for getters. Checks the
     * JS object data integrity and accesses the native
     * instance pointer, passing it to the getter that
     * was created for that property name. Returns
     * `undefined` if no such property was defined.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int getter_proxy(api::context_type ctx)
    {
      api stack(ctx);
      try {
        stack.top(2);             // [target, key]
        string key = stack.get<std::string>(1);
        stack.top(1);             // [target]
        stack.get_prototype(0);   // [target proto]
        stack.get_prop_string(-1, key); // [target proto prop]
        if(stack.is_callable(-1)) return 1; // it's a method
        stack.top(1);             // [target]
        stack.get_prop_string(-1, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(accessor == nullptr) throw script_error(std::string("Native method not called with 'this' being a native object."));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        auto it = accessor->getters_.find(key);
        if(it == accessor->getters_.end()) {
          if((key.find("Symbol.toPrimitive") != key.npos) || (key.find("valueOf") != key.npos)) return 0;
          throw script_error(string("Native object does not have the property '") + key + "'");
        }
        stack.top(1);             // [target]
        stack.get_prop_string(-1, "\xff_op");
        value_type* ptr = static_cast<value_type*>(stack.get_pointer(-1, nullptr));
        stack.top(0);
        it->second(stack, *ptr);
        return (stack.top() > 0) ? 1 : 0;
      } catch(const engine_error&) {
        throw;
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

    /**
     * Duktape C function proxy for setters. Checks the
     * JS object data integrity and accesses the native
     * instance pointer, passing it to the setter that
     * was created for that property name. Throws
     * a JS exception if the property explicitly registered.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int setter_proxy(api::context_type ctx)
    {
      api stack(ctx);
      try {
        stack.top(3);             // [target key value]
        string key = stack.get<std::string>(1);
        stack.swap(1, 2);         // [target value key]
        stack.top(2);             // [target value]
        stack.swap(0, 1);         // [value target]
        stack.get_prop_string(-1, "\xff_op");
        value_type* ptr = static_cast<value_type*>(stack.get_pointer(-1, nullptr));
        stack.top(2);             // [target value]
        stack.get_prototype(-1);  // [value target proto]
        stack.get_prop_string(-1, key); // [value target proto prop]
        if(stack.is_callable(-1)) throw script_error(std::string("Native methods are not to be overwritten."));
        stack.top(2);             // [value target]
        stack.get_prop_string(-1, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(accessor == nullptr) throw script_error(std::string("Native setter not called with 'this' being a native object."));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        auto its = accessor->setters_.find(key);
        if(its == accessor->setters_.end()) {
          if(accessor->getters_.find(key) == accessor->getters_.end()) {
            throw script_error(string("Native object does not have the property '") + key + "'");
          } else {
            throw script_error(string("Native object property " + key + " is readonly."));
          }
        }
        stack.top(1);
        if(!ptr) throw script_error(string("Native setter: native object is missing."));
        its->second(stack, *ptr);
        return 0;
      } catch(const engine_error&) {
        throw;
      } catch(const script_error& e) {
        return stack.throw_exception(e.what());
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

    /**
     * Proxy trap for property deletion, which is not allowed.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int delprop_proxy(api::context_type ctx)
    { api(ctx).throw_exception("Properties of native objects cannot be deleted."); return 0; }

    /**
     * Duktape C function proxy trap for property existence.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int hasprop_proxy(api::context_type ctx)
    {
      api stack(ctx);
      try {
        stack.top(2);             // [target, key]
        string key = stack.get<std::string>(1);
        stack.get_prop_string(0, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(accessor == nullptr) throw script_error(std::string("Native method not called with 'this' being a native object."));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        stack.push(accessor->getters_.find(key) != accessor->getters_.end()); // intentionally not write-only, too.
        return 1;
      } catch(const engine_error&) {
        throw;
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

    /**
     * Duktape C function proxy trap for property listing
     * and enumeration.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int ownkeys_proxy(api::context_type ctx)
    {
      using namespace std;
      api stack(ctx);
      try {
        stack.get_prop_string(0, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(accessor == nullptr) throw script_error(std::string("Native method not called with 'this' being a native object."));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        auto v = vector<string>();
        for(const auto& e:accessor->getters_) v.push_back(e.first);
        stack.push(v);
        return 1;
      } catch(const engine_error&) {
        throw;
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

    /**
     * Constructs a JS object representing a native c++ instance, and
     * assigns metadata that are used for verivication and instance
     * access.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int constructor_proxy(api::context_type ctx)
    {
      using defflags = duktape::detail::defprop_flags;
      api stack(ctx);
      try {
        if(!stack.is_constructor_call()) throw script_error("Function has to be called as constructor (forgot new?)");
        auto top = stack.top();
        stack.push_current_function();
        stack.get_prop_string(-1, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        stack.top(top);
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        void* o = accessor->constructor_(stack);
        if(o == nullptr) throw script_error("Failed to create native object instance.");
        stack.top(0);
        stack.push_this();
        stack.push_string("\xff_op");
        stack.push_pointer(o);
        stack.def_prop(-3, defflags::convert(defflags::restricted));
        stack.push_string("\xff_accessor");
        stack.push_pointer(accessor);
        stack.def_prop(-3, defflags::convert(defflags::restricted));
        stack.push_c_function(finalizer_proxy, 1);
        stack.set_finalizer(-2);
        stack.freeze(-1);
        //-- proxy
        stack.push_bare_object();
        stack.push_string("deleteProperty");
        stack.push_c_function(delprop_proxy, 2);
        stack.def_prop(-3, defflags::convert(defflags::restricted));
        stack.push_string("has");
        stack.push_c_function(hasprop_proxy, 2);
        stack.def_prop(-3, defflags::convert(defflags::restricted));
        stack.push_string("ownKeys");
        stack.push_c_function(ownkeys_proxy, 1);
        stack.def_prop(-3, defflags::convert(defflags::restricted));
        stack.push_c_function(getter_proxy, 3);
        stack.put_prop_string(-2, "get");
        stack.push_c_function(setter_proxy, 4);
        stack.put_prop_string(-2, "set");
        stack.push_proxy();
        stack.seal(-1);
        stack.freeze(-1);
        return 1;
      } catch(const engine_error&) {
        throw;
      } catch(const script_error& e) {
        return stack.throw_exception(e.what());
      } catch(const std::exception& e) {
        throw engine_error(std::string("Fatal error constructing native object:") + e.what());
      }
      return 0;
    }

    /**
     * Returns JS type name and c++ (dynamic typeid) name of the object.
     * Can be overwritten with `method("toString", ...)` for more detailed
     * string data.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int default_tostring(api::context_type ctx)
    {
      api stack(ctx);
      try {
        stack.push_this();
        stack.get_prop_string(-1, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        stack.top(1);
        stack.get_prop_string(-1, "\xff_op");
        value_type* ptr = static_cast<value_type*>(stack.get_pointer(-1, nullptr));
        stack.top(0);
        if(!ptr) { stack.push("nullptr"); return 1; } // assumption: not my object or not initialised correctly.
        stack.push(((std::string("[") + accessor->name_ + " object (native: " + typeid(value_type).name()) + ")]"));
        return 1;
      } catch(const engine_error&) {
        throw;
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

    /**
     * Relay function for calling native c++ instance methods via JS.
     *
     * @param api::context_type ctx
     * @return int
     */
    static int method_proxy(api::context_type ctx)
    {
      api stack(ctx);
      try {
        api::index_type argtop = stack.top();
        stack.push_this();
        stack.get_prop_string(-1, "\xff_accessor");
        native_object* accessor = static_cast<native_object*>(stack.get_pointer(-1, nullptr));
        if(accessor == nullptr) throw script_error(std::string("Native method not called with 'this' being a native object."));
        if(instance_.get() != accessor) throw engine_error("Inconsistent native object properties.");
        stack.top(argtop+1);
        stack.get_prop_string(-1, "\xff_op");
        value_type* ptr = static_cast<value_type*>(stack.get_pointer(-1, nullptr));
        if(!ptr) throw script_error("Native object missing.");
        stack.top(argtop);
        stack.push_current_function();
        stack.get_prop_string(-1, "\xff_mp");
        int method_index = stack.get<int>(-1);
        stack.top(argtop);
        if((method_index < 0) || (api::size_type(method_index) >= accessor->methods_.size())) throw engine_error("Inconsistent native object properties (unregistered method).");
        return std::get<1>(accessor->methods_[method_index])(stack, *ptr);
      } catch(const engine_error&) {
        throw;
      } catch(const std::exception& e) {
        return stack.throw_exception(e.what());
      }
      return 0;
    }

  public:

    const std::string& name() const noexcept
    { return name_; }

    /**
     * Used to define a `new` c++ instance depending on the
     * arguments passed as duktape stack. Must be used exactly
     * once for a `native_object`.
     *
     * @param constructor_type ctor
     * @return native_object&
     */
    native_object& constructor(constructor_type ctor)
    { constructor_ = ctor; return *this; }

    /**
     * Registers a wrapper method in the JS engine for a method
     * of the c++ instance. Can return values, and receives the
     * parameters via the duktape stack for flexibility. Can optionally
     * throw a `script_error()` exception if the given values are not
     * acceptable.
     *
     * @param std::string name
     * @param method_type fn
     * @param int nargs
     * @return native_object&
     */
    native_object& method(std::string name, method_type fn, int nargs=DUK_VARARGS)
    { methods_.push_back(make_tuple(std::move(name), std::move(fn), nargs)); return *this; }

    /**
     * Getter relay for a JS property. The given getter may directly return public c++ instance
     * variables, function or method results, whatever needed. Push the result on top of the
     * duktape stack.
     *
     * @param std::string name
     * @param getter_type fn
     * @return native_object&
     */
    native_object& getter(std::string name, getter_type fn)
    { getters_.insert(std::make_pair(std::move(name), std::move(fn))); return *this; }

    /**
     * Setter relay for a JS property. The given setter function can retrieve the value to
     * set from the top of the duktape stack perform a setting action in the c++ instance,
     * or optionally throw a `script_error()` exception if the given value is not acceptable.
     *
     * @param name
     * @param fn
     * @return
     */
    native_object& setter(std::string name, setter_type fn)
    { setters_.insert(std::make_pair(std::move(name), std::move(fn))); return *this; }

  public:

    /**
     * Registers the current `native_object` in the JS engine.
     *
     * @param engine& js
     * @return native_object&
     */
    template <typename M, bool S>
    native_object& define_in(detail::basic_engine<M,S>& js, bool sealed=false)
    {
      using defflags = duktape::detail::defprop_flags;
      instance_ = std::make_unique<native_object>(*this);
      api& stack = js.stack();
      constexpr defflags::type acf = defflags::restricted;
      constexpr defflags::type ace = defflags::enumerable;
      std::string name = name_;
      name = js.define_base(name, ace);               // [... parent]
      stack.push_string(name);                        // [... parent classname]
      stack.push_c_function(constructor_proxy);       // [... parent classname ctor]
      stack.def_prop(-3, defflags::convert(acf));     // [... parent]
      stack.top(0);                                   // []
      stack.select(name_);                            // [ctor]
      stack.push_string("\xff_accessor");             // [ctor acc_key]
      stack.push_pointer(instance_.get());            // [ctor acc_key acc_ptr]
      stack.def_prop(-3, defflags::convert(acf));     // [ctor]
      stack.push_string("prototype");                 // [ctor proto_key]
      stack.push_bare_object();                       // [ctor proto_key proto]
      stack.def_prop(-3, defflags::convert(acf));     // [ctor]
      stack.freeze(-1);                               // [ctor]
      stack.top(0);                                   // []
      //--
      stack.select(name_ + ".prototype");             // [... proto]
      stack.swap_top(0);                              // [proto ...]
      stack.top(1);                                   // [proto]
      stack.push_string("toString");                  // [proto key]
      stack.push_c_function(default_tostring, 0);     // [proto key fn]
      stack.def_prop(-3, defflags::convert(defflags::defaults)); // [proto]
      //--
      int i=0;
      for(const auto& method:methods_) {
        stack.top(1);                                 // [proto]
        stack.push_string(std::get<0>(method));       // [proto fnname]
        stack.push_c_function(method_proxy, std::get<2>(method)); // [proto fnname fn]
        stack.push_string("\xff_mp");                 // [proto fnname fn key]
        stack.push(i);                                //
        stack.def_prop(-3, defflags::convert(acf));   // [proto fnname fn]
        stack.def_prop(-3, defflags::convert(acf));   // [proto]
        ++i;
      }
      if(sealed) {
        stack.top(1);                                 // [proto]
        stack.freeze(-1);                             // [proto]
      }
      stack.top(0);                                   // []
      return *this;
    }

  private:

    const std::string name_;
    constructor_type constructor_;
    std::vector<std::tuple<std::string, method_type, int>> methods_;
    std::unordered_map<std::string, getter_type> getters_;
    std::unordered_map<std::string, setter_type> setters_;
    static std::unique_ptr<native_object> instance_; // NOLINT: Private lazy initialized singleton.
  };

  template <typename T>
  std::unique_ptr<native_object<T>> native_object<T>::instance_ = nullptr; // NOLINT: Private lazy singleton.

}

/**
 * Main "engine", corresponds to a destinct Duktape context.
 */
namespace duktape { namespace detail {

  /**
   * This class represents a "duktape" engine with its own heap.
   * Engines are independent from another.
   *
   * The allocated heap is freed during destruction.
   */
  template <typename MutexType, bool StrictInclude>
  class basic_engine
  {
  public:

    using api_type = ::duktape::api;
    using stack_guard_type = ::duktape::stack_guard;
    using lock_guard_type = std::lock_guard<MutexType>;
    using defflags = defprop_flags;
    using engine_lock_type = detail::basic_engine_lock<MutexType,StrictInclude>;
    friend class basic_engine_lock<MutexType,StrictInclude>;

  public:

    /**
     * c' tor
     */
    explicit basic_engine() : stack_(), define_flags_(defflags::defaults), mutex_()
    { clear(); }

    /**
     * d' tor
     */
    virtual ~basic_engine() noexcept
    { if(ctx()) ::duk_destroy_heap(ctx()); }

    basic_engine(const basic_engine&) = delete;
    basic_engine(basic_engine&&) noexcept = delete;
    basic_engine& operator=(const basic_engine&) = delete;
    basic_engine& operator=(basic_engine&&) noexcept = delete;

  public:

    /**
     * Returns the `duk_context*` of this engine.
     * @return api_type&
     */
    api_type& stack() noexcept
    { return stack_; }

    /**
     * Returns the `duk_context*` of this engine.
     * @return typename api_type::context_type
     */
    typename api_type::context_type ctx() const noexcept
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

  public:

    #if(0 && JSDOC)
    /**
     * Reference to the global object, mainly useful
     * for accessing in strict mode ('use strict';).
     * @var {object}
     */
    var global = {};
    #endif
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
      #ifdef WITH_DUKTAPE_OBJECT_EXTENSIONS
        // Remove some Duktape methods which may not be intended to be
        // available and unknown to the programmer.
        undef("Duktape.env");
        undef("Duktape.Pointer");
        undef("Duktape.info");
        undef("Duktape.errCreate");
        undef("Duktape.errThrow");
      #endif
      // Global object (in case DUK_USE_GLOBAL_BINDING not set)
      stack().push_global_object();
      stack().push_string("global");
      stack().push_global_object();
      stack().def_prop(0, DUK_DEFPROP_HAVE_VALUE|DUK_DEFPROP_HAVE_WRITABLE|DUK_DEFPROP_WRITABLE|DUK_DEFPROP_HAVE_ENUMERABLE|DUK_DEFPROP_HAVE_CONFIGURABLE|DUK_DEFPROP_CONFIGURABLE);
      stack().top(0);
      stack().gc();
    }

    /**
     * Includes a file, throws a `duktape::script_error` on fail.
     * @param const char* path
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false>
    ReturnType include(const char* path, bool use_strict=StrictInclude)
    { return include<ReturnType, StrictReturn>(std::string(path), use_strict); }

    /**
     * Includes a file, throws a `duktape::script_error` on fail.
     * @param std::string path
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false>
    ReturnType include(std::string path, bool use_strict=StrictInclude)
    {
      std::ifstream is;
      is.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
      std::string code((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
      if(!is) throw script_error(std::string("Failed to read include file '") + path + "'");
      is.close();
      return eval<ReturnType, StrictReturn>(std::move(code), path, use_strict);
    }

    /**
     * Includes a file, throws a `duktape::script_error` on fail. Optionally a file name
     * can be specified for tracing purposes in the JS engine.
     * @param const char* code
     * @param const char* file=nullptr
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false>
    ReturnType eval(const char* code, const char* file=nullptr, bool use_strict=false)
    { return eval<ReturnType,StrictReturn>(std::string(code), std::string(file ? file : "(eval)"), use_strict); }

    /**
     * Evaluate code given as string, throws a `duktape::script_error` on fail. Optionally a
     * file name can be specified for tracing purposes in the JS engine.
     * @param const std::string& code
     * @param std::string file="(eval)"
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false>
    ReturnType eval(const std::string& code, std::string file="(eval)", bool use_strict=false)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx(), true);
      stack().require_stack(2);
      stack().push_string(code);
      stack().push_string(file);
      auto ok = bool();
      try {
        ok = (stack().eval_raw(0, 0, DUK_COMPILE_EVAL | DUK_COMPILE_SAFE | DUK_COMPILE_SHEBANG | (use_strict ? DUK_COMPILE_STRICT : 0)) == 0);
      } catch(const exit_exception&) {
        stack().clear();
        throw;
      } catch(const engine_error&) {
        stack().clear();
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
      } else if(!StrictReturn) {
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

    /**
     * Call a function, fetch the (strict) return value.
     * @param std::string funct
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false, typename ...Args>
    ReturnType call(const char* funct, Args ...args)
    { if(!funct) throw engine_error("BUG: engine::call(nullptr, ...)");
      return call<ReturnType, StrictReturn, Args...>(std::string(funct), args...);
    }

    /**
     * Call a function, fetch the (strict) return value.
     * @param std::string funct
     * @return typename ReturnType
     */
    template <typename ReturnType=void, bool StrictReturn=false, typename ...Args>
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
        stack().clear(); // to invoke already possible finalisations before next call stack frame.
        throw;
      } catch(const engine_error&) {
        stack().clear();
        throw;
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
      } else if(!StrictReturn) {
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
    { stack_guard_type sg(ctx()); define_r(name, define_flags_); }

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
      stack().def_prop(-3, defflags::convert(define_flags_));
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
      define_function_proxy(name, fn, nargs);
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
      stack().def_prop(-3, defflags::convert(define_flags_));
      stack().get_prop_string(-1, name);
      stack().push_string("\xff_fp");
      stack().push_pointer(fn);
      stack().def_prop(-3, defflags::convert(define_flags_));
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
      stack().def_prop(-3, defflags::convert(define_flags_));
    }

    /**
     * Registers native object in this engine.
     *
     * @param native_object<NativeClassType> registrar
     */
    template <typename NativeClassType>
    void define(native_object<NativeClassType>& registrar)
    {
      lock_guard_type lck(mutex_);
      stack_guard_type sg(ctx());
      registrar.define_in(*this);
    }

    /**
     * Recursively defines empty parent objects of the given (canonical) name
     * and returns the object key (which is the last part of the given name).
     *
     * @param std::string name
     * @param defflags::type flags
     * @return std::string name
     */
    std::string define_base(std::string name, defflags::type flags)
    {
      std::string base, tail;
      if(!aux<>::split_selector(name, base, tail)) {
        throw engine_error(std::string("Invalid selector: '") + name + "'");
      }
      define_r(base, flags);
      return tail;
    }

  private:

    /**
     * Recursively defines empty parent objects of the given (canonical) name
     * and returns the object key (which is the last part of the given name).
     * Private function because it depends on the internal state of the engine,
     * meaning the `define_flags_`.
     *
     * @param std::string name
     * @return std::string name
     */
    std::string define_base(std::string name)
    { return define_base(name, define_flags_); }

    /**
     * Defines empty objects recursively from a canonical name.
     *
     * @param std::string name
     */
    void define_r(std::string name, defflags::type flags)
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
              stack().def_prop(-3, defflags::convert(flags));
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
          stack().def_prop(-3, defflags::convert(flags));
          stack().get_prop_string(-1, s);
        } else {
          stack().get_prop_string(-1, s);
        }
      }
    }

    /**
     * Registers a native function in the engine context,
     * without thread locking and stack reset.
     *
     * @param std::string name
     * @param native_function_type fn
     * @param int nargs
     */
    void define_function_proxy(std::string name, native_function_type fn, int nargs)
    {
      stack().push_string(name);
      stack().push_c_function(function_proxy<int, api_type&>::func, nargs >= 0 ? nargs : DUK_VARARGS);
      stack().def_prop(-3, defflags::convert(define_flags_));
      stack().get_prop_string(-1, name);
      stack().push_string("\xff_fp");
      stack().push_pointer(fn);
      stack().def_prop(-3, defflags::convert(define_flags_));
    }

  private:

    api_type stack_;
    typename defflags::type define_flags_;
    MutexType mutex_;
  };
}}

#endif
