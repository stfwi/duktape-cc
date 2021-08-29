/**
 * @file duktape/mod/ext/mod.ext.serial_port.hh
 * @package de.atwillys.cc.duktape.ext
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++14 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional binding to the `serial_port.hh` library.
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
#ifndef DUKTAPE_MOD_EXT_SERIALPORT_HH
#define DUKTAPE_MOD_EXT_SERIALPORT_HH
#include "../../../duktape.hh"
#include "./serial_port.hh"
#include <algorithm>
#include <optional>
#include <chrono>
#include <deque>

#pragma GCC diagnostic push


namespace duktape { namespace detail { namespace ext { namespace serial_port {

  /**
   * Unfortunately not all OS do explicitly close all descriptors and
   * resources on exit process cleanup, so a manual tracking of open
   * ports is needed.
   */
  template <typename=void>
  struct open_port_tracking
  {
  public:

    using descriptor_type = sw::com::serial_port::descriptor_type;

  public:

    static inline open_port_tracking& instance() noexcept
    { return instance_; }

  public:

    explicit inline open_port_tracking() noexcept : open_() {}
    open_port_tracking(const open_port_tracking&) = delete;
    open_port_tracking(open_port_tracking&&) = default;
    open_port_tracking& operator=(const open_port_tracking&) = delete;

    ~open_port_tracking() noexcept
    {
      for(auto& fd:open_) {
        #ifdef __WINDOWS__
        ::CloseHandle(fd);
        #else
        ::close(fd);
        #endif
      }
    }

  public:

    open_port_tracking& operator+=(descriptor_type fd)
    { open_.push_back(fd); return *this; }

    open_port_tracking& operator-=(descriptor_type fd)
    { open_.erase(std::remove(open_.begin(), open_.end(), fd), open_.end()); return *this; }

  private:

    std::deque<descriptor_type> open_;
    static open_port_tracking instance_;
  };

  template <typename T>
  open_port_tracking<T> open_port_tracking<T>::instance_ = open_port_tracking<T>();

}}}}

namespace duktape { namespace mod { namespace ext { namespace serial_port {

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
    using namespace duktape::detail::ext::serial_port;
    using tracker = duktape::detail::ext::serial_port::open_port_tracking<void>;
    using native_serialport = sw::com::serial_port;
    using native_tty = sw::com::serial_tty;

    #if(0 && JSDOC)
    /**
     * Serial port handling object constructor, optionally with
     * initial port settings like '<port>,115200n81'.
     *
     * @constructor
     * @throws {Error}
     * @param {string|undefined} settings
     *
     * @property {string}  port         - Port setting (e.g. "/dev/ttyS0", "COM98", "usbserial1"). Effective after re-opening.
     * @property {number}  baudrate     - Baud rate setting (e.g. 9600, 115200). Effective after re-opening.
     * @property {number}  databits     - Data bit count setting (7, 8). Effective after re-opening.
     * @property {number}  stopbits     - Stop bit count setting (1, 1.5, 2). Effective after re-opening.
     * @property {string}  parity       - Parity setting ("n", "o", "e"). Effective after re-opening.
     * @property {string}  flowcontrol  - Flow control selection ("none", "xonxoff", "rtscts").
     * @property {string}  settings     - String representation of all port config settings.
     * @property {number}  timeout      - Default reading timeout setting in milliseconds. Effective after re-opening.
     * @property {string}  txnewline    - Newline character for sending data (e.g. "\n", "\r", "\r\n", etc)
     * @property {string}  rxnewline    - Newline character for receiving data (end-of-line detection for line based reading).
     * @property {boolean} closed       - Holds true when the port is not opened.
     * @property {boolean} isopen       - Holds true when the port is opened.
     * @property {boolean} rts          - Current state of the RTS line (only if supported by the port/driver).
     * @property {boolean} cts          - Current state of the CTS line (only if supported by the port/driver).
     * @property {boolean} dtr          - Current state of the DTR line (only if supported by the port/driver).
     * @property {boolean} dsr          - Current state of the DSR line (only if supported by the port/driver).
     * @property {boolean} error        - Error code of the last method call.
     * @property {string}  errormessage - Error string representation of the last method call.
     */
    sys.serialport = function(optional_settings) {};
    #endif
    js.define(
      // Wrapped class specification and type.
      duktape::native_object<native_tty>("sys.serialport")
      // Native constructor from script arguments.
      .constructor([](duktape::api& stack) {
        if(stack.top()==0) {
          return new native_tty();
        } else if((stack.top()==1) && (stack.is<string>(0))) {
          return new native_tty(stack.get<string>(0));
        } else {
          throw duktape::script_error("sys.serialport constructor needs either a data string (e.g. '<port>,115200n81') or no arguments.");
        }
      })
      .getter("port", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.port());
      })
      .setter("port", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<native_serialport::port_type>(0)) throw duktape::script_error("sys.serialport: port must be a string like 'ttyS0' or 'com1'.");
        instance.port(stack.get<native_serialport::port_type>(0));
      })
      .getter("baudrate", [](duktape::api& stack, native_tty& instance) {
        stack.push(int(instance.baudrate()));
      })
      .setter("baudrate", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<native_serialport::baudrate_type>(0)) throw duktape::script_error("sys.serialport: baudrate must be an integer like 9600, 115200, 921600, etc.");
        instance.baudrate(native_serialport::baudrate_type(stack.get<int>(0)));
      })
      .getter("databits", [](duktape::api& stack, native_tty& instance) {
        stack.push(int(instance.databits()));
      })
      .setter("databits", [](duktape::api& stack, native_tty& instance) {
        const int db = stack.to<int>(0);
        if((db != 7) && (db != 8)) throw duktape::script_error("sys.serialport: databits must be 7 or 8.");
        instance.databits(native_serialport::databits_type(db));
      })
      .getter("stopbits", [](duktape::api& stack, native_tty& instance) {
        switch(instance.stopbits()) {
          case native_serialport::stopbits_1:  { stack.push(1);   return; }
          case native_serialport::stopbits_15: { stack.push(1.5); return; }
          case native_serialport::stopbits_2:  { stack.push(2);   return; }
          default: throw duktape::script_error("Unexpected stopbits in the native object.");
        }
      })
      .setter("stopbits", [](duktape::api& stack, native_tty& instance) {
        const double sb = stack.to<double>(0);
        if(sb == 1) {
          instance.stopbits(native_serialport::stopbits_1);
        } else if(sb == 1.5) {
          instance.stopbits(native_serialport::stopbits_15);
        } else if(sb == 2) {
          instance.stopbits(native_serialport::stopbits_2);
        } else {
          throw duktape::script_error("sys.serialport: stopbits must be numeric 1, 1.5, or 2.");
        }
      })
      .getter("parity", [](duktape::api& stack, native_tty& instance) {
        switch(instance.parity()) {
          case native_serialport::parity_none: { stack.push("n"); return; }
          case native_serialport::parity_even: { stack.push("e"); return; }
          case native_serialport::parity_odd:  { stack.push("o"); return; }
          default: throw duktape::script_error("Unexpected parity in the native object.");
        }
      })
      .setter("parity", [](duktape::api& stack, native_tty& instance) {
        const string parity = stack.get<string>(0);
        if(parity == "n") {
          instance.parity(native_serialport::parity_none);
        } else if(parity == "e") {
          instance.parity(native_serialport::parity_even);
        } else if(parity == "o") {
          instance.parity(native_serialport::parity_odd);
        } else if(parity == "m") {
          throw duktape::script_error("sys.serialport: parity 'mark' not supported.");
        } else {
          throw duktape::script_error("sys.serialport: parity must be 'n' (none), 'e' (even), 'o' (odd) or 'm' (mark).");
        }
      })
      .getter("timeout", [](duktape::api& stack, native_tty& instance) {
        stack.push(int(instance.timeout()));
      })
      .setter("timeout", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<native_serialport::timeout_type>(0)) throw duktape::script_error("sys.serialport: timeout must be an integer in milliseconds.");
        instance.timeout(native_serialport::timeout_type(stack.get<int>(0)));
      })
      .getter("settings", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.settings());
      })
      .setter("settings", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<string>(0)) throw duktape::script_error("sys.serialport: settings must be a string like '115200n81' or '9600e71'.");
        instance.settings(stack.get<string>(0));
        if(instance.error()) throw duktape::script_error(string("sys.serialport: settings invalid: ") + instance.error_message());
      })
      .getter("txnewline", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.tx_newline());
      })
      .setter("txnewline", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<string>(0)) throw duktape::script_error("sys.serialport: txnewline must be a string like e.g. '\n' (=LF), '\r'(=CR), or '\r\n' (=CRLF).");
        instance.tx_newline(stack.get<string>(0));
      })
      .getter("rxnewline", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.rx_newline());
      })
      .setter("rxnewline", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<string>(0)) throw duktape::script_error("sys.serialport: rxnewline must be a string like e.g. '\n' (=LF), '\r'(=CR), or '\r\n' (=CRLF).");
        instance.rx_newline(stack.get<string>(0));
      })
      .getter("closed", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.closed());
      })
      .getter("isopen", [](duktape::api& stack, native_tty& instance) {
        stack.push(!instance.closed());
      })
      .getter("rts", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.rts());
      })
      .setter("rts", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<bool>(0)) throw duktape::script_error("sys.serialport: RTS assignment must be boolean.");
        instance.rts(stack.get<bool>(0));
      })
      .getter("cts", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.cts());
      })
      .setter("cts", [](duktape::api& stack, native_tty& instance) {
        (void)stack; (void)instance;
        throw duktape::script_error("sys.serialport: You cannot set CTS, it's an input.");
      })
      .getter("dtr", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.dtr());
      })
      .setter("dtr", [](duktape::api& stack, native_tty& instance) {
        if(!stack.is<bool>(0)) throw duktape::script_error("sys.serialport: DTR assignment must be boolean.");
        instance.dtr(stack.get<bool>(0));
      })
      .getter("dsr", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.dsr());
      })
      .setter("dsr", [](duktape::api& stack, native_tty& instance) {
        (void)stack; (void)instance;
        throw duktape::script_error("sys.serialport: You cannot set DSR, it's an input.");
      })
      .getter("error", [](duktape::api& stack, native_tty& instance) {
        stack.push(int(instance.error()));
      })
      .getter("errormessage", [](duktape::api& stack, native_tty& instance) {
        stack.push(instance.error_message());
      })
      .getter("flowcontrol", [](duktape::api& stack, native_tty& instance) {
        switch(instance.flowcontrol()) {
          case native_serialport::flowcontrol_none:    { stack.push("none");    return; }
          case native_serialport::flowcontrol_xonxoff: { stack.push("xonxoff"); return; }
          case native_serialport::flowcontrol_rtscts:  { stack.push("rtscts");  return; }
          default: stack.push("none");
        }
      })
      .setter("flowcontrol", [](duktape::api& stack, native_tty& instance) {
        string flow = "-";
        if(stack.is<string>(0)) flow = stack.get<string>(0);
        if(flow.empty() || flow=="none") {
          instance.flowcontrol(native_serialport::flowcontrol_none);
        } else if(flow=="xonxoff") {
          instance.flowcontrol(native_serialport::flowcontrol_xonxoff);
        } else if(flow=="rtscts") {
          instance.flowcontrol(native_serialport::flowcontrol_rtscts);
        } else {
          throw duktape::script_error("sys.serialport: flowcontrol: Value must be 'none', 'xonxoff', or 'rtscts'.");
        }
      })
      #if(0 && JSDOC)
      /**
       * Closes the port, resets errors.
       * @return {sys.serialport}
       */
      sys.serialport.prototype.close = function() {};
      #endif
      .method("close", [](duktape::api& stack, native_tty& instance) {
        tracker::instance() -= instance.descriptor();
        instance.close();
        (void)stack;
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Opens the port, optionally with given settings.
       *
       * @throws {Error}
       * @param {string|undefined} port
       * @param {string|undefined} settings
       * @return {sys.serialport}
       */
      sys.serialport.prototype.open = function(port, settings) {};
      #endif
      .method("open", [](duktape::api& stack, native_tty& instance) {
        auto settings = string();
        auto port = string();
        bool ok = false;
        if(stack.top() > 0) {
          if(!stack.is<string>(0)) throw duktape::script_error("sys.serialport: open(port, ...) First argument (port name/path) must be a string. Optional second string argument with settings.");
          auto port = stack.get<string>(0);
          if(stack.top() > 1) {
            if(!stack.is<string>(1)) throw duktape::script_error("sys.serialport: open(port, settings) Second argument (settings) must be a string like '115200n81'.");
            settings = stack.get<string>(1);
          }
          stack.top(0);
          ok = settings.empty() ? instance.open(port) : instance.open(port, settings);
        } else {
          ok = instance.open();
        }
        if(!ok) {
          throw duktape::script_error(string("sys.serialport: Failed to open: ") + instance.error_message());
        }
        tracker::instance() += instance.descriptor();
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Clears input and output buffers of the port.
       *
       * @return {sys.serialport}
       */
      sys.serialport.prototype.purge = function() {};
      #endif
      .method("purge", [](duktape::api& stack, native_tty& instance) {
        stack.top(0);
        instance.purge();
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Reads received data, aborts after `timeout_ms` has expired.
       * Returns an empty string if nothing was received or timeout.
       *
       * @param {number} timeout_ms
       * @return {string}
       */
      sys.serialport.prototype.read = function(timeout_ms) {};
      #endif
      .method("read", [](duktape::api& stack, native_tty& instance) {
        int timeout = (stack.top()==0) ? (-1) : stack.to<int>(0);
        auto out = string();
        out.reserve(32768);
        if(!instance.read(out, timeout)) throw duktape::script_error(string("Read failed: " + instance.error_message()));
        stack.push(out);
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Sends off the the given data.
       *
       * @param {string} data
       * @return {sys.serialport}
       */
      sys.serialport.prototype.write = function(data) {};
      #endif
      .method("write", [](duktape::api& stack, native_tty& instance) {
        if((stack.top()==0) || (stack.is_undefined(0))) throw duktape::script_error("write() no value to write given.");
        auto tx = string();
        if(stack.is<string>(0)) {
          tx = stack.get<string>(0);
        } else {
          throw duktape::script_error("Only string output supported for serial write.");
        }
        while(!tx.empty()) {
          auto n = native_serialport::size_type(0);
          if(!instance.write(tx, n)) throw duktape::script_error(string("Read failed: " + instance.error_message()));
          if(n >= tx.size()) break;
          tx = tx.substr(n);
        }
        stack.top(0);
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Reads a line from the received data. Optionally with a given
       * timeout (else the default timeout), and supression of empty
       * lines. Returns `undefined` if no line was received, the fetched
       * line otherwise.
       *
       * @throws {Error}
       * @param {number}  timeout_ms
       * @param {boolean} ignore_empty
       * @return {string|undefined}
       */
      sys.serialport.prototype.readln = function(timeout_ms, ignore_empty) {};
      #endif
      .method("readln", [](duktape::api& stack, native_tty& instance) {
        int timeout = (stack.top() > 0) ? stack.to<int>(0) : (-1);
        bool ignore_empty = (stack.top() > 1) ? stack.to<bool>(1) : false;
        stack.top(0);
        vector<string> out;
        switch(instance.readln(out, timeout, ignore_empty)) {
          case native_tty::readln_result::nothing:
            stack.push_undefined();
            return false;
          case native_tty::readln_result::received:
            stack.push(out);
            return true;
          case native_tty::readln_result::error:
          default:
            throw duktape::script_error(string("Read failed: " + instance.error_message()));
        }
        return false;
      })
      #if(0 && JSDOC)
      /**
       * Sends a line, t.m. `this.txnewline` is automatically appended.
       *
       * @throws {Error}
       * @param {string}  data
       * @return {sys.serialport}
       */
      sys.serialport.prototype.writeln = function(data) {};
      #endif
      .method("writeln", [](duktape::api& stack, native_tty& instance) {
        if((stack.top()==0) || (stack.is_undefined(0))) throw duktape::script_error("write() no value to write given.");
        if(!stack.is<string>(0)) throw duktape::script_error("Only string output supported for serial write.");
        const auto tx = stack.get<string>(0);
        if(!instance.writeln(tx)) throw duktape::script_error(string("Write failed: " + instance.error_message()));
        stack.top(0);
        stack.push_this();
        return true;
      })
    );

    #if(0 && JSDOC)
    /**
     * Returns a plain object containing short names and paths for
     * eligible serial devices. Both keys and values are accepted
     * as `port` setting.
     *
     * @throws {Error}
     * @return {object}
     */
    sys.serialport.portlist = function(data) {};
    #endif
    js.define("sys.serialport.portlist", [](duktape::api& stack) {
      stack.top(0);
      stack.check_stack(3);
      stack.push_bare_object();
      const auto device_map = native_serialport::device_list();
      for(const auto& kv:device_map) {
        stack.push(kv.first);
        stack.push(kv.second);
        stack.def_prop(-3);
      }
      return 1;
    });
  }

}}}}

#pragma GCC diagnostic pop
#endif
