/**
 * @package de.atwillys.cc.swl
 * @license MIT
 * @author Stefan Wilhelm (stfwi)
 * @file app_attachment.hh
 * @ccflags
 * @ldflags
 * @platform linux, windows
 * @standard >= c++11
 *
 * -----------------------------------------------------------------------------
 *
 * Allows to load data appended to the executable file. The data can be of
 * any type and appended after the application has was built. Requires an
 * additional patching step (in the Makfile) after compiling and linking,
 * where an auxilliary program is temporarily compiled and executed.
 *
 * C++ code usage in the application:
 *
 *  // read string
 *  string appended_data = sw::util::read_executable_attachment<string>();
 *
 *  // read vector
 *  auto appended_data = sw::util::read_executable_attachment<vector<uint8_t>>();
 *
 *  // Example main.cc:
 *  #include <sw/util/executable_attachment.hh>
 *  #include <iostream>
 *  #include <string>
 *  using namespace std;
 *
 *  int main(int argc, const char** argv) {
 *   (void)argc; (void)argv;
 *    auto data = sw::util::read_executable_attachment<string>();
 *    cout << "'" << data << "'\n";
 *    return 0;
 *  }
 *
 * -----------------------------------------------------------------------------
 *
 * Building (example Makefile extraction)
 *
 * CXX=g++
 * HOST_CXX=g++
 * STRIP=strip
 * CXXFLAGS=-std=c++11 -W -Wall -Wextra -pedantic -Os [...]
 * AUX_BINARY=patch-application
 * BINARY=my-application
 *
 * [...]
 *
 * $(BINARY): main.cc $(AUX_BINARY)
 *    $(CXX) -o $@ $< $(CXXFLAGS)
 *    $(STRIP) --strip-all --discard-locals --discard-all $@
 *    ./$(AUX_BINARY) $@
 *    rm -f $(AUX_BINARY)
 * $(AUX_BINARY): patch.cc
 *    $(HOST_CXX) -o $@ $< -std=c++11 -W -Wall -Wextra -pedantic
 *
 * [...]
 *
 * -----------------------------------------------------------------------------
 *
 * Attaching data to the file later on:
 *
 * user@host$ cat other-data-file >> my-application
 *
 * -----------------------------------------------------------------------------
 *
 * Notes:
 *
 *  - Windows: - Unix tools are helpfull, e.g. install GIT with global %path% integration (rm.exe and cat.exe).
 *             - In the Makefile you need to add ".exe" to the file names.
 *             - In the Makefile you may have to invoke "$(AUX_BINARY)", not "./$(AUX_BINARY)"
 *
 * -----------------------------------------------------------------------------
 * +++ MIT license +++
 * Copyright (c) 2015-2017, Stefan Wilhelm <cerbero s@atwilly s.de>
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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#ifndef SW_EXECUTABLE_ATTACHMENT_HH
#define SW_EXECUTABLE_ATTACHMENT_HH
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
  #include <windows.h>
#else
  #include <climits>
  #include <cstring>
#endif

namespace sw { namespace util { namespace detail {

  /**
   * Boundary marker. The auxiliary patching program replaces this
   * placeholder (in the readonly data section) with a random sequence
   * The reversed sequence will be appended to the binary file, and
   * it is unique in the whole file. The reading function searches
   * for this boundary sequence and returns the data read until EOF.
   */
  template <typename=void>
  struct basic_executable_attachment {

    static const char boundary_marker[256];

    /***
     * Returns the full path to the own executable.
     * @return string
     */
    static inline std::string path_to_self() noexcept
    {
      std::vector<char> app_path(PATH_MAX, '\0');
      #if !defined(_WIN32) && !defined(_MSC_VER)
        char lnk_path[PATH_MAX];
        memset(lnk_path, 0, sizeof(lnk_path));
        #if defined (__linux__)
        ::strncpy(lnk_path, "/proc/self/exe", sizeof(lnk_path)-1);
        #elif defined (__NetBSD__)
        ::strncpy(lnk_path, "/proc/curproc/exe", sizeof(lnk_path)-1);
        #elif defined (__FreeBSD__) || defined (__OpenBSD__)
        int ic[4];
        ic[0] = CTL_KERN; ic[1] = KERN_PROC; ic[2] = KERN_PROC_PATHNAME; ic[3] = -1;
        size_t sz = sizeof(lnk_path)-1;
        if(sysctl(ic, 4, lnk_path, &sz, nullptr, 0)) lnk_path[0] = '\0';
        #elif defined (__DragonFly__)
        ::strncpy(lnk_path, "/proc/curproc/file", sizeof(lnk_path)-1);
        #elif defined (__APPLE__) && __MACH__
        uint32_t sz = sizeof(lnk_path);
        if(_NSGetExecutablePath(lnk_path, &sz)) lnk_path[0] = '\0';
        #endif
        if(!::realpath(lnk_path, &app_path[0])) return "";
      #else
        if(::GetModuleFileNameA(nullptr, &app_path[0], app_path.size()) <= 0) return "";
      #endif
      return std::string(app_path.data());
    }

    /**
     * Serializer
     */
    template<typename OutContainer, typename InContainer>
    static inline OutContainer serialize(const InContainer& in_data)
    { return serialize<OutContainer, InContainer>(in_data, boundary_marker); }

    template<typename OutContainer, typename InContainer>
    static inline OutContainer serialize(const InContainer& in_data, const char* boundary_mark) // @sw todo: container instead of raw pointer.
    {
      static_assert(std::is_integral<typename InContainer::value_type>::value && std::is_integral<typename OutContainer::value_type>::value, "Container elements have to be integral.");
      static_assert(sizeof(typename InContainer::value_type) == sizeof(typename OutContainer::value_type),"Container elements have to have identical sizes.");
      auto xo = typename OutContainer::value_type(0);
      auto out_data = OutContainer();
      out_data.reserve(in_data.size());
      for(size_t i=0; i<in_data.size(); ++i) {
        xo = (xo<<8) | boundary_mark[i & 0xff];
        out_data.push_back(in_data[i] ^ xo); // @see serialize()
      }
      return out_data;
    }

    /**
     * Unserializer
     */
    template<typename OutContainer, typename InContainer>
    static inline OutContainer unserialize(const InContainer& in_data)
    {
      static_assert(std::is_integral<typename InContainer::value_type>::value && std::is_integral<typename OutContainer::value_type>::value,"Container elements have to be integral.");
      static_assert(sizeof(typename InContainer::value_type) == sizeof(typename OutContainer::value_type),"Container elements have to have identical sizes.");
      auto xo = typename OutContainer::value_type(0);
      auto out_data = OutContainer();
      out_data.reserve(in_data.size());
      for(size_t i=0; i<in_data.size(); ++i) {
        xo = (xo<<8) | boundary_marker[i & 0xff];
        out_data.push_back(in_data[i] ^ xo); // @see serialize()
      }
      return out_data;
    }

    /**
     * Auxiliary function to patch the compiled application file,
     * directly callable from main, first and only argument is the
     * path of the compiled binary, which uses the
     * executable_attachment::read() function. The auxiliary binary
     * code can be:
     *
     *  #include <sw/util/executable_attachment.hh>
     *  int main(int argc, const char* argv[]) {
     *   [...] // Parse args
     *   return sw::util::detail::executable_attachment_application_patch(path, verbose, attachment_contents);
     *  }
     */
    #ifdef APP_ATTACHMENT_PATCHING_BINARY
    template <typename=void>
    static inline int patch_application(std::string path, bool verbose=false, std::string attachment_contents="")
    {
      using namespace std;

      // hexdump
      auto hexdump = [](const vector<char>& data, std::ostream& os) {
        size_t n = 0;
        for(auto e:data) {
          if(!(n & 63)) os << "\n" << hex << setw(8) << setfill('0') << n << ": ";
          os << hex << setw(2) << setfill('0') << ((int)(e&0xff));
          ++n;
        }
        os << dec << "\n";
      };

      // file size using stat to double check later read
      size_t filesize = 0;
      {
        struct ::stat st;
        if(::stat(path.c_str(), &st) != 0) {
          cerr << "stat failed for " << path << "\n";
          return 1;
        } else {
          filesize = st.st_size;
          if(verbose) {
            cerr << "File size: " << dec << ((long)filesize) << " (0x" << hex << ((long)filesize) << ")\n";
          }
        }
      }

      // same placeholder as in the application
      vector<char> reverse_boundary_key(256);
      std::copy_n(
        basic_executable_attachment<void>::boundary_marker,
        sizeof(basic_executable_attachment<void>::boundary_marker),
        reverse_boundary_key.begin()
      );

      if(verbose) {
        cerr << "----------------------\n";
        cerr << "reverse_boundary_key:\n";
        hexdump(reverse_boundary_key, cerr);
        cerr << "----------------------\n";
      }

      // Read whole application
      vector<char> contents;
      {
        std::ifstream fis(path.c_str(), std::ios::in|std::ios::binary);
        contents.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
        if(contents.size() != filesize) {
          cerr << "file size mismatch\n"
              << "- file size : " << filesize << "\n"
              << "- file size read: " << contents.size() << "\n";
          return 1;
        }
      }

      if(verbose) {
        std::ofstream fos((path+".unpatched.tmp").c_str(), std::ios::out|std::ios::binary);
        hexdump(contents, fos);
        fos.close();
      }

      // Fill up the file until it has 4k size (app checks the start of each read 4k block to save time).
      while((contents.size() & (4095u)) != 0) contents.push_back(0);
      if(verbose) cerr << "File size after alignment fill: " << dec << ((long)contents.size()) << " (0x" << hex << ((long)contents.size()) << ")\n";

      // Find placeholder
      auto key_patch_position_it = std::search(contents.begin(), contents.end(), reverse_boundary_key.begin(), reverse_boundary_key.end());
      {
        if(key_patch_position_it == contents.end()) {
          cerr << "reverse boundary placeholder not found\n";
          return 1;
        }
        if(verbose) {
          size_t pos = static_cast<size_t>(key_patch_position_it - contents.begin()) / sizeof(decltype(contents)::iterator::difference_type);
          cerr << "Boundary placeholder at: " << pos << "\n";
        }
      }

      // Make random boundary key, check that it is unique in the whole file.
      vector<char> patch_key;
      {
        int retries = 10;
        const std::string ofs = std::string("[<") + std::to_string(long(contents.size())) + ">]";
        while(--retries) {
          patch_key.clear();
          patch_key.reserve(reverse_boundary_key.size());
          default_random_engine rd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
          mt19937 mt(rd());
          uniform_int_distribution<char> db(33, 124);
          while(patch_key.size() < reverse_boundary_key.size()) patch_key.push_back(char(db(mt)));
          reverse(patch_key.begin(), patch_key.end());
          std::copy(ofs.begin(), ofs.end(), patch_key.begin());
          auto check_it = std::search(contents.begin(), contents.end(), patch_key.begin(), patch_key.end());
          auto check_sym = patch_key;
          reverse(check_sym.begin(), check_sym.end());
          if((check_it == contents.end()) && (check_sym != patch_key)) break;
        }
        if(!retries) {
          cerr << "failed to generate collisionless boundary\n";
          return 1;
        }
        if(verbose) {
          cerr << "----------------------\n";
          cerr << "patch key:'" << ofs << "'\n";
          hexdump(patch_key, cerr);
          cerr << "----------------------\n";
        }
      }
      // Patch the placeholder position with the generated key.
      std::copy_n(patch_key.begin(), patch_key.size(), key_patch_position_it);
      // Reverse the key and write it at the end.
      reverse(patch_key.begin(), patch_key.end());
      for(auto e:patch_key) contents.push_back(e);
      if(verbose) cerr << "File size after adding the boundary: " << dec << ((long)contents.size()) << " (0x" << hex << ((long)contents.size()) << ")\n";
      std::ofstream fos(path.c_str(), ofstream::out|ofstream::binary);
      if(!fos.write(contents.data(), contents.size())) { cerr << "failed to write file\n" ; return 1; }
      if(verbose) {
        std::ofstream dump_fos((path+".patched.tmp").c_str(), std::ios::out|std::ios::binary);
        hexdump(contents, dump_fos);
        dump_fos.close();
      }
      if(!attachment_contents.empty()) {
        if(verbose) cerr << "[verb] Attaching " << long(attachment_contents.size()>>10) << "kb data ...\n";
        reverse(patch_key.begin(), patch_key.end());
        const auto wdata = basic_executable_attachment<void>::serialize<vector<char>, string>(attachment_contents, patch_key.data());
        copy(wdata.begin(), wdata.end(), ostreambuf_iterator<char>(fos));
      }
      return 0;
    }
    #endif

  };

  template <typename T>
  const char basic_executable_attachment<T>::boundary_marker[256] = {
    'A','T','T','A','C','H','M','E','N','T','_','B','O','U','N','D',
    'A','R','Y','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_'
  };

}}}

namespace sw { namespace util {

  /**
   * Reads appended data from the (own/"self") executable file by searching
   * a unique sequence marking the end of the compiled and linked executable
   * file. Everything read after finding the boundary marker are data attached
   * to the file and returned, together with the offset in the file where the
   * data start. The offset is zero and the container empty on any error. The
   * offset is nonzero and the container empty if no attachment is present.
   *
   * @return std::pair<typename Container, typename SizeType>
   */
  template <typename Container, typename SizeType>
  std::pair<Container, SizeType> read_executable_attachment(size_t max_size=size_t(-1))
  {
    using namespace std;
    using attachment = detail::basic_executable_attachment<>;
    const auto empty_return = make_pair(Container(), SizeType());
    const string app_path = attachment::path_to_self();
    if(app_path.empty()) return empty_return;
    vector<char> reverse_boundary_key(256, '\0');
    copy_n(attachment::boundary_marker, sizeof(attachment::boundary_marker), reverse_boundary_key.begin());
    if((reverse_boundary_key[0] != '[') || (reverse_boundary_key[1] != '<')) return empty_return;
    auto sofs = string();
    auto it = reverse_boundary_key.begin()+1;
    while(++it != reverse_boundary_key.end() && std::isdigit(*it)) { sofs.push_back(*it); }
    if((it==reverse_boundary_key.end()) || (*it != '>')) return empty_return;
    if((++it==reverse_boundary_key.end()) || (*it != ']')) return empty_return;
    const long offset = atol(sofs.c_str());
    reverse(reverse_boundary_key.begin(), reverse_boundary_key.end());
    vector<char> buf(4096), data;
    fstream f(app_path.c_str(), ios::in|ios::binary);
    if((!f.is_open()) || (!f.good())) return empty_return;
    bool found = false;
    f.seekg(offset);
    auto return_offset = SizeType(0);
    if(f.good() && !f.eof()) {
      if(!((!f.read(buf.data(), buf.size()) && !f.eof()) || (f.gcount() < std::streamsize(reverse_boundary_key.size())))) {
        if(equal(reverse_boundary_key.begin(), reverse_boundary_key.end(), buf.begin())) {
          return_offset = size_t(offset) + reverse_boundary_key.size();
          found = true;
          if(f.gcount() > std::streamsize(reverse_boundary_key.size())) {
            size_t sz = size_t(f.gcount()) - reverse_boundary_key.size();
            data.resize(sz);
            std::copy_n(buf.begin()+reverse_boundary_key.size(), sz, data.begin());
          }
        }
      }
    }
    if(found) {
      while(f.read(buf.data(), buf.size()) || (f.eof() && f.gcount() > 0)) {
        data.reserve(data.size()+f.gcount());
        data.insert(data.end(), buf.begin(), buf.end());
        if(f.gcount() < streamsize(buf.size())) data.resize(data.size()-buf.size()+f.gcount()); // unlikely case, therefore separated
        if(data.size() >= max_size) break;
      }
      if((!f.good()) && (!f.eof())) return empty_return;
      if(data.size() > max_size) data.resize(max_size);
    }
    return make_pair(attachment::unserialize<Container>(data), return_offset);
  }

  /**
   * Reads appended data from the (own/"self") executable file by searching
   * a unique sequence marking the end of the compiled and linked executable
   * file. Everything read after finding the boundary marker are data attached
   * to the file and returned.
   *
   * @return typename Container
   */
  template <typename Container>
  Container read_executable_attachment(size_t max_size=size_t(-1))
  { return read_executable_attachment<Container, size_t>(max_size).first; }

  template <typename Container>
  int write_executable_attachment(const std::string& output_file, const Container& data)
  {
    using namespace std;
    using attachment = detail::basic_executable_attachment<>;
    static_assert(sizeof(typename Container::value_type)==1,"string liertal is necessary in c++14");
    const auto exec_size = read_executable_attachment<string, std::streamsize>().second;
    if(exec_size == 0) {
      return -1;
    } else {
      ifstream is(output_file.c_str(), ios::ate|ios::binary);
      if((is.is_open()) || (is.good())) return -2;
      is.close();
    }
    ifstream is(attachment::path_to_self().c_str(), ios::binary);
    if(!is.good()) return -3;
    ofstream os(output_file.c_str(), ios::binary);
    if(!is.good()) return -4;
    copy_n(istreambuf_iterator<char>(is), exec_size, ostreambuf_iterator<char>(os));
    is.close();
    os.flush();
    const auto wpos_executable = os.tellp();
    if(wpos_executable != exec_size) return -5;
    const auto wdata = attachment::serialize<Container, string>(data);
    copy(wdata.begin(), wdata.end(), ostreambuf_iterator<char>(os));
    const auto wpos_data = os.tellp();
    if(size_t(wpos_data) != (size_t(wpos_executable) + wdata.size())) return -5;
    return int(os.tellp());
  }

}}

#endif
