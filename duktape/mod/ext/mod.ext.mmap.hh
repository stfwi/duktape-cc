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
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional mmap (memory mapped file) functionality.
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
#ifndef DUKTAPE_MOD_EXT_MEMORY_MAPPED_FILE_HH
#define DUKTAPE_MOD_EXT_MEMORY_MAPPED_FILE_HH

#include "../mod.sys.os.hh"
#if defined(OS_LINUX)
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
#elif defined(OS_WINDOWS)
  #include <windows.h>
#else
  #error "Unsupported OS."
#endif
#include <type_traits>
#include <limits>
#include <cstdint>

/* From sw::ipc::memory_mapped_file, slightly updated to 17 */
namespace sw { namespace ipc {

  template<typename ValueType, typename StringType, typename PathType=StringType>
  class memory_mapped_file
  {
  public:

    using path_type = std::decay_t<PathType>;
    using value_type = std::decay_t<ValueType>;
    using string_type = StringType;
    using size_type = std::size_t;
    using offset_type = size_type;
    using index_type = size_type;
    using error_message_type = string_type;

    typedef uint32_t flags_type;
    static constexpr auto flag_nocreate     = flags_type(0x01u);  // File must exist already.
    static constexpr auto flag_readwrite    = flags_type(0x02u);  // Open/map read-write for this process.
    static constexpr auto flag_shared       = flags_type(0x04u);  // Other processes may open/map the same file.
    static constexpr auto flag_protected    = flags_type(0x08u);  // Other processes may read but not write.

    #ifdef OS_LINUX
      using error_type = int;
      using descriptor_type = int;
      static constexpr descriptor_type invalid_descriptor() { return descriptor_type(-1); }
    #else
      using error_type = DWORD;
      using descriptor_type = HANDLE;
      static descriptor_type invalid_descriptor() { return reinterpret_cast<descriptor_type>(INVALID_HANDLE_VALUE); } // pointer, can't be constexpr
    #endif

    static constexpr size_type max_byte_size()  { return 128ul<<20u; }

  public:

    explicit memory_mapped_file() noexcept
      : path_(), handles_(), adr_(nullptr), size_(), offset_(), error_()
      {
        using namespace std;
        static_assert(
          (is_arithmetic_v<value_type> ||  is_trivial_v<value_type>) && (!is_pointer_v<value_type>)
          , "Incompatible value type (must be auto memory)"
        );
      }

    memory_mapped_file(const memory_mapped_file&) = delete;
    memory_mapped_file(memory_mapped_file&&) noexcept = default;
    memory_mapped_file& operator=(const memory_mapped_file&) = delete;
    memory_mapped_file& operator=(memory_mapped_file&&) noexcept = default;
    ~memory_mapped_file() noexcept { close(); }

  public:

    inline const path_type& path() const noexcept
    { return path_; }

    inline size_type size() const noexcept
    { return size_; }

    inline offset_type offset() const noexcept
    { return offset_; }

    inline flags_type flags() const noexcept
    { return flags_; }

    inline bool closed() const noexcept
    { return (handles_.fd == invalid_descriptor()); }

    inline error_type error() const noexcept
    { return error_; }

    inline error_message_type error_message() const noexcept
    {
      if(!error_) return error_message_type();
      #ifdef OS_LINUX
        return error_message_type(::strerror(error_));
      #else
        error_message_type s(256, 0);
        const size_t n = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &s[0], s.size()-1, nullptr);
        if(!n) return error_message_type();
        s.resize(n);
        return s;
      #endif
    }

  public:

    inline void close() noexcept
    {
      const auto size = size_;
      size_ = 0;
      error_ = 0;
      if(closed()) return;
      #ifdef OS_LINUX
        if(adr_) ::munmap(adr_, size);
        if(handles_.fd != invalid_descriptor()) ::close(handles_.fd);
      #else
        if(handles_.fm) ::CloseHandle(handles_.fm);
        if(handles_.fd != invalid_descriptor()) ::CloseHandle(handles_.fd);
        if(adr_) ::UnmapViewOfFile(adr_);
        handles_.fm = nullptr;
        (void)size;
      #endif
      handles_.fd = invalid_descriptor();
      offset_ = 0;
      adr_ = nullptr;
    }

    inline bool open(const path_type& file_path, const flags_type flags, const size_type num_value_elements, const offset_type element_offset) noexcept
    {
      using namespace std;
      static_assert(max_byte_size() < std::numeric_limits<uint32_t>::max(), "Large mappings untested.");
      close();
      path_ = file_path;
      flags_ = flags & (flag_nocreate|flag_readwrite|flag_shared|flag_protected);
      offset_ = element_offset;
      const auto file_size = size_type(element_offset * sizeof(value_type)) + (num_value_elements * sizeof(value_type));
      #ifdef OS_LINUX
      {
        if((file_size <= 0) || (file_size > max_byte_size())) {
          error_ = ERANGE;
          return false;
        } else {
          const auto failed = [this](){ const auto e=errno; close(); error_=e; return false; };
          // File open operation.
          {
            auto open_flags = int(O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
            if(flags & flag_readwrite) {
              open_flags |= (O_RDWR) | ((flags & flag_nocreate) ? (0) : (O_CREAT));
            } else {
              open_flags |= (O_RDONLY);
            }
            auto file_mode = ::mode_t(S_IRUSR);
            if(flags & flag_readwrite) file_mode |= ::mode_t(S_IWUSR);
            if(flags & flag_shared) file_mode |= ::mode_t(S_IRGRP|S_IROTH);
            if(!(flags & flag_protected)) file_mode |= ::mode_t(S_IWGRP|S_IWOTH);
            if((handles_.fd=::open(file_path.c_str(), open_flags, file_mode)) < 0) return failed();
            struct ::stat st;
            if(::fstat(handles_.fd, &st) < 0) return failed();
            if(st.st_size < long(file_size) && (::ftruncate(handles_.fd, file_size)<0)) return failed();
          }
          // Memory mapping.
          {
            auto protection_flags = int(PROT_READ);
            auto map_flags = int(MAP_SHARED|MAP_POPULATE);
            if(flags & flag_readwrite) protection_flags |= PROT_WRITE;
            adr_ = ::mmap(nullptr, sizeof(value_type)*num_value_elements, protection_flags, map_flags, handles_.fd, sizeof(value_type)*element_offset);
            if(adr_ == nullptr) return failed();
          }
        }
      }
      #else
      {
        if((file_size <= 0) || (file_size > max_byte_size())) {
          error_ = ERROR_NOT_ENOUGH_MEMORY;
          return false;
        } else {
          // File open operation.
          const auto failed = [this](){ const auto e=::GetLastError(); close(); error_=e; return false; };
          {
            ::InitializeSecurityDescriptor(&handles_.sd, SECURITY_DESCRIPTOR_REVISION);
            ::SetSecurityDescriptorDacl(&handles_.sd, true, nullptr, false);
            handles_.sa.nLength = sizeof(handles_.sa);
            handles_.sa.lpSecurityDescriptor = &handles_.sd;
            handles_.sa.bInheritHandle = true;
            DWORD access_mode = GENERIC_READ;
            DWORD share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE; // <-- No idea why FILE_SHARE_WRITE has to be always specified. Messes up the function of `flag_protected`.
            DWORD creation_disposition = OPEN_EXISTING;
            DWORD flags_attr = FILE_ATTRIBUTE_NORMAL;
            if(flags & flag_readwrite) {
              access_mode |= GENERIC_WRITE;
              if(!(flags & flag_protected)) share_mode |= FILE_SHARE_WRITE;
              if(!(flags & flag_nocreate)) creation_disposition = OPEN_ALWAYS;
            }
            handles_.fd = ::CreateFileA(file_path.c_str(), access_mode, share_mode, &handles_.sa, creation_disposition, flags_attr, nullptr);
            if(handles_.fd == invalid_descriptor()) return failed();
          }
          // File size adaption.
          {
            LARGE_INTEGER size_ull;
            if(::GetFileSizeEx(handles_.fd, &size_ull) && (size_t(size_ull.QuadPart) < file_size)) {
              auto n_left = file_size - size_type(size_ull.QuadPart);
              auto zeros = std::array<uint8_t, 4096>();
              zeros.fill(0);
              if(flags & flag_readwrite) {
                while(n_left > 0) {
                  DWORD n_written = 0;
                  const DWORD n_wr = std::min(n_left, zeros.max_size());
                  if(!::WriteFile(handles_.fd, zeros.data(), n_wr, &n_written, nullptr)) return failed();
                  n_left -= n_written;
                }
              } else {
                close();
                error_ = E_INVALIDARG;
                return false;
              }
            }
          }
          // Memory mapping
          {
            string name = file_path.c_str();
            name.erase(std::remove_if(name.begin(), name.end(), [](const char c){ return (c<0x20)||(c>0x7e)||(c=='\\')||(c=='/');}), name.end());
            name += to_string(long(element_offset*sizeof(value_type))) + "_" + to_string(long(num_value_elements*sizeof(value_type)));
            const DWORD protect = (flags & flag_readwrite) ? (PAGE_READWRITE) : (PAGE_READONLY);;
            const DWORD map_access = (flags & flag_readwrite) ? (FILE_MAP_ALL_ACCESS) : (FILE_MAP_READ);
            handles_.fm = ::CreateFileMappingA(handles_.fd, &handles_.sa, protect, 0, uint32_t(), name.c_str());
            if(handles_.fm == nullptr) return failed();
            adr_ = ::MapViewOfFileEx(handles_.fm, map_access, 0, DWORD(element_offset*sizeof(value_type)), DWORD(num_value_elements*sizeof(value_type)), nullptr);
            if(adr_ == nullptr) return failed();
          }
        }
      }
      #endif
      size_ = num_value_elements; // Setting the size very last prevents locking requirements, as get()/set() early return on size_==0.
      return true;
    }

    inline bool sync() noexcept
    {
      if(closed() || (!(flags_ & flag_readwrite)) || (!adr_) || (!size_)) return false;
      #ifdef OS_LINUX
        return (::msync(adr_, size_, MS_ASYNC)==0);
      #else
        ::SetFilePointer(handles_.fd, 0, nullptr, FILE_BEGIN);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(adr_);
        size_t pos = 0;
        while(pos < size_) {
          DWORD n_written = 0;
          if(!::WriteFile(handles_.fd, &data[pos], (size_-pos), &n_written, nullptr) || (!n_written)) return false;
          pos += n_written;
        }
        return true;
      #endif
    }

    inline bool set(const index_type index, const value_type value) noexcept
    {
      if(index >= size() || (!adr_) || (!(flags_ & flag_readwrite))) return false;
      reinterpret_cast<value_type*>(adr_)[index] = value;
      return true;
    }

    inline value_type get(const index_type index, const value_type default_value) const noexcept
    { return (index >= size() || (!adr_)) ? default_value : reinterpret_cast<const value_type*>(adr_)[index]; }

    inline value_type get(const index_type index) const noexcept
    { return get(index, value_type()); }

  public:

    inline const descriptor_type& descriptor() const noexcept
    { return handles_.fd; }

  protected:

    using address_type = void*;

    struct handles_type {
      #ifdef OS_LINUX
      descriptor_type fd;
      handles_type() noexcept : fd(invalid_descriptor()) {}
      #else
      handles_type() noexcept : fd(invalid_descriptor()), fm(INVALID_HANDLE_VALUE), sa(), sd() {}
      descriptor_type fd;
      HANDLE fm;
      SECURITY_ATTRIBUTES sa;
      SECURITY_DESCRIPTOR sd;
      #endif
    };

  private:

    path_type path_;
    handles_type handles_;
    address_type adr_;
    size_type size_;
    offset_type offset_;
    flags_type flags_;
    error_type error_;
  };

}}


#include <duktape/duktape.hh>
#include <duktape/mod/mod.fs.hh>

#include <algorithm>
#include <cstdint>
#include <string>
#include <memory>
#pragma GCC diagnostic push

namespace duktape { namespace detail { namespace ext { namespace mmap {

  /**
   * Unfortunately not all OS do explicitly close all descriptors and
   * resources on exit process cleanup, so a manual tracking is needed.
   */
  template <typename T>
  struct instance_tracking
  {
  public:

    using descriptor_type = typename T::descriptor_type;

  public:

    static inline instance_tracking& instance() noexcept
    { return instance_; }

  public:

    explicit inline instance_tracking() noexcept : open_() {}
    instance_tracking(const instance_tracking&) = delete;
    instance_tracking(instance_tracking&&) = default;
    instance_tracking& operator=(const instance_tracking&) = delete;

    ~instance_tracking() noexcept
    {
      for(auto& fd:open_) {
        #ifdef OS_WINDOWS
        ::CloseHandle(fd);
        #else
        ::close(fd);
        #endif
      }
    }

  public:

    instance_tracking& operator+=(descriptor_type fd)
    { open_.push_back(fd); return *this; }

    instance_tracking& operator-=(descriptor_type fd)
    { open_.erase(std::remove(open_.begin(), open_.end(), fd), open_.end()); return *this; }

  private:

    std::deque<descriptor_type> open_;
    static instance_tracking instance_;
  };

  template <typename T>
  instance_tracking<T> instance_tracking<T>::instance_ = instance_tracking<T>();

}}}}

namespace duktape { namespace mod { namespace ext { namespace mmap {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * Returns true if library code was evaluated.
   * @param duktape::engine& js
   */
  template <typename PathAccessor=duktape::detail::filesystem::path_accessor<std::string>>
  static void define_in(duktape::engine& js)
  {
    using namespace std;
    using namespace sw;
    using native_mmap = ::sw::ipc::memory_mapped_file<uint8_t, std::string>;
    using tracker = duktape::detail::ext::mmap::instance_tracking<native_mmap>;

    #if(0 && JSDOC)
    /**
     * Memory mapped file accessor.
     * - The `path` is the filesystem path to the file referred to the mapping,
     * - The `size` is the number of bytes to be mapped into RAM,
     * - The `flags` is a string, which characters have the meanings:
     *    - 'r': Readonly: Open/map readonly (for this application).
     *    - 'w': Read-Write: Open/map read-write ("w" and "rw" is identical).
     *    - 's': Shared: Allow other processes to access the map.
     *    - 'p': Protected: Do not allow other processes to write to the mapped range.
     *    - 'n': No-create: For 'w', the file must already exist, throw otherwise.
     *
     * @constructor
     * @throws {Error}
     * @param {string} path
     * @param {string} flags
     * @param {string} size
     *
     * @property {number}  size         - Size of the mapped range.
     * @property {number}  length       - Size of the mapped range (alias of size).
     * @property {number}  offset       - Offset of the mapped range.
     * @property {boolean} closed       - Holds true when the file is not opened/nothing mapped.
     * @property {string}  error        - Error string representation of the last method call.
     */
    sys.mmap = function(path, flags) {};
    #endif
    js.define(
      // Wrapped class specification and type.
      duktape::native_object<native_mmap>("sys.mmap")
      // Native constructor from script arguments.
      .constructor([](duktape::api& stack) {
        if((stack.top()!=3) || (!stack.is<string>(0)) || (!stack.is<string>(1)) || (!stack.is<int>(2))) {
          throw duktape::script_error("sys.mmap() constructor needs the arguments path, flags, and size, e.g. new sys.mmap(<path>, \"rws\", 4096)'.");
        }
        const auto path = PathAccessor::to_sys(stack.get<string>(0));
        const auto flags_chars = stack.get<string>(1);
        const auto size = stack.get<int>(2);
        if(flags_chars.find_first_not_of("rwnsp") != flags_chars.npos) {
          throw duktape::script_error("sys.mmap: Unknown flag in '" + flags_chars + "'.");
        } else if(size <= 0) {
          throw duktape::script_error("sys.mmap: Size must be > 0.");
        } else if(path.empty()) {
          throw duktape::script_error("sys.mmap: No file path given.");
        }
        auto flags = native_mmap::flags_type();
        if(flags_chars.find('n') != flags_chars.npos) flags |= native_mmap::flag_nocreate;
        if(flags_chars.find('w') != flags_chars.npos) flags |= native_mmap::flag_readwrite;
        if(flags_chars.find('s') != flags_chars.npos) flags |= native_mmap::flag_shared;
        if(flags_chars.find('p') != flags_chars.npos) flags |= native_mmap::flag_protected;
        stack.top(0);
        auto instance = std::make_unique<native_mmap>();
        if(!instance) throw duktape::script_error("sys.mmap: Failed to allocate native object.");
        if(!instance->open(path, flags, native_mmap::size_type(size), 0)) throw duktape::script_error(string("sys.mmap: ") + instance->error_message());
        tracker::instance() += instance->descriptor();
        return instance.release();
      })
      .getter("size", [](duktape::api& stack, native_mmap& instance) {
        stack.push(instance.size());
      })
      .getter("length", [](duktape::api& stack, native_mmap& instance) {
        stack.push(instance.size());
      })
      .getter("offset", [](duktape::api& stack, native_mmap& instance) {
        stack.push(instance.offset());
      })
      .getter("path", [](duktape::api& stack, native_mmap& instance) {
        stack.push(PathAccessor::to_js(instance.path()));
      })
      .getter("error", [](duktape::api& stack, native_mmap& instance) {
        stack.push(instance.error_message());
      })
      .getter("closed", [](duktape::api& stack, native_mmap& instance) {
        stack.push(instance.closed());
      })
      #if(0 && JSDOC)
      /**
       * Closes and invalidates the memory map.
       * @return {sys.mmap}
       */
      sys.mmap.prototype.close = function() {};
      #endif
      .method("close", [](duktape::api& stack, native_mmap& instance) {
        if(!instance.closed()) tracker::instance() -= instance.descriptor();
        instance.close();
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Synchronizes the RAM state to the filesystem.
       *
       * @return {sys.mmap}
       */
      sys.mmap.prototype.sync = function() {};
      #endif
      .method("sync", [](duktape::api& stack, native_mmap& instance) {
        stack.top(0);
        instance.sync();
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Reads bytes (by offset and size) from the mapped memory,
       * returns a buffer if `size`>0, a single byte if `size`
       * is 0 or omitted.
       *
       * @param {number} offset
       * @param {number} size
       * @return {buffer|number}
       */
      sys.mmap.prototype.get = function(offset, size) {};
      #endif
      .method("get", [](duktape::api& stack, native_mmap& instance) {
        if(stack.top()<1) throw duktape::script_error("sys.mmap.get: No offset (nor optional length) given.");
        const auto offset = stack.to<long>(0);
        if((offset < 0) || (size_t(offset) >= instance.size())) throw duktape::script_error("sys.mmap.get: Offset exceeds memory map range.");
        const auto length = (stack.top() < 2) ? 0 : stack.to<long>(1);
        if((length < 0) || (size_t(offset+length) > instance.size())) throw duktape::script_error("sys.mmap.get: Length and offset exceeds the memory map range.");
        stack.top(0);
        if(length == 0) {
          stack.push(instance.get(size_t(offset)));
          return true;
        } else {
          auto* buffer = reinterpret_cast<native_mmap::value_type*>(stack.push_array_buffer(size_t(length), true));
          if(!buffer) throw duktape::script_error("sys.mmap.get: No memory for allocating the return value buffer.");
          for(size_t i=0; i<size_t(length); ++i) buffer[i] = instance.get(offset+i);
        }
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Writes data data to the mapped memory, stating at position
       * `offset`. The value can be a single byte or a buffer.
       *
       * @param {number} offset
       * @param {buffer|number} data|value
       * @return {sys.mmap}
       */
      sys.mmap.prototype.set = function(data) {};
      #endif
      .method("set", [](duktape::api& stack, native_mmap& instance) {
        if((stack.top()!=2) || (!stack.is<int>(0)) || stack.is_undefined(1)) {
          throw duktape::script_error("sys.mmap.set: Need arguments offset (number) and value (buffer or number).");
        } else if(stack.is_buffer(1)) {
          const auto offset = stack.get<int>(0);
          if((offset<0) || (size_t(offset)>=instance.size())) throw duktape::script_error(string("sys.mmap.set: Offset exceeds memory map range:  ") + to_string(offset));
          size_t buffer_size = 0;
          const auto* buffer = reinterpret_cast<const native_mmap::value_type*>(stack.get_buffer(1, buffer_size));
          if(size_t(buffer_size+offset) > instance.size()) {
            throw duktape::script_error("sys.mmap.set: Input buffer size (with offset) exceeds the memory map range.");
          } else if(!buffer) {
            throw duktape::script_error("sys.mmap.set: Input buffer is invalid (null).");
          }
          for(size_t i=0; i<buffer_size; ++i) instance.set(size_t(offset)+i, buffer[i]);
          stack.top(0);
          stack.push_this();
          return true;
        } else if(stack.is<int>(0) && stack.is<int>(1)) {
          const auto offset = stack.get<int>(0);
          const auto value = stack.get<int>(1);
          if((offset<0) || (offset>=long(instance.size()))) throw duktape::script_error(std::string("sys.mmap.set: Offset/index exceeds memory map range: ") + to_string(offset) + ".");
          if((value<0) || (value>0xff)) throw duktape::script_error(std::string("sys.mmap.set: Invalid byte value (allowed 0..255)."));
          if(!instance.set(native_mmap::index_type(offset), native_mmap::value_type(value))) throw duktape::script_error(std::string("sys.mmap.set: Setting value by offset failed."));
          stack.top(0);
          stack.push_this();
          return true;
        } else {
          throw duktape::script_error("sys.mmap.set: Only buffers or byte values are accepted as mmap set values.");
        }
      })
    );
  }

}}}}

#pragma GCC diagnostic pop
#endif
