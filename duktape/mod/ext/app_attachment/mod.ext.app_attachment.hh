/**
 * @file duktape/mod/ext/mod.ext.app_attachment.hh
 * @package de.atwillys.cc.duktape.ext
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++14
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++14 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional CLI application attachment library reader.
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
#ifndef DUKTAPE_MOD_EXT_APP_ATTACHMENT_HH
#define DUKTAPE_MOD_EXT_APP_ATTACHMENT_HH
#include <duktape/mod/mod.sys.hh>
#include <duktape/mod/mod.fs.hh>
#include "../../../duktape.hh"
#include "./app_attachment.hh"
#include <ctype.h>
#include <regex>
#pragma GCC diagnostic push


namespace duktape { namespace mod { namespace ext { namespace app_attachment {

  template <typename=void>
  static std::string read_attachment()
  {
    using namespace std;
    auto attachment = sw::util::read_executable_attachment<string>();
    if(attachment.empty()) return attachment;
    return attachment;
  }

  template <typename=void>
  static void write_attachment(std::string new_executable_file_path, std::string data)
  {
    using namespace std;
    switch(sw::util::write_executable_attachment(new_executable_file_path, data)) {
      case -1: throw duktape::script_error(string("Failed to determine own size"));
      case -2: throw duktape::script_error(string("Output file already exists: '") + new_executable_file_path + "'");
      case -3: throw duktape::script_error(string("Failed to open self"));
      case -4: throw duktape::script_error(string("Failed to write output file '") + new_executable_file_path + "'");
      case -5: throw duktape::script_error(string("Not all bytes written to output file '") + new_executable_file_path + "'");
      default: break;
    }
  }

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * Returns true if library code was evaluated.
   * @param duktape::engine& js
   * @return bool
   */
  template <typename=void>
  static bool define_in(duktape::engine& js)
  {
    using namespace std;
    js.define("sys.app.attachment.write", write_attachment);
    js.define("sys.app.attachment.read", read_attachment);
    auto attachment = read_attachment();
    if(attachment.empty()) return false;
    // Shall run with c++14 and minimum dependencies, so the lambdas represent `to_lower(std::filesystem::stem(path))`.
    constexpr auto filen = [](string s) { const char *p = basename(&s.front()); return (!p) ? string() : string(p); };
    constexpr auto noext = [](string s) { const auto p = s.find_last_of("."); if((p != s.npos) && (p > 1) && (p < s.size()-1)) {s.resize(p);} return s; };
    constexpr auto to_lower = [](string s) { transform(s.begin(), s.end(), s.begin(), ::tolower); return s; };
    const auto appname_ref = to_lower(noext(js.eval<string>("sys.app.name")));
    const auto appname_act = to_lower(noext(filen(duktape::detail::system::application_path())));
    // Basic protection against unintended library code. The program has to be at least renamed to allow library code execution.
    if(appname_ref == appname_act) {
      throw duktape::script_error(string("Applications with library attachment cannot be named '") + appname_act + "'.");
    }
    if((attachment.find("#!/") == 0) && ( (attachment.find(appname_ref) != attachment.npos) || (attachment.find(appname_act) != attachment.npos) )) {
      js.eval<void>(attachment, "(library code)");
      js.define("sys.app.attachment.included", true);
      return true;
    }
    return false;
  }

}}}}

#pragma GCC diagnostic pop
#endif
