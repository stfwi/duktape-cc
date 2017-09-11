/**
 * Test: alert() and print().
 */
#include "../testenv.hh"

using namespace std;

void test(duktape::engine& js)
{
  std::stringstream ssin, sout;
  test_include_script(js);
  test_pass("No exception.");
}
