/**
 * @file duktape/mod/mod.stdio.hh
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
 * Optional I/O functions for stdin, stdout and stderr.
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
#ifndef DUKTAPE_STDIO_HH
#define DUKTAPE_STDIO_HH

#include "../duktape.hh"
#include "mod.sys.os.hh"
#include <iostream>
#include <sstream>
#include <streambuf>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef OS_WINDOWS
  #include <errno.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <poll.h>
  #include <termios.h>
#else
  #include <windows.h>
#endif


namespace duktape { namespace detail {

  template <typename=void>
  struct stdio
  {
  public:

    /**
     * Export main relay. Adds all module functions to the specified engine.
     * @param duktape::engine& js
     */
    static void define_in(duktape::engine& js)
    {
      #if(0 && JSDOC)
      /**
       * print(a,b,c, ...). Writes data stringifyed to STDOUT.
       *
       * @param {...*} args
       */
      print = function(args) {};
      #endif
      js.define("print", print);

      #if(0 && JSDOC)
      /**
       * alert(a,b,c, ...). Writes data stringifyed to STDERR.
       *
       * @param {...*} args
       */
      alert = function(args) {};
      #endif
      js.define("alert", alert);

      #if(0 && JSDOC)
      /**
       * Returns a first character entered as string. First function argument
       * is a text that is printed before reading the input stream (without newline,
       * "?" or ":"). The input is read from STDIN.
       *
       * @param {string} text
       * @return {string}
       */
      confirm = function(text) {};
      #endif
      js.define("confirm", confirm);

      #if(0 && JSDOC)
      /**
       * Returns a line entered via the input stream STDIN.
       *
       * @return {string}
       */
      prompt = function() {};
      #endif
      js.define("prompt", prompt);

      #if(0 && JSDOC)
      /**
       * C style formatted output to STDOUT. Note that not
       * all formats can be used like in C/C++ because ECMA
       * is has no strong type system. E.g. %u, %ul etc does
       * not make sense because numbers are intrinsically
       * floating point, and the (data type) size of the number
       * matters when using %u. If unsupported arguments are
       * passed the method will throw an exception.
       *
       * Supported formatters are:
       *
       *  - %d, %ld, %lld: The ECMA number is coerced
       *    into an integer and printed. Digits and sign
       *    are supported (%4d, %+06l).
       *
       *  - %f, %lf, %g: Floating point format, also with additional
       *    format specs like %.8f, %+10.5f etc.
       *
       *  - %x: Hexadecimal representation of numbers (also %08x
       *    or %4X etc supported).
       *
       *  - %o: Octal representation of numbers (also %08o)
       *
       *  - %s: String coercing with optional minimum width (e.g. %10s)
       *
       *  - %c: When number given, the ASCII code of the lowest byte
       *        (e.g. 65=='A'). When string given the first character
       *        of the string.
       *
       * Not supported:
       *
       *  - Parameter argument (like "%2$d", the "2$" would apply the
       *    same format to two arguments passed to the function).
       *
       *  - Dynamic width (like "%*f", the "*" would be used to pass
       *    the output width of the floating point number as argument).
       *
       *  - Unsigned %u, %n
       *
       * @param {string} format
       * @param {...*} args
       */
      printf = function(format, args) {};
      #endif
      js.define("printf", printf_stdout);

      #if(0 && JSDOC)
      /**
       * C style formatted output into a string. For
       * formatting details please @see printf.
       *
       * @param {string} format
       * @param {...*} args
       * @return {string}
       */
      sprintf = function(format, args) {};
      #endif
      js.define("sprintf", printf_string);

      #if(0 && JSDOC)
      /**
       * Console object known from various JS implementations.
       *
       * @var {object}
       */
      var console = {};

      /**
       * Writes a line to the log stream (automatically appends a newline).
       * The default log stream is STDERR.
       *
       * @param {...*} args
       */
      console.log = function(args) {};
      #endif
      js.define("console.log", console_log);

      #if(0 && JSDOC)
      /**
       * Read from STDIN (default until EOF). Using the argument `arg`
       * it is possible to use further functionality:
       *
       *  - By default (if `arg` is undefined or not given) the function
       *    returns a string when all data are read.
       *
       *  - If `arg` is boolean and `true`, then the input is read into
       *    a buffer variable, not a string. The function returns when
       *    all data are read.
       *
       * - If `arg` is a function, then a string is read line by line,
       *   and each line passed to this callback function. If the function
       *   returns `true`, then the line is added to the output. The
       *   function may also preprocess line and return a String. In this
       *   case the returned string is added to the output.
       *   Otherwise, if `arg` is not `true` and no string, the line is
       *   skipped.
       *
       * - If `arg` is a number, then reading from a pipe or tty/console
       *   is nonblocking if supported by the device, returning maximum
       *   `arg` characters. Returns `undefined` on EOF or read error
       *   (e.g. broken-pipe). Console interfaces are temporarily set to
       *   non-canonical mode for this purpose (settings restored after
       *   reading).
       *
       * @param {function|boolean} arg
       * @return {string|buffer}
       */
      console.read = function(arg) {};
      #endif
      js.define("console.read", console_read);

      #if(0 && JSDOC)
      /**
       * Write to STDOUT without any conversion, whitespaces between the given arguments,
       * and without newline at the end.
       *
       * @param {...*} args
       */
      console.write = function(args) {};
      #endif
      js.define("console.write", console_write);

      #if(0 && JSDOC)
      /**
       * Blocking string line input.
       * @return {string}
       */
      console.readline = function(args) {};
      #endif
      js.define("console.readline", console_readline);

      #if(0 && JSDOC)
      /**
       * Enable/disable VT100 processing (default: enabled).
       * Only relevant on Windows operating systems.
       *
       * @param {boolean} enable
       */
      console.vt100 = function(enable) {};
      #endif
      js.define("console.vt100", win32vt100);
      win32vt100(true);
    }

  public:

    static int print(duktape::api& stack)
    { return print_to(stack, out_stream); }

    static int alert(duktape::api& stack)
    { return print_to(stack, err_stream); }

    static int confirm(duktape::api& stack)
    {
      if(!in_stream) return 0;
      if(!!out_stream) {
        if(stack.top() > 0) {
          (*out_stream) << stack.to<std::string>(0);
        } else {
          (*out_stream) << "[press ENTER to continue ...]";
        }
      }
      std::string s;
      std::getline(*in_stream, s);
      if(s.length() > 0) s = s[0];
      stack.push(s);
      return 1;
    }

    static int prompt(duktape::api& stack)
    {
      if(!in_stream) return 0;
      if(!!out_stream) {
        if(stack.top() > 0) {
          (*out_stream) << stack.to<std::string>(0);
        }
      }
      std::string s;
      std::getline(*in_stream, s);
      stack.push(s);
      return 1;
    }

    static int console_log(duktape::api& stack)
    { return print_to(stack, log_stream); }

    static int console_write(duktape::api& stack)
    {
      if(!out_stream) return 0;
      int nargs = stack.top();
      for(int i=0; i<nargs; ++i) {
        if(stack.is_buffer(i)) {
          const char *buf = nullptr;
          duk_size_t sz = 0;
          if((buf = (const char *) stack.get_buffer(0, sz)) && (sz > 0)) {
            (*out_stream).write(buf, sz);
          }
        } else {
          (*out_stream) << stack.to<std::string>(i);
        }
      }
      (*out_stream).flush();
      return 0;
    }

    static int console_read(duktape::api& stack)
    {
      if(!in_stream) return 0;
      int nargs = stack.top();
      if((nargs > 0) && stack.is_callable(0)) {
        // Filtered string from lines
        std::ostringstream ss;
        while((*in_stream).good()) {
          std::string s;
          std::getline((*in_stream), s);
          stack.dup(0);
          stack.push_string(s);
          stack.call(1);
          if(stack.is_string(-1)) {
            s = stack.get<std::string>(-1);
            ss << s << "\n";
          } else if(stack.get<bool>(-1)) {
            ss << s << "\n";
          }
          stack.top(1);
        }
        stack.top(0);
        stack.push(ss.str());
        return 1;
      } else if((nargs > 0) && stack.is_number(0)) {
        // Jaja, von wegen cin.readsome() ... "heavily implementation defined."
        // -> for(auto n=in_stream->readsome(buf, sizeof(buf)); n > 0; n=in_stream->readsome(buf, sizeof(buf))) s.append(buf, n);
        static constexpr size_t buffer_size = 1024;
        const size_t arg = stack.get<size_t>(0);
        const size_t max_chars = ((arg>0) && (arg<buffer_size)) ? (arg) : (buffer_size);
        auto s = std::string();
        #ifdef OS_WINDOWS
        {
          const auto hnd = ::GetStdHandle(STD_INPUT_HANDLE);
          if(hnd == INVALID_HANDLE_VALUE) return 0; // Unusual for stdin, return undefined.
          const auto file_type = ::GetFileType(hnd);
          switch(file_type) {
            case FILE_TYPE_CHAR: {
              // Console
              DWORD console_mode = 0;
              if(::GetConsoleMode(hnd, &console_mode)) {
                if(::WaitForSingleObject(hnd, 0) == WAIT_OBJECT_0) {
                  char buf[buffer_size];
                  DWORD n_read = 0;
                  ::SetConsoleMode(hnd, console_mode & ~DWORD(ENABLE_LINE_INPUT));
                  if((!::ReadConsoleA(hnd, buf, max_chars, &n_read, nullptr)) || (!n_read)) break;
                  ::SetConsoleMode(hnd, console_mode);
                  // Console is user input text mode, so normalizing CR to LF is valid.
                  for(size_t i=0; i<n_read; ++i) { if(buf[i]=='\r') { buf[i] = '\n'; } }
                  s.append(buf, n_read);
                }
              } else {
                // Not a console but something entirely unexpected, read blocking.
                std::string buf((std::istreambuf_iterator<char>(*in_stream)), std::istreambuf_iterator<char>());
                s.swap(buf);
              }
              break;
            }
            case FILE_TYPE_PIPE: {
              DWORD n_avail = 0;
              char buf[buffer_size];
              while(::PeekNamedPipe(hnd, nullptr, max_chars, nullptr, &n_avail, nullptr) && (n_avail > 0)) {
                if(n_avail > max_chars) n_avail = max_chars;
                DWORD n_read = 0;
                if(!::ReadFile(hnd, buf, n_avail, &n_read, nullptr) || (!n_read)) break;
                s.append(buf, n_read);
              }
              break;
            }
          }
        }
        #else
        {
          bool return_undefined = false;
          const auto fl = ::fcntl(STDIN_FILENO, F_GETFL);
          if(fl < 0) return 0; // File error (broken pipe, disconnect, etc) -> undefined
          ::termios tcget, tcset;
          const bool setterm = (::isatty(STDIN_FILENO)) && (::tcgetattr(STDIN_FILENO, &tcget)==0);
          if(setterm) {
            tcset = tcget;
            tcset.c_lflag &= ~(ICANON);
            ::tcsetattr(STDIN_FILENO, TCSANOW, &tcset);
          }
          auto pfd = ::pollfd{STDIN_FILENO, POLLIN|POLLHUP, 0};
          const auto r = ::poll(&pfd, 1, 0);
          if(r < 0) {
            if(errno != EAGAIN) return_undefined = true; // poll error, undefined.
          } else if(!r) {
            (void)0; // timeout, return empty string.
          } else if(pfd.revents & (POLLIN|POLLHUP)) {
            char buf[buffer_size];
            ::fcntl(STDIN_FILENO, F_SETFL, fl|O_NONBLOCK);
            const auto r = ::read(STDIN_FILENO, buf, max_chars);
            ::fcntl(STDIN_FILENO, F_SETFL, fl);
            if(r < 0) {
              if(errno != EAGAIN) return_undefined = true; // e.g. broken pipe or eof reached before -> undefined.
            } else if(r > 0) {
              s.append(buf, size_t(r));
            } else {
              return_undefined = true; // eof -> undefined
            }
          }
          if(setterm) {
            ::tcsetattr(STDIN_FILENO, TCSANOW, &tcget);
          }
          if(return_undefined) return 0;
        }
        #endif
        stack.push_string(s);
        return 1;
      } else {
        std::string s((std::istreambuf_iterator<char>(*in_stream)), std::istreambuf_iterator<char>());
        if((nargs > 0) && stack.is<bool>(0) && stack.get<bool>(0)) {
          // Binary without filter function
          char* buf = reinterpret_cast<char*>(stack.push_buffer(s.length(), false));
          if(!buf) {
            throw script_error("Failed to read binary data from console (buffer allocation failed)");
          } else {
            std::copy(s.begin(), s.end(), buf);
          }
        } else {
          // String without filter function
          stack.push_string(s);
        }
        return 1;
      }
    }

    static int console_readline(duktape::api& stack)
    {
      if(!in_stream) return 0;
      std::string s;
      std::getline(*in_stream, s);
      stack.push(s);
      return 1;
    }

    static int printf_stdout(duktape::api& stack)
    { return printf_to(stack, out_stream); }

    static int printf_string(duktape::api& stack)
    {
      std::stringstream ss;
      printf_to(stack, &ss);
      if(stack.is_error(-1)) return 0;
      stack.push(ss.str());
      return 1;
    }

  public:

    static int printf_to(duktape::api& stack, std::ostream* stream)
    {
      if(!stream) return 0;
      std::string fmt;
      if(!stack.is_string(0) || (fmt=stack.get<std::string>(0)).empty()) {
        return stack.throw_exception("First argument must be the format given as string.");
      }
      // we need to carefully pre-check the format before passing then (separated) to snprintf calls.
      int stack_i = 0;
      std::string out;
      while(!fmt.empty()) {
        auto p = fmt.find('%');
        if(p == fmt.npos) {
          // no more formats
          out += fmt;
          fmt.clear();
          continue;
        } else if(p != 0) {
          // includes % at begin
          out += fmt.substr(0, p);
          fmt = fmt.substr(p);
          continue;
        } else if((fmt.size() > 1) && fmt[1] == '%') {
          // escaped %%
          out += '%';
          fmt = fmt.substr(2);
          continue;
        } else {
          ++stack_i;
          p = fmt.find_first_of("%diufFeEgGxXoscpaAn", 1); // possible ends (type field)
          if((p == fmt.npos) || (fmt[p] == '%')) return stack.throw_exception("Unterminated printf format (missing type specification character)");
          std::string curfmt = fmt.substr(0, p+1);
          fmt = fmt.substr(p+1);
          if(curfmt.find_first_not_of("%0123456789+-. diufFeEgGxXoscpaAnlL") != curfmt.npos) {
            // note: most important this excludes * and $
            return stack.throw_exception("Format contains invalid or unsupported characters");
          } else if(stack_i >= stack.top()) {
            return stack.throw_exception("Not enough arguments provided for the given format string");
          } else if(curfmt.back() == 's') {
            if((!stack.is_string(stack_i)) && (!stack.is_number(stack_i))) {
              return stack.throw_exception("No string argument for the format %s given");
            }
            std::string ts = stack.to<std::string>(stack_i);
            if(curfmt == "%s") {
              out += ts;
            } else {
              {
                // width needs to be double checked to prevent segfault: (e.g. for "%8000s")
                std::string cks = curfmt;
                cks.erase(std::remove_if(cks.begin(), cks.end(), [](char c){return c<'0'||c>'9';}), cks.end());
                while(!cks.empty() && (cks.front() == '0')) cks = cks.erase(0);
                long l = ::atol(cks.c_str());
                if(l < 0 || l > 2048) return stack.throw_exception("String format length specification too large");
              }
              char o[4096]; ::memset(o, 0, sizeof(o));
              auto r = ::snprintf(o, 4090, curfmt.c_str(), ts.c_str());
              if((r < 0) || (size_t(r) >= 4090)) {
                return stack.throw_exception("Formatting failed");
              } else {
                out += o;
              }
            }
          } else if(curfmt.back() == 'c') {
            char c = 0;
            if(stack.is_string(stack_i)) {
              std::string ts = stack.to<std::string>(stack_i);
              c = ts.empty() ? '\0' : ts[0];
            } else if(stack.is_number(stack_i)) {
              unsigned u = stack.to<unsigned>(stack_i);
              c = char(((unsigned char)u) & 0xff);
            } else {
              return stack.throw_exception("No string or number argument for the format %c given");
            }
            std::string o(4096, '\0');
            auto r = ::snprintf(&o[0], o.size()-1, curfmt.c_str(), c);
            if((r < 0) || (size_t(r) >= o.size()-1)) return stack.throw_exception("Formatting failed");
            out += o.c_str();
          } else if(std::string("dixXo").find(curfmt.back()) != std::string::npos) {
            if(!stack.is_number(stack_i)) return stack.throw_exception(std::string("No number argument for the format %") + curfmt.back() + " given");
            std::string lc = curfmt;
            for(auto& e:lc) e = ::tolower(e);
            std::string o(64, '\0');
            int r = -1;
            if(lc.find("ll") != lc.npos) {
              long long l = stack.to<long long>(stack_i);
              r = ::snprintf(&o[0], o.size()-1, curfmt.c_str(), l);
            } else if(lc.find('l') != lc.npos) {
              long l = stack.to<long>(stack_i);
              r = ::snprintf(&o[0], o.size()-1, curfmt.c_str(), l);
            } else {
              int l = stack.to<long>(stack_i);
              r = ::snprintf(&o[0], o.size()-1, curfmt.c_str(), l);
            }
            if((r < 0) || (size_t(r) >= o.size()-1)) return stack.throw_exception("Formatting failed");
            out += o.c_str();
          } else if(std::string("fFeEgGaA").find(curfmt.back()) != std::string::npos) {
            if(!stack.is_number(stack_i)) return stack.throw_exception(std::string("No number argument for the format %") + curfmt.back() + " given");
            double d = stack.get<double>(stack_i);
            std::string o(64, '\0');
            auto r = ::snprintf(&o[0], o.size()-1, curfmt.c_str(), d);
            if((r < 0) || (size_t(r) >= o.size()-1)) return stack.throw_exception("Formatting failed");
            out += o.c_str();
          } else {
            return stack.throw_exception(std::string("Unsupported format type '") + curfmt.back() + "'");
          }
        }
      }
      *stream << out;
      return 0;
    }

  protected:

    static int print_to(duktape::api& stack, std::ostream* stream)
    {
      if(!stream) return 0;
      int nargs = stack.top();
      if((nargs == 1) && stack.is_buffer(0)) {
        const char *buf = nullptr;
        duk_size_t sz = 0;
        if((buf = reinterpret_cast<const char*>(stack.get_buffer(0, sz))) && (sz > 0)) {
          (*stream).write(buf, sz);
          (*stream).flush();
        }
      } else if(nargs > 0) {
        (*stream) << stack.to<std::string>(0);
        for(int i=1; i<nargs; i++) {
          std::string sss = stack.to<std::string>(i);
          (*stream) << " " << sss;
        }
        (*stream) << "\n";
        (*stream).flush();
      }
      return 0;
    }

    static void win32vt100(bool enable)
    {
      #ifndef OS_WINDOWS
      (void)enable;
      #else
      {
        #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
          #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
        #endif
        const auto hout = ::GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD conmode = 0;
        if(hout && ::GetConsoleMode(hout, &conmode)) {
          if(!enable) {
            ::SetConsoleMode(hout, conmode & ~(ENABLE_VIRTUAL_TERMINAL_PROCESSING));
          } else {
            ::SetConsoleMode(hout, conmode|ENABLE_VIRTUAL_TERMINAL_PROCESSING);
          }
        }
      }
      #ifdef WITH_WIN32VT100_INPUT_PROCESSING
      {
          #ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
          #define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
        #endif
        const auto hin = ::GetStdHandle(STD_INPUT_HANDLE);
        DWORD conmode = 0;
        if(hin && ::GetConsoleMode(hin, &conmode)) {
          if(!enable) {
            ::SetConsoleMode(hin, conmode & ~(ENABLE_VIRTUAL_TERMINAL_INPUT));
          } else {
            ::SetConsoleMode(hin, conmode|ENABLE_VIRTUAL_TERMINAL_INPUT);
          }
        }
      }
      #endif
      #endif
    }

  public:

    static std::ostream* out_stream;
    static std::ostream* err_stream;
    static std::ostream* log_stream;
    static std::istream* in_stream;
  };

  template <typename T>
  std::ostream* stdio<T>::out_stream = &std::cout;

  template <typename T>
  std::ostream* stdio<T>::err_stream = &std::cerr;

  template <typename T>
  std::ostream* stdio<T>::log_stream = &std::cerr;

  template <typename T>
  std::istream* stdio<T>::in_stream = &std::cin;

}}

namespace duktape { namespace mod {
  using stdio = detail::stdio<void>;
}}

#endif
