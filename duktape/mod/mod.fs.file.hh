/**
 * @file duktape/mod/mod.fs.file.hh
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
 * Optional file object functionality (fs.file).
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
#ifndef DUKTAPE_MOD_FILESYSTEM_FILEOBJECT_HH
#define DUKTAPE_MOD_FILESYSTEM_FILEOBJECT_HH

#include "../duktape.hh"
#include "mod.fs.hh"    /* Required includes defined already there */
#include "mod.stdio.hh" /* printf() */
#include <string>

namespace duktape { namespace detail { namespace filesystem { namespace fileobject { namespace {

  #ifndef OS_WINDOWS
    template <typename=void>
    struct native_file_handling
    {
      using descriptor_type = int;
      static constexpr descriptor_type invalid_descriptor = descriptor_type(-1);

      static bool is_open(descriptor_type fd)
      { return fd >= 0; }

      static void close(descriptor_type fd)
      { if(fd >= 0) ::close(fd); }

      static void open(descriptor_type& fd, std::string path, std::string options)
      {
        // Compose flags and mode, the caller guarantees that the option
        // positions are always the same.
        int flags = O_CLOEXEC; // also use O_NOFOLLOW ??
        if(options[1] == 'w') {
          flags |= ((options[0] == 'r') ? O_RDWR : O_WRONLY);
          if(options[2] == 'a') flags |= O_APPEND;
          if(options[3] != 'e') flags |= O_CREAT;
          if(options[5] == 'x') flags |= O_EXCL;
          if(options[6] != 'p') flags |= O_TRUNC;
        } else {
          flags |= O_RDONLY;
        }
        if(options[7] == 'n') flags |= O_NONBLOCK;
        if(options[8] == 's') flags |= O_SYNC;
        ::mode_t mode = (::mode_t(options[ 9]-'0'))<<6
                      | (::mode_t(options[10]-'0'))<<3
                      | (::mode_t(options[11]-'0'))<<0;
        fd = ::open(path.c_str(), flags, mode);
        if(fd < 0) {
          const char* t = ::strerror(errno);
          fd = invalid_descriptor;
          throw std::runtime_error(std::string("Failed to open '") + path + "' (" + std::string(t?t:"Unspecified error") + ")");
        }
      }

      static std::string read(descriptor_type fd, size_t size, bool& iseof)
      {
        ssize_t n = -1;
        std::string s;
        iseof = false;
        if(!size) {
          int i;
          n = ::read(fd, &i, 0);
        } else {
          s.resize(size);
          if((n=::read(fd, &s[0], s.size())) > 0) {
            s.resize(size_t(n));
          } else if(n == 0) {
            std::string().swap(s);
            iseof = true;
          }
        }
        if(n < 0) {
          switch(errno) {
            case EAGAIN:
            case EINTR:
              std::string().swap(s);
              break;
            case EPIPE:
            case EBADF:
            default: {
              iseof = true;
              const char* msg = ::strerror(errno);
              throw std::runtime_error(std::string("Failed to read file (") + std::string(msg?msg:"Unspecified error") + ")");
            }
          }
        }
        return s;
      }

      static size_t write(descriptor_type fd, std::string& s)
      {
        if(!s.size()) return 0;
        size_t n_written = 0;
        while(s.size() > 0) {
          ssize_t n = ::write(fd, &s[0], s.size());
          if(n >= 0) {
            if(n >= ssize_t(s.size())) {
              std::string().swap(s);
            } else {
              s = s.substr(n); // caller can decide if to call write() in a loop.
            }
            n_written += size_t(n);
          } else {
            switch(errno) {
              case EINTR:
              case EAGAIN:
                return n_written;
              default: {
                const char* msg = ::strerror(errno);
                throw std::runtime_error(std::string("Failed to read file (") + std::string(msg?msg:"Unspecified error") + ")");
              }
            }
          }
        }
        return n_written;
      }

      static size_t tell(descriptor_type fd)
      {
        auto offs = ::lseek(fd, 0, SEEK_CUR);
        if(offs < 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to get file position (") + std::string(msg?msg:"Unspecified error") + ")");
        } else {
          return size_t(offs);
        }
      }

      static size_t seek(descriptor_type fd, size_t pos, int whence)
      {
        auto offs = ::lseek(fd, off_t(pos), (whence==0) ? SEEK_CUR : ((whence>0) ? SEEK_SET : SEEK_END));
        if(offs < 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to set file position (") + std::string(msg?msg:"Unspecified error") + ")");
        } else {
          return size_t(offs);
        }
      }

      static void flush(descriptor_type fd)
      {
        (void)fd; // syscall based file i/o does not need flushing, @see sync
      }

      static size_t size(descriptor_type fd)
      {
        auto offs = ::lseek(fd, 0, SEEK_CUR);
        auto sz = offs;
        if(offs < 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to get file size (") + std::string(msg?msg:"Unspecified error") + ")");
        } else if((sz=::lseek(fd, 0, SEEK_END)) < 0) {
          const char* cmsg = ::strerror(errno);
          auto msg = std::string(cmsg ? cmsg:"Unspecified error");
          ::lseek(fd, offs, SEEK_SET);
          throw std::runtime_error(std::string("Failed to get file size (") + msg + ")");
        } else if(::lseek(fd, offs, SEEK_SET) < 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to set file size (") + std::string(msg?msg:"Unspecified error") + ")");
        } else {
          return size_t(sz);
        }
      }

      static struct ::stat stat(descriptor_type fd)
      {
        struct ::stat st;
        if(::fstat(fd, &st) != 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to set file stat (") + std::string(msg?msg:"Unspecified error") + ")");
        } else {
          return st;
        }
      }

      static void sync(descriptor_type fd, bool content_only)
      {
        int r = content_only ? ::fdatasync(fd) : ::fsync(fd);
        if(r < 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to sync file (") + std::string(msg?msg:"Unspecified error") + ")");
        }
      }

      static bool lock(descriptor_type fd, char access)
      {
        int r = ::flock(fd, access=='s' ? LOCK_SH : LOCK_EX);
        if(r < 0) {
          switch(errno) {
            case EAGAIN:
            case EINTR:
              return false;
            default: {
              const char* msg = ::strerror(errno);
              throw std::runtime_error(std::string("Failed to lock file (") + std::string(msg?msg:"Unspecified error") + ")");
            }
          }
        }
        return true;
      }

      static void unlock(descriptor_type fd)
      {
        ::flock(fd, LOCK_UN);
      }

    };
  #endif

  #ifdef OS_WINDOWS
    template <typename=void>
    struct native_file_handling
    {
      using descriptor_type = long;
      static constexpr descriptor_type invalid_descriptor = descriptor_type(-1);
      static HANDLE fd2handle(descriptor_type fd)  { return ((HANDLE)((LONG_PTR)fd)); }
      static descriptor_type handle2fd(HANDLE hnd) { return descriptor_type(((long)((LONG_PTR)hnd))); }

      static std::string error_message(DWORD eno)
      {
        std::string s(256,0);
        size_t n = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, eno, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &s[0], s.size()-1, nullptr);
        if(!n) return std::string();
        s.resize(n);
        return s;
      }

      static std::string error_message()
      { return error_message(::GetLastError()); }

      static bool is_open(descriptor_type fd)
      { return fd != invalid_descriptor; }

      static void close(descriptor_type fd)
      { if(fd != invalid_descriptor) (void)::CloseHandle(fd2handle(fd)); }

      static void open(descriptor_type& fd, std::string path, std::string options)
      {
        DWORD access_mode = GENERIC_READ;
        DWORD share_mode = FILE_SHARE_READ; //|FILE_SHARE_WRITE;
        DWORD creation_disposition = OPEN_EXISTING;
        DWORD flags_attr = FILE_ATTRIBUTE_NORMAL;
        const bool w=options.find('w') != options.npos;
        const bool r=options.find('r') != options.npos;
        const bool a=options.find('a') != options.npos;
        const bool e=options.find('e') != options.npos;
        const bool x=options.find('x') != options.npos;
        const bool p=options.find('p') != options.npos;
        if(w || a) {
          access_mode = r ? GENERIC_READ|GENERIC_WRITE : GENERIC_WRITE;
          if(e) {
            creation_disposition = p ? OPEN_EXISTING : TRUNCATE_EXISTING;
          } else {
            creation_disposition = x ? CREATE_NEW : ( p ? OPEN_ALWAYS : CREATE_ALWAYS);
          }
        } else {
          access_mode = GENERIC_READ;
          creation_disposition = OPEN_EXISTING;
        }
        fd = handle2fd(::CreateFileA(path.c_str(), access_mode, share_mode, nullptr, creation_disposition, flags_attr, nullptr));
        if(fd == invalid_descriptor) {
          throw std::runtime_error(std::string("Failed to open '") + path + "' (" + error_message() + ")");
        }
        if(a) {
          if(!::SetFilePointerEx(fd2handle(fd), LARGE_INTEGER(), nullptr, FILE_END)) {
            throw std::runtime_error(std::string("Failed to set file position (") + error_message() + ")");
          }
        }
      }

      static std::string read(descriptor_type fd, size_t max_size, bool& iseof)
      {
        iseof = false;
        std::string data;
        if((fd == invalid_descriptor) || (!max_size)) { iseof=true; return data; }
        constexpr auto size_limit = size_t(std::numeric_limits<ssize_t>::max()-4096);
        if(max_size > size_limit) max_size = size_limit; // as many as possible / limit
        for(;;) {
          char buffer[4096+1];
          DWORD n_read=0, size=sizeof(buffer)-1;
          if(size + data.size() >= max_size) size = max_size - data.size();
          if(!size) return data;
          if(!::ReadFile(fd2handle(fd), buffer, size, &n_read, nullptr)) {
            switch(::GetLastError()) {
              case ERROR_MORE_DATA:
                break;
              case ERROR_HANDLE_EOF:
              case ERROR_BROKEN_PIPE:
              case ERROR_PIPE_NOT_CONNECTED:
                iseof = true;
              case ERROR_PIPE_BUSY:
              case ERROR_NO_DATA:
                return data;
              case ERROR_INVALID_HANDLE:
              default:
                iseof = true;
                throw std::runtime_error(std::string("Failed to read file (") + error_message() + ")");
            }
          } else if(!n_read) {
            return data;
          } else {
            buffer[n_read] = 0;
            data += std::string(buffer, buffer+n_read);
          }
        }
        return data;
      }

      static size_t write(descriptor_type fd, std::string& data)
      {
        bool keep_writing = true;
        size_t n_written = 0;
        while(!data.empty() && keep_writing) {
          DWORD n = 0;
          DWORD n_towrite = data.size() > 4096 ? 4096 : data.size();
          if(!::WriteFile(fd2handle(fd), data.data(), n_towrite, &n, nullptr)) {
            switch(::GetLastError()) {
              case ERROR_PIPE_BUSY:
              case ERROR_NO_DATA:
                keep_writing = false;
                break;
              case ERROR_BROKEN_PIPE:
              case ERROR_PIPE_NOT_CONNECTED:
              case ERROR_INVALID_HANDLE:
              default:
                throw std::runtime_error(std::string("Failed to write file (") + error_message() + ")");
            }
          }
          if(n) {
            if(n >= data.size()) {
              n_written += data.size(); // ensures that the caller does not get higher values (for whatever reason)
              data.clear();
              keep_writing = false;
            } else {
              n_written += n;
              data = data.substr(n);
            }
          }
          if(n < n_towrite) {
            keep_writing = false;
          }
        }
        return n_written;
      }

      static bool is_eof(descriptor_type fd)
      {
        // unfortunately no way to use readfile without actually reading
        LARGE_INTEGER pos, size;
        size.QuadPart = 0;
        return (::SetFilePointerEx(fd2handle(fd), size, &pos, FILE_CURRENT))
            && (GetFileSizeEx(fd2handle(fd), &size))
            && (pos.QuadPart >= size.QuadPart);
      }

      static size_t tell(descriptor_type fd)
      {
        LARGE_INTEGER offs = LARGE_INTEGER();
        if(!::SetFilePointerEx(fd2handle(fd), LARGE_INTEGER(), &offs, FILE_CURRENT)) {
          throw std::runtime_error(std::string("Failed to get file position (") + error_message() + ")");
        } else {
          return size_t(offs.QuadPart);
        }
      }

      static size_t seek(descriptor_type fd, size_t pos, int whence)
      {
        LARGE_INTEGER offs = LARGE_INTEGER();
        LARGE_INTEGER soffs; soffs.QuadPart = pos;
        if(!::SetFilePointerEx(fd2handle(fd), soffs, &offs, (whence==0) ? FILE_CURRENT : ((whence>0) ? FILE_BEGIN : FILE_END))) {
          throw std::runtime_error(std::string("Failed to set file position (") + error_message() + ")");
        } else {
          return size_t(offs.QuadPart);
        }
      }

      static void flush(descriptor_type fd)
      {
        ::FlushFileBuffers(fd2handle(fd));
      }

      static size_t size(descriptor_type fd)
      {
        LARGE_INTEGER offs = LARGE_INTEGER();
        if(!::GetFileSizeEx(fd2handle(fd), &offs)) {
          throw std::runtime_error(std::string("Failed to get file size (") + error_message() + ")");
        } else {
          return size_t(offs.QuadPart);
        }
      }

      static struct ::stat stat(descriptor_type fd)
      {
        std::string path;
        {
          char cpath[MAX_PATH+1];
          if(!::GetFinalPathNameByHandleA(fd2handle(fd), cpath, MAX_PATH, FILE_NAME_OPENED|VOLUME_NAME_DOS)) {
            throw std::runtime_error(std::string("Failed to get file stat (") + error_message() + ")");
          }
          cpath[MAX_PATH] = '\0';
          int i=0;
          while(cpath[i] && cpath[i] != ':') ++i;
          if(cpath[i] == ':' && (i>0)) --i; else i=0;
          path = &cpath[i];
        }
        struct ::stat st;
        if(::stat(path.c_str(), &st) != 0) {
          const char* msg = ::strerror(errno);
          throw std::runtime_error(std::string("Failed to get file stat (") + std::string(msg?msg:"Unspecified error") + ")");
        } else {
          return st;
        }
      }

      static void sync(descriptor_type fd, bool content_only)
      { (void)content_only; flush(fd); }

      static bool lock(descriptor_type fd, char access)
      {
        return ::LockFile(fd2handle(fd), 0u,0u, DWORD(0xffffffffu),DWORD(0x7fffffffu));
        (void)access; // LockFileEx could lock exclusively, but no OVERLAPPED hazzle for now.
      }

      static void unlock(descriptor_type fd)
      { ::UnlockFile(fd2handle(fd), 0u,0u, DWORD(0xffffffffu),DWORD(0x7fffffffu)); }

    };
  #endif

  using nfh = native_file_handling<>;

  /**
   * Retrieves descriptor and options from a file object. Returns true
   * on success. On fail an exception is placed on the stack.
   *
   * @param duktape::api& stack
   * @param api::index_t obj_index
   * @param nfh::descriptor_type& fd
   * @param std::string& opts
   * @return bool
   */
  template <typename=void>
  bool get_file_object_data(duktape::api& stack, api::index_t obj_index, nfh::descriptor_type& fd, std::string& opts)
  {
    int top = stack.top();
    if(  (!stack.is_object(obj_index))
      || (!stack.get_prop_string_hidden(obj_index, "fd"))
      || (!stack.get_prop_string_hidden(obj_index, "options"))
    ) {
      fd = nfh::invalid_descriptor;
      opts.clear();
      stack.top(top);
      stack.throw_exception("File methods have to be called on a file objects.");
      return false;
    } else {
      fd = stack.get<nfh::descriptor_type>(-2);
      opts = stack.get<std::string>(-1);
      stack.top(top);
      opts.resize(12,'-');
      return true;
    }
  }

  /**
   * Retrieves the file descriptor from a file object. Returns true
   * on success. On fail an exception is placed on the stack.
   *
   * @param duktape::api& stack
   * @param api::index_t obj_index
   * @param nfh::descriptor_type& fd
   * @return bool
   */
  template <typename=void>
  bool get_file_object_data(duktape::api& stack, api::index_t obj_index, nfh::descriptor_type& fd)
  { std::string o; return get_file_object_data(stack, obj_index, fd, o); }

  /**
   * Implementation used in the constructor and file.open().
   *
   * @param duktape::api& stack
   * @return int
   */
  template <typename PathAccessor>
  int file_open_stack(duktape::api& stack)
  {
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    std::string path = stack.get<std::string>(1);
    std::string options = stack.get<std::string>(2);
    if(!get_file_object_data(stack, 0, fd)) return 0;
    if(nfh::is_open(fd)) nfh::close(fd);
    stack.push(nfh::invalid_descriptor);
    stack.put_prop_string_hidden(0, "fd");
    stack.push("");
    stack.put_prop_string_hidden(0, "path");
    stack.push("");
    stack.put_prop_string_hidden(0, "options");
    stack.push(true);
    stack.put_prop_string_hidden(0, "feof");

    fd = nfh::invalid_descriptor;

    // Options are sanatized, reordered and parsed in a more detailed
    // way than ANSI to enable additional functionality.
    // Also options are passed to the native file handling functions
    // as string - this is a tradeoff between option readability and
    // performance.
    {
      bool r=false,w=false,a=false,u=false,e=false,b=false;
      bool x=false,n=false,p=false,s=false;
      std::string mod; // mode when creating, only if OS/FS supports it
      for(auto c:options) {
        switch(c) {
          case 'R': case 'r': r = true; break; // read
          case 'W': case 'w': w = true; break; // write
          case 'A': case 'a': a = true; break; // append
          case 'B': case 'b': b = true; break; // binary
          case 'T': case 't': b = false;break; // aka "text" on some platforms
          case 'X': case 'x': x = true; break; // exclusive (fail create if existing)
          case '+': u = true; break; // update, for ANSI-C corrections
          case 'E': case 'e': e = true; break; // existing only (no-create)
          case 'C': case 'c': e = false;break; // create included as valid option
          case 'P': case 'p': p = true; break; // preserve file contents on write.
          case 'S': case 's': s = true; break; // sync (synchronised)
          case 'N': case 'n': n = true; break; // nonblocking
          default:
            if((c >= '0') && (c <= '7')) {
              mod += c;
            } else if(c=='-' || c==',' || c==';' || c==' ') {
              // ignore option separators
            } else {
              return stack.throw_exception(std::string("Invalid file open option '") + c + "'");
            }
        }
      }
      if((mod.size() == 4) && (mod[0] == '0')) {
        mod = mod.substr(1); // explicit octal representaion like "0644" are valid
      }
      if(mod.empty()) {
        mod = "644"; // default mode, affected by umask
      } else if(mod.size() != 3) {
        // intentionally the sticky but and suid,sgid bits are not settable.
        return stack.throw_exception(std::string("Invalid file creation mode '") + mod + "'");
      }
      // recap of ansi:
      // r = read,nocreate      -> identical in options
      // w = write,create,reset -> identical
      // a = create,append      -> identical
      // b = binary             -> identical (read returns buffer)
      // x = exclusive          -> identical (fail if file already exists)
      if(u) {
        if(a) {
          // a+= read,create,append. seek/tell only input
          r = true;
        } else if(r) {
          // r+ = read,write,no-create,preserve
          w = true;
          p = true;
          e = true;
        } else if(w) {
          // w+ = read,write,create,no-preserve
          r = true;
        }
      }
      if((!w && !a) || e) x = false;
      if(a) p = true;
      // Compose option flags, assuring that positions are preserved.
      options = std::string(12, '-');
      if(r) options[0] = 'r';
      if(w) options[1] = 'w';
      if(a) options[2] = 'a';
      if(e) options[3] = 'e';
      if(b) options[4] = 'b';
      if(x) options[5] = 'x';
      if(p) options[6] = 'p';
      if(n) options[7] = 'n';
      if(s) options[8] = 's';
      options[9] = mod[0];
      options[10] = mod[1];
      options[11] = mod[2];
    }
    nfh::open(fd, PathAccessor::to_sys(path), options);
    stack.push(fd);
    stack.put_prop_string_hidden(0, "fd");
    stack.push(path);
    stack.put_prop_string_hidden(0, "path");
    stack.push(options);
    stack.put_prop_string_hidden(0, "options");
    stack.push(false);
    stack.put_prop_string_hidden(0, "feof");
    return 0;
  }

  /**
   * Finalizer ("destructor") of the ECMA fs.file
   * object. C callback closing the file.
   *
   * @param duk_context *ctx
   * @return duk_ret_t
   */
  template <typename PathAccessor>
  duk_ret_t file_finalizer(duk_context *ctx)
  {
    // "Destructors must not throw ;-)" Similar rules with finalizers
    // expected. Duktape ignores error objects, and c++ exceptions
    // should also be ignored here. However, if explicitly an engine_error
    // is thrown we have to forward this because it explicitly means that
    // the Duktape engine heap might be corrupt or the like.
    try {
      duktape::api stack(ctx);
      nfh::descriptor_type fd = nfh::invalid_descriptor;
      if(stack.is_object(0) && stack.get_prop_string_hidden(0, "fd")) {
        fd = stack.get<nfh::descriptor_type>(1);
        if((fd != nfh::invalid_descriptor) && nfh::is_open(fd)) nfh::close(fd);
        stack.push(nfh::invalid_descriptor);
        stack.put_prop_string_hidden(0, "fd");
        stack.top(1);
      }
    } catch(const duktape::engine_error&) {
      throw;
    } catch(const duktape::exit_exception&) {
      // ignore
    } catch(const std::exception&) {
      // ignore
    }
    return 0;
  }

  template <typename PathAccessor>
  int file_constructor(duktape::api& stack)
  {
    // Intentionally not checking for stack.is_constructor_call()
    stack.push_object();
    stack.insert(0);
    stack.get_global_string("fs");
    stack.get_prop_string(-1, "file");
    stack.swap(-1,-2);
    stack.pop();
    stack.get_prototype(-1);
    stack.swap(-1,-2);
    stack.pop();
    stack.set_prototype(0);
    stack.push_c_function(file_finalizer<PathAccessor>, 1);
    stack.set_finalizer(0);
    stack.push(nfh::invalid_descriptor);
    stack.put_prop_string_hidden(0, "fd");
    stack.push("");
    stack.put_prop_string_hidden(0, "path");
    stack.push("");
    stack.put_prop_string_hidden(0, "options");
    stack.push("");
    stack.put_prop_string(0, "newline");
    if(!stack.is_undefined(1)) file_open_stack<PathAccessor>(stack);
    if(stack.is_error(-1)) return 0;
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_open(duktape::api& stack)
  {
    stack.push_this();
    stack.insert(0);
    file_open_stack<PathAccessor>(stack);
    if(stack.is_error(-1)) return 0;
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_close(duktape::api& stack)
  {
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    stack.top(0);
    stack.push_this();
    if(!get_file_object_data(stack, 0, fd)) return 0;
    nfh::close(fd);
    stack.push(nfh::invalid_descriptor);
    stack.put_prop_string_hidden(0, "fd");
    stack.top(1);
    stack.push(true);
    stack.put_prop_string_hidden(0, "feof");
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_closed(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.pop();
    stack.push(!nfh::is_open(fd));
    return 1;
  }

  template <typename PathAccessor>
  int file_opened(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.pop();
    stack.push(nfh::is_open(fd));
    return 1;
  }

  template <typename PathAccessor>
  int file_eof(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    bool iseof = true;
    if(stack.get_prop_string_hidden(0, "feof")) {
      iseof = stack.get<bool>(-1);
    }
    stack.top(0);
    stack.push(iseof);
    return 1;
  }

  template <typename PathAccessor>
  int file_read(duktape::api& stack)
  {
    auto max_size = stack.to<int>(0);
    stack.top(0);
    stack.push_this();
    std::string openopts;
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd, openopts)) return 0;
    stack.pop();
    std::string out;
    bool iseof = false;
    if(max_size <= 0) {
      max_size = 4096;
      std::string s = nfh::read(fd, max_size, iseof);
      while(s.size() > 0) {
        out.append(s);
        s = nfh::read(fd, max_size, iseof);
      }
    } else {
      out = nfh::read(fd, max_size, iseof);
    }
    stack.push_this();
    stack.push(iseof);
    stack.put_prop_string_hidden(0, "feof");
    stack.top(0);
    if(out.empty() && iseof) {
      return 0;
    } else if(openopts[4] != 'b') {
      stack.push(out);
    } else {
      void* buf = stack.push_dynamic_buffer(out.size());
      if(!buf) {
        return stack.throw_exception("File reading failed: no memory for buffer object.");
      } else {
        char* p = reinterpret_cast<char*>(buf);
        for(auto e:out) *p++ = e; // std::copy
      }
    }
    return 1;
  }

  template <typename PathAccessor>
  int file_readln(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    std::string openopts;
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd, openopts)) return 0;
    if(openopts.find('n') != openopts.npos) {
      return stack.throw_exception("You cannot use the file printf() method in combination with nonblocking "
              "I/O because it is not guaranteed entirely written, and you do not have the buffered formatted "
              "output.");
    }
    std::string nl;
    stack.get_prop_string(0, "newline");
    if(stack.is_string(-1)) nl = stack.get<std::string>(-1);
    bool autonl = false;
    if(nl.empty()) {
      autonl = true;
      nl = "\n";
    }
    stack.top(0);
    char lastchar = nl.back();
    bool iseof = false;
    std::string out;
    std::string s = nfh::read(fd, 1, iseof);
    while(s.size() > 0) {
      out.append(s);
      if(s.back() == lastchar) {
        if(autonl) {
          out.pop_back(); // LF
          if((!out.empty()) && (out.back() == '\r')) out.pop_back(); // CR
          break;
        } else {
          auto p = out.rfind(nl);
          if(p != out.npos) {
            out.resize(p);
            break;
          }
        }
      }
      s = nfh::read(fd, 1, iseof);
    }
    stack.push_this();
    stack.push(iseof);
    stack.put_prop_string_hidden(0, "feof");
    stack.top(0);
    if(out.empty() && iseof) {
      return 0;
    } else {
      stack.push(out);
      return 1;
    }
  }

  template <typename PathAccessor>
  int file_write(duktape::api& stack)
  {
    std::string data = stack.is_buffer(0) ? stack.buffer<std::string>(0) : stack.to<std::string>(0);
    stack.top(0);
    stack.push_this();
    std::string openopts;
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd, openopts)) return 0;
    stack.pop();
    stack.push(nfh::write(fd, data));
    return 1;
  }

  template <typename PathAccessor>
  int file_writeln(duktape::api& stack)
  {
    std::string data = stack.to<std::string>(0);
    stack.top(0);
    stack.push_this();
    std::string openopts;
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd, openopts)) return 0;
    if(openopts.find('n') != openopts.npos) {
      return stack.throw_exception("You cannot use the file printf() method in combination with nonblocking "
              "I/O because it is not guaranteed entirely written, and you do not have the buffered formatted "
              "output.");
    }
    std::string nl;
    stack.get_prop_string(0, "newline");
    if(stack.is_string(-1)) nl = stack.get<std::string>(-1);
    if(nl.empty()) {
      #ifdef OS_WINDOWS
      nl = "\r\n";
      #else
      nl = "\n";
      #endif
    }
    data += nl;
    stack.top(0);
    nfh::write(fd, data);
    if(!data.empty()) {
      return stack.throw_exception("Not all data written to file");
    }
    stack.push(true);
    return 1;
  }

  template <typename PathAccessor>
  int file_printf(duktape::api& stack)
  {
    std::string data;
    {
      std::stringstream ss;
      duktape::mod::stdio::printf_to(stack, &ss);
      if(stack.is_error(-1)) return 0;
      data = ss.str();
    }
    stack.top(0);
    stack.push_this();
    std::string openopts;
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd, openopts)) return 0;
    stack.pop();
    if(openopts.find('n') != openopts.npos) {
      // we better talk about that issue directly
      return stack.throw_exception("You cannot use the file printf() method in combination with nonblocking "
              "I/O because it is not guaranteed entirely written, and you do not have the buffered formatted "
              "output.");
    }
    nfh::write(fd, data);
    if(!data.empty()) {
      return stack.throw_exception("Not all data written to file");
    }
    stack.top(0);
    stack.push(true);
    return 1;
  }

  template <typename PathAccessor>
  int file_tell(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.pop();
    stack.push(nfh::tell(fd));
    return 1;
  }

  template <typename PathAccessor>
  int file_seek(duktape::api& stack)
  {
    long pos = stack.to<long>(0);
    std::string whence = stack.is_undefined(1) ? std::string() : stack.to<std::string>(1);
    std::transform(whence.begin(), whence.end(), whence.begin(), ::tolower);
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.pop();
    int i_whence = 0;
    if(pos < 0) {
      return stack.throw_exception("Invalid negative seek position given");
    } else if(whence.empty() || (whence == "set") || (whence == "seek_set") || (whence == "begin") || (whence == "start") ) {
      i_whence = 1;
    } else if(whence.empty() || (whence == "end") || (whence == "seek_end")) {
      i_whence = -1;
    } else if(whence.empty() || (whence == "cur") || (whence == "seek_cur") || (whence == "current")) {
      i_whence = 0;
    } else {
      return stack.throw_exception("Invalid seek whence given (''|'set'|'begin'|'start' -> begin, 'end' -> end, 'cur'|'current' -> current)");
    }
    stack.push(nfh::seek(fd, pos, i_whence));
    return 1;
  }

  template <typename PathAccessor>
  int file_size(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.pop();
    stack.push(nfh::size(fd));
    return 1;
  }

  template <typename PathAccessor>
  int file_stat(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    stack.get_prop_string_hidden(0, "path");
    std::string path = stack.to<std::string>(-1);
    stack.top(1);
    struct ::stat st = nfh::stat(fd);
    stack.require_stack_top(5);
    return filesystem::basic::push_filestat<PathAccessor>(stack, st, path);
  }

  template <typename PathAccessor>
  int file_flush(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    nfh::flush(fd);
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_sync(duktape::api& stack)
  {
    bool content_only = stack.to<bool>(0);
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    nfh::sync(fd, content_only);
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_lock(duktape::api& stack)
  {
    char access = ::tolower(stack.to<std::string>(0).c_str()[0]);
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    if(!nfh::lock(fd, access)) {
      stack.throw_exception("Failed to lock file.");
    }
    stack.top(1);
    return 1;
  }

  template <typename PathAccessor>
  int file_unlock(duktape::api& stack)
  {
    stack.top(0);
    stack.push_this();
    nfh::descriptor_type fd = nfh::invalid_descriptor;
    if(!get_file_object_data(stack, 0, fd)) return 0;
    nfh::unlock(fd);
    stack.top(1);
    return 1;
  }

}}}}}

namespace duktape { namespace mod { namespace filesystem { namespace fileobject {

  using namespace ::duktape::detail::filesystem;
  using namespace ::duktape::detail::filesystem::fileobject;

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename PathAccessor=path_accessor<std::string>>
  static void define_in(duktape::engine& js)
  {
    using namespace std;
    {
      auto flags = js.define_flags();
      js.define_flags(duktape::engine::defflags::restricted);
      js.define("fs");

      #if(0 && JSDOC)
      /**
       * File object constructor, creates a fs.file object when
       * invoked with the `new` keyword. Optionally, path and openmode
       * can be specified to directly open the file. See `File.open()`
       * for details.
       *
       * @constructor
       * @throws {Error}
       * @param {string} [path]
       * @param {string} [openmode]
       * @return {fs.file}
       */
      fs.file = function(path, openmode) {};
      #endif
      js.define("fs.file", file_constructor<PathAccessor>, 2);

      #if(0 && JSDOC)
      /**
       * Opens a file given the path and corresponding "open mode". The
       * mode is a string defining flags from adding or omitting characters,
       * where the characters are (with exception of mode "a+") compliant
       * to the ANSI C `fopen()` options. Additional characters enable
       * further file operations and functionality. The options are:
       *
       * - "r": Open for `r`eading. The file must exist. Set the position
       *        to the beginning of the file (ANSI C).
       *
       * - "w": Open for `w`riting. Create if the file is not existing yet,
       *        truncate the file (discard contents). Start position is the
       *        beginning of the file. (ANSI C).
       *
       * - "a": Open for `a`ppending. Create if the file is not existing yet,
       *        set the write position to the end of the file. (ANSI C).
       *
       * - "r+": Open for `r`eading and writing, the file must exist. The
       *        read/write position is set to the beginning of the file
       *        (ANSI C).
       *
       * - "w+": Open for `w`riting and reading. Create if the file is not
       *        existing yet, truncate the file (discard contents). Start
       *        position is the beginning of the file. (ANSI C).
       *
       * - "a+": Open for `a`ppending and reading. Create if the file is not
       *        existing yet. Start position is the beginning of the file.
       *        Warning: The write position is guaranteed to be the end of
       *        the file, but ANSI C specifies that the read position is
       *        separately handled from the write position, and the write
       *        position is implicitly always the end of the file. This is
       *        NOT guaranteed in this implementation. When reading you must
       *        `seek()` to the read position yourself.
       *
       * - "b": Optional flag: Open to read/write `b`inary. Reading will return
       *        a `Buffer` in this case. Writing a `Buffer` will write binary
       *        data.
       *
       * - "t": Optional flag: Open to read/write `t`ext (in contrast to binary,
       *        this is already the default and only accepted because it is
       *        known on some platforms).
       *
       * - "x": Optional flag: E`x`clusive creation. This causes opening for write
       *        to fail with an exception if the file already exists. You can
       *        use this to prevent accidentally overwriting existing files.
       *
       * - "e": Optional flag: Open `e`xisting files only. This is similar to "r+"
       *        and can be used for higher verbosity or ensuring that no file will
       *        be created. The open() call fails with an exception if the file
       *        does not exist.
       *
       * - "c": Optional flag: `C`reate file if not existing. This is the explicit
       *        specification of the default open for write/append behaviour. This
       *        flag implicitly resets the `e` flag.
       *
       * - "p": Optional flag: `P`reserve file contents. This is an explicit order
       *        that opening for write does not discard the current file contents.
       *
       * - "s": Optional flag: `S`ync. Means that file operations are implicitly
       *        forced to be read from / written to the disk or device. Ignored if
       *        the platform does not support it.
       *
       * - "n": Optional flag: `N`onblocking. Means that read/write operations that
       *        would cause the function to "sleep" until data are available return
       *        directly with empty return value. Ignored if the platform does not
       *        support it (or not implemented for the platform).
       *
       * The flags (characters) are not case sensitive, so `file.open(path, "R")` and
       * `file.open(path, "r")` are identical.
       *
       * Although the ANSI open flags are supported it is at a second glance more explicit
       * to use the optional flags in combination with "r" and "w" or "a", e.g.
       *
       * - `file.open(path, "rwcx")`         --> open for read/write, create if not yet existing,
       *                                         and only if not yet existing.
       *
       * - `file.open(path, "wep")`          --> open for write, only existing, preserve contents.
       *
       * - `file.open("/dev/cdev", "rwens")` --> open a character device for read/write, must exist,
       *                                         nonblocking, sync.
       *
       * The function returns the reference to `this`.
       *
       * @throws {Error}
       * @param {string} [path]
       * @param {string} [openmode]
       * @return {fs.file}
       */
      fs.file.open = function(path, openmode) {};
      #endif
      js.define("fs.file.prototype.open", file_open<PathAccessor>, 2);

      #if(0 && JSDOC)
      /**
       * Closes a file. Returns `this` reference.
       *
       * @return {fs.file}
       */
      fs.file.close = function() {};
      #endif
      js.define("fs.file.prototype.close", file_close<PathAccessor>,0);

      #if(0 && JSDOC)
      /**
       * Returns true if a file is closed.
       *
       * @return {boolean}
       */
      fs.file.closed = function() {};
      #endif
      js.define("fs.file.prototype.closed", file_closed<PathAccessor>,0);

      #if(0 && JSDOC)
      /**
       * Returns true if a file is opened.
       *
       * @return {boolean}
       */
      fs.file.opened = function() {};
      #endif
      js.define("fs.file.prototype.opened", file_opened<PathAccessor>,0);

      #if(0 && JSDOC)
      /**
       * Returns true if the end of the file is reached. This
       * is practically interpreted as:
       *
       *  - when the file or pipe signals EOF,
       *  - when the file is not opened,
       *  - when a pipe is not connected or broken
       *
       * @return {boolean}
       */
      fs.file.eof = function() {};
      #endif
      js.define("fs.file.prototype.eof", file_eof<PathAccessor>,0);

      #if(0 && JSDOC)
      /**
       * Reads data from a file, where the maximum number of bytes
       * to read can be specified. If `max_size` is not specified,
       * then as many bytes as possible are read (until EOF, until
       * error or until the operation would block).
       *
       * Note: If the end of the file is reached, the `eof()`
       *       method will return true and the `read()` method
       *       will return `undefined` as indication.
       *
       * @throws {Error}
       * @param {number} [max_bytes]
       * @return {string|buffer}
       */
      fs.file.read = function(max_size) {};
      #endif
      js.define("fs.file.prototype.read", file_read<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * Read string data from the opened file and return when
       * detecting a newline character. The newline character
       * defaults to the operating system newline and can be
       * changed for the file by setting the `newline` property
       * of the file (e.g. `myfile.newline = "\r\n"`).
       *
       * Note: This function cannot be used in combination
       * with the nonblocking I/O option.
       *
       * Note: If the end of the file is reached, the `eof()`
       *       method will return true and the `read()` method
       *       will return `undefined` to indicate that no
       *       empty line was read but nothing at all.
       *
       * Note: This function is slower than `fs.file.read()` or
       *       `fs.readfile()` because it has to read unbuffered
       *       char-by-char. If you intend to read an entire file
       *       and filter the lines prefer `fs.readfile()` with
       *       line processing callback.
       *
       * @throws {Error}
       * @return {string}
       */
      fs.file.readln = function() {};
      #endif
      js.define("fs.file.prototype.readln", file_readln<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * Write data to a file, returns the number of bytes written.
       * Normally all bytes are written, except if nonblocking i/o
       * was specified when opening the file.
       *
       * @throws {Error}
       * @param {string|buffer} data
       * @return {number}
       */
      fs.file.write = function(data) {};
      #endif
      js.define("fs.file.prototype.write", file_write<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * Write string data to a file and implicitly append
       * a newline character. The newline character defaults
       * to the operating system newline (Windows CRLF, else
       * LF, no old Mac CR). This character can be changed
       * for the file by setting the `newline` property of
       * the file (e.g. myfile.newline = "\r\n").
       * Note: This function cannot be used in combination
       * with the nonblocking I/O option. The method throws
       * an exception if not all data could be written.
       *
       * @throws {Error}
       * @param {string} data
       */
      fs.file.writeln = function(data) {};
      #endif
      js.define("fs.file.prototype.writeln", file_writeln<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * C style formatted output to the opened file.
       * The method is used identically to `printf()`.
       * Note: This function cannot be used in combination
       * with the nonblocking I/O option. The method
       * throws an exception if not all data could be
       * written.
       *
       * @throws {Error}
       * @param {string} format
       * @param {...*} args
       */
      fs.file.printf = function(format, args) {};
      #endif
      js.define("fs.file.prototype.printf", file_printf<PathAccessor>);

      #if(0 && JSDOC)
      /**
       * Flushes the file write buffer. Ignored on platforms where this
       * is not required. Returns reference to `this`.
       *
       * @throws {Error}
       * @return {fs.file}
       */
      fs.file.flush = function() {};
      #endif
      js.define("fs.file.prototype.flush", file_flush<PathAccessor>, 0);

      #if(0 && JSDOC)
      /**
       * Returns the current file position.
       *
       * @throws {Error}
       * @return {number}
       */
      fs.file.tell = function() {};
      #endif
      js.define("fs.file.prototype.tell", file_tell<PathAccessor>, 0);

      #if(0 && JSDOC)
      /**
       * Sets the new file position (read and write). Returns the
       * actual position (from the beginning of the file) after the
       * position was set. The parameter whence specifies from where
       * the position shall be set:
       *
       *  - "begin" (or "set"): From the beginning of the file (SEEK_SET)
       *  - "end"             : From the end of the file backward (SEEK_END)
       *  - "current" ("cur") : From the current position forward (SEEK_CUR)
       *
       * @throws {Error}
       * @param {number} position
       * @param {string} [whence=begin]
       * @return {number}
       */
      fs.file.seek = function(position, whence) {};
      #endif
      js.define("fs.file.prototype.seek", file_seek<PathAccessor>, 2);

      #if(0 && JSDOC)
      /**
       * Returns the current file size in bytes.
       *
       * @throws {Error}
       * @return {number}
       */
      fs.file.size = function() {};
      #endif
      js.define("fs.file.prototype.size", file_size<PathAccessor>, 0);

      #if(0 && JSDOC)
      /**
       * Returns details about the file including path, size, mode
       * etc. @see fs.stat() for details.
       *
       * @throws {Error}
       * @return {object}
       */
      fs.file.stat = function() {};
      #endif
      js.define("fs.file.prototype.stat", file_stat<PathAccessor>, 0);

      #if(0 && JSDOC)
      /**
       * Forces the operating system to write the file to a remote device
       * or block device (disk). This is a Linux/UNIX explicit variant of
       * flush. However, flush is not sync, and sync is only needed in
       * special situations. On operating systems that cannot sync this
       * function is an alias of `fs.flush()`. Returns reference to `this`.
       *
       * The optional argument `no_metadata` specifies (when true) that
       * only the contents of the file shall be synced, but not the file
       * system meta information.
       *
       * @throws {Error}
       * @param {boolean} [no_metadata]
       * @return {fs.file}
       */
      fs.file.sync = function() {};
      #endif
      js.define("fs.file.prototype.sync", file_sync<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * Locks the file. By default exclusively, means no other process
       * can read or write. Optionally the file can be locked exclusively
       * or shared depending on the `access` argument:
       *
       *  - "x", "" : Exclusive lock
       *  - "s"     : Shared lock
       *
       * @throws {Error}
       * @param {string} access
       * @return {fs.file}
       */
      fs.file.lock = function(access) {};
      #endif
      js.define("fs.file.prototype.lock", file_lock<PathAccessor>, 1);

      #if(0 && JSDOC)
      /**
       * Unlocks a previously locked file. Ignored if the platform does not
       * support locking.
       *
       * @throws {Error}
       * @return {fs.file}
       */
      fs.file.unlock = function() {};
      #endif
      js.define("fs.file.prototype.unlock", file_unlock<PathAccessor>, 0);


      js.define_flags(flags);
    }
    {
      duktape::stack_guard sg(js.stack());
      duktape::api& stack = js.stack();
      stack.require_stack(5);
      stack.get_global_string("fs");
      stack.get_prop_string(-1, "file");
      stack.get_prop_string(-1, "prototype");
      stack.set_prototype(-2);
    }
  }

}}}}

#endif
