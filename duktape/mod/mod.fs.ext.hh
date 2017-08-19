/**
 * @file duktape/mod/mod.fs.ext.hh
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
 * Optional extended file system functionality.
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
#ifndef DUKTAPE_MOD_BASIC_FILESYSTEM_EXT_HH
#define DUKTAPE_MOD_BASIC_FILESYSTEM_EXT_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "mod.fs.hh" /* All settings and definitions of fs apply */
#define return_false { stack.push(false); return 1; }
#define return_true { stack.push(true); return 1; }
#define return_undefined { return 0; }
// </editor-fold>

namespace duktape { namespace detail { namespace filesystem { namespace extended {

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
        pattern = stack.get_prop_string<std::string>(1, "name", std::string());
        ftype = stack.get_prop_string<std::string>(1, "type", std::string());
        depth = stack.get_prop_string<int>(1, "depth", depth);
        logical_find = stack.get_prop_string<bool>(1, "logical", true);
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
        recursive = stack.get_prop_string<bool>(2, "recursive", false);
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
        recursive = stack.get_prop_string<bool>(1, "recursive", false);
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

namespace duktape { namespace mod { namespace filesystem { namespace extended {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  using namespace ::duktape::detail::filesystem;
  using namespace ::duktape::detail::filesystem::extended;
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

// <editor-fold desc="undefs" defaultstate="collapsed">
#undef return_true
#undef return_false
#undef return_undefined
// </editor-fold>

#endif
