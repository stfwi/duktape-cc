/**
 * @file mod.ext.sysinfo.hh
 * @package de.atwillys.cc.duktape.ext
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++17 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 * @requires WIN32 LDFLAGS -lsetupapi
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional binding to the system information functionality.
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
#ifndef DUKTAPE_MOD_EXT_SYSINFO_HH
#define DUKTAPE_MOD_EXT_SYSINFO_HH
#include <duktape/duktape.hh>
#include "./usb_devices.hh"
#include <algorithm>
#include <optional>
#include <chrono>

#pragma GCC diagnostic push


namespace duktape { namespace mod { namespace ext { namespace sysinfo {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * Returns true if library code was evaluated.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace std;
    using namespace sw;

    js.define("sys.lsusb", [](duktape::api& stack) {
      stack.top(0);
      stack.check_stack(3);
      stack.push_array();
      int index = 0;
      const auto devices = sw::sys::usb_device_list::lsusb();
      for(const auto& device:devices) {
        stack.push_bare_object();
        stack.set("path", device.path);
        if(!device.vid.empty()) stack.set("vid", device.vid);
        if(!device.pid.empty()) stack.set("pid", device.pid);
        if(!device.description.empty()) stack.set("name", device.description);
        if(!device.port_name.empty()) stack.set("port", device.port_name);
        stack.put_prop_index(-2, index++);
      }
      return 1;
    });
  }

}}}}

#pragma GCC diagnostic pop
#endif
