#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.fs.ext.hh>

using namespace std;

void mk_test_tree()
{
  test_comment( "testdir = " << TEST_DIR );
  sysexec("rm -rf ", TEST_DIR);

  #define TEST_NUMDIRS (12)
  sysexec("mkdir -p ", TEST_DIR);
  sysexec("mkdir -p ", TEST_DIR, "/a/b/c");
  sysexec("mkdir -p ", TEST_DIR, "/b/b/c");
  sysexec("mkdir -p ", TEST_DIR, "/c/b/c");
  sysexec("mkdir -p ", TEST_DIR, "/d/b/c");

  #define TEST_NUMFILES (6)
  sysexec("touch ", TEST_DIR, "/z");
  sysexec("touch ", TEST_DIR, "/y");
  sysexec("touch ", TEST_DIR, "/x");
  sysexec("touch ", TEST_DIR, "/a/w");
  sysexec("touch ", TEST_DIR, "/b/b/v");
  sysexec("touch ", TEST_DIR, "/c/b/c/u");

  #define TEST_NUMSYMLINKS (1)
  sysexec("ln -s ", TEST_DIR, "/a ", TEST_DIR, "/a/b/l");
}

void test(duktape::engine& js)
{
#ifndef WINDOWS
  duktape::mod::filesystem::basic::define_in<>(js);
  duktape::mod::filesystem::extended::define_in<>(js);
  js.define("testdir", TEST_DIR);
  js.define("num_symlinks", TEST_NUMSYMLINKS);
  js.define("num_files", TEST_NUMFILES);
  js.define("num_dirs", TEST_NUMDIRS);
  mk_test_tree();
  js.include("test.js");
  test_expect(sysexec("rm -rf ", TEST_DIR) == 0);
#else
  (void) js;
#endif
}
