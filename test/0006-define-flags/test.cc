#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>

using namespace std;

void test(duktape::engine& js)
{
  {
    // Defaults
    js.define("test.default_defined");
    js.define("test.default_defined.val1");
    js.define("test.default_defined.val2");

    // should be not configurable
    test_expect( js.eval<bool>("typeof(test.default_defined) == 'object'") );
    test_expect_noexcept( js.eval("delete test.default_defined;") ); // should be ignored
    test_expect( js.eval<bool>("typeof(test.default_defined) == 'object'") ); // should be still there

    // should be enumerable --> val1, val2 counted
    test_expect( js.eval<int>("n=0; for(var i in test.default_defined) n++; n;") == 2 );

    // should not be writable
    test_expect_noexcept( js.eval("test.default_defined = 10;") );
    test_expect( js.eval<bool>("typeof(test.default_defined) == 'object'") ); // should be still there
    test_expect( js.eval<bool>("typeof(test.default_defined.val1) == 'object'") ); // should be still there
  }

  // Not enumerable, not writable, not configurable
  {
    js.define_flags(duktape::engine::defflags::restricted);
    js.define("test.default_defined1");
    js.define("test.default_defined1.val1");
    js.define("test.default_defined1.val2");

    // should be not configurable
    test_expect( js.eval<bool>("typeof(test.default_defined1) == 'object'") );
    test_expect_noexcept( js.eval("delete test.default_defined1;") ); // should be ignored
    test_expect( js.eval<bool>("typeof(test.default_defined1) == 'object'") ); // should be still there

    // should be enumerable --> val1, val2 counted
    test_expect( js.eval<int>("n=0; for(var i in test.default_defined1) n++; n;") == 0 );

    // should not be writable
    test_expect_noexcept( js.eval("test.default_defined1 = 10;") );
    test_expect( js.eval<bool>("typeof(test.default_defined1) == 'object'") ); // should be still there
    test_expect( js.eval<bool>("typeof(test.default_defined1.val1) == 'object'") ); // should be still there
  }

  // Enumerable, writable, configurable
  {
    js.define_flags(
      duktape::engine::defflags::writable |
      duktape::engine::defflags::configurable |
      duktape::engine::defflags::configurable
    );
    js.define("test.default_defined2");
    js.define("test.default_defined2.val1");
    js.define("test.default_defined2.val2");

    // should be enumerable --> val1, val2 counted
    test_expect( js.eval<int>("n=0; for(var i in test.default_defined2) n++; n;") == 0 );

    // should be configurable
    test_expect( js.eval<bool>("typeof(test.default_defined2) == 'object'") );
    test_expect_noexcept( js.eval("delete test.default_defined2;") ); // should be ignored
    test_expect( js.eval<bool>("typeof(test.default_defined2) == 'undefined'") );

    js.define("test.default_defined2");
    js.define("test.default_defined2.val1");
    js.define("test.default_defined2.val2");

    // should not be writable
    test_expect_noexcept( js.eval("test.default_defined2 = 10;") );
    test_expect( js.eval<bool>("typeof(test.default_defined2) == 'number'") ); // should be 10
    test_expect( js.eval<bool>("typeof(test.default_defined2.val1) == 'undefined'") ); // should be vanished
  }
}
