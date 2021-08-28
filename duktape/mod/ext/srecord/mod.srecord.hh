/**
 * @file mod.srecord.hh
 * @package de.atwillys.cc.duktape.ext
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++17 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional binding to the `srecord-cc` library.
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
#ifndef DUKTAPE_MOD_SRECORD_HH
#define DUKTAPE_MOD_SRECORD_HH
#include <duktape/duktape.hh>
#include <sw/srecord.hh>
#include <sw/crc.hh>
#include <sw/sha1.hh>
#include <sw/sha512.hh>
#include <string>
#include <cstdint>


namespace duktape { namespace detail { namespace srecord { namespace {

  using native_srecord = sw::srecord;

  template <typename T>
  inline std::string to_hex_string(T t)
  {
    using namespace std;
    ostringstream os; os << "0x" << hex << setfill('0') << setw(std::min(sizeof(T)*2, size_t(8))) << t;
    return os.str();
  }

  template <typename T>
  inline std::string hex_byte(T val)
  {
    const auto n1 = int((val>>4) & 0x0f);
    const auto n0 = int((val>>0) & 0x0f);
    return std::string{ char((n1<10) ? ('0'+n1) : ('a'-10+n1)), char((n0<10) ? ('0'+n0) : ('a'-10+n0)) };
  }

  template <typename T, native_srecord::endianess_type E>
  bool js_method_get_integral_value(duktape::api& stack, native_srecord& instance)
  {
    if((stack.top()!=1) || (!stack.is<native_srecord::address_type>(0))) throw duktape::script_error("SRecord address needed as argument, and it must be integral.");
    const auto adr = stack.get<native_srecord::address_type>(0);
    const auto val = instance.get<T>(adr, E);
    if(!val.has_value()) throw duktape::script_error(std::string("SRecord failed to read integral data from address ") + to_hex_string(adr) + ".");
    stack.push(val.value());
    return true;
  }

  template <typename T, native_srecord::endianess_type E>
  bool js_method_set_integral_value(duktape::api& stack, native_srecord& instance)
  {
    if((stack.top()!=2) || (!stack.is<native_srecord::address_type>(0))) throw duktape::script_error("SRecord address and integral value needed as arguments.");
    if(!stack.is<T>(1)) throw duktape::script_error("SRecord value (2nd arg) not a number or out of range.");
    const auto adr = stack.get<native_srecord::address_type>(0);
    const auto val = stack.get<T>(1);
    instance.set(adr, E, val);
    if(instance.error()) throw duktape::script_error(std::string("SRecord failed to set data adr: ") + to_hex_string(adr) + "=" + std::to_string(val) + ".");
    stack.pop();
    stack.push_this();
    return true;
  }

  template <typename=void>
  bool js_method_cksum(duktape::api& stack, native_srecord& instance)
  {
    if((stack.top()!=3) || (!stack.is<native_srecord::address_type>(0))) throw duktape::script_error("SRecord cksum: start address, size, and checksum name needed (crc32,crc32-ccitt,sha1,...).");
    if(!stack.is<native_srecord::address_type>(0)) throw duktape::script_error("SRecord address (1nd arg) not a number or out of range.");
    if(!stack.is<native_srecord::size_type>(1)) throw duktape::script_error("SRecord size (2nd arg) not a number or out of range.");
    if(!stack.is<std::string>(2)) throw duktape::script_error("SRecord crc name not a string.");
    const auto adr = stack.get<native_srecord::address_type>(0);
    const auto size = stack.get<native_srecord::size_type>(1);
    const auto crc_type = stack.get<std::string>(2);
    const auto data = instance.get_range(adr, adr+size);
    if((crc_type == "crc32") || (crc_type == "crc32-ccitt")) {
      stack.push(sw::crc32::calculate(data.bytes().data(), data.bytes().size()));
      return true;
    } else if(crc_type == "crc16") {
      stack.push(sw::crc16::calculate(data.bytes().data(), data.bytes().size()));
    } else if(crc_type == "sha1") {
      stack.push(sw::sha1::calculate(data.bytes().data(), data.bytes().size()));
    } else if(crc_type == "crc8") {
      stack.push(sw::crc8(data.bytes().data(), data.bytes().size()));
    } else if(crc_type == "sha512") {
      stack.push(sw::sha512::calculate(data.bytes().data(), data.bytes().size()));
    } else {
      throw duktape::script_error(std::string("Unknown CRC type '") + crc_type + "', allowed are crc32, crc32-ccitt, crc16, crc8, sha1, sha512");
    }
    stack.pop();
    stack.push_this();
    return true;
  }

  template <typename=void>
  std::vector<uint8_t> range_data(duktape::api& stack, native_srecord& instance)
  {
    if((stack.top()<2) || (stack.top()>3)) throw duktape::script_error("range getter needs start address and size as arguments.");
    if(!stack.is<native_srecord::address_type>(0)) throw duktape::script_error("range getter: Address has to be a numeric address value.");
    if(!stack.is<native_srecord::size_type>(1)) throw duktape::script_error("range getter: Size has to be a numeric size value.");
    if((stack.top()==3) && (!stack.is<native_srecord::value_type>(2))) throw duktape::script_error("range getter: Default value, if given, must be a numeric byte value.");
    const auto sadr = stack.get<native_srecord::address_type>(0);
    const auto size = stack.get<native_srecord::size_type>(1);
    const auto eadr = sadr+size;
    if(sadr < instance.sadr()) throw duktape::script_error("range getter: Start address exceeds the begin of the srecord range.");
    if(eadr > instance.eadr()) throw duktape::script_error("range getter: End address (start+size) exceeds the end of the srecord range.");
    const auto fill = (stack.top()==3) ? stack.get<native_srecord::value_type>(2) : instance.default_value();
    const auto data = instance.get_range(sadr, eadr, fill).bytes();
    static_assert(sizeof(typename decltype(data)::value_type)==1, "data value type no byte?!");
    return data;
  }

}}}}

namespace duktape { namespace mod { namespace srecord {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace std;
    using namespace sw;
    using namespace duktape::detail::srecord;

    js.define(
      // Wrapped class specification and type.
      duktape::native_object<native_srecord>("SRecord")
      // Native constructor from script arguments.
      .constructor([](duktape::api& stack) {
        native_srecord* p;
        if(stack.top()==0) {
          p = new native_srecord();
        } else if((stack.top()==1) && (stack.is<string>(0))) {
          p = new native_srecord(stack.get<string>(0));
        } else {
          throw duktape::script_error("SRecord constructor needs either a S19 data string or no arguments.");
        }
        if(p) {
          p->default_value(0xff);
        }
        return p;
      })
      // Fill/default value for unset ranges.
      .getter("default_value", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.default_value());
      })
      .setter("default_value", [](duktape::api& stack, native_srecord& instance) {
        if(!stack.is<native_srecord::value_type>(0)) throw duktape::script_error("SRecord default_value set-value not compliant with the data type.");
        instance.default_value(stack.get<native_srecord::value_type>(0));
      })
      // Header byte data
      .getter("header_data", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.header());
      })
      .setter("header_data", [](duktape::api& stack, native_srecord& instance) {
        if(!stack.is<native_srecord::data_type>(0)) throw duktape::script_error("SRecord header_data set-value not compliant with the data type.");
        instance.header(stack.get<native_srecord::data_type>(0));
      })
      // Header string data
      .getter("header_string", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.header_str());
      })
      .setter("header_string", [](duktape::api& stack, native_srecord& instance) {
        if(!stack.is<string>(0)) throw duktape::script_error("SRecord header_string set-value not a string.");
        instance.header_str(stack.get<string>(0));
      })
      // Start address
      .getter("sadr", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.sadr());
      })
      // End address
      .getter("eadr", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.eadr());
      })
      // Error code
      .getter("error_code", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.error());
      })
      // Error message
      .getter("error", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.error_message());
      })
      // Error address
      .getter("error", [](duktape::api& stack, native_srecord& instance) {
        stack.push(instance.error_address());
      })
      // Clear method
      .method("clear", [](duktape::api& stack, native_srecord& instance) {
        instance.clear();
        (void)stack;
        stack.push_this();
        return true;
      })
      // Parse method
      .method("parse", [](duktape::api& stack, native_srecord& instance) {
        const string data = stack.get<string>(0, "");
        if(data.empty()) throw duktape::script_error("SRecord: No S19 string data given to parse.");
        if(!instance.parse(data)) throw duktape::script_error(string("SRecord: Failed to parse data: ") + instance.error_message());
        return true;
      })
      // Load method
      .method("load", [](duktape::api& stack, native_srecord& instance) {
        const string path = stack.get<string>(0, "");
        if(path.empty()) throw duktape::script_error("SRecord: No file path given to load.");
        if(!native_srecord::load(path, instance)) throw duktape::script_error(string("SRecord: Failed to load file '") + path + "': " + instance.error_message());
        stack.pop();
        stack.push_this();
        return true;
      })
      // Save method
      .method("save", [](duktape::api& stack, native_srecord& instance) {
        const auto path = stack.get<string>(0, "");
        const auto line_length = stack.get<native_srecord::size_type>(1, 0);
        if(path.empty()) throw duktape::script_error("SRecord: No file path given to save to.");
        if(instance.error()) throw duktape::script_error(string("SRecord: Failed to save, srecord had errors (") + instance.error_message() + ")");
        const auto data = instance.compose(line_length);
        auto os = ofstream(path, ios::out|ios::binary|ios::app|ios::ate);
        if(os.tellp()!=0) { os.close(); throw duktape::script_error(string("SRecord: Failed to save, file already exists and not empty: ") + path); }
        os << data;
        bool ok = os.good();
        os.close();
        if(!ok) throw duktape::script_error(string("SRecord: Failed to save file ") + path);
        stack.pop();
        stack.push_this();
        return true;
      })
      // Compose method
      .method("compose", [](duktape::api& stack, native_srecord& instance) {
        const string data = instance.compose((stack.top()==0) ? (0) : (stack.get<native_srecord::size_type>(0)));
        if(instance.error() != native_srecord::error_type::e_ok) {
          throw duktape::script_error(string("SRecord composing failed: ") + instance.error_message());
        } else {
          stack.push(data);
        }
        return true;
      })
      // dump method
      .method("dump", [](duktape::api& stack, native_srecord& instance) {
        std::ostringstream ss;
        instance.dump(ss);
        stack.push(ss.str());
        return 1;
      })
      // to_string -> dump
      .method("toString", [](duktape::api& stack, native_srecord& instance) {
        const string data = instance.compose((stack.top()==0) ? (0) : (stack.get<native_srecord::size_type>(0)));
        if(instance.error() != native_srecord::error_type::e_ok) {
          throw duktape::script_error(string("SRecord composing failed: ") + instance.error_message());
        } else {
          stack.push(data);
        }
        return true;
      })
      // Find method, returns the start address of the found sequence, or undefined if not found.
      .method("find", [](duktape::api& stack, native_srecord& instance) {
        if(stack.top() < 1) throw duktape::script_error("SRecord find() Missing arguments (1st is a byte array what to search, optional second the start address).");
        if(stack.top() > 2) throw duktape::script_error("SRecord find() Too many arguments.");
        if((stack.top()==2) && (!stack.is<native_srecord::address_type>(1))) throw duktape::script_error("find() 2st is not an integral address.");
        const auto adr  = (stack.top()==2) ? stack.is<native_srecord::address_type>(1) : native_srecord::address_type();
        const auto data = stack.get<native_srecord::data_type>(0);
        if(data.empty()) throw duktape::script_error("SRecord find() 1st argument not a byte array or empty.");
        const auto found_adr = instance.find(data, adr);
        if(found_adr != instance.eadr()) {
          stack.push(found_adr);
          return true;
        }
        return false; // undefined
      })
      // Add method, adds a S19 string to the current the ranges do not overlap
      .method("add", [](duktape::api& stack, native_srecord& instance) {
        if((stack.top() != 1) || (!stack.is<string>(0))) throw duktape::script_error("SRecord add() Missing srecord string to add.");
        const auto instr = stack.get<string>(0);
        if(instr.empty()) throw duktape::script_error("SRecord add() Input string is empty.");
        auto srec = native_srecord(instr);
        if(srec.error()) {
          throw duktape::script_error(string("SRecord add() Input string not a valid s-record (parse error: ") + srec.error_message() + ").");
        }
        if((!instance.blocks().empty()) && (instance.sadr() != instance.eadr())) {
          if(srec.eadr() <= instance.sadr()) {
            // ok
          } else if(srec.sadr() >= instance.eadr()) {
            // ok
          } else {
            throw duktape::script_error(
              string("SRecord add() Ranges of own data an input to merge overlap: ") +
              "own: [" + to_hex_string(instance.sadr()) + "->" + to_hex_string(instance.eadr()) + "] "
              "add: [" + to_hex_string(srec.sadr()) + "->" + to_hex_string(srec.eadr()) + "]."
            );
          }
        }
        for(auto& block:srec.blocks()) {
          instance.set_range(block);
        }
        stack.pop();
        stack.push_this();
        return true;
      })
      // Fill method, fills a given range with a byte value
      .method("fill", [](duktape::api& stack, native_srecord& instance) {
        if(stack.top() < 1) throw duktape::script_error("SRecord fill() Missing arguments (1st is a start address, 2nd size, optional 3rd fill value (byte).");
        if(stack.top() > 3) throw duktape::script_error("SRecord fill() Too many arguments.");
        const auto adr   = stack.get<native_srecord::address_type>(0);
        const auto size  = stack.get<native_srecord::size_type>(1, 0);
        const auto value = (stack.top()==3) ? stack.get<int32_t>(2) : instance.default_value();
        constexpr auto max_adr = native_srecord::address_type(0x7fffffff);
        if(adr > max_adr) throw duktape::script_error(string("SRecord fill() Invalid address: ") + to_hex_string(adr));
        if((size > max_adr) || (size <= 0)) throw duktape::script_error(string("SRecord fill() Invalid size: ") + to_string(size));
        if((size+adr) > max_adr) throw duktape::script_error(string("SRecord fill() adr+size exceeds valid range: ") + to_hex_string(size+adr));
        if((value < 0x00) || (value > 0xff)) throw duktape::script_error(string("SRecord fill() Invalid value: ") + to_hex_string(value));
        auto data = native_srecord::data_type();
        for(size_t i=0; i<size; ++i) data.push_back(value); // not guaranteed that the container is vector<>, so no data_type(n, value);
        instance.set_range(adr, data);
        if(instance.error()) throw duktape::script_error(string("SRecord: Failed to fill: ") + instance.error_message() + ".");
        stack.pop();
        stack.push_this();
        return true;
      })
      // Merge method, combines all blocks to one block, filling gaps with the default value.
      .method("merge", [](duktape::api& stack, native_srecord& instance) {
        auto fill_value = instance.default_value();
        instance.merge(fill_value);
        if(instance.error()) throw duktape::script_error(string("SRecord: Failed to merge, srecord had errors (") + instance.error_message() + ")");
        stack.push_this();
        return true;
      })
      // Blocks method, returns a list of all unconnected/unmerged blocks in the file.
      .method("blocks", [](duktape::api& stack, native_srecord& instance) {
        stack.top(0);
        stack.check_stack(3);
        stack.push_array();
        size_t i=0;
        for(const auto& block: instance.blocks()) {
          stack.push_bare_object();
          stack.push("start");
          stack.push(block.sadr());
          stack.def_prop(-3);
          stack.push("size");
          stack.push(block.eadr() - block.sadr());
          stack.def_prop(-3);
          stack.put_prop_index(0, i++);
        }
        return true;
      })
      // range reading method
      .method("range", [](duktape::api& stack, native_srecord& instance) {
        const auto data = detail::srecord::range_data(stack, instance);
        auto ret = string();
        ret.reserve(data.size()*2+4);
        for(const auto val:data) ret += detail::srecord::hex_byte(val);
        stack.top(0);
        stack.push(ret);
        return true;
      })
      // uint32_t getter method from a specified address, little endian.
      .method("getu32le", duktape::detail::srecord::js_method_get_integral_value<uint32_t, native_srecord::endianess_type::little_endian>)
      // uint32_t getter method from a specified address, big endian.
      .method("getu32be", duktape::detail::srecord::js_method_get_integral_value<uint32_t, native_srecord::endianess_type::big_endian>)
      // uint32_t getter method from a specified address, little endian.
      .method("gets32le", duktape::detail::srecord::js_method_get_integral_value<int32_t, native_srecord::endianess_type::little_endian>)
      // uint32_t getter method from a specified address, big endian.
      .method("gets32be", duktape::detail::srecord::js_method_get_integral_value<int32_t, native_srecord::endianess_type::big_endian>)
      // uint16_t getter method from a specified address, little endian.
      .method("getu16le", duktape::detail::srecord::js_method_get_integral_value<uint16_t, native_srecord::endianess_type::little_endian>)
      // uint16_t getter method from a specified address, big endian.
      .method("getu16be", duktape::detail::srecord::js_method_get_integral_value<uint16_t, native_srecord::endianess_type::big_endian>)
      // int16_t getter method from a specified address, little endian.
      .method("gets16le", duktape::detail::srecord::js_method_get_integral_value<int16_t, native_srecord::endianess_type::little_endian>)
      // int16_t getter method from a specified address, big endian.
      .method("gets16be", duktape::detail::srecord::js_method_get_integral_value<int16_t, native_srecord::endianess_type::big_endian>)
      // uint8_t getter method from a specified address
      .method("getu8", duktape::detail::srecord::js_method_get_integral_value<uint8_t, native_srecord::endianess_type::little_endian>)
      // int8_t getter method from a specified address
      .method("gets8", duktape::detail::srecord::js_method_get_integral_value<int8_t, native_srecord::endianess_type::little_endian>)
      // uint32_t setter method from a specified address, little endian.
      .method("setu32le", duktape::detail::srecord::js_method_set_integral_value<uint32_t, native_srecord::endianess_type::little_endian>)
      // uint32_t setter method from a specified address, big endian.
      .method("setu32be", duktape::detail::srecord::js_method_set_integral_value<uint32_t, native_srecord::endianess_type::big_endian>)
      // uint32_t setter method from a specified address, little endian.
      .method("sets32le", duktape::detail::srecord::js_method_set_integral_value<int32_t, native_srecord::endianess_type::little_endian>)
      // uint32_t setter method from a specified address, big endian.
      .method("sets32be", duktape::detail::srecord::js_method_set_integral_value<int32_t, native_srecord::endianess_type::big_endian>)
      // uint16_t setter method from a specified address, little endian.
      .method("setu16le", duktape::detail::srecord::js_method_set_integral_value<uint16_t, native_srecord::endianess_type::little_endian>)
      // uint16_t setter method from a specified address, big endian.
      .method("setu16be", duktape::detail::srecord::js_method_set_integral_value<uint16_t, native_srecord::endianess_type::big_endian>)
      // int16_t setter method from a specified address, little endian.
      .method("sets16le", duktape::detail::srecord::js_method_set_integral_value<int16_t, native_srecord::endianess_type::little_endian>)
      // int16_t setter method from a specified address, big endian.
      .method("sets16be", duktape::detail::srecord::js_method_set_integral_value<int16_t, native_srecord::endianess_type::big_endian>)
      // uint8_t setter method from a specified address
      .method("setu8", duktape::detail::srecord::js_method_set_integral_value<uint8_t, native_srecord::endianess_type::little_endian>)
      // int8_t setter method from a specified address
      .method("sets8", duktape::detail::srecord::js_method_set_integral_value<int8_t, native_srecord::endianess_type::little_endian>)
      // Checksum method
      .method("cksum", duktape::detail::srecord::js_method_cksum<>)
    );
  }

}}}

#endif
