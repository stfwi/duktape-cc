/**
 * Test: alert() and print().
 */
#include "../testenv.hh"

using namespace std;

void test(duktape::engine& js)
{
  test_include_script(js);
  test_expect( js.eval<string>("typeof(js_declared_function)") == "function" );
  test_pass("No exception.");
}
