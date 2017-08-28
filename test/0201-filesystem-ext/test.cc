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
  test_expect( js.eval<bool>("fs.copy(testabspath('z'), testabspath('CP'))") && exists(test_path("CP")) );
  test_expect( js.eval<bool>("fs.copy(testrelpath('z'),testrelpath('CP2'))") && exists(test_path("CP2")) );

  // file copy: not recursive
  test_expect_except( js.eval<bool>("fs.copy(testabspath('a', testabspath('DIR'))") && !exists(test_path("DIR")) );
  test_expect_except( js.eval<bool>("fs.copy(testrelpath('a'),testrelpath('DIR'))") && !exists(test_path("DIR")) );

  // file copy: copy into directory
  test_expect( js.eval<bool>("fs.copy(testrelpath('CP'),testrelpath('a'))") && exists(test_path("a/CP")) );

  // file copy: recursive copy
  test_expect_except( js.eval<bool>("fs.copy(testrelpath('a'),testrelpath('b'))") && !exists(test_path("a/b")) );
  test_expect( js.eval<bool>("fs.copy(testrelpath('a'),testrelpath('b/'),{recursive:true})") && exists(test_path("b/a")) );

  // file copy: recursive, explicit new target file name
  test_expect( js.eval<bool>("fs.copy(testrelpath('a'),testrelpath('b/a/d'),{recursive:true})") && exists(test_path("b/a/d")) );
  test_expect( js.eval<bool>("fs.copy(testabspath('b'),testabspath('a/'),'r')") && exists(test_path("a/b")) );

  // option masking
  test_expect( js.eval<bool>("fs.copy(testrelpath('z'),testrelpath('-K'))") && exists(test_path("-K")) );
}
// </editor-fold>

// <editor-fold desc="test_move_function" defaultstate="collapsed">
void test_move_function(duktape::engine& js)
{
  test_comment("test_move_function");
  test_makefiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.move(testabspath('z'), testabspath('ZZ')) // rename file") && exists(test_path("ZZ")) && !exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move(testrelpath('ZZ'), testrelpath('z')) // rename file") && !exists(test_path("ZZ")) && exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move(testabspath('a/y'), testabspath('b')) // move file to dir") && exists(test_path("b/y")) && !exists(test_path("a/y")) );
  test_expect( js.eval<bool>("fs.move(testrelpath('b/y'), testrelpath('.')) // move file to dir") && exists(test_path("y")) && !exists(test_path("b/y")) );
  test_expect( js.eval<bool>("fs.move(testrelpath('b/w with whitespace'), testrelpath('a'))") && exists(test_path("a/w with whitespace")) && !exists(test_path("b/w with whitespace"))  );
  test_expect( js.eval<bool>("fs.move(testrelpath('b'), testrelpath('a')) // move dir recursively to dir") && exists(test_path("a/b")) && !exists(test_path("b")) );
  test_expect( js.eval<bool>("fs.isfile(testrelpath('z')) ") && exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move()") );
  test_expect_except( js.eval<bool>("fs.move(undefined, undefined)") );
  test_expect_except( js.eval<bool>("fs.move(null, undefined)") );
  test_expect_except( js.eval<bool>("fs.move(testrelpath('z'))") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('', '')") );
  test_expect_except( js.eval<bool>("fs.move('', 'z')") );
  test_expect_except( js.eval<bool>("fs.move('z', '')") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('z', null)") );
  test_expect( exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.move('*', 'a')") );
  test_expect( exists(test_path("a")) );
  test_expect_except( js.eval<bool>("fs.move(testabspath('*'), 'a')") );
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
  test_expect( js.eval<bool>("fs.remove(testabspath('z'))") && !exists(test_path("z")) );
  test_expect_except( js.eval<bool>("fs.remove(testabspath('z')) // remove not existing file is ok") );
  test_expect( js.eval<bool>("fs.remove(testrelpath('a/y'))") && !exists(test_path("a/y")) );
  test_expect_except( js.eval<bool>("fs.remove(testrelpath('a/y'))") );
  test_expect( js.eval<bool>("fs.remove(testabspath('a'))") && !exists(test_path("a")) );
  test_expect_except( js.eval<bool>("fs.remove('a')") );
  test_expect_except( js.eval<bool>("fs.remove('b')") && exists(test_path("b")) );
  test_expect( js.eval<bool>("fs.remove(testabspath('b'), '-r')") && !exists(test_path("b")) );
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
    #if !defined(WINDOWS) || defined(WITH_EXPERIMENTAL)
    test_copy_function(js);
    test_move_function(js);
    test_remove_function(js);
    #endif
    test_rmfiletree();
  } catch(...) {
    test_rmfiletree();
    throw;
  }
}
// </editor-fold>
