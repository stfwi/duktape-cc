/**
 * Test: alert() and print().
 */
#include "../testenv.hh"

using namespace std;

void test(duktape::engine& js)
{
  std::stringstream ssin, sout;
  js.include("test.js");
  test_pass("No exception.");
}

