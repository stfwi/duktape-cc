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
#ifndef DUKTAPE_MOD_EXT_SYSINFO_HH
#define DUKTAPE_MOD_EXT_SYSINFO_HH


/* (FROM sw::os::win32::adapters::usb::*) */
#ifndef SW_USB_DEVICES_HH
  #define SW_USB_DEVICES_HH
  #if defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
    #ifndef __WINDOWS__
      #define __WINDOWS__
    #endif
    #include <windows.h>
    #include <cfgmgr32.h>
    #include <setupapi.h>
    #include <usb100.h>
    #if defined(__MINGW32__) || defined(__MINGW64__)
      #include <stdint.h>
    #else
      #error "no idea which stdint.h to use."
    #endif
  #else
    #include <stdint.h>
    #include <cstring>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <ctype.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <time.h>
    #ifdef __linux__
      #include <sys/file.h>
      #include <sys/stat.h>
      #include <sys/poll.h>
      #include <dirent.h>
    #endif
  #endif
  #include <tuple>
  #include <string>
  #include <regex>
  #include <vector>
  #include <sstream>
  #include <iostream>


  namespace sw { namespace sys { namespace detail {

  template <typename=void>
  class basic_usb_device_list
  {
  public:

    using string_type = std::string;

    struct device_list_element {
      string_type path;
      string_type description;
      string_type pid;
      string_type vid;
      string_type port_name;
    };

    using device_list_type = std::vector<device_list_element>;

  public:

    static device_list_type lsusb()
    {
      #ifdef __WINDOWS__
      return usb_device_list();
      #endif
      #ifdef __linux__
      return device_list_type(); // -> not yet implemented, -> sysfs.
      #endif
    }

  private:

    #ifdef __WINDOWS__

    struct hkey_guard
    {
      hkey_guard() = delete;
      hkey_guard(const hkey_guard&) = delete;
      hkey_guard(hkey_guard&&) = default;
      hkey_guard& operator=(const hkey_guard&) = delete;
      hkey_guard& operator=(hkey_guard&&) = default;
      explicit hkey_guard(HKEY key) : hkey(key) {}
      ~hkey_guard() noexcept { if(valid()) ::RegCloseKey(hkey); }
      bool valid() const noexcept { return (hkey != INVALID_HANDLE_VALUE); }
      ::HKEY hkey;
    };

    struct di_guard
    {
      ::HDEVINFO hnd;
      ~di_guard() noexcept { if(hnd && (hnd!=INVALID_HANDLE_VALUE)) {::SetupDiDestroyDeviceInfoList(hnd); }}
      di_guard(::HDEVINFO h) noexcept : hnd(h) {}
      di_guard() = delete;
      di_guard(const di_guard&) = delete;
      di_guard(di_guard&&) = default;
      di_guard& operator=(const di_guard&) = delete;
      di_guard& operator=(di_guard&&) = default;
    };

    static device_list_type usb_device_list()
    {
      using namespace std;
      auto devices = device_list_type();
      auto di = di_guard(::SetupDiGetClassDevsA(nullptr, "USB", nullptr, DIGCF_PRESENT|DIGCF_ALLCLASSES));
      if(di.hnd == (INVALID_HANDLE_VALUE)) return devices;
      const auto re_pidvid = std::regex("VID_([0-9A-F]+)&PID_([0-9A-F]+)", std::regex::ECMAScript|std::regex::icase|std::regex::optimize);
      for(size_t i=0;; ++i) {
        // CONFIGRET r;
        auto item = device_list_element();
        auto did = ::SP_DEVINFO_DATA();
        did.cbSize = sizeof(did);
        if(!::SetupDiEnumDeviceInfo(di.hnd, i, &did)) { break; }
        // Device identifier
        {
          char devid[MAX_DEVICE_ID_LEN+1];
          if(::CM_Get_Device_IDA(did.DevInst, devid, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) continue;
          devid[MAX_DEVICE_ID_LEN] = '\0';
          item.path = devid;
          std::smatch matches;
          if(std::regex_search(item.path, matches, re_pidvid)) {
            item.vid = matches[1].str();
            item.pid = matches[2].str();
          }
        }
        // Description
        {
          DWORD size, reg_datatype;
          char desc[1024];
          ::SetupDiGetDeviceRegistryPropertyA(di.hnd, &did, SPDRP_DEVICEDESC, &reg_datatype, (PBYTE)desc, sizeof(desc), &size);
          if((!desc[0]) || (!size) || (size>=sizeof(desc))) continue;
          desc[size] = '\0';
          item.description = desc;
        }
        // Old COM name
        {
          const auto reg_key = hkey_guard(::SetupDiOpenDevRegKey(di.hnd, &did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
          if(reg_key.valid()) {
            char data[256];
            auto data_size = DWORD(sizeof(data)-1);
            auto type_data = DWORD(0);
            if((::RegQueryValueExA(reg_key.hkey, "PortName", nullptr, &type_data, reinterpret_cast<LPBYTE>(data), &data_size) == ERROR_SUCCESS) && (data_size < sizeof(data))) {
              data[data_size] = '\0';
              item.port_name = data;
            }
          }
        }
        devices.push_back(std::move(item));
      }
      return devices;
    }

    #endif
  };
  }}}

  namespace sw { namespace sys {
    using usb_device_list = detail::basic_usb_device_list<>;
  }}
#endif



#include <duktape/duktape.hh>
#include <algorithm>
#include <optional>
#include <chrono>

#pragma GCC diagnostic push


namespace duktape { namespace mod { namespace ext { namespace usb {

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
