/**
 * Include, function definitions
 */
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>

using namespace std;

void test(duktape::engine& js)
{
  std::stringstream ssin, sout;
  duktape::mod::stdio::in_stream = &ssin;
  duktape::mod::stdio::err_stream = &sout;
  duktape::mod::stdio::out_stream = &sout;
  duktape::mod::stdio::log_stream = &sout;
  duktape::mod::stdio::define_in(js);      // Note: That also overrides alert and print.

  ssin.clear(); ssin.str("y\n");
  test_expect( js.eval<string>("confirm('Test confirm: ');") == "y");
  ssin.clear(); ssin.str("prompt-line\n");
  test_expect( js.eval<string>("prompt('Test prompt: ');") == "prompt-line");
  ssin.clear(); ssin.str("test line 1\ntest line 2\ntest line 3\n");
  test_expect( js.eval<string>("console.read();") == "test line 1\ntest line 2\ntest line 3\n");
  ssin.clear(); ssin.str("1\n\n2\n3\n");
  test_expect(js.eval<string>("console.read(function(a){return a!='';});") == "1\n2\n3\n");
}
