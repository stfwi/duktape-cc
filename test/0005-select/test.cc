#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>

using namespace std;

void test(duktape::engine& js)
{
  js.define("test.test1.test2");
  duktape::api stack(js);
  {
    duktape::stack_guard sg(stack);
    stack.select("test.test1.test2");
    test_expect(stack.is_object(-1));
  }
  {
    duktape::stack_guard sg(stack);
    stack.select("test.test1");
    test_expect(stack.has_prop_string(-1, "test2"));
    test_expect(!stack.has_prop_string(-1, "test1"));
  }
  {
    duktape::stack_guard sg(stack);
    stack.select("test");
    test_expect(stack.has_prop_string(-1, "test1"));
    test_expect(!stack.has_prop_string(-1, "test2"));
  }
}
