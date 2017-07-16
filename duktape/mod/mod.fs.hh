/**
 * @file duktape/mod/mod.fs.hh
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
 * Optional file system functionality.
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
#ifndef DUKTAPE_MOD_BASIC_FILESYSTEM_HH
#define DUKTAPE_MOD_BASIC_FILESYSTEM_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#if defined(_MSCVER)
  #error "not implemented yet"
#endif

#include "../duktape.hh"
#include "mod.sys.hh"
#include <iostream>
#include <streambuf>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#if defined(__MINGW32__) || defined(__MINGW64__)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
  #define DIRECTORY_SEPARATOR "\\"
  #define S_ISLNK(X) (false)
  #define S_IFLNK (0)
  #define S_IFSOCK (0)
  #include <libgen.h>
  #include <sys/utime.h>
  #include <windows.h>
  #include <winbase.h>
  #include <process.h>
  #include <direct.h>
  #include <dirent.h>
  #include <Lmcons.h>
  #include <Shlobj.h>
  #include <io.h>
  #include "accctrl.h"
  #include "aclapi.h"
#else
  #define DIRECTORY_SEPARATOR "/"
  #include <fnmatch.h>
  #include <fts.h>
  #include <glob.h>
  #include <sys/times.h>
  #include <dirent.h>
  #include <libgen.h>
  #include <pwd.h>
  #include <grp.h>
  #include <utime.h>
  #if defined(__linux__)
    #include <wait.h>
    #include <sys/sendfile.h>
    #include <fcntl.h>
  #elif defined __APPLE__ & __MACH__
    #define MACINTOSH /* let's jsut say macintosh, we know what't meant. */
    #include <sys/wait.h>
  #else
    #include <sys/wait.h>
  #endif
#endif

#define return_false { stack.push(false); return 1; }
#define return_true { stack.push(true); return 1; }
#define return_undefined { return 0; }

// </editor-fold>

// <editor-fold desc="path_accessor (trait)" defaultstate="collapsed">
namespace duktape { namespace detail { namespace filesystem {

  /**
   * Extend or clone the functionality of this trait to ...
   *
   *  -> Check paths
   *  -> Restrict the access (e.g. using a defined root directory, you can
   *     throw a std::exception if the path is not allowed.
   *  -> Convert windows to *nix paths and vice versa
   *  -> etc.
   *
   * and derive the filesystem class below passing your traits:
   *
   *  using my_filesystem = ::duktape::detail::filesystem<your_type>;
   *
   * This default class is a simple pass-through.
   */
  template <typename StringType=std::string>
  struct path_accessor
  {
    using string_type = StringType;

    /**
     * Converts a path in the file system to a path in the JS engine.
     * E.g. if unix like paths are to be used under windows, the
     * conversion c:\whatever -> /c/whatever can be done.
     * @param string_type path
     * @return string_type
     */
    static string_type to_js(string_type path) { return path; }

    /**
     * Converts a path in the JS engine to a path in the file system.
     * E.g. if unix like paths are to be used under windows, the
     * conversion /c/whatever -> c:\whatever  can be done.
     *
     * Note: If you want to limit the access in of your scripts, this
     * is the right point to throw a `duktape::script_error`, the
     * script engine will then forward this as javascript exception.
     * Maybe think about calling ck_sys() below after converting the
     * path.
     *
     * @param string_type path
     * @return string_type
     */
    static string_type to_sys(string_type path) { return path; }

    /**
     * Used when file system access have to be checked in the filesystem
     * function here. Context: it is not enough to only check paths
     * passed as input once, as other system paths might be calculated
     * or e.g. come up when "currentdir/../../other dir" is used. So
     * functions will call the this `to_sys()` function to double check.
     *
     * Note: If you want to limit the access in of your scripts, this
     *       is the right point to throw a `duktape::script_error`, the
     *       script engine will then forward this as javascript exception.
     *       If not simply leave it empty.
     *
     * @param string_type path
     * @return string_type
     */
    static void ck_sys(string_type path) { (void)path; }
  };

}}}
// </editor-fold>

// <editor-fold desc="generic c++ file i/o" defaultstate="collapsed">
namespace duktape { namespace detail { namespace filesystem { namespace generic {

  // <editor-fold desc="fileread (c++ iostream)" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Reads a file, returns the contents or undefined on error.
   *
   * - Normally the file is read as text file, no matter if the data in the file
   *   really correspond to a readable text.
   *
   * - If `conf` is a string containing the word "binary", the data are read
   *   binary and returned as `buffer`.
   *
   * - If `conf` is a callable function, the data are read as text line by line,
   *   and for each line the callback is invoked, where the line text is passed
   *   as argument. The callback ("filter function") can:
   *
   *    - return `true` to keep the passed line in the file read output.
   *    - return `false` or `undefined` to exclude the line from the output.
   *    - return a `string` to put the returned string into the output instead
   *      of the original passed text (inline editing).
   *
   * - Binary reading and using the filter callback function excludes another.
   *
   * - Not returning anything/undefined has a purpose of storing the line data
   *   somewhere else, e.g. when parsing:
   *
   *     var config = {};
   *     fs.readfile("whatever.conf", function(s) {
   *       // parse, parse ... get some key and value pair ...
   *       config[key] = value;
   *       // no return return statement, output of fs.filter()
   *       // is not relevant.
   *     });
   *
   * @param String path
   * @param undefined|String|function conf
   * @return String|Buffer
   */
  fs.readfile = function(path, conf) {};
  #endif
  template <typename PathAccessor>
  int fileread(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    bool binary = false;
    duktape::api::index_t filter_function = 0;

    if(!stack.is_undefined(1)) {
      if(stack.is<std::string>(1)) {
        // string flags
        std::string flags = stack.to<std::string>(1);
        if(flags.find("binary") != flags.npos) binary = true;
      } else if(stack.is_function(1)) {
        // Filter function
        filter_function = 1;
      } else if(stack.is_object(1)) {
        // { binary: true }
        if(stack.has_prop_string(1, "binary")) {
          stack.get_prop_string(1, "binary");
          binary = stack.to<bool>(2);
          stack.pop();
        }
        if(stack.has_prop_string(1, "filter")) {
          stack.get_prop_string(1, "filter");
          if(stack.is_function(2)) {
            filter_function = 2;
          } else {
            stack.throw_exception("The filter setting for reading a file must be a function.");
            return 0;
          }
        }
      } else {
        stack.throw_exception("Invalid configuration for file read function.");
        return 0;
      }
    }

    if(binary && filter_function) {
      stack.throw_exception("file read function: You can't use (text) filters when reading binary data.");
      return 0;
    }

    std::string contents;
    try {
      if(!filter_function) {
        std::ifstream fis(path.c_str(), binary ? (std::ios::in|std::ios::binary) : (std::ios::in));
        contents.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
        if(!fis.good() && !fis.eof()) return 0; // not accessible/existing/read error
        if(!binary) {
          stack.push(contents);
          return 1;
        } else {
          char* buffer = reinterpret_cast<char*>(stack.push_array_buffer(contents.length(), true));
          if(!buffer) return 0;
          if(contents.length() > 0) std::copy(contents.begin(), contents.end(), buffer);
          return 1;
        }
      } else {
        std::ifstream fis(path.c_str());
        if(!fis.good() && !fis.eof()) return 0;
        std::string line;
        std::string contents;
        stack.require_stack_top(5);
        bool islast = false;
        while(fis.good()) {
          if(!std::getline(fis, line).good()) {
            if(!fis.eof()) break; // io error
            if(line.empty()) break; // eof and nothing more to do
            islast = true;
          }
          // if(line.empty()) continue; .... no, it is possible that someone counts the lines in the callback
          stack.dup(filter_function);
          stack.push(line);
          stack.call(1);
          if(stack.is<std::string>(-1)) {
            // 1. Filter returns a string: Means a modified version of the line shall be added.
            contents += stack.to<std::string>(-1);
            if(!islast) contents += '\n';
          } else if(stack.is<bool>(-1)) {
            if(stack.get<bool>(-1)) {
              // 2. Filter returns true: Means, yes, add this line.
              contents += line;
              if(!islast) contents += '\n';
            } else {
              // 3. Filter returns true: Means, no, don't want this line.
            }
          } else if(stack.is_undefined(-1) || stack.is_null(-1)) {
              // 4. Filter returns undefined: Means, no, don't want this line,
              //    similar to false, but covers the context that there is a
              //    string or object somewhere accessible from the filter function
              //    where the line data are stored somehow, and the file reading
              //    function output is ignored anyway after it returns.
              //
          } else {
            stack.throw_exception("The file reading filter function must return a string, true/false or nothing (undefined).");
            return 0;
          }
          stack.pop();
        }
        stack.push(contents);
        return 1;
      }
    } catch(const std::exception& e) {
      return 0; // Return undefined on error.
    }
    return 0; // invalid execution path.
  }
  // </editor-fold>

  // <editor-fold desc="filewrite (c++ iostream)" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Writes data into a file Reads a file and returns the contents as text.
   *
   * @param String path
   * @param String|Buffer|Number|Boolean|Object data
   * @param String|Object flags
   * @return Boolean
   */
  fs.filewrite = function(path, data, flags) {};
  #endif
  template <typename PathAccessor, bool Append=false>
  int filewrite(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) { stack.push(false); return 1; }
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    std::ios::openmode mode = std::ios::out;
    if(Append) mode |= std::ios::app;
    std::string data;
    if(stack.is_undefined(1)) {
      stack.throw_exception("The file write function needs a data argument (2nd argument).");
      return 0;
    } else if(stack.is_function(1)) {
      stack.throw_exception("The file write function cannot use functions as data argument.");
      return 0;
    } else if(stack.is_buffer(1)) {
      data = stack.get_buffer<std::string>(1);
      mode |= std::ios::binary;
    } else {
      data = stack.to<std::string>(1);
    }
    try {
      std::ofstream fos(path.c_str(), mode);
      if(!fos.good()) { stack.push(false); return 1; }
      fos << data;
      stack.push(fos.good());
      return 1;
    } catch(const std::exception& e) {
      stack.push(false);
      return 1;
    }
  }
  // </editor-fold>

}}}}

namespace duktape { namespace mod { namespace filesystem { namespace generic {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  using namespace ::duktape::detail::filesystem;
  using namespace ::duktape::detail::filesystem::generic;
  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename PathAccessor=path_accessor<std::string>>
  static void define_in(duktape::engine& js)
  {
    //
    // Note: The names "readfile" and "writefile" were chosen
    //       instead of "read" and "write" to clarify that file
    //       operation FUNCTIONS are meant, similar to "readlink"
    //       or "readdir". A "fs.file" object would have the methods
    //       "read" and "write" and "writeline" or the like.
    //
    js.define("fs.readfile", fileread<PathAccessor>, 2);
    js.define("fs.writefile", filewrite<PathAccessor, false>, 2);
    js.define("fs.appendfile", filewrite<PathAccessor, true>, 2);
  }
  // </editor-fold>

}}}}

// </editor-fold>

// <editor-fold desc="basic filesystem operations" defaultstate="collapsed">
namespace duktape { namespace detail { namespace filesystem { namespace basic {

  // <editor-fold desc="location getters (cwd, tmp, home, ...)" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Returns the current working directory or `undefined` on error.
   * Does strictly not accept arguments.
   *
   * @return String | undefined
   */
  fs.cwd = function() {};
  #endif
  template <typename PathAccessor>
  int cwd(duktape::api& stack)
  {
    char apath[PATH_MAX+1];
    if(!::getcwd(apath, PATH_MAX)) return 0;
    apath[PATH_MAX] = '\0';
    stack.push(PathAccessor::to_js(apath));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns the temporary directory or `undefined` on error.
   * Does strictly not accept arguments.
   *
   * @return String | undefined
   */
  fs.tmpdir = function() {};
  #endif
  template <typename PathAccessor>
  int tmpdir(duktape::api& stack)
  {
    #ifdef WINDOWS
    char bf[MAX_PATH+1];
    DWORD r = ::GetTempPathA(sizeof(bf), bf);
    if(!r || r > sizeof(bf)) return 0;
    bf[r] = bf[sizeof(bf)-1] = '\0';
    while(--r > 0 && bf[r] == '\\') bf[r] = '\0';
    stack.push(PathAccessor::to_js(bf));
    return 1;
    #else
    stack.push(PathAccessor::to_js("/tmp"));
    return 1;
    #endif
  }

#if(0)
  #if(0 && JSDOC)
  /**
   * Returns the path for a temporary file. The file is
   * NOT created yet. Note that using this function for
   * creating temporary files is unsafe, as other processes
   * might create the same file before this program has
   * created it.
   * Accepts one optional argument, the file prefix.
   *
   * @param undefined|String prefix
   * @return String | undefined
   */
  fs.tempnam = function(prefix) {};
  #endif
  template <typename PathAccessor>
  int tempnam(duktape::api& stack)
  {
    char path[PATH_MAX];
    ::memset(path, 0, sizeof(path));
    std::string prefix;
    if(stack.is<std::string>(0)) prefix = stack.to<std::string>(0);
    for(auto& e:prefix) { if(!::isalnum(e)) e = '_'; }
    if(prefix.empty()) prefix = "tmpf";
    if(prefix.length() > PATH_MAX-10) prefix.resize(PATH_MAX-10);
    prefix += "_XXXXXX";
    ::strncpy(path, prefix.c_str(), PATH_MAX-1);
    int fd = ::mkstemp(path);
    if(fd < 0) return 0;
    ::unlink(path);
    ::close(fd);
    stack.push(PathAccessor::to_js(path));
    return 1;
  }
#endif

  #if(0 && JSDOC)
  /**
   * Returns the home directory of the current used or `undefined` on error.
   * Does strictly not accept arguments.
   *
   * @return String | undefined
   */
  fs.home = function() {};
  #endif
  template <typename PathAccessor>
  int homedir(duktape::api& stack)
  {
    #ifdef WINDOWS
    char p[MAX_PATH];
    if(SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, p))) {
      stack.push(PathAccessor::to_js(p));
      return 1;
    } else {
      return 0;
    }
    #else
    char name[256];
    struct ::passwd pw, *ppw;
    if((::getpwuid_r(::getuid(), &pw, name, sizeof(name), &ppw) == 0) && pw.pw_dir) {
      stack.push(PathAccessor::to_js((const char*)pw.pw_dir));
      return 1;
    } else {
      return 0;
    }
    #endif
  }
  // </editor-fold>

  // <editor-fold desc="path analysis (realpath, basename, dirname ...)" defaultstate="collapsed">

  #if(0 && JSDOC)
  /**
   * Returns the real, full path (with resolved symbolic links) or `undefined`
   * on error or if the file does not exist.
   * Does strictly require one String argument (the path).
   *
   * @param String path
   * @return String | undefined
   */
  fs.realpath = function(path) {};
  #endif
  template <typename PathAccessor>
  int realpath(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    #ifdef WINDOWS
    if(path.empty()) return 0;
    if(path == "~") return homedir<PathAccessor>(stack);
    char apath[PATH_MAX];
    std::string s = path;
    while(!s.empty() && (s[s.length()-1] == '\\')) s.resize(s.length()-1);
    if((path.length() > 2) && (path[0] == '~') && ((path[1] == '\\') || (path[1] == '/'))) {
      char hdir[PATH_MAX];
      if(!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, hdir))) return 0;
      hdir[sizeof(hdir)-1] = '\0';
      s = std::string(hdir) + s.substr(1);
    }
    if(::_fullpath(apath, s.c_str(), MAX_PATH) == NULL) return 0;
    stack.push(PathAccessor::to_js(std::string(apath)));
    return 1;
    #else
    if(path.empty()) {
      return 0;
    } else if(::access(path.c_str(), F_OK) != 0) {
      // knowingly existing
    } else if(path[0] == '~') {
      if(path.length() == 1) {
        return homedir<PathAccessor>(stack);
      } else if(path[1] == '/') {
        char name[256];
        struct ::passwd pw, *ppw;
        if((::getpwuid_r(::getuid(), &pw, name, sizeof(name), &ppw) != 0) || (!pw.pw_dir)) return 0;
        path = std::string(pw.pw_dir) + path.substr(1);
      }
    } else if(path[0] == '.') {
      if(path.length() == 1) {
        return cwd<PathAccessor>(stack);
      } else if(path[1] == '/') {
        char apath[PATH_MAX+1];
        if(!::getcwd(apath, sizeof(apath)-1)) return 0;
        apath[sizeof(apath)-1] = '\0';
        path = std::string(apath) + path.substr(1);
      }
    }
    char outpath[PATH_MAX];
    outpath[PATH_MAX-1] = '\0';
    if(!::realpath(path.c_str(), outpath)) return 0;
    stack.push(PathAccessor::to_js(std::string(outpath)));
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns directory part of the given path (without tailing slash/backslash)
   * or `undefined` on error.
   * Does strictly require one String argument (the path).
   *
   * @param String path
   * @return String | undefined
   */
  fs.dirname = function(path) {};
  #endif
  template <typename PathAccessor>
  int getdirname(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string inpath = PathAccessor::to_sys(stack.to<std::string>(0));
    inpath += '\0';
    char *p = ::dirname(&inpath.front());
    if(!p) return 0;
    stack.push(PathAccessor::to_js(std::string(p)));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns file base part of the given path (name and extension, without parent directory)
   * or `undefined` on error.
   * Does strictly require one String argument (the path).
   *
   * @param String path
   * @return String | undefined
   */
  fs.basename = function(path) {};
  #endif
  template <typename PathAccessor>
  int getbasename(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string inpath = PathAccessor::to_sys(stack.to<std::string>(0));
    char *p = basename(&inpath.front());
    if(!p) return 0;
    stack.push(PathAccessor::to_js(std::string(p)));
    return 1;
  }

  // </editor-fold>

  // <editor-fold desc="file information (stat wrappers)" defaultstate="collapsed">

  #if(0 && JSDOC)
  /**
   * Returns a string representation of a file mode bit mask, e.g.
   * for a (octal) mode `0755` directory the output will be '755' or 'rwxrwxrwx' (see flags below).
   * Does strictly require one (integral) Number argument (the input mode) at first.
   *
   * If flags are given they modify the output as follows:
   *
   *  flags == 'o' (octal)    : returns a string with the octal representation (like 755 or 644)
   *  flags == 'l' (long)     : returns a string like 'rwxrwxrwx', like `ls -l` but without preceeding file type character.
   *  flags == 'e' (extended) : output like `ls -l` ('d'=directory, 'c'=character device, 'p'=pipe, ...)
   *
   * @param Number mode
   * @param String flags [optional]
   * @return String | undefined
   */
  fs.mod2str = function(mode, flags='') {};
  #endif
  template <typename=void>
  std::string mod2str(unsigned mod, char how='o')
  {
    if(how != 'l' && how != 'e') { // (o)ctal == not (l)ong, not (e)xtended
      std::string mode("000");
      // note: this looks silly but using the definitions prevents endianess problems and the like.
      if(mod & S_IXUSR) mode[0] |= 0x01;
      if(mod & S_IWUSR) mode[0] |= 0x02;
      if(mod & S_IRUSR) mode[0] |= 0x04;
      if(mod & S_IXGRP) mode[1] |= 0x01;
      if(mod & S_IWGRP) mode[1] |= 0x02;
      if(mod & S_IRGRP) mode[1] |= 0x04;
      if(mod & S_IXOTH) mode[2] |= 0x01;
      if(mod & S_IWOTH) mode[2] |= 0x02;
      if(mod & S_IROTH) mode[2] |= 0x04;
      return mode;
    } else { // long or extended
      std::string mode("---------");
      if(mod & S_IRUSR) mode[0] = 'r';
      if(mod & S_IWUSR) mode[1] = 'w';
      if(mod & S_IXUSR) mode[2] = 'x';
      if(mod & S_IRGRP) mode[3] = 'r';
      if(mod & S_IWGRP) mode[4] = 'w';
      if(mod & S_IXGRP) mode[5] = 'x';
      if(mod & S_IROTH) mode[6] = 'r';
      if(mod & S_IWOTH) mode[7] = 'w';
      if(mod & S_IXOTH) mode[8] = 'x';
      if(how == 'e') {
        mode = std::string("-") + mode;
        if(S_ISREG(mod)) mode[0] = '-';
        else if(S_ISDIR(mod)) mode[0] = 'd';
        else if(S_ISLNK(mod)) mode[0] = 'l';
        else if(S_ISFIFO(mod)) mode[0] = 'p';
        else if(S_ISCHR(mod)) mode[0] = 'c';
        else if(S_ISBLK(mod)) mode[0] = 'b';
        #ifdef S_ISSOCK
        else if(S_ISSOCK(mod)) mode[0] = 's';
        #endif
      }
      return mode;
    }
  }

  template <typename PathAccessor>
  int mod2str(duktape::api& stack)
  {
    if(!stack.is<long>(0)) return 0;
    unsigned mod = stack.to<long>(0);
    std::string how = stack.to<std::string>(1);
    if(how.empty()) how = "o";
    stack.push(mod2str(mod, how[0]));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns a numeric representation of a file mode bit mask given as string, e.g.
   * "755", "rwx------", etc.
   * Does strictly require one argument (the input mode). Note that numeric arguments
   * will be reinterpreted as string, so that 755 is NOT the bit mask 0x02f3, but seen
   * as 0755 octal.
   *
   * @param String mode
   * @return Number | undefined
   */
  fs.str2mod = function(mode) {};
  #endif
  template <typename=void>
  bool str2mod(std::string mode, unsigned& mod)
  {
    unsigned umod = 0;
    if(mode.empty()) {
      return false;
    } else if(::isdigit(mode[0])) { // octal format
      if(mode.length() == 4) mode = mode.substr(1); // leading '0' (or other, invalid ignored here)
      if(mode.length() != 3) return false;
      // Note: Explicit usage of lib macros although the results should be already the same.
      //       ... to prevent little/big endian problems or any unknown conventions.
      int m[3] = { mode[0]-'0', mode[1]-'0', mode[2]-'0' };
      if((m[0] < 0 || m[0] > 7) || (m[1] < 0 || m[1] > 7) || (m[2] < 0 || m[2] > 7)) return 0;
      umod |= (m[0] & 0x01 ? S_IXUSR:0) | (m[0] & 0x02 ? S_IWUSR:0) | (m[0] & 0x04 ? S_IRUSR:0);
      umod |= (m[1] & 0x01 ? S_IXGRP:0) | (m[1] & 0x02 ? S_IWGRP:0) | (m[1] & 0x04 ? S_IRGRP:0);
      umod |= (m[2] & 0x01 ? S_IXOTH:0) | (m[2] & 0x02 ? S_IWOTH:0) | (m[2] & 0x04 ? S_IROTH:0);
    } else { // rwx format
      if(mode.length() == 10) mode = mode.substr(1); // 10 = sizeof("drwxrwxrwx")-1
      else if(mode.length() != 9) return false; // 9 = sizeof("rwxrwxrwx")-1
      if(mode[0+0] == 'r') umod |= S_IRUSR; else if(mode[0+0] != '-') return false;
      if(mode[1+0] == 'w') umod |= S_IWUSR; else if(mode[1+0] != '-') return false;
      if(mode[2+0] == 'x') umod |= S_IXUSR; else if(mode[2+0] != '-') return false;
      if(mode[0+3] == 'r') umod |= S_IRGRP; else if(mode[0+3] != '-') return false;
      if(mode[1+3] == 'w') umod |= S_IWGRP; else if(mode[1+3] != '-') return false;
      if(mode[2+3] == 'x') umod |= S_IXGRP; else if(mode[2+3] != '-') return false;
      if(mode[0+6] == 'r') umod |= S_IROTH; else if(mode[0+6] != '-') return false;
      if(mode[1+6] == 'w') umod |= S_IWOTH; else if(mode[1+6] != '-') return false;
      if(mode[2+6] == 'x') umod |= S_IXOTH; else if(mode[2+6] != '-') return false;
    }
    mod = umod;
    return true;
  }

  template <typename PathAccessor>
  int str2mod(duktape::api& stack)
  {
    if(!stack.is<std::string>(0) && !stack.is<long>(0)) return 0;
    unsigned mod = 0;
    if(!str2mod(stack.to<std::string>(0), mod)) return 0;
    stack.push(mod);
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns a plain object containing information about a given file,
   * directory or `undefined` on error.
   * Does strictly require one String argument (the path).
   *
   * The returned object has the properties:
   *
   * {
   *    path: String,   // given path (function argument)
   *    size: Number,   // size in bytes
   *    mtime: Date,    // Last modification time
   *    ctime: Date,    // Creation time
   *    atime: Date,    // Last accessed time
   *    owner: String,  // User name of the file owner
   *    group: String,  // Group name of the file group
   *    uid: Number,    // User ID of the owner
   *    gid: Number,    // Group ID of the group
   *    inode: Number,  // Inode of the file
   *    device: Number, // Device identifier/no of the file
   *    mode: String,   // Octal mode representation like "644" or "755"
   *    modeval: Number // Numeric file mode bitmask, use `fs.mod2str(mode)` to convert to a string like 'drwxr-xr-x'.
   * }
   *
   * @param String path
   * @return Object | undefined
   */
  fs.stat = function(path) {};
  #endif
  template <typename PathAccessor, bool LinkStat>
  int filestat(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(LinkStat) {
      #ifndef WINDOWS
      if(::lstat(path.c_str(), &st) != 0) return 0;
      #else
      if(::stat(path.c_str(), &st) != 0) return 0;
      #endif
    } else {
      if(::stat(path.c_str(), &st) != 0) return 0;
    }
    stack.require_stack_top(5);
    stack.pop();
    stack.push_object();
    stack.set("path", PathAccessor::to_js(path));
    stack.set("size", st.st_size);
    #ifndef WINDOWS
    {
      stack.set("mtime", unix_timestamp(st.st_mtim));
      stack.set("ctime", unix_timestamp(st.st_ctim));
      stack.set("atime", unix_timestamp(st.st_atim));
      {
        char name[256];
        struct ::passwd pw, *ppw;
        struct ::group gr, *pgr;
        if((::getpwuid_r(st.st_uid, &pw, name, sizeof(name), &ppw) == 0) && pw.pw_name) {
          stack.set("owner", (const char*)pw.pw_name);
        }
        if((::getgrgid_r(st.st_gid, &gr, name, sizeof(name), &pgr) == 0) && gr.gr_name) {
          stack.set("group", (const char*)gr.gr_name);
        }
      }
      stack.set("uid", st.st_uid);
      stack.set("gid", st.st_gid);
      stack.set("inode", st.st_ino);
      stack.set("device", st.st_dev);
      stack.set("mode", mod2str(st.st_mode, 'o'));
      stack.set("modeval", st.st_mode);
    }
    #else
    stack.set("mtime", unix_timestamp(st.st_mtime));
    stack.set("ctime", unix_timestamp(st.st_ctime));
    stack.set("atime", unix_timestamp(st.st_atime));
    // not sure about the following values:
    stack.set("uid", st.st_uid);
    stack.set("gid", st.st_gid);
    stack.set("inode", st.st_ino);
    stack.set("device", st.st_rdev);
    stack.set("mode", mod2str(st.st_mode, 'o'));
    stack.set("modeval", st.st_mode);
    #endif
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns the file size in bytes of a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Number | undefined
   */
  fs.size = function(path) {};
  #endif
  template <typename PathAccessor>
  int filesize(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    stack.push(st.st_size);
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns the name of the file owner of a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return String | undefined
   */
  fs.owner = function(path) {};
  #endif
  template <typename PathAccessor>
  int fileowner(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    #ifndef WINDOWS
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    char name[256];
    struct ::passwd pw, *ppw;
    if((::getpwuid_r(st.st_uid, &pw, name, sizeof(name), &ppw) == 0) && pw.pw_name) {
      stack.push((const char*)pw.pw_name);
      return 1;
    } else {
      return 0;
    }
    #else
    PSID pSidOwner = NULL;
    char name[4096];
    DWORD namesz = 4096;
    SID_NAME_USE eUse = SidTypeUnknown;
    HANDLE hFile;
    PSECURITY_DESCRIPTOR pSD = NULL;
    memset(name, 0, sizeof(name));
    hFile = ::CreateFileA(path.c_str(), GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    bool ok = false;
    if(hFile == INVALID_HANDLE_VALUE) {
      return 0;
    } else if(::GetSecurityInfo(hFile,SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&pSidOwner,NULL,NULL,NULL,&pSD) != ERROR_SUCCESS) {
      ok = false;
    } else if(!LookupAccountSidA(NULL,pSidOwner, name, (LPDWORD)&namesz, NULL, NULL, &eUse)) {
      ok = false;
    } else {
      ok = true;
    }
    ::CloseHandle(hFile);
    if(!ok) {
      return 0;
    } else {
      stack.push(name);
      return 1;
    }
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns the group name of a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return String | undefined
   */
  fs.group = function(path) {};
  #endif
  template <typename PathAccessor>
  int filegroup(duktape::api& stack)
  {
    #ifndef WINDOWS
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    char name[256];
    struct ::group gr, *pgr;
    if((::getgrgid_r(st.st_gid, &gr, name, sizeof(name), &pgr) == 0) && gr.gr_name) {
      stack.push((const char*)gr.gr_name);
      return 1;
    } else {
      return 0;
    }
    #else
    (void)stack;
    return 0;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns last modified time a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Date | undefined
   */
  fs.mtime = function(path) {};
  #endif
  template <typename PathAccessor>
  int filemtime(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    #ifdef MACINTOSH
    stack.push(unix_timestamp(st.st_mtimespec.tv_sec));
    #elif defined(WINDOWS)
    stack.push(unix_timestamp(st.st_mtime));
    #else
    stack.push(unix_timestamp(st.st_mtim));
    #endif
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns last access time a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Date | undefined
   */
  fs.atime = function(path) {};
  #endif
  template <typename PathAccessor>
  int fileatime(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    #ifdef MACINTOSH
    stack.push(unix_timestamp(st.st_atimespec.tv_sec));
    #elif defined(WINDOWS)
    stack.push(unix_timestamp(st.st_atime));
    #else
    stack.push(unix_timestamp(st.st_atim));
    #endif
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns creation time a given file path, or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Date | undefined
   */
  fs.ctime = function(path) {};
  #endif
  template <typename PathAccessor>
  int filectime(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::stat st;
    if(::stat(path.c_str(), &st) != 0) return 0;
    #ifdef MACINTOSH
    stack.push(unix_timestamp(st.st_ctimespec.tv_sec));
    #elif defined(WINDOWS)
    stack.push(unix_timestamp(st.st_ctime));
    #else
    stack.push(unix_timestamp(st.st_ctim));
    #endif
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Returns true if a given path points to an existing "node" in the file system (file, dir, pipe, link ...),
   * false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.exists = function(path) {};
  #endif
  template <typename PathAccessor>
  int exists(duktape::api& stack)
  {
    #ifndef WINDOWS
    struct ::stat st;
    stack.push((stack.is<std::string>(0))
      && (::stat(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &st) == 0)
      && ((S_ISREG(st.st_mode) || S_ISDIR(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISLNK(st.st_mode) ||
           S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode)))
    );
    return 1;
    #else
    DWORD fa;
    stack.push((stack.is<std::string>(0))
      && ((fa=::GetFileAttributesA(PathAccessor::to_sys(stack.to<std::string>(0)).c_str())) != INVALID_FILE_ATTRIBUTES)
    );
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if a given path points to a regular file, false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.isfile = function(path) {};
  #endif
  template <typename PathAccessor>
  int isfile(duktape::api& stack)
  {
    #ifndef WINDOWS
    struct ::stat st;
    stack.push(stack.is<std::string>(0) && (::stat(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &st) == 0) && S_ISREG(st.st_mode));
    return 1;
    #else
    DWORD fa;
    stack.push((stack.is<std::string>(0))
      && ((fa=::GetFileAttributesA(PathAccessor::to_sys(stack.to<std::string>(0)).c_str())) != INVALID_FILE_ATTRIBUTES)
      && ((fa & FILE_ATTRIBUTE_DIRECTORY) == 0)
    );
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if a given path points to a directory, false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.isdir = function(path) {};
  #endif
  template <typename PathAccessor>
  int isdir(duktape::api& stack)
  {
    #ifndef WINDOWS
    struct ::stat st;
    stack.push(stack.is<std::string>(0) && (::stat(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &st) == 0) && S_ISDIR(st.st_mode));
    return 1;
    #else
    DWORD fa;
    stack.push((stack.is<std::string>(0))
      && ((fa=::GetFileAttributesA(PathAccessor::to_sys(stack.to<std::string>(0)).c_str())) != INVALID_FILE_ATTRIBUTES)
      && ((fa & FILE_ATTRIBUTE_DIRECTORY) != 0)
    );
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if a given path points to a link, false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.islink = function(path) {};
  #endif
  template <typename PathAccessor>
  int islink(duktape::api& stack)
  {
    #ifndef WINDOWS
    struct ::stat st; stack.push(stack.is<std::string>(0) && (::lstat(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &st) == 0) && S_ISLNK(st.st_mode));
    return 1;
    #else
    return_false; /// @todo: implement windows.islink
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if a given path points to a fifo (named pipe), false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.isfifo = function(path) {};
  #endif
  template <typename PathAccessor>
  int isfifo(duktape::api& stack)
  {
    #ifndef WINDOWS
    struct ::stat st;
    stack.push(stack.is<std::string>(0) && (::stat(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &st) == 0) && S_ISFIFO(st.st_mode));
    return 1;
    #else
    return_false;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if the current user has write permission to a given path,
   * false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.iswritable = function(path) {};
  #endif
  template <typename PathAccessor>
  int is_writable(duktape::api& stack)
  {
    #ifndef WINDOWS
    stack.push(stack.is<std::string>(0) && ::access(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), W_OK) == 0);
    return 1;
    #else
    stack.push(stack.is<std::string>(0) && _access(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), W_OK) == 0);
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if the current user has read permission to a given path,
   * false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.isreadable = function(path) {};
  #endif
  template <typename PathAccessor>
  int is_readable(duktape::api& stack)
  {
    #ifndef WINDOWS
    stack.push(stack.is<std::string>(0) && ::access(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), R_OK) == 0);
    return 1;
    #else
    stack.push(stack.is<std::string>(0) && _access(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), 4) == 0);
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns true if the current user has execution permission to a given path,
   * false otherwise or undefined on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.isexecutable = function(path) {};
  #endif
  template <typename PathAccessor>
  int is_executable(duktape::api& stack)
  {
    #ifndef WINDOWS
    stack.push(stack.is<std::string>(0) && ::access(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), X_OK) == 0);
    return 1;
    #else
    DWORD r=0;
    stack.push(stack.is<std::string>(0) && (::GetBinaryTypeA(PathAccessor::to_sys(stack.to<std::string>(0)).c_str(), &r) != 0));
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Returns the target path of a symbolic link, returns a String or `undefined` on error.
   * Does strictly require one String argument (the path).
   * Note: Windows: returns undefined, not implemented.
   *
   * @param String path
   * @return String | undefined
   */
  fs.readlink = function(path) {};
  #endif
  template <typename PathAccessor>
  int readlink(duktape::api& stack)
  {
    #ifndef WINDOWS
    if(!stack.is<std::string>(0)) return 0;
    std::string inpath = PathAccessor::to_sys(stack.to<std::string>(0));
    char outpath[PATH_MAX];
    ssize_t n = 0;
    if((n=::readlink(inpath.c_str(), outpath, sizeof(outpath))) <= 0) return 0; // error or empty return.
    outpath[n] = '\0';
    stack.push(PathAccessor::to_js(std::string(outpath)));
    return 1;
    #else
    (void) stack;
    return 0; // @todo: implement windows.readlink
    #endif
  }
  // </editor-fold>

  // <editor-fold desc="basic operations (c wrappers)" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Switches the current working directory to the specified path. Returns true on success, false on error.
   * Does strictly require one String argument (the input path).
   *
   * @param String path
   * @return Boolean
   */
  fs.chdir = function(path) {};
  #endif
  template <typename PathAccessor>
  int chdir(duktape::api& stack)
  {
    stack.push(stack.is<std::string>(0) && ::chdir(PathAccessor::to_sys(stack.to<std::string>(0)).c_str()) == 0);
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Creates a new empty directory for the specified path. Returns true on success, false on error.
   * Require one String argument (the input path), and one optional option argument.
   * If the options is "p" or "parents" (similar to unix `mkdir -p`), parent directories will be
   * created recursively. If the directory already exists, the function returns success, if the
   * creation of the directory or a parent directory fails, the function returns false.
   * Note that it is possible that the path might be only partially created in this case.
   *
   * @param String path
   * @param String options
   * @return Boolean
   */
  fs.mkdir = function(path, options) {};
  #endif
  template <typename PathAccessor>
  int mkdir(duktape::api& stack)
  {
    const mode_t mode = 0755;
    if(!stack.is<std::string>(0)) return_false;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    std::string options = stack.is<std::string>(1) ? stack.get<std::string>(1) : std::string();
    bool recursive = (!options.empty()) && ((options[0] == 'p') || (options[0] == 'r'));
    if((path.length() > PATH_MAX)) return_false;
    #ifndef WINDOWS
    if(::mkdir(path.c_str(), mode) == 0) {
      // ok, created
      return_true;
    } else if(errno == EEXIST) {
      // Race condition, someone else has created a directory or something else in between:
      // re-stat at end of function.
    } else if(recursive) {
      // With parent creation, we do this conventionally and with the absolute path.
      char dir[PATH_MAX];
      memset(dir, 0, PATH_MAX);
      std::copy(path.begin(), path.end(), dir);
      dir[PATH_MAX-1] = '\0';
      for(char *p=strchr(dir+1, '/'); p; p=strchr(p+1, '/')) {
        if(*(p-1) == '\\') continue; // escaped slash
        *p='\0';
        PathAccessor::ck_sys(PathAccessor::to_js(dir)); // access control for parent directories
        if((::mkdir(dir, mode) != 0) && (errno != EEXIST)) break;
        *p='/';
      }
      ::mkdir(path.c_str(), mode);
    }
    #else
    (void) mode;
    // if mkdir fails, the is-dir check below will indicate if failed or already existing.
    if((_mkdir(path.c_str()) == 0)) {
      // created
      return_true;
    } else if(recursive) {
      // With parent creation, we do this conventionally and with the absolute path.
      char dir[PATH_MAX];
      memset(dir, 0, PATH_MAX);
      std::copy(path.begin(), path.end(), dir);
      dir[PATH_MAX-1] = '\0';
      for(char *p=strchr(dir+1, '\\'); p; p=strchr(p+1, '\\')) {
        *p='\0';
        PathAccessor::ck_sys(PathAccessor::to_js(dir));
        if((_mkdir(dir) != 0) && (errno != EEXIST)) {
          std::cout << "errno= " << errno << " : " << ::strerror(errno) << std::endl;
          break;
        }
        *p='\\';
      }
      _mkdir(path.c_str());
    }
    #endif
    struct ::stat st;
    stack.push((::stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode)); // true if dir was created and accessible.
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Removes an empty directory specified by a given path. Returns true on success, false on error.
   * Does strictly require one String argument (the input path).
   * Note that the function also fails if the directory is not empty (no recursion),
   *
   * @param String path
   * @return Boolean
   */
  fs.rmdir = function(path) {};
  #endif
  template <typename PathAccessor>
  int rmdir(duktape::api& stack)
  {
    stack.push((stack.is<std::string>(0)) && (::rmdir(PathAccessor::to_sys(stack.to<std::string>(0)).c_str()) == 0));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Removes a file or link form the file system. Returns true on success, false on error.
   * Does strictly require one String argument (the input path).
   * Note that the function also fails if the given path is a directory. Use `fs.rmdir()` in this case.
   *
   * @param String path
   * @return Boolean
   */
  fs.unlink = function(path) {};
  #endif
  template <typename PathAccessor>
  int unlink(duktape::api& stack)
  {
    stack.push((stack.is<std::string>(0)) && (::unlink(PathAccessor::to_sys(stack.to<std::string>(0)).c_str()) == 0));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Changes the modification and access time of a file or directory. Returns true on success, false on error.
   * Does strictly require three argument: The input path (String), the last-modified time (Date) and the last
   * access time (Date).
   *
   * @param String path
   * @param Date mtime
   * @param Date atime
   * @return Boolean
   */
  fs.utime = function(path, mtime, atime) {};
  #endif
  template <typename PathAccessor>
  int utime(duktape::api& stack)
  {
    bool mtime_set = stack.is<struct ::timespec>(1);
    bool atime_set = stack.is<struct ::timespec>(2);
    if(!stack.is<std::string>(0) || (!mtime_set && !atime_set)) return_false;
    if(!mtime_set && !stack.is_undefined(1)) return_false;
    if(!atime_set && !stack.is_undefined(2)) return_false;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    struct ::utimbuf ut;
    if(!mtime_set || !atime_set) {
      struct ::stat st;
      if(::stat(path.c_str(), &st) != 0) return 0;
      #ifdef MACINTOSH
      ut.modtime = st.st_mtimespec.tv_sec;
      ut.actime = st.st_atimespec.tv_sec;
      #elif defined(WINDOWS)
      ut.modtime = st.st_mtime;
      ut.actime = st.st_atime;
      #else
      ut.modtime = st.st_mtim.tv_sec;
      ut.actime = st.st_atim.tv_sec;
      #endif
    }
    if(mtime_set) ut.modtime = stack.to<struct ::timespec>(1).tv_sec;
    if(atime_set) ut.actime = stack.to<struct ::timespec>(2).tv_sec;
    stack.push(::utime(path.c_str(), &ut) == 0);
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Changes the name of a file or directory. Returns true on success, false on error.
   * Does strictly require two String arguments: The input path and the new name path.
   * Note that this is a basic filesystem i/o function that fails if the parent directory,
   * or the new file does already exist.
   *
   * @param String path
   * @param String new_path
   * @return Boolean
   */
  fs.rename = function(path, new_path) {};
  #endif
  template <typename PathAccessor>
  int rename(duktape::api& stack)
  {
    stack.push(stack.is<std::string>(0) && stack.is<std::string>(1) && (::rename(
      PathAccessor::to_sys(stack.to<std::string>(0)).c_str(),
      PathAccessor::to_sys(stack.to<std::string>(1)).c_str()
    ) == 0));
    return 1;
  }

  #if(0 && JSDOC)
  /**
   * Creates a symbolic link, returns true on success, false on error.
   *
   * @param String path
   * @param String link_path
   * @return Boolean
   */
  fs.symlink = function(path, link_path) {};
  #endif
  template <typename PathAccessor>
  int symlink(duktape::api& stack)
  {
    #ifdef WINDOWS
      // Here we have a problem: It's not understandable to people why
      // "shortcuts" should not be available. Hence, here we throw to
      // clearify that actual symlinking is meant, not .lnk files.
      #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0601)
      stack.throw_exception("Your windows version does not support symlinks (this is not creating .lnk files).");
      return 0;
      #else
      if(!stack.is<std::string>(0) || !stack.is<std::string>(1)) { stack.push(false); return 1; }
      std::string src = PathAccessor::to_sys(stack.to<std::string>(0)).c_str();
      std::string dst = PathAccessor::to_sys(stack.to<std::string>(1)).c_str();
      bool dir = (::GetFileAttributesA(src.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
      stack.push(::CreateSymbolicLinkA(src.c_str(), dst.c_str(), dir ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0x00) != 0);
      return 1;
      #endif
    #else
    stack.push(stack.is<std::string>(0) && stack.is<std::string>(1) && (::symlink(
      PathAccessor::to_sys(stack.to<std::string>(0)).c_str(),
      PathAccessor::to_sys(stack.to<std::string>(1)).c_str()
    ) == 0));
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Creates a (hard) link, returns true on success, false on error.
   *
   * @param String path
   * @param String link_path
   * @return Boolean
   */
  fs.hardlink = function(path, link_path) {};
  #endif
  template <typename PathAccessor>
  int hardlink(duktape::api& stack)
  {
    #ifdef WINDOWS
    // @todo: implement hardlink for windows >=0x0601
    (void) stack;
    return 0;
    #else
    stack.push(stack.is<std::string>(0) && stack.is<std::string>(1) && (::link(
      PathAccessor::to_sys(stack.to<std::string>(0)).c_str(),
      PathAccessor::to_sys(stack.to<std::string>(1)).c_str()
    ) == 0));
    return 1;
    #endif
  }

  #if(0 && JSDOC)
  /**
   * Creates a (hard) link, returns true on success, false on error.
   *
   * @param String path
   * @param String|Number mode
   * @return Boolean
   */
  fs.chmod = function(path, mode) {};
  #endif
  template <typename PathAccessor>
  int chmod(duktape::api& stack)
  {
    #ifdef WINDOWS
    (void) stack;
    return 0;
    #else
    if(!stack.is<std::string>(0) || (!stack.is<std::string>(1) && !stack.is<unsigned>(1))) { stack.push(false); return 1; }
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0)).c_str();
    unsigned mode = 0xffff;
    if(!str2mod(stack.to<std::string>(1), mode)) {
      stack.push(false);
      return 1;
    }
    stack.push(::chmod(path.c_str(), mode) == 0);
    return 1;
    #endif
  }

  // </editor-fold>

  // <editor-fold desc="directory listing" defaultstate="collapsed">

  #ifdef WINDOWS
  namespace {
    template <typename=void>
    int win32_glob_push_stack(duktape::api& stack, std::string path_pattern)
    {
      struct dir_guard {
        HANDLE hFind;
        explicit dir_guard() noexcept : hFind(INVALID_HANDLE_VALUE) {}
        ~dir_guard() noexcept { if(hFind != INVALID_HANDLE_VALUE) FindClose(hFind); }
      };
      WIN32_FIND_DATAA ffd;
      dir_guard dir;
      DWORD err=0;
      if(path_pattern.length() >= MAX_PATH) return 0;
      stack.require_stack_top(5);
      duktape::api::array_index_t i=0;
      auto array_stack_index = stack.push_array();
      if((dir.hFind = FindFirstFileA(path_pattern.c_str(), &ffd)) == INVALID_HANDLE_VALUE) {
        err = GetLastError();
      } else {
        do {
          if((!ffd.cFileName) || (!ffd.cFileName[0])) continue;
          std::string s(ffd.cFileName);
          if((s == ".") || (s == "..")) continue;
          stack.push(s);
          if(!stack.put_prop_index(array_stack_index, i)) return 0;
          ++i;
        }
        while(FindNextFileA(dir.hFind, &ffd) != 0);
        err = GetLastError();
        FindClose(dir.hFind);
        dir.hFind = INVALID_HANDLE_VALUE;
      }
      return (err == ERROR_NO_MORE_FILES) ? 1 : 0;
    }
  }
  #endif

  #if(0 && JSDOC)
  /**
   * Lists the contents of a directory (basenames only), undefined if the function failed to open the directory
   * for reading. Results are unsorted.
   *
   * @param String path
   * @return Array|undefined
   */
  fs.readdir = function(path) {};
  #endif
  template <typename PathAccessor>
  int readdir(duktape::api& stack)
  {
    std::string path;
    if(stack.is_undefined(0)) {
      path = ".";
    } else if(!stack.is<std::string>(0)) {
      return 0;
    } else {
      path = PathAccessor::to_sys(stack.to<std::string>(0));
    }
    #ifndef WINDOWS
    struct dir_guard {
      DIR* dir;
      explicit dir_guard() noexcept : dir(nullptr) {}
      ~dir_guard() noexcept {if(dir) ::closedir(dir); }
    };
    dir_guard dir; // calls ::closedir if ::opendir does not return nullptr.
    struct ::dirent entry;
    struct ::dirent *de = nullptr;
    if(!(dir.dir = ::opendir(path.c_str()))) return 0;
    stack.require_stack_top(5);
    duktape::api::array_index_t i=0;
    auto array_stack_index = stack.push_array();
    int error = 0;
    while(((error = ::readdir_r(dir.dir, &entry, &de)) == 0) && (de != nullptr)) {
      if(de->d_name[0] == '.') {
        if((de->d_name[1] == '\0') || ((de->d_name[1] == '.') && (de->d_name[2] == '\0'))) {
          continue;
        }
      }
      stack.push(de->d_name);
      if(!stack.put_prop_index(array_stack_index, i)) return 0;
      ++i;
    }
    return 1;
    #else
    if(path.length() > ((MAX_PATH)-3)) return 0;
    path += "\\*";
    return win32_glob_push_stack(stack, path);
    #endif
  }

  #if(0 && JSDOC)
  /**
   * File pattern (fnmatch) based listing of files.
   *
   * @param String pattern
   * @return Array|undefined
   */
  fs.glob = function(pattern) {};
  #endif
  template <typename PathAccessor>
  int glob(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = stack.to<std::string>(0);
    #ifndef WINDOWS
    {
      struct glob_data {
        glob_t data;
        explicit inline glob_data() noexcept : data()
        { memset(&data, 0, sizeof(data)); }
        ~glob_data() noexcept
        { if(data.gl_pathv) { ::globfree(&data); }}
      };

      glob_data gb;
      if(::glob(path.c_str(), GLOB_DOOFFS, NULL, &gb.data) != 0) {
        switch(errno) {
          case GLOB_NOMATCH:
            stack.push_array();
            return 1; // empty array
          case GLOB_NOSPACE:
          case GLOB_ABORTED:
          default:
            return 0;
        }
      } else {
        duktape::api::array_index_t array_item_index=0;
        auto array_stack_index = stack.push_array();
        for(size_t i=0; (i < gb.data.gl_pathc) && (gb.data.gl_pathv[i]); ++i) {
          stack.push(gb.data.gl_pathv[i]);
          if(!stack.put_prop_index(array_stack_index, array_item_index)) return 0;
          ++array_item_index;
        }
        return 1;
      }
    }
    #else
    return win32_glob_push_stack(stack, path);
    #endif
  }

  // </editor-fold>

}}}}

namespace duktape { namespace mod { namespace filesystem { namespace basic {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  using namespace ::duktape::detail::filesystem;
  using namespace ::duktape::detail::filesystem::basic;

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename PathAccessor=path_accessor<std::string>>
  static void define_in(duktape::engine& js)
  {
    js.define("fs.cwd", cwd<PathAccessor>, 0);
    js.define("fs.pwd", cwd<PathAccessor>, 0);
    js.define("fs.tmpdir", tmpdir<PathAccessor>, 0);
//    js.define("fs.tempnam", tempnam<PathAccessor>, 1);
    js.define("fs.home", homedir<PathAccessor>, 0);
    js.define("fs.realpath", realpath<PathAccessor>, 1);
    js.define("fs.dirname", getdirname<PathAccessor>, 1);
    js.define("fs.basename", getbasename<PathAccessor>, 1);
    js.define("fs.stat", filestat<PathAccessor, false>, 1);
    js.define("fs.lstat", filestat<PathAccessor, true>, 1);
    js.define("fs.mtime", filemtime<PathAccessor>, 1);
    js.define("fs.ctime", filectime<PathAccessor>, 1);
    js.define("fs.atime", fileatime<PathAccessor>, 1);
    js.define("fs.owner", fileowner<PathAccessor>, 1);
    js.define("fs.group", filegroup<PathAccessor>, 1);
    js.define("fs.size", filesize<PathAccessor>, 1);
    js.define("fs.mod2str", mod2str<PathAccessor>, 2);
    js.define("fs.str2mod", str2mod<PathAccessor>, 1);

    js.define("fs.exists", exists<PathAccessor>, 1);
    js.define("fs.iswritable", is_writable<PathAccessor>, 1);
    js.define("fs.isreadable", is_readable<PathAccessor>, 1);
    js.define("fs.isexecutable", is_executable<PathAccessor>, 1);
    js.define("fs.isdir", isdir<PathAccessor>, 1);
    js.define("fs.isfile", isfile<PathAccessor>, 1);
    js.define("fs.islink", islink<PathAccessor>, 1);
    js.define("fs.isfifo", isfifo<PathAccessor>, 1);

    js.define("fs.chdir", chdir<PathAccessor>, 1);
    js.define("fs.mkdir", mkdir<PathAccessor>, 2);
    js.define("fs.rmdir", rmdir<PathAccessor>, 1);
    js.define("fs.unlink", unlink<PathAccessor>, 1);
    js.define("fs.rename", rename<PathAccessor>, 2);

    js.define("fs.readdir", readdir<PathAccessor>, 1);
    js.define("fs.glob", glob<PathAccessor>, 1);

    js.define("fs.symlink", symlink<PathAccessor>, 2);
    js.define("fs.utime", utime<PathAccessor>, 3);

    //#ifndef WINDOWS
    js.define("fs.hardlink", hardlink<PathAccessor>, 2);
    js.define("fs.readlink", readlink<PathAccessor>);
    js.define("fs.chmod", chmod<PathAccessor>, 2);
    //#endif
  }
  // </editor-fold>

}}}}
// </editor-fold>

// <editor-fold desc="enhanced filesystem operations" defaultstate="collapsed">
namespace duktape { namespace detail { namespace filesystem { namespace enhanced {

  // <editor-fold desc="find" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Recursive directory walking. The argument `path` specifies the root directory
   * of the file search - that is not a pattern with wildcards, but a absolute or
   * relative path. The second argument `options` can be
   *
   *  - a string: then it is the pattern to filter by the file name.
   *
   *  - a plain object with one or more of the properties:
   *
   *      - name: [String] Filter by file name match pattern (fnmatch based, means with '*','?', etc).
   *
   *      - type: [String] Filter by file type, where
   *
   *          - "d": Directory
   *          - "f": Regular file
   *          - "l": Symbolic link
   *          - "p": Fifo (pipe)
   *          - "s": Socket
   *          - "c": Character device (like /dev/tty)
   *          - "b": Block device (like /dev/sda)
   *
   *      - depth: [Number] Maximum directory recursion depth. `0` lists nothing, `1` the contents of the
   *               root directory, etc.
   *
   *      - logical: [Boolean] Default is `true`. This affects symbolic links. Normally people are interested
   *                 in the files that links refer to rather than the links themselves. Therefore the logical
   *                 search is default. In contrast, a physical search will refer to the links themselves
   *                 (set `logical:false`) and not to the files pointed to.
   *                 THAT IS ESPESIALLY IMPORTANT IF YOU WANT TO DELETE FILES BASED ON A FIND SEARCH.
   *
   *      - filter: [Function A callback invoked for each file that was not yet filtered out with the
   *                criteria listed above. The callback gets the file path as first argument. With that
   *                you can:
   *
   *                  - Add it to the output by returning `true`.
   *
   *                  - Not add it to the output list by returning `false`, `null`, or `undefined`. That is
   *                    useful e.g. if you don't want to list any files, but process them instead, or update
   *                    global/local accessible variables depending on the file paths you get.
   *
   *                  - Add a modified path or other string by returning a String. That is really useful
   *                    e.g. if you want to directly return the contents of files, or checksums etc etc etc.
   *                    You get a path, and specify the output yourself.
   *
   *
   * @param String path
   * @param String|Object options
   * @return Array|undefined
   */
  fs.find = function(path, options) {};
  #endif
  template <typename PathAccessor>
  int findfiles(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return 0;
    std::string path = PathAccessor::to_sys(stack.to<std::string>(0));
    std::string pattern, ftype;
    int depth = std::numeric_limits<int>::max();
    bool logical_find = true;
    bool no_outside = true;
    mode_t mode = 0;
    duktape::api::index_t filter_function = 0;

    if(!stack.is_undefined(1)) {
      if(stack.is<std::string>(1)) {
        pattern = stack.to<std::string>(1);
      } else if(stack.is_object(1)) {
        pattern = stack.get_optional_property<std::string>(1, "name");
        ftype = stack.get_optional_property<std::string>(1, "type");
        depth = stack.get_optional_property<int>(1, "depth", depth);
        logical_find = stack.get_optional_property<bool>(1, "logical", true);
        if(!ftype.empty()) {
          if(ftype.find('l') != ftype.npos) mode |= S_IFLNK;
          if(ftype.find('d') != ftype.npos) mode |= S_IFDIR;
          if(ftype.find('f') != ftype.npos) mode |= S_IFREG;
          if(ftype.find('p') != ftype.npos) mode |= S_IFIFO;
          if(ftype.find('c') != ftype.npos) mode |= S_IFCHR;
          if(ftype.find('b') != ftype.npos) mode |= S_IFBLK;
          if(ftype.find('s') != ftype.npos) mode |= S_IFSOCK;
        }
        if(stack.has_prop_string(1, "filter")) {
          stack.get_prop_string(1, "filter");
          if(stack.is_function(-1)) {
            filter_function = stack.top()-1;
          } else {
            stack.throw_exception("The filter setting for reading a directory must be a function.");
            return 0;
          }
        }
      } else {
        stack.throw_exception("Invalid configuration for find function.");
        return 0;
      }
    }

    #ifndef WINDOWS
    struct fts_guard {
      ::FTS* ptr;
      explicit fts_guard() noexcept : ptr(nullptr) {}
      ~fts_guard() noexcept { if(ptr) ::fts_close(ptr); }
    };

    auto fts_entcmp = [](const ::FTSENT **a, const ::FTSENT **b) {
      return ::strcmp((*a)->fts_name, (*b)->fts_name);
    };

    fts_guard tree;
    ::FTSENT *f;
    {
      char apath[PATH_MAX];
      memset(apath, 0, sizeof(apath));
      std::copy(path.begin(), path.end(), apath);
      char *ppath[] = { apath, nullptr };
      if(!(tree.ptr = ::fts_open(ppath, (logical_find ? FTS_LOGICAL:FTS_PHYSICAL) | ((!mode) ? FTS_NOSTAT:0), fts_entcmp))) return 0;
    }
    errno = 0;
    duktape::api::array_index_t array_item_index=0;
    auto array_stack_index = stack.push_array();
    while((f=::fts_read(tree.ptr))) {
      switch(f->fts_info) {
        case FTS_DNR:
        case FTS_ERR:
        case FTS_NS: // Add to skipped list
          continue;
        case FTS_DOT:
        case FTS_DP:
          continue;
        default:
          if(0
          || (no_outside && (f->fts_level < 0)) || (f->fts_level > depth) // respect max depth, do not include parent directories.
          // array address, never nullptr : || (!f->fts_name) || (!f->fts_path) // pointer checks
          ) {
            continue;
          } else if(mode) {
            if(!f->fts_statp || !(f->fts_statp->st_mode & mode)) {
              continue;
            } else {
              // optimise!
              mode_t m = f->fts_statp->st_mode;
              if(!(0
                || (S_ISREG(m) && (ftype.find('f') != ftype.npos))
                || (S_ISDIR(m) && (ftype.find('d') != ftype.npos))
                || (S_ISLNK(m) && (ftype.find('l') != ftype.npos))
                || (S_ISFIFO(m) && (ftype.find('p') != ftype.npos))
                || (S_ISCHR(m) && (ftype.find('c') != ftype.npos))
                || (S_ISBLK(m) && (ftype.find('b') != ftype.npos))
                || (S_ISSOCK(m) && (ftype.find('s') != ftype.npos))
              )) {
                continue;
              };
            }
          }
          std::string item(f->fts_path);
          if(pattern.empty() || ::fnmatch(pattern.c_str(), f->fts_name, FNM_PERIOD) == 0) {
            if(filter_function) {
              stack.dup(filter_function);
              stack.push(f->fts_path);
              stack.call(1);
              if(stack.is<std::string>(-1)) {
                // 1. Filter returns a string: Means a modified version of the path shall be added.
                item = stack.to<std::string>(-1);
              } else if(stack.is<bool>(-1)) {
                if(stack.get<bool>(-1)) {
                  // 2. Filter returns true: add.
                } else {
                  // 3. Filter returns false: don't add.
                  item.clear();
                }
              } else if(stack.is_undefined(-1) || stack.is_null(-1)) {
                  // 4. Filter returns undefined: Means, don't add, the callback
                item.clear();
              } else {
                stack.throw_exception("The 'find.filter' function must return a string, true/false or nothing (undefined).");
                return 0;
              }
              stack.pop();
            }
            if(!item.empty()) {
              stack.push(item);
              if(!stack.put_prop_index(array_stack_index, array_item_index)) return 0;
              ++array_item_index;
            }
          } else if(f->fts_info == FTS_DC) {
            // recursion warning list?
          } else {
            // no match
          }
      }
    }
    ::fts_close(tree.ptr);
    tree.ptr = nullptr;
    return (!errno);
    #else
    /// @todo: implement findfiles for windows
    (void) path;
    (void) logical_find;
    (void) no_outside;
    return 0;
    #endif
  }

  // </editor-fold>

  // <editor-fold desc="auxiliary execution" defaultstate="collapsed">
  namespace {
    /**
     * Executes args[0], passing args as arguments. Optionally passes
     * a C-string as stdin and closes the stdin-pipe. Implicitly redirects
     * stdout and stderr to /dev/null. Programs must be specified with
     * full path, no environment is passed to the child process.
     *
     * @param std::vector<std::string>&& args
     * @param const char* pipe_stdin=nullptr
     * @return int
     */
    #ifndef WINDOWS
    template <typename=void>
    int sysexec(std::vector<std::string>&& args, const char* pipe_stdin=nullptr)
    {
      // { std::string s; for(auto e:args) { s += string("'") + e + "' "; } std::cerr << s << std::endl; }
      using fd_t = int;
      fd_t pi[2] = {-1,-1};
      ::pid_t pid = -1;
      std::vector<const char*> argv;
      for(auto& e:args) argv.push_back(e.c_str());
      argv.push_back(nullptr);
      argv.push_back(nullptr);
      if(::pipe(pi)) return -1;
      if((pid = ::fork()) < 0) {
        ::close(pi[0]);
        ::close(pi[1]);
        return -2;
      } else if(pid == 0) {
        if(::dup2(pi[0], STDIN_FILENO) < 0) _exit(1);
        ::close(STDOUT_FILENO); ::open("/dev/null", O_WRONLY);
        ::close(STDERR_FILENO); ::open("/dev/null", O_WRONLY);
        for(fd_t i=3; i<128; ++i) ::close(i);
        ::execv(argv[0], (char* const*)(&argv[0]));
        _exit(1);
      } else {
        if(pipe_stdin && pipe_stdin[0]) {
          ssize_t r=::write(pi[1], pipe_stdin, ::strlen(pipe_stdin));
          (void)r;
        }
        ::close(pi[1]);
        int status = -1;
        pid_t p = -1;
        while((p=::waitpid(pid, &status, 0)) <= 0) {
          if(p < 0) {
            switch(errno) {
              case EAGAIN:
              case EINTR:
                break;
              case ECHILD:
              default:
                return -4;
            }
          }
        }
        return WEXITSTATUS(status);
      }
      return -5;
    }
    #endif /*ndef windows*/
  }
  // </editor-fold>

  // <editor-fold desc="copy" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Copies a file from one location `source_path` to another (`target_path`),
   * similar to the `cp` shell command. The argument `options` can  encompass
   * the key-value pairs
   *
   *    {
   *      "recursive": Boolean (default false)
   *    }
   *
   * Optionally, it is possible to specify the string 'r' or '-r' instead of
   * `{recursive:true}` as third argument.
   *
   * @param String source_path
   * @param String target_path
   * @param undefined|Object options
   * @return Boolean
   */
  fs.copy = function(source_path, target_path, options) {};
  #endif
  template <typename PathAccessor>
  int copyfile(duktape::api& stack)
  {
    if(!stack.is<std::string>(0) || !stack.is<std::string>(1)) return_false;
    std::string src = PathAccessor::to_sys(stack.to<std::string>(0));
    std::string dst = PathAccessor::to_sys(stack.to<std::string>(1));
    bool recursive = false;
    if(!stack.is_undefined(2)) {
      if(stack.is_object(2)) {
        recursive = stack.get_optional_property<bool>(2, "recursive", false);
      } else if(stack.is_string(2)) {
        std::string s = stack.get_string(2);
        if(s == "r" || s == "R" || s == "-r" || s == "-R") {
          recursive = true;
        } else if(!s.empty()) {
          stack.throw_exception("String options can be only 'r' for recursive copying.");
          return 0;
        }
      } else {
        stack.throw_exception("Invalid configuration for copy function (must be plain object or string).");
        return 0;
      }
    }

    // Empty / identical paths check
    if(src.empty() || dst.empty() || src == dst) return_false;

    #ifndef WINDOWS
    // Composition of args, invoke POSIX cp
    std::vector<std::string> args;
    args.emplace_back("/bin/cp");
    std::string ops = "-f";
    if(recursive) ops += 'R';
    args.emplace_back(std::move(ops));
    args.emplace_back("--");
    args.emplace_back(src);
    args.emplace_back(dst);
    stack.push(sysexec<>(std::move(args)) == 0);
    return 1;
    #else
    /// @ŧodo implement copyfile for windows
    (void) stack;
    (void) recursive;

    return 0;
    #endif
  }

  // </editor-fold>

  // <editor-fold desc="move" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Moves a file or directory from one location `source_path` to another (`target_path`),
   * similar to the `mv` shell command.
   *
   * @param String source_path
   * @param String target_path
   * @return Boolean
   */
  fs.move = function(source_path, target_path) {};
  #endif
  template <typename PathAccessor>
  int movefile(duktape::api& stack)
  {
    if(!stack.is<std::string>(0) || !stack.is<std::string>(1)) return_false;
    std::string src = PathAccessor::to_sys(stack.to<std::string>(0));
    std::string dst = PathAccessor::to_sys(stack.to<std::string>(1));

    // Empty / identical paths check
    if(src.empty() || dst.empty() || src == dst) return_false;

    #ifndef WINDOWS
    // Composition of args, invoke POSIX cp
    std::vector<std::string> args;
    args.emplace_back("/bin/mv");
    args.emplace_back("--");
    args.emplace_back(src);
    args.emplace_back(dst);
    stack.push(sysexec<>(std::move(args)) == 0);
    return 1;
    #else
    /// @ŧodo implement movefile for windows
    return 0;
    #endif
  }

  // </editor-fold>

  // <editor-fold desc="remove" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Deletes a file or directory (`target_path`), similar to the `rm` shell
   * command. The argument `options` can  encompass the key-value pairs
   *
   *    {
   *      "recursive": Boolean (default false)
   *    }
   *
   * Optionally, it is possible to specify the string 'r' or '-r' instead of
   * `{recursive:true}` as third argument.
   *
   * Removing is implicitly forced (like "rm -f").
   *
   * @param String target_path
   * @param undefined|Object options
   * @return Boolean
   */
  fs.remove = function(target_path, options) {};
  #endif
  template <typename PathAccessor>
  int removefile(duktape::api& stack)
  {
    if(!stack.is<std::string>(0)) return_false;
    std::string dst = PathAccessor::to_sys(stack.to<std::string>(0));
    bool recursive = false;
    if(!stack.is_undefined(1)) {
      if(stack.is_object(1)) {
        recursive = stack.get_optional_property<bool>(1, "recursive", false);
      } else if(stack.is_string(1)) {
        std::string s = stack.get_string(1);
        if(s == "r" || s == "-r") {
          recursive = true;
        } else if(!s.empty()) {
          stack.throw_exception("String options can be only 'r' for recursive removing.");
          return 0;
        }
      } else {
        stack.throw_exception("Invalid configuration for remove function (must be plain object or string).");
        return 0;
      }
    }

    // Empty / identical paths check
    if(dst.empty()) return_false;

    #ifndef WINDOWS
    // Composition of args, invoke POSIX rm
    std::vector<std::string> args;
    args.emplace_back("/bin/rm");
    std::string ops = "-f";
    if(recursive) ops += 'r';
    args.emplace_back(std::move(ops));
    args.emplace_back("--");
    args.emplace_back(dst);
    stack.push(sysexec<>(std::move(args)) == 0);
    return 1;
    #else
    /// @ŧodo implement removefile for windows
    (void) stack;
    (void) recursive;
    return 0;
    #endif
  }

  // </editor-fold>

}}}}

namespace duktape { namespace mod { namespace filesystem { namespace enhanced {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  using namespace ::duktape::detail::filesystem;
  using namespace ::duktape::detail::filesystem::enhanced;
  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename PathAccessor=path_accessor<std::string>>
  static void define_in(duktape::engine& js)
  {
    js.define("fs.find", findfiles<PathAccessor>, 2);
    js.define("fs.copy", copyfile<PathAccessor>, 3);
    js.define("fs.move", movefile<PathAccessor>, 3);
    js.define("fs.remove", removefile<PathAccessor>, 2);
  }
  // </editor-fold>

}}}}
// </editor-fold>

// <editor-fold desc="undefs" defaultstate="collapsed">
#undef return_true
#undef return_false
#undef return_undefined
// </editor-fold>

#endif
