/**
 * @test stdlib
 *
 * Script include and program exit checks.
 */
#include "../testenv.hh"
#include <mod/mod.stdlib.hh>

using namespace std;



void test(duktape::engine& js)
{
  duktape::mod::stdlib::define_in(js);

  // Standard includes return data of JS and JSON files.
  {
    test_expect_except( js.eval<void>("include()") ); // missing arg
    test_expect_except( js.eval<void>("include('')") ); // empty path
    test_expect_except( js.eval<void>("include(undefined)") ); // missing arg
    test_expect( js.eval<double>("include('test-stdlib-include1.js')") == 4.2 ); // file function return capture in include.
    test_expect( js.eval<string>("JSON.stringify(include('test-stdlib-include2.js'))") == "[1,2,3]" ); // file last statement as return value
    test_expect( js.eval<int>("global.intval1") == 1 ); // Global variable set with `global.`
    test_expect( js.eval<int>("intval1") == 1 ); // Global variable set `global.`
    test_expect( js.eval<string>("test_stdlib_include2") == "test-stdlib-include2" ); // Global variable set with `var`, nonstrict include
    test_expect( js.eval<string>("JSON.stringify(TEST_STDLIB_INCLUDE2_NOVAR)") == "{\"value\":\"test\"}" ); // Global variable set without `var`, nonstrict include
    test_expect( js.eval<string>("JSON.stringify(include('test_stdlib-include3.json'))") == "{\"key\":\"value\"}" ); // JSON file include
    test_expect( js.eval<bool>("include('test_stdlib-include4.json') === undefined") ); // Empty JSON file include
    test_expect_except( js.eval<void>("include('test_stdlib-include5.json') ") ); // File with invalid JSON
  }
  // Exception handling
  {
    try {
      js.eval<void>("include('test-stdlib-include6.js')");
      test_fail("No script error 'Test error' thrown");
    } catch(const duktape::script_error& e) {
      test_note(e.what());
      test_expect(string(e.what()) == "Error: Test error");
    } catch(...) {
      test_fail("Unexpected exception other than script error thrown.");
    }
    try {
      js.eval<void>("include('test-stdlib-include7.js')");
      test_fail("No exit exception thrown");
    } catch(const duktape::exit_exception& e) {
      test_note("message: '" << e.what() << "', exit code:" << e.exit_code());
      test_expect(string(e.what()) == "exit");
      test_expect(e.exit_code() == 6);
    } catch(...) {
      test_fail("Unexpected exception other than duktape::exit_exception thrown.");
    }
  }
}
