#include "../testenv.hh"
#include <duktape.hh>
#include <mod/mod.stdio.hh>
#include <mod/mod.stdlib.hh>
#include <sstream>

using namespace std;

duktape::engine* pjs = nullptr; // Used to tackle the possibility have a global js engine.

bool throwing_cc_function_called = false;

// In case someone uses js as global c++ object and invokes call
int wrapped_engine_call(duktape::api& stack)
{
  try {
    if(!pjs) {
      test_fail("Global test js object pointer not set.");
    } else {
      test_note("-> wrapped_engine_call()");
      pjs->call<void>(stack.to<string>(0));
      test_fail("wrapped_engine_call() did not catch an exception");
    }
    return 0;
  } catch(const duktape::script_error& e) {
    test_pass(string("wrapped_engine_call(): ") + e.what() );
    throw;
  }
}

// In case someone uses js as global c++ object and invokes eval
int wrapped_engine_eval(duktape::api& stack)
{
  try {
    if(!pjs) {
      test_fail("Global test js object pointer not set.");
    } else {
      test_note("-> wrapped_engine_eval()");
      pjs->eval<void>(stack.to<string>(0));
      test_fail("wrapped_engine_eval() did not catch an exception");
    }
    return 0;
  } catch(const duktape::script_error& e) {
    test_pass(string("wrapped_engine_eval(): ") + e.what() );
    throw;
  }
}

// Stack call should cause an exception in the js engine after
// the `stackcall` function returns.
int wrapped_apistack_call(duktape::api& stack)
{
  test_note("-> wrapped_apistack_call()");
  try {
    if(!pjs) {
      test_fail("Global test js object pointer not set.");
    } else {
      // This one should not throw a c++ duktape::script_error
      if(stack.top() < 1) throw duktape::script_error("No function defined to call");
      stack.get_global_string(stack.to<string>(0));
      stack.call(0); // error object will be on the stack on exception
      test_fail("wrapped_apistack_call() did not catch an exception");
    }
    return 0;
  } catch(const duktape::script_error& e) {
    test_pass(string("wrapped_engine_eval(): ") + e.what() );
    throw;
  }
}

void wrapped_native_function_throwing_script_error()
{
  test_note("-> wrapped_native_function_throwing_script_error()");
  throwing_cc_function_called = true;
  throw duktape::script_error("c++ duktape::script_error");
  test_fail("wrapped_native_function_throwing_script_error() did throw");
}

void test(duktape::engine& js)
{
  pjs = &js;
  duktape::mod::stdio::define_in(js);
  duktape::mod::stdlib::define_in(js);
  js.define("wrapped_engine_call", wrapped_engine_call, 1);
  js.define("wrapped_engine_eval", wrapped_engine_eval, 1);
  js.define("wrapped_apistack_call", wrapped_apistack_call, 1);
  js.define("wrapped_native_function_throwing_script_error", wrapped_native_function_throwing_script_error);

  // c++ script error handling
  try {
    js.include("test.js");
    test_fail("test() no exception was thrown");
  } catch(const duktape::script_error& e) {
    test_pass(string("test() catch: caught:") + e.what());
    test_note("test() catch: context:" << js.stack().dump_context());
    test_note("test() catch: callstack:" << endl << e.callstack());
  }

  if(!throwing_cc_function_called) {
    test_fail("Test did not throw from the expected location.");
  }

}
