/**
 * Test: alert() and print().
 */
#include "../testenv.hh"

using namespace std;

void test(duktape::engine& js)
{
  js.include("test.js");
  test_pass("No exception.");
}

