#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.stdlib.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.fs.ext.hh>
#include <mod/mod.fs.file.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.sys.exec.hh>
#include <mod/mod.sys.hash.hh>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>

using namespace std;



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

  // reset some stdio to to testenv
  js.define("print", ecma_print); // may be overwritten by stdio
  js.define("alert", ecma_warn); // may be overwritten by stdio
  js.define("callstack", ecma_callstack);
  test_include_script(js);
}
