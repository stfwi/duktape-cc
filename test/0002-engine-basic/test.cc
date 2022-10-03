/**
 * Include, function definitions
 */
#include "../testenv.hh"

using namespace std;

namespace {

  // Raw C duktape function
  // Note: Prefer the c++ function below because of std::exception catching
  //       if you are using c++ functions that may throw.
  //
  int prime_check(duk_context *ctx)
  {
    duktape::api duk(ctx);
    // long value of first argument (stack index 0)
    long val = duk.to<long>(0);

    // Value <= 0 --> not a prime number (0 also applies to conversion error)
    if(val <= 0) { duk_push_false(ctx); return 1; }

    // Iterate to max value to check, and check if it is dividable (simple check).
    long end = ((long)sqrt(val))+1;
    for (long i = 2; i <= end; ++i) {
      if(!(val % i)) {
        // Push return value, return 1 to indicate that there is a return value.
        duk.push_false();
        return 1;
      }
    }

    // Yes, it's prime, push and return.
    duk.push_true();
    return 1;
  }

  // Simple faculty calculation in a c++ native duktape function.
  int faculty(duktape::api stack)
  {
    long val = stack.to<long>(0);
    val = val >= 0 ? val : 0;
    double acc = val;
    while(--val) acc *= val;
    stack.push(acc);
    return 1;
  }

  // c++ template defined function
  double phytargoras(double a, double b)
  { return sqrt((a*a) + (b*b)); }

  string test_function()
  { return string("test_function() was called."); }

  template <typename T>
  bool test_define_readback(duktape::engine& js, const char* name, T val)
  { js.define(name, val); return js.eval<T>(name) == val; }

  template <>
  bool test_define_readback(duktape::engine& js, const char* name, const char* val)
  { js.define(name, val); return js.eval<string>(name) == val; }

  void test_define_and_call(duktape::engine& js)
  {
    js.define("prim", prime_check, 2);      // Define raw duktape function
    js.define("faculty", faculty);          // c++ raw/native duktape function
    js.define("phytargoras", phytargoras);  // c++ template defined function
    js.define("faculty", faculty);          // c++ raw/native duktape function
    js.define("test.obj1");                 // Define empty object obj1 in global object test.
    js.define("test.obj1.obj");             // Add empty sub object in test.obj1.
    js.define("test.obj2");                 // Add empty object obj2 in test.

    test_expect( test_define_readback(js, "int_ten", 10) );
    test_expect( test_define_readback(js, "str_one", "1") );
    test_expect( test_define_readback(js, "flt_two", 2.1) );

    test_expect( js.include<double>("test.js") == 5 ); // js last line is: phytargoras(3,4);
    test_expect( js.call<double>("faculty", 10) == 10.0*9*8*7*6*5*4*3*2*1 );
    test_expect( js.call<string>("sum", 10, 23) == "33"); // dev.js defined {return a+b;}
    test_expect( js.call<string>("sum", "10", 23) == "1023"); // dev.js defined -> string conversion
    test_expect( js.call<int>("twenty") == 20 ); // dev.js defined, {return 20;}.

    // Canonical names
    js.define("test.functions.test_function", test_function);
    test_expect( js.eval<string>("typeof(test.functions.test_function)")  == "function" );
    test_expect( js.eval<string>("test.functions.test_function()")  == test_function() );
    test_expect( js.eval<bool>("(typeof test == 'object')") );
    test_expect( js.eval<bool>("(typeof test.obj1 == 'object')") );
    test_expect( js.eval<bool>("(typeof test.obj1.obj == 'object')") );
    test_expect( js.eval<bool>("(typeof test.obj2 == 'object')") );
  }

}

namespace {

  void test_define_flags(duktape::engine& js)
  {
    // Defaults
    {
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

}

namespace {

  void test_select(duktape::engine& js)
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
}

namespace {

  void test_undef(duktape::engine& js)
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
}

void test(duktape::engine& js)
{
  test_define_and_call(js);
  test_define_flags(js);
  test_select(js);
  test_undef(js);
}
