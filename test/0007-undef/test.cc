#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>

using namespace std;

void test(duktape::engine& js)
{
  js.define_flags(duktape::engine::defflags::restricted);
  js.define("test_object");
  test_expect( js.eval<bool>("typeof(test_object) == 'object'") );
  test_expect_noexcept( js.eval("delete test_object") ); // should be ignored
  test_expect( js.eval<bool>("typeof(test_object) == 'object'") );
  test_expect_noexcept( js.undef("test_object") ); // should NOT be ignored
  test_expect( js.eval<bool>("typeof(test_object) == 'undefined'") );

  // again, this time set as JS plain object.
  test_expect_noexcept( js.eval("test_object={}") );
  test_expect( js.eval<bool>("typeof(test_object) == 'object'") );
  test_expect_noexcept( js.undef("test_object") ); // should NOT be ignored
  test_expect( js.eval<bool>("typeof(test_object) == 'undefined'") );

  // again, this time a constant
  js.define_flags(duktape::engine::defflags::restricted);
  js.define("test.int_value", 101);
  test_expect( js.eval<int>("test.int_value") == 101 );
  test_expect_noexcept( js.undef("test.int_value") );
  test_expect( js.eval<bool>("typeof(test.int_value) == 'undefined'") );
}
