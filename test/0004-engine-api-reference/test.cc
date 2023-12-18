#include "../testenv.hh"
#include <duktape.hh>
#include <mod/mod.stdio.hh>
#include <mod/mod.stdlib.hh>

using namespace std;

int use_engine_stack(duktape::api& stack)
{
  duktape::engine& js = stack.parent_engine();
  test_expect(js.stack().ctx() == stack.ctx());
  js.stack().push(123456789);
  return 1;
}

void test(duktape::engine& js)
{
  duktape::mod::stdio::define_in(js);
  duktape::mod::stdlib::define_in(js);
  js.define("use_engine_stack", use_engine_stack, 1);
  test_expect(js.eval<int>("use_engine_stack()") == 123456789);

  // negative test:
  test_info("Removing engine pointer from stack context ...");
  js.stack().push_heap_stash();
  js.stack().del_prop_string(-1, "_engine_");
  try {
    js.eval<int>("use_engine_stack()");
    test_fail("Expected duktape::engine_error was not thrown.");
  } catch(const duktape::engine_error& e) {
    test_pass("Expected duktape::engine_error was thrown.");
  } catch(const std::exception& e) {
    test_fail(string("Expected duktape::engine_error, but std::exception was thrown:") + e.what());
  } catch(...) {
    test_fail("Expected duktape::engine_error, but another exception was thrown.");
  }
}
