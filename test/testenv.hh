#ifndef DUKTAPE_HH_TESTING_ENVIRONMENT_HH
#define	DUKTAPE_HH_TESTING_ENVIRONMENT_HH

#include "../duktape/duktape.hh"
#include "microtest.hh"
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
#endif

#ifndef TEST_DIR
  #define TEST_DIR (::sw::utest::tmpdir::path())
#endif

// <editor-fold desc="auxiliary fs test functions" defaultstate="collapsed">
#ifndef WINDOWS
inline int sysshellexec(std::string cmd) { int r=::system(cmd.c_str()); return WEXITSTATUS(r); }
inline bool exists(std::string file) { struct ::stat st; return (::stat(file.c_str(), &st)==0); }
inline std::string test_path(std::string path="") { return path.empty() ? std::string(TEST_DIR) : (std::string(TEST_DIR) + "/" + path); }
#else
inline int sysshellexec(std::string cmd) { return ::system(cmd.c_str()); }
inline bool exists(std::string file) { struct ::stat st; return (::stat(file.c_str(), &st)==0); }
inline std::string test_path(std::string path="") { return path.empty() ? std::string(TEST_DIR) : (std::string(TEST_DIR) + "\\" + path); }
#endif
inline void test_mkfile(std::string path) { std::ofstream fos(test_path(path)); fos << path << std::endl; }

namespace {
  inline std::string to_string(std::string s) { return s; }
  inline std::string to_string(const char* c) { return std::string(c); }
  template<typename Head> inline std::string joined_to_string(Head&& h) { return to_string(h); }
  template<typename Head, typename ...Tail> inline std::string joined_to_string(Head&& h, Tail&& ...t) { return to_string(h) + joined_to_string(t...); }
}
template<typename ...Args>
inline int sysexec(Args&&... args) { return sysshellexec(joined_to_string(args...)); }
// </editor-fold>

/**
 * THIS IS THE FUNCTION TO IMPLEMENT FOR THE TEST
 * @param duktape::engine& js
 */
void test(duktape::engine& js);

int ecma_assert(duk_context *ctx) {
  (void)ctx; return 1;
}

int ecma_print(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if((nargs == 1) && duk.is_buffer(0)) {
    const char *buf = NULL;
    duk_size_t sz = 0;
    if((buf = (const char *) duk.get_buffer(0, sz)) && sz > 0) {
      std::cout.write(buf, sz);
      std::cout.flush();
    }
  } else if(nargs > 0) {
    std::cout << duk.to<std::string>(0);
    for(int i=1; i<nargs; i++) std::cout << " " << duk.to<std::string>(i);
    std::cout << std::endl;
    std::cout.flush();
  }
  return 0;
}

int ecma_alert(duk_context *ctx) {
  duktape::api duk(ctx);
  int nargs = duk.top();
  if(!nargs) return 0;
  std::cerr << duk.to<std::string>(0);
  for(int i=1; i<nargs; i++) std::cerr << " " << duk.to<std::string>(i);
  std::cerr << std::endl;
  std::cerr.flush();
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


int test_main(int argc, const char **argv) {
  (void)argc;
  (void)argv;
  try {
    duktape::engine js;
    js.define("print", ecma_print);
    js.define("alert", ecma_alert);
    js.define("fail", ecma_fail);
    js.define("pass", ecma_pass);
    js.define("expect", ecma_expect);
    js.define("comment", ecma_comment);

    try {
      test(js);
    } catch(std::exception& e) {
      test_fail("Unexpected exception: '", e.what(), "'.");
    } catch (...) {
      test_fail("Unexpected exception.");
    }
  } catch(std::exception& e) {
    test_fail("Unexpected exception: '", e.what(), "'.");
  } catch (...) {
    test_fail("Unexpected exception.");
  }
  return sw::utest::test::summary();
}

/* implicit main function */
int main(int argc, const char** argv)
{ return test_main(argc, argv); }

#endif
