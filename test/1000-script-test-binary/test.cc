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
  js.define("print", ecma_print); // may be overwritten by stdio
  js.define("alert", ecma_warn); // may be overwritten by stdio
  js.define("callstack", ecma_callstack);
  test_include_script(js);
}
