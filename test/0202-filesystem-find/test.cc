#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.fs.ext.hh>

using namespace std;
using namespace testenv;

void mk_test_tree()
{
  test_comment( "testdir = " << test_path() );
  test_rmfiletree();

  #define TEST_NUMDIRS (12)
  test_makedir("a");
  test_makedir("b");
  test_makedir("c");
  test_makedir("d");
  test_makedir("a/b");
  test_makedir("b/b");
  test_makedir("c/b");
  test_makedir("d/b");
  test_makedir("a/b/c");
  test_makedir("b/b/c");
  test_makedir("c/b/c");
  test_makedir("d/b/c");

  #define TEST_NUMFILES (6)
  test_makefile("z");
  test_makefile("y");
  test_makefile("x");
  test_makefile("a/w");
  test_makefile("b/b/v");
  test_makefile("c/b/c/u");

  #ifndef WINDOWS
  #define TEST_NUMSYMLINKS int(1)
  test_makesymlink("a", "/a/b/l");
  #else
  #define TEST_NUMSYMLINKS int(0)
  #endif
}

void test(duktape::engine& js)
{
  duktape::mod::filesystem::basic::define_in<>(js);
  duktape::mod::filesystem::extended::define_in<>(js);
  js.define("testdir", test_path());
  js.define("num_symlinks", TEST_NUMSYMLINKS);
  js.define("num_files", TEST_NUMFILES);
  js.define("num_dirs", TEST_NUMDIRS);
  mk_test_tree();
  test_include_script(js);
  test_rmfiletree();
}
