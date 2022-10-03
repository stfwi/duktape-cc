/**
 * @file duktape/mod/mod.resource_import.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++11
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++11 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional resource import feature.
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
#ifndef DUKTAPE_MOD_RESOURCE_IMPORT_HH
#define DUKTAPE_MOD_RESOURCE_IMPORT_HH
#include "../../duktape.hh"
#include "../mod.sys.os.hh"
#include "../mod.sys.hash.hh"
#include "../mod.fs.hh"
#include <cstdint>
#include <string>
#include <array>

namespace duktape { namespace detail { namespace resource_blob { namespace {

  template <typename=void> static const std::string bstr() noexcept { return std::string(); }
  template <typename=void> static const std::string bstr(const char* s) noexcept { return std::string(s); }
  template <typename=void> static const std::string bstr(long long n) noexcept { return std::to_string(n); }

  template <size_t N>
  const std::array<uint8_t, N>& binseed() noexcept
  {
    using namespace std;
    static auto seed = array<uint8_t, N>();
    static bool seed_set = false;
    if(seed_set) return seed;

    const auto s = bstr(
      #ifdef WITH_RESOURCE_IMPORT
        WITH_RESOURCE_IMPORT
      #endif
    );
    auto k = uint32_t(0x8280|N);
    const auto slen = s.length();
    for(size_t i=0; i<N; ++i) {
      k = ((k*7)>>2) + ((!slen) ? (0) : (s[i % slen]));
      seed[i] = uint8_t(k & 0xff);
    }
    seed_set = true;
    return seed;
  }

  template<typename Container>
  void bin_conv(Container& data)
  {
    const auto& bin = binseed<64>();
    auto k = size_t(0);
    for(auto& e:data) { e ^= bin[(++k) & (63)]; }
  }

  template<typename PathAccessor>
  inline int load_resource(duktape::api& stack)
  {
    using namespace std;
    if((stack.top()<1) || (!stack.is<string>(0))) return stack.throw_exception("No resource path given.");
    const auto path = PathAccessor::to_sys(stack.to<string>(0));
    stack.top(0);
    string contents;
    {
      ifstream fis(path.c_str(), ios::in|ios::binary);
      if(!fis.good()) { stack.push(string()); return 1; } // Return empty if the file does not exist.
      contents.assign((istreambuf_iterator<char>(fis)), istreambuf_iterator<char>());
      if((!fis.good() && !fis.eof()) || (contents.empty())) return stack.throw_exception(string("Failed to load resource '") + path + "'.");
      bin_conv(contents);
    }
    const auto integrity_error = [&](){
      stack.top(0);
      return stack.throw_exception(string("Inconsistent resource file: '") + path + "'.");
    };
    const auto pop = [&](){ const auto b=uint8_t(contents.back()); contents.pop_back(); return b; };
    if(contents.size() < (sizeof(uint32_t)+1)) return integrity_error();
    uint32_t crc = 0;
    for(size_t i=0; i<sizeof(uint32_t); ++i) { crc <<= 8u; crc |= pop(); }
    if(::sw::crc32::calculate(contents.data(), contents.size()) != crc) return integrity_error();
    const auto type = pop();
    switch(type) {
      case 's': {
        stack.push(contents);
        return 1;
      }
      case 'b': {
        char* buffer = reinterpret_cast<char*>(stack.push_array_buffer(contents.length(), true));
        if(!buffer) return stack.throw_exception("Out of memory for resource loading buffer allocation.");
        if(contents.length() > 0) std::copy(contents.begin(), contents.end(), buffer);
        return 1;
      }
      case 'j': {
        stack.push(contents);
        stack.json_decode(0);
        return (stack.is_error(0)) ? integrity_error() : 1;
      }
      default: {
        return integrity_error();
      }
    }
  }

  template<typename PathAccessor>
  inline int save_resource(duktape::api& stack)
  {
    using namespace std;
    if((stack.top()<1) || (!stack.is<string>(0))) return stack.throw_exception("No resource path given.");
    if(stack.top()<2) return stack.throw_exception("No resource data given to save.");
    if(stack.top()>2) return stack.throw_exception("Too many arguments.");
    const auto path = PathAccessor::to_sys(stack.to<string>(0));
    string contents;
    if(stack.is_buffer(1)) {
      contents = stack.buffer<string>(1);
      contents.push_back('b');
    } else if(stack.is<string>(1)) {
      contents = stack.get<string>(1);
      contents.push_back('s');
    } else {
      contents = stack.json_encode(1);
      contents.push_back('j');
    }
    stack.top(0);
    uint32_t crc = ::sw::crc32::calculate(contents.data(), contents.size());
    for(size_t i=0; i<sizeof(uint32_t); ++i) { contents.push_back(static_cast<char>(crc & 0xff)); crc >>= 8u; }
    bin_conv(contents);
    bool ok = true;
    try {
      ofstream fos(path.c_str(), ios::out|ios::binary);
      if(fos.good()) fos.write(contents.data(), contents.size());
      ok = fos.good();
      fos.close();
    } catch(const exception& e) {
      ok = false;
    }
    if(!ok) {
      return stack.throw_exception(string("Failed to save resource data to '") + path + "'");
    } else {
      stack.push(true);
      return 1;
    }
  }

}}}}


namespace duktape { namespace mod { namespace ext { namespace resource_blob { namespace {

  template <typename PathAccessor=::duktape::detail::filesystem::path_accessor<std::string>>
  void define_in(duktape::engine& js)
  {
      #if(0 && JSDOC)
      /**
       * Loads and returns the contents of a binary encoded
       * resource file (saved with `sys.resource.save()`).
       *
       * @param {string} path
       * @return {any}
       */
      sys.resource.load = function(path) {};
      #endif
      js.define("sys.resource.load", duktape::detail::resource_blob::load_resource<PathAccessor>);

      #if(0 && JSDOC)
      /**
       * Saves data to a binary encoded resource file, throws
       * on error. The data can be a buffer, string or any JSON
       * serializable object.
       *
       * @param {string} path
       * @param {any} data
       */
      sys.resource.load = function(path, data) {};
      #endif
      js.define("sys.resource.save", duktape::detail::resource_blob::save_resource<PathAccessor>);
  }

}}}}}

#endif
