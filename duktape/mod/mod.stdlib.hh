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
#ifndef DUKTAPE_STDLIB_HH
#define DUKTAPE_STDLIB_HH

#include "../duktape.hh"
#include <iostream>
#include <streambuf>
#include <sstream>
#include <regex>

namespace duktape { namespace mod { namespace stdlib { namespace detail { namespace {

  int exit_js(api& stack)
  { throw exit_exception((stack.top() <= 0) ? 0 : stack.to<int>(-1)); return 0; }

  int include_file(api& stack)
  {
    std::string path = stack.get_string(0);
    std::ifstream is;
    is.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
    std::string code((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    if(!is) return stack.throw_exception(std::string("Failed to read include file '") + path + "'");
    is.close();
    if(!std::regex_search(path, std::regex("\\.json$", std::regex_constants::icase|std::regex_constants::nosubs|std::regex_constants::ECMAScript))) {
      stack.top(0);
      stack.require_stack(3);
      stack.push_string(code);
      stack.push_string(path);
      try {
        stack.eval_raw(0, 0, DUK_COMPILE_EVAL | DUK_COMPILE_SHEBANG);
        return 1;
      } catch(const exit_exception& e) {
        stack.top(0);
        stack.gc();
        throw;
      }
    } else {
      stack.top(0);
      if(code.empty()) return 0; // File is there but empty. Content undefined.
      stack.require_stack(3);
      stack.get_global_string("JSON");
      stack.push("parse");
      stack.push_string(code);
      if(stack.pcall_prop(0, 1)==0) return 1;
      if(!stack.is_error(-1)) {
        stack.top(0);
        return stack.throw_exception(std::string("JSON parse error in '") + path + "'.");
      } else {
        const auto message = stack.to<std::string>(-1);
        stack.top(0);
        return stack.throw_exception(message + " (file '" + path + "')");
      }
      return 0;
    }
  }

}}}}}


namespace duktape { namespace mod { namespace stdlib {

  /**
   * Define stdlib functions in the standard locations.
   *
   * @param duktape::engine& js
   */
  void define_in(duktape::engine& js)
  {
    #if(0 && JSDOC)
    /**
     * Exits the script interpreter with a specified exit code.
     *
     * @param {number} status_code
     */
    exit = function(status_code) {};
    #endif
    js.define("exit", detail::exit_js);

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
    js.define("include", detail::include_file);
  }

  #if(0 && JSDOC)
  /**
   * Contains the environment variables of the application
   * as key-value (string->string) plain object. If env is
   * not explicitly enabled in the binary application, the
   * object is undefined.
   *
   * @var {object} sys.env
   */
  sys.env = {};
  #endif
  void define_env(duktape::engine& js, const char** envv)
  {
    using namespace std;
    if(!envv) return;
    js.define("sys.env");
    auto& stack = js.stack();
    duktape::stack_guard sg(stack, true);
    stack.select("sys.env");
    for(int i=0; envv[i]; ++i) {
      const auto e = string(envv[i]);
      const auto pos = e.find('=');
      if((pos != e.npos) && (pos > 0)) {
        string key = e.substr(0, pos);
        string val = e.substr(pos+1);
        stack.set(move(key), move(val));
      }
    }
  }

}}}

#endif
