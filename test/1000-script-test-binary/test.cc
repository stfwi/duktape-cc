#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.stdlib.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.fs.ext.hh>
#include <mod/mod.fs.file.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.sys.exec.hh>
#include <mod/mod.sys.hash.hh>
#include <mod/mod.xlang.hh>
#include <mod/ext/mod.conv.hh>
#include <mod/ext/mod.ext.mmap.hh>
#include <mod/ext/mod.ext.serial_port.hh>
#include <mod/ext/mod.ext.resource_blob.hh>
#include <mod/mod.sys.os.hh>
#ifdef WITH_EXTERNAL_DEPENDENCIES
  #include <mod/ext/mod.ext.srecord.hh>
#endif
#ifdef WITH_EXPERIMENTAL
  #include <mod/exp/mod.experimental.hh>
#endif
#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>

using namespace std;


int ecma_eval_sandboxed(duktape::api& stack)
{
  const auto code_sequence   = stack.to<vector<string>>(0);
  const auto options         = (stack.top()<2) ? (string()) : (stack.to<string>(1));
  const auto without_eval    = options.find("no-eval")!=options.npos;
  const auto without_builtin = options.find("no-builtin")!=options.npos;
  const auto compile_strict  = options.find("strict")!=options.npos;
  stack.clear();
  stack.push_object();

  duktape::engine js_box;
  size_t code_index = 0;

  if(!without_builtin) {
    js_box.define("print", ecma_print);
    js_box.define("alert", ecma_warn);
    js_box.define("callstack", ecma_callstack);
    js_box.define("sandboxed", true);
  }
  try {
    if(code_sequence.empty()) {
      throw runtime_error("TEST BUG: No code to evaluate or compile.");
    }
    // Dependencies:
    for(code_index=0; code_index<code_sequence.size()-1; ++code_index) {
      js_box.eval<void>(code_sequence[code_index]);
    }
    // Test:
    js_box.stack().clear();
    if(js_box.stack().pcompile_string(compile_strict ? duktape::api::compile_strict : duktape::api::compile_default, code_sequence.back()) != 0) {
      stack.property("error", js_box.stack().to<string>(-1));
      stack.property("type", "script_error");
      stack.property("compile_error", true);
    }
    if(!without_eval) {
      if(js_box.stack().pcall(0) == 0) {
        stack.property("data", js_box.stack().to<string>(-1));
      } else {
        stack.property("error", js_box.stack().to<string>(-1));
        stack.property("type", "script_error");
      }
    }
  } catch(const duktape::exit_exception&) {
    stack.property("exit", true);
  } catch(const duktape::script_error& e) {
    stack.property("error", e.what());
    stack.property("type", "script_error");
    if(code_index < code_sequence.size()-1) {
      stack.property("dependency_index", code_index);
    }
  } catch(const duktape::engine_error& e) {
    stack.property("error", e.what());
    stack.property("type", "engine_error");
    if(code_index < code_sequence.size()-1) {
      stack.property("dependency_index", code_index);
    }
  } catch(const std::exception& e) {
    stack.property("error", e.what());
    stack.property("type", "exception");
  } catch(...) {
    stack.property("error", "!fatal");
    stack.property("type", "uncaught_exception");
  }
  return 1;
}


void test(duktape::engine& js)
{
  duktape::mod::stdlib::define_in(js);
  duktape::mod::stdio::define_in(js);
  duktape::mod::filesystem::generic::define_in(js);
  duktape::mod::filesystem::basic::define_in(js);
  duktape::mod::filesystem::extended::define_in(js);
  duktape::mod::filesystem::fileobject::define_in(js);
  duktape::mod::system::define_in(js);
  duktape::mod::system::exec::define_in(js);
  duktape::mod::system::hash::define_in(js);
  duktape::mod::xlang::define_in(js);
  duktape::mod::ext::conv::define_in(js);
  duktape::mod::ext::mmap::define_in(js);
  duktape::mod::ext::serial_port::define_in(js);
  #ifdef WITH_EXTERNAL_DEPENDENCIES
    duktape::mod::ext::srecord::define_in(js);
  #endif
  #ifdef WITH_EXPERIMENTAL
    duktape::mod::experimental::define_in(js);
  #endif
  // Re-route basic stdio to testenv
  js.define("print", ecma_print);
  js.define("alert", ecma_warn);
  js.define("callstack", ecma_callstack);
  js.define("test_sandboxed", ecma_eval_sandboxed);
  test_include_script(js);
}
