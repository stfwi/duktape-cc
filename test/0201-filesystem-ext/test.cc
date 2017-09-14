// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.fs.ext.hh>
using namespace std;
using namespace testenv;
// </editor-fold>

// <editor-fold desc="test_copy_function" defaultstate="collapsed">
void test_copy_function(duktape::engine& js)
{
  test_comment("test_copy_function");
  test_makefiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  // file copy: file only
  test_expect( js.eval<bool>("fs.copy(test_abspath('z'), test_abspath('CP'))") && exists(test_path("CP")) );
  test_expect( js.eval<bool>("fs.copy(test_relpath('z'),test_relpath('CP2'))") && exists(test_path("CP2")) );

  // file copy: not recursive
  test_expect_except( js.eval<bool>("fs.copy(test_abspath('a', test_abspath('DIR'))") && !exists(test_path("DIR")) );
  test_expect_except( js.eval<bool>("fs.copy(test_relpath('a'),test_relpath('DIR'))") && !exists(test_path("DIR")) );

  // file copy: copy into directory
  test_expect( js.eval<bool>("fs.copy(test_relpath('CP'),test_relpath('a'))") && exists(test_path("a/CP")) );

  // file copy: recursive copy
  test_expect_except( js.eval<bool>("fs.copy(test_relpath('a'),test_relpath('b'))") && !exists(test_path("a/b")) );
  test_expect( js.eval<bool>("fs.copy(test_relpath('a'),test_relpath('b/'),{recursive:true})") && exists(test_path("b/a")) );

  // file copy: recursive, explicit new target file name
  test_expect( js.eval<bool>("fs.copy(test_relpath('a'),test_relpath('b/a/d'),{recursive:true})") && exists(test_path("b/a/d")) );
  test_expect( js.eval<bool>("fs.copy(test_abspath('b'),test_abspath('a/'),'r')") && exists(test_path("a/b")) );

  // option masking
  test_expect( js.eval<bool>("fs.copy(test_relpath('z'),test_relpath('-K'))") && exists(test_path("-K")) );
}
// </editor-fold>

// <editor-fold desc="test_move_function" defaultstate="collapsed">
void test_move_function(duktape::engine& js)
{
  test_comment("test_move_function");
  test_makefiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.move(test_abspath('z'), test_abspath('ZZ')) // rename file") && exists(test_path("ZZ")) && !exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move(test_relpath('ZZ'), test_relpath('z')) // rename file") && !exists(test_path("ZZ")) && exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move(test_abspath('a/y'), test_abspath('b')) // move file to dir") && exists(test_path("b/y")) && !exists(test_path("a/y")) );
  test_expect( js.eval<bool>("fs.move(test_relpath('b/y'), test_relpath('.')) // move file to dir") && exists(test_path("y")) && !exists(test_path("b/y")) );
  test_expect( js.eval<bool>("fs.move(test_relpath('b/w with whitespace'), test_relpath('a'))") && exists(test_path("a/w with whitespace")) && !exists(test_path("b/w with whitespace"))  );
  test_expect( js.eval<bool>("fs.move(test_relpath('b'), test_relpath('a')) // move dir recursively to dir") && exists(test_path("a/b")) && !exists(test_path("b")) );
  test_expect( js.eval<bool>("fs.isfile(test_relpath('z')) ") && exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move()") );
  test_expect_except( js.eval<bool>("fs.move(undefined, undefined)") );
  test_expect_except( js.eval<bool>("fs.move(null, undefined)") );
  test_expect_except( js.eval<bool>("fs.move(test_relpath('z'))") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('', '')") );
  test_expect_except( js.eval<bool>("fs.move('', 'z')") );
  test_expect_except( js.eval<bool>("fs.move('z', '')") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('z', null)") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('*', 'a')") );
  test_expect( exists(test_path("a")) );
  test_expect_except( js.eval<bool>("fs.move(test_abspath('*'), 'a')") );
  test_expect_except( js.eval<bool>("fs.move('z', \"'z'\") // no shell quotes allowed, even if escaped") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('notexisting', 'a')") );
}
// </editor-fold>

// <editor-fold desc="test_remove_function" defaultstate="collapsed">
void test_remove_function(duktape::engine& js)
{
  test_comment("test_remove_function");
  test_makefiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect_except( js.eval<bool>("fs.remove()") );
  test_expect_except( js.eval<bool>("fs.remove('')") );
  test_expect_except( js.eval<bool>("fs.remove(undefined)") );
  test_expect_except( js.eval<bool>("fs.remove(null)") );
  test_expect( js.eval<bool>("fs.remove(test_abspath('z'))") && !exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.remove(test_abspath('z')) // remove not existing file is ok") );
  test_expect( js.eval<bool>("fs.remove(test_relpath('a/y'))") && !exists(test_path("a/y")) );
  test_expect_except( js.eval<bool>("fs.remove(test_relpath('a/y'))") );
  test_expect( js.eval<bool>("fs.remove(test_abspath('a'))") && !exists(test_path("a")) );
  test_expect_except( js.eval<bool>("fs.remove('a')") );
  test_expect_except( js.eval<bool>("fs.remove('b')") && exists(test_path("b")) );
  test_expect( js.eval<bool>("fs.remove(test_abspath('b'), '-r')") && !exists(test_path("b")) );
}
// </editor-fold>

// <editor-fold desc="test main" defaultstate="collapsed">
void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);
  duktape::mod::filesystem::generic::define_in<>(js);
  duktape::mod::filesystem::basic::define_in<>(js);
  duktape::mod::filesystem::extended::define_in<>(js);
  js.define("testdir", test_path());
  try {
    test_copy_function(js);
    test_move_function(js);
    test_remove_function(js);
    test_rmfiletree();
  } catch(...) {
    test_rmfiletree();
    throw;
  }
}
// </editor-fold>
