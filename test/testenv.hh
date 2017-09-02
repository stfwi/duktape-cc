#ifndef DUKTAPE_HH_TESTING_ENVIRONMENT_HH
#define	DUKTAPE_HH_TESTING_ENVIRONMENT_HH

#include "../duktape/duktape.hh"
#include "microtest.hh"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <clocale>

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
#endif

// <editor-fold desc="auxiliary fs test functions" defaultstate="collapsed">
namespace testenv {
  #ifndef WINDOWS
  int sysshellexec(std::string cmd) {
    int r=::system(cmd.c_str());
    return WEXITSTATUS(r);
  }

  bool exists(std::string file) {
    struct ::stat st;
    return (::stat(file.c_str(), &st)==0);
  }

  std::string test_path(std::string path="") {
    while(!path.empty() && path.front() == '/') path = path.substr(1);
    return path.empty() ? ::sw::utest::tmpdir::path() : (::sw::utest::tmpdir::path() + "/" + path);
  }

  void test_makesymlink(std::string src, std::string dst) {
    src = test_path(src);
    dst = test_path(dst);
    if(::symlink(src.c_str(), dst.c_str()) != 0) {
      test_fail(std::string("test_makesymlink(") + src + "," + dst + ") failed");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
  }
  #else
  int sysshellexec(std::string cmd) { return ::system(cmd.c_str()); }
  bool exists(std::string file) {
    for(auto& e:file) if(e=='/') e='\\';
    struct ::stat st; return (::stat(file.c_str(), &st)==0);
  }

  std::string test_path(std::string path="") {
    while(!path.empty() && path.front() == '/') path = path.substr(1);
    path = path.empty() ? ::sw::utest::tmpdir::path() : (::sw::utest::tmpdir::path() + "\\" + path);
    for(auto& e:path) if(e=='/') e='\\';
    return path;
  }

  void test_makesymlink(std::string src, std::string dst) {
    (void) src;
    (void) dst;
  }
  #endif

  void test_makefile(std::string path) {
    std::ofstream fos(test_path(path));
    fos << path << std::endl;
  }

  namespace {
    inline std::string to_string(std::string s) { return s; }
    inline std::string to_string(const char* c) { return std::string(c); }
    template<typename Head> inline std::string joined_to_string(Head&& h) { return to_string(h); }
    template<typename Head, typename ...Tail> inline std::string joined_to_string(Head&& h, Tail&& ...t) { return to_string(h) + joined_to_string(t...); }
  }
  template<typename ...Args>
  inline int sysexec(Args&&... args) { return sysshellexec(joined_to_string(args...)); }

  void test_rmfiletree()
  {
    using namespace std;
    char pwd[PATH_MAX+1];
    ::memset(pwd, 0, sizeof(pwd));
    if(!::getcwd(pwd, PATH_MAX)) {
      test_fail("test_rmfiletree() failed");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    if(::chdir(test_path().c_str()) == 0) {
      char s[PATH_MAX+2];
      ::memset(s, 0, sizeof(s));
      if(::getcwd(s, PATH_MAX) && (test_path() == s)) {
        sysexec(std::string("rm -rf -- *"));
      } else {
        if(::chdir(pwd) != 0) {}
        test_fail("test_rmfiletree() failed");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
    if(::chdir(pwd) != 0) {}
  }

  void test_makedir(std::string path, bool ignore_error=false)
  {
    #ifndef WINDOWS
    if(::mkdir(test_path(path).c_str(), 0755) != 0) {
    #else
    if(::mkdir(test_path(path).c_str()) != 0) {
    #endif
      if(!ignore_error) {
        test_fail(std::string("test_makedir(") + test_path(path) + ") failed");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
  }

  void test_makefiletree()
  {
    using namespace std;
    test_comment("(Re)building test temporary file tree '" << test_path() << "'");
    test_rmfiletree();
    test_makedir(test_path(), true);
    #ifndef WINDOWS
    {
      if(::chdir("/tmp") != 0) {
        test_fail("Failed to chdir to /tmp");
        throw std::runtime_error("Aborted due to failed test assertion.");
      } else if(test_path().find("/tmp/") != 0) {
        test_fail("test_path() not in /tmp");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
    #endif
    if(!exists(test_path()) || (::chdir(test_path().c_str()) != 0)) {
      test_fail("Test base temp directory does not exist or failed to chdir into it.");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    test_makedir("a");
    test_makedir("b");
    test_makefile("null");
    test_makefile("undefined");
    test_makefile("z");
    test_makefile("a/y");
    test_makefile("b/x");
    test_makefile("b/w with whitespace");
    test_makesymlink("a/y", "ly");
    test_makesymlink("a", "b/la");
    test_makesymlink("b/w with whitespace", "lw with whitespace");
  }
}
// </editor-fold>

/**
 * THIS IS THE FUNCTION TO IMPLEMENT FOR THE TEST
 * @param duktape::engine& js
 */
void test(duktape::engine& js);

// returns an absolute path to the given unix path (relative to test temp directory)
int ecma_testabspath(duk_context *ctx) {
  duktape::api duk(ctx);
  if(!duk.top() || (!duk.is_string(0))) return 0;
  duk.push(testenv::test_path(duk.get<std::string>(0)));
  return 1;
}

// returns a relative path to the given unix path (relative to test temp directory)
int ecma_testrelpath(duk_context *ctx) {
  duktape::api duk(ctx);
  if(!duk.top() || (!duk.is_string(0))) return 0;
  std::string path = duk.get<std::string>(0);
  while(!path.empty() && path.front() == '/') path = path.substr(1);
  #ifdef WINDOWS
  for(auto& e:path) if(e=='/') e='\\';
  #endif
  duk.push(path);
  return 1;
}

int ecma_assert(duk_context *ctx) {
  (void)ctx; return 1;
}

int ecma_print(duk_context *ctx) {
  duktape::api duk(ctx);
  std::stringstream ss;
  int nargs = duk.top();
  if((nargs == 1) && duk.is_buffer(0)) {
    const char *buf = NULL;
    duk_size_t sz = 0;
    if((buf = (const char *) duk.get_buffer(0, sz)) && sz > 0) {
      ss.write(buf, sz);
    }
  } else if(nargs > 0) {
    ss << duk.to<std::string>(0);
    for(int i=1; i<nargs; i++) ss << " " << duk.to<std::string>(i);
  }
  ::sw::utest::test::comment("(stdout)", 0, ss.str());
  return 0;
}

int ecma_alert(duk_context *ctx) {
  duktape::api duk(ctx);
  std::stringstream ss;
  int nargs = duk.top();
  if(!nargs) return 0;
  ss << duk.to<std::string>(0);
  for(int i=1; i<nargs; i++) ss << " " << duk.to<std::string>(i);
  ::sw::utest::test::comment("(stderr)", 0, ss.str());
  return 0;
}

int ecma_pass(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if(!nargs) return 0;
  std::string s(duk.to<std::string>(0));
  for(int i=1; i<nargs; i++) s += std::string(" ") + duk.to<std::string>(i);
  ::sw::utest::test::pass("(js)", 0, s);
  duk.push(true);
  return 1;
}

int ecma_fail(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if(!nargs) return 0;
  std::string s(duk.to<std::string>(0));
  for(int i=1; i<nargs; i++) s += std::string(" ") + duk.to<std::string>(i);
  ::sw::utest::test::fail("(js)", 0, s);
  duk.push(false);
  return 1;
}

int ecma_comment(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if(!nargs) return 0;
  std::string s(duk.to<std::string>(0));
  for(int i=1; i<nargs; i++) s += std::string(" ") + duk.to<std::string>(i);
  ::sw::utest::test::comment("(js)", 0, s);
  duk.push(true);
  return 1;
}

int ecma_expect(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if(!nargs) return 0;
  bool passed = duk.to<bool>(0);
  if(nargs > 1) {
    std::string s(duk.to<std::string>(1));
    for(int i=2; i<nargs; i++) s += std::string(" ") + duk.to<std::string>(i);
    if(passed) {
      ::sw::utest::test::pass("(js)", 0, s);
    } else {
      ::sw::utest::test::fail("(js)", 0, s);
    }
  }
  duk.push(passed);
  return 1;
}

int test_main(int argc, const char **argv)
{
  try {
    std::locale::global(std::locale("C"));
    ::setlocale(LC_ALL, "C");

    duktape::engine js;
    js.define("print", ecma_print); // may be overwritten by stdio
    js.define("alert", ecma_alert); // may be overwritten by stdio
    js.define("fail", ecma_fail);
    js.define("pass", ecma_pass);
    js.define("expect", ecma_expect);
    js.define("comment", ecma_comment);
    js.define("testabspath", ecma_testabspath, 1);
    js.define("testrelpath", ecma_testrelpath, 1);
    {
      std::vector<std::string> args;
      for(int i=1; i<argc && argv[i]; ++i) {
        args.emplace_back(argv[i]);
        js.define("sys.args", args);
      }
    }
    try {
      test(js);
    } catch(duktape::exit_exception& e) {
      test_note(std::string("Exit with code ") + std::to_string(e.exit_code()) + "'.");
    } catch(duktape::script_error& e) {
      test_fail("Unexpected script error: ", e.callstack());
    } catch(duktape::engine_error& e) {
      test_fail("Unexpected engine error: ", e.what());
    } catch(std::exception& e) {
      test_fail("Unexpected exception: '", e.what(), "'.");
    } catch (...) {
      test_fail("Unexpected exception.");
    }
  } catch(std::exception& e) {
    test_fail("Unexpected init exception: '", e.what(), "'.");
  } catch (...) {
    test_fail("Unexpected init exception.");
  }
  ::sw::utest::tmpdir::remove();
  return sw::utest::test::summary();
}

/* implicit main function */
int main(int argc, const char** argv)
{ return test_main(argc, argv); }

#endif
