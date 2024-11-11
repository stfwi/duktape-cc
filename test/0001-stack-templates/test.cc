/**
 * Stack tests
 */
#include "../testenv.hh"
#include <limits>
#include <chrono>

using namespace std;

/**
 * c++ native aux functions used in the tests.
 */
namespace {

  /**
   * From https://duktape.org/api.html#duk_safe_call
   * Used to test `duktape::api::safe_call()`.
   */
  typedef struct { int floor; } my_safe_args;
  ::duk_ret_t duktape_safe_call_compliant_function(::duk_context* ctx, void* data)
  {
    my_safe_args *args = (my_safe_args*)data;
    double a, b, c, t;
    a = ::duk_get_number(ctx, -3);
    b = ::duk_get_number(ctx, -2);
    c = ::duk_get_number(ctx, -1); (void)c; // ignored on purpose
    t = a + b;
    if(args->floor) { t = floor(t); }
    ::duk_push_number(ctx, t);
    return 1;
  }

  ::duk_ret_t duktape_c_function_argument_count(::duk_context *ctx)
  { ::duk_push_int(ctx, ::duk_get_top(ctx)); return 1; }

  unsigned duktape_c_array_to_test_data_pointers[8] = {0,1,2,3,4,5,6,7};

  int ctor_function(duktape::api& stack)
  {
    test_expect( stack.is_constructor_call() );
    test_expect( stack.is_strict_call() );
    stack.push_this();
    return 1;
  }

  duk_codepoint_t map_char_function_all_to_a(void *udata, duk_codepoint_t codepoint)
  {
    (void)udata;
    (void)codepoint;
    return duk_codepoint_t('A');
  }

  void decode_char_function_all_to_a(void *udata, duk_codepoint_t codepoint)
  { (void)udata; (void)codepoint; }

}

/**
 * duktape::engine::api_type c'tors checkup.
 */
namespace {

  void test_api_construction(duktape::engine& js)
  {
    using api_type = duktape::engine::api_type;
    auto stack_copy = api_type(js.stack());
    test_expect( stack_copy.ctx() == js.stack().ctx() );
    test_expect( &stack_copy != &(js.stack()) );
    auto stack_moved = api_type(std::move(stack_copy));
    test_expect( stack_moved.ctx() == js.stack().ctx() );
    test_expect( &stack_moved != &(js.stack()) );
    auto stack_move_assigned = api_type();
    stack_move_assigned = std::move(stack_moved);
    test_expect( stack_move_assigned.ctx() == js.stack().ctx() );
    test_expect( &stack_move_assigned != &(js.stack()) );
    const auto move_constructed = api_type(std::move(stack_move_assigned));
    test_expect( move_constructed.ctx() == js.stack().ctx() );
    test_expect( &move_constructed != &(js.stack()) );
  }
}

/**
 * duktape::api type conversions and typed value handling checks.
 */
namespace {

  constexpr unsigned num_iterations = 100;
  constexpr duk_double_t duk_number_max_excact_value = duk_double_t(numeric_limits<int32_t>::max());
  constexpr duk_double_t duk_number_min_excact_value = duk_double_t(numeric_limits<int32_t>::min());

  // log readability, non-integers passed through. @todo: Note to self: Update microtest library - test_pass(), test_fail() etc should do that already.
  template <typename T> typename std::enable_if< std::is_integral<T>::value && sizeof(T)>=2, string>::type itos(const T& v) { return std::to_string(v); }
  template <typename T> typename std::enable_if< std::is_integral<T>::value && sizeof(T)==1, string>::type itos(const T& v) { return std::to_string(int(v)); }
  template <typename T> typename std::enable_if<!std::is_integral<T>::value, T>::type itos(const T& v) { return v; }

  template <typename T>
  inline void print_type_info()
  {
    test_note("Type: c++:" << duktape::detail::conv<T>::cc_name() << " / ecma:" << duktape::detail::conv<T>::ecma_name());
    test_expect( duktape::detail::conv<T>::nret() == 1 );
  }

  template <>
  inline void print_type_info<void>()
  {
    test_note("Type: c++:" << duktape::detail::conv<void>::cc_name() << " / ecma:" << duktape::detail::conv<void>::ecma_name());
    test_expect( duktape::detail::conv<void>::nret() == 0 );
  }

  template <typename T>
  inline void check_typed_value(duktape::api& stack, const T& val)
  {
    duktape::stack_guard sg(stack);
    stack.push(val);
    if(stack.top() != 1) test_fail("stack.top() == 1");
    if(!stack.is<T>(-1)) test_fail("stack.is<T>(-1)");
    T ret = stack.get<T>(-1);
    if(ret != val) {
      test_fail(itos(ret), " != ", itos(val));
    } else {
      test_pass(itos(ret), " == ", itos(val));
    }
  }

  template <typename T>
  void check_type(duktape::api& stack)
  {
    print_type_info<T>();
    T min = numeric_limits<T>::min();
    T max = numeric_limits<T>::max();

    if(numeric_limits<T>::is_iec559) {
      if(sizeof(T) > sizeof(duk_double_t)) {
        if(max > T(numeric_limits<duk_double_t>::max())) max = T(numeric_limits<duk_double_t>::max());
        if(min < T(numeric_limits<duk_double_t>::min())) min = T(numeric_limits<duk_double_t>::min());
      }
    } else if(numeric_limits<T>::is_integer) {
      max = std::min(max, T(duk_number_max_excact_value));
      min = std::max(min, T(duk_number_min_excact_value));
    } else {
      test_fail("not covered:", duktape::detail::conv<T>::cc_name());
    }

    if(min != T(0)) check_typed_value(stack, T(0));
    check_typed_value(stack, T(1));
    check_typed_value(stack, min);
    check_typed_value(stack, max);
    if(std::numeric_limits<T>::is_signed) check_typed_value(stack, T(-1));
    if(numeric_limits<T>::is_iec559) check_typed_value(stack, numeric_limits<T>::infinity());
    if(numeric_limits<T>::is_iec559) check_typed_value(stack, -numeric_limits<T>::infinity());
    test_note("min:" << min << ", max:" << max);
    for(unsigned i=0; i<num_iterations; ++i) check_typed_value(stack, sw::utest::random<T>(min, max));
  }

  template <>
  void check_type<bool>(duktape::api& stack)
  {
    print_type_info<bool>();
    check_typed_value(stack, true);
    check_typed_value(stack, false);
  }

  template <>
  void check_type<long double>(duktape::api& stack)
  {
    print_type_info<long double>();
    check_typed_value(stack, numeric_limits<double>::min());
    check_typed_value(stack, numeric_limits<double>::max());
    check_typed_value(stack, sw::utest::random<double>());
  }

  void test_typed_stack_getters_and_setters(duktape::engine& js)
  {
    test_note("duk_number_min_excact_value=" << duk_number_min_excact_value << ", duk_number_max_excact_value=" << duk_number_max_excact_value);

    duktape::api stack(js.stack());
    check_type<bool>(stack);
    check_type<char>(stack);
    check_type<short>(stack);
    check_type<int>(stack);
    check_type<long>(stack);
    check_type<long long>(stack);

    check_type<signed char>(stack);
    check_type<signed short>(stack);
    check_type<signed int>(stack);
    check_type<signed long>(stack);
    check_type<signed long long>(stack);

    check_type<unsigned char>(stack);
    check_type<unsigned short>(stack);
    check_type<unsigned int>(stack);
    check_type<unsigned long>(stack);
    check_type<unsigned long long>(stack);

    check_type<float>(stack);
    check_type<double>(stack);
    check_type<long double>(stack);

    check_typed_value(stack, string("TEST"));

    print_type_info<void>();

    {
      const char* val = "TEST";
      string cmp(val);
      test_info("type: 'const char*'");
      test_info("value: ", val);
      duktape::stack_guard sg(stack);
      stack.push(val);
      test_expect(stack.top() == 1);
      test_expect(stack.is<const char*>(-1));
      test_expect(stack.is<string>(-1));
      test_expect(stack.get<string>(-1) == cmp);
    }
    {
      stack.clear();
      stack.push(1);
      test_expect( stack.get_type(-1) == (DUK_TYPE_NUMBER) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_NUMBER) );
      test_expect( string("Number") == stack.get_typename(-1) );
      test_expect(!stack.is_nan(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push(std::numeric_limits<double>::quiet_NaN());
      test_expect( stack.is_nan(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push_nan();
      test_expect( stack.is_nan(-1) );
      stack.push("1");
      test_expect( stack.get_type(-1) == (DUK_TYPE_STRING) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_STRING) );
      test_expect( string("String") == stack.get_typename(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push_string(string("1"));
      test_expect( stack.get_type(-1) == (DUK_TYPE_STRING) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_STRING) );
      test_expect( string("String") == stack.get_typename(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.clear();
      stack.push_object();
      test_expect( stack.get_type(-1) == (DUK_TYPE_OBJECT) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_OBJECT) );
      test_expect( string("Object") == stack.get_typename(-1) );
      test_expect(!stack.is_primitive(-1) );
      stack.push_array();
      test_expect( stack.get_type(-1) == (DUK_TYPE_OBJECT) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_OBJECT) );
      test_expect( string("Object") == stack.get_typename(-1) );
      test_expect(!stack.is_primitive(-1) );
      stack.push(true);
      test_expect( stack.get_type(-1) == (DUK_TYPE_BOOLEAN) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_BOOLEAN) );
      test_expect( string("Boolean") == stack.get_typename(-1) );
      stack.push_buffer(1, false);
      test_expect( stack.get_type(-1) == (DUK_TYPE_BUFFER) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_BUFFER) );
      test_expect(!stack.is_dynamic_buffer(-1) );
      test_expect( stack.is_fixed_buffer(-1) );
      test_expect( string("Buffer") == stack.get_typename(-1) );
      test_expect(!stack.is_primitive(-1) );
      stack.push_buffer_object(-1, 1, 0, DUK_BUFOBJ_UINT8ARRAY);
      test_expect( stack.get_type(-1) == (DUK_TYPE_OBJECT) );
      stack.clear();
      stack.push_fixed_buffer(1);
      test_expect( stack.get_type(-1) == (DUK_TYPE_BUFFER) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_BUFFER) );
      test_expect(!stack.is_dynamic_buffer(-1) );
      test_expect( stack.is_fixed_buffer(-1) );
      stack.push_dynamic_buffer(1);
      test_expect( stack.get_type(-1) == (DUK_TYPE_BUFFER) );
      test_expect( stack.get_type_mask(-1) == (DUK_TYPE_MASK_BUFFER) );
      test_expect( stack.is_dynamic_buffer(-1) );
      test_expect(!stack.is_fixed_buffer(-1) );
      test_expect( stack.resize_buffer(-1, 2) != nullptr );
      test_expect( stack.is_dynamic_buffer(-1) );
      stack.push_undefined();
      test_expect( stack.get_type(-1) == (DUK_TYPE_UNDEFINED) );
      test_expect( string("undefined") == stack.get_typename(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push_null();
      test_expect( stack.get_type(-1) == (DUK_TYPE_NULL) );
      test_expect( string("null") == stack.get_typename(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push_pointer(duktape_c_array_to_test_data_pointers);
      test_expect( stack.get_type(-1) == (DUK_TYPE_POINTER) );
      test_expect( string("Pointer") == stack.get_typename(-1) );
      test_expect( stack.is_primitive(-1) );
      stack.push_c_function(duktape_c_function_argument_count); // we don't use LIGHTFUNC, a normal function should not be seen as LIGHTFUNC.
      test_expect( stack.get_type(-1) != (DUK_TYPE_LIGHTFUNC) );
      test_expect( string("Function pointer") != stack.get_typename(-1) );
      test_expect(!stack.is_lightfunc(-1) );
      test_expect(!stack.is_primitive(-1) );
    }
    js.stack().clear();
  }

}

/**
 * duktape::stack_guard checks.
 */
namespace {

  void test_stack_guarded_api_fn(duktape::api& stack)
  {
    duktape::stack_guard sg(stack);
    stack.push("data5");
    test_expect(stack.top()==5);
    stack.push("data6");
    test_expect(stack.top()==6);
    test_expect( sg.ctx() == stack.ctx() );
    // -> stack_guard destructor must pop these.
  }

  void test_stack_guard_native_guarded_fn(int arg0, int arg1, int arg2)
  {
    (void)arg0; (void)arg1; (void)arg2;
    test_pass("Native void stack guarded function invoked.");
  }

  void test_stack_guard(duktape::engine& js)
  {
    auto& stack = js.stack();
    js.stack().top(0);
    js.stack().push("data1");
    test_expect(stack.top()==1);
    js.stack().push("data2");
    test_expect(stack.top()==2);
    js.stack().push("data3");
    test_expect(stack.top()==3);
    js.stack().push("data4");
    test_expect(stack.top()==4);
    test_stack_guarded_api_fn(stack);
    test_expect(stack.top()==4);
    js.define("native_sg_function", test_stack_guard_native_guarded_fn);
    js.eval("native_sg_function(1,2,3)"); // no return, no stack push, must be still 4
    test_expect(stack.top()==4);
    js.undef("native_sg_function");
    stack.clear();
  }

}

/**
 * Basic native class wrapper related API functions.
 */
namespace {

  // Check stats for the native class.
  static volatile unsigned vec2_ctor_calls = 0;
  static volatile unsigned vec2_dtor_calls = 0;
  static volatile unsigned vec2_set_calls = 0;
  static volatile unsigned vec2_abs_calls = 0;

  // Testing aggregate to use.
  template<typename T>
  struct vect2d
  {
    static_assert(std::is_arithmetic<T>::value, "Rectangle value types are numbers.");
    T x, y;
    explicit vect2d() noexcept : x(0), y(0) { ++vec2_ctor_calls; }
    ~vect2d() noexcept { ++vec2_dtor_calls; }
    vect2d& set(T newx, T newy) noexcept { x=newx; y=newy; ++vec2_set_calls; return *this; }
    T abs() noexcept { ++vec2_abs_calls; return std::sqrt(x*x + y*y); }
  };

  void test_native_class_wrapping_related(duktape::engine& normal_js)
  {
    (void)normal_js;
    auto js = duktape::engine();
    testenv_init(js);
    using vec2 = vect2d<double>;
    // Add native class to engine.
    {
      duktape::native_object<vec2>("Vec2")
        .getter("x", [](duktape::api& stack, vec2& instance) {stack.push(instance.x); })
        .setter("x", [](duktape::api& stack, vec2& instance){ instance.x = stack.get<double>(-1); })
        .getter("y", [](duktape::api& stack, vec2& instance){ stack.push(instance.y); })
        .setter("y", [](duktape::api& stack, vec2& instance){ instance.y = stack.get<double>(-1); })
        .getter("length", [](duktape::api& stack, vec2& instance) { stack.push(instance.abs()); return 1; })
        .method("abs", [](duktape::api& stack, vec2& instance) { stack.push(instance.abs()); return 1; })
        .method("toString", [](duktape::api& stack, vec2& instance) { stack.push(std::string("Vec2{x:") + std::to_string(instance.x) + std::string(",y:") + std::to_string(instance.y) + std::string("}")); return 1; })
        .method("set", [](duktape::api& stack, vec2& instance) {
          using namespace duktape;
          if(stack.top()!=2) throw script_error("Vec2.set(x,y) -> need x and y.");
          instance.set(stack.get<double>(0),stack.get<double>(1));
          stack.push_this();
          return true;
        })
        .getter("throwing_rte", [](duktape::api& stack, vec2& instance) { (void)stack; (void)instance; throw std::runtime_error("test-exception"); })
        .setter("throwing_rte", [](duktape::api& stack, vec2& instance){ (void)stack; (void)instance; throw std::runtime_error("test-exception"); })
        .getter("throwing_ee", [](duktape::api& stack, vec2& instance) { (void)stack; (void)instance; throw duktape::engine_error("test-exception"); })
        .setter("throwing_ee", [](duktape::api& stack, vec2& instance){ (void)stack; (void)instance; throw duktape::engine_error("test-exception"); })
        .define_in(js, true/*sealed*/);
    }
    // Instantiate to trigger proxies and finalizer.
    {
      test_note( js.eval<string>("global.v=new Vec2(); v.toString()") );
      test_expect( vec2_ctor_calls == 1 );
      test_expect( vec2_dtor_calls == 0 );
      test_expect( js.eval<double>("v.x=3; v.x") == 3.0 );
      test_expect( js.eval<double>("v.y=4; v.y") == 4.0 );
      test_expect( js.eval<double>("v.length") == 5.0 );
      test_expect( js.eval<double>("v.abs()") == 5.0 );
      test_expect( js.eval<double>("v.set(0,0); v.abs()") == 0.0 );
      test_expect( vec2_set_calls == 1 );
      test_expect( vec2_abs_calls == 3 );
      test_note( js.eval<string>("v=undefined") );
      test_expect_noexcept( js.stack().gc() );
      test_expect( vec2_ctor_calls == 1 );
      test_expect( vec2_dtor_calls == 1 );
      // native exception catching
      test_note( js.eval<string>("global.v=new Vec2(); v.toString()") );
      test_expect_except( js.eval<void>("v.area") );
      test_expect_except( js.eval<void>("v.throwing_rte") );
      test_expect_except( js.eval<void>("v.throwing_rte=1") );
      test_expect_except( js.eval<void>("v.throwing_ee") );
      test_expect_except( js.eval<void>("v.throwing_ee=1") );
      test_expect_except( js.eval<void>("delete v.throwing_ee") );
      test_expect_except( js.eval<void>("v.hasOwnProperty('throwing_ee'") );
      test_expect_except( js.eval<void>("v.hasOwnProperty('notthere'") );
      test_expect_noexcept( js.eval<void>("Object.keys(v)") );
      test_note( js.eval<string>("v.toString()") );
      test_note( js.eval<string>("JSON.stringify(Object.keys(v))") );
      test_note( js.eval<string>("v.toString()") );
    }
  }

}

/**
 * duktape::api method wrapping C API compliancy checks.
 */
namespace {

  // Checks to verify that the c++ wrapper methods invoke the
  // duktape API according to the examples and tests specified
  // on https://duktape.org/api.html. These are no tests for
  // the C API functions.
  void test_direct_stack_method_use(duktape::engine& js)
  {
    auto& stack = js.stack();
    // compile()
    {
      stack.clear();
      stack.push("function a_fn(){return 'function a_fn()'}");
      stack.push("filename");
      stack.compile(duktape::api::compile_default);
      test_note("stack.compile(duktape::api::compile_default) ...");
      test_expect( stack.is_function(-1) );
    }
    // compile_string()
    {
      stack.clear();
      stack.compile_string(duktape::api::compile_default, "print('program code');");
      test_note("stack.compile_string(duktape::api::compile_default, \"print('program code');\"");
      test_expect( stack.is_function(-1) );
    }
    // eval()
    {
      stack.clear();
      stack.push("123;");
      stack.eval();
      test_note("stack.eval(), code: 123;" );
      test_expect( stack.get<int>(-1) == 123);
    }
    // eval_string()
    {
      stack.clear();
      stack.eval_string("'testString'.toUpperCase()");
      test_note("stack.eval_string(\"'testString'.toUpperCase()\");");
      test_expect( stack.get<string>(-1) == "TESTSTRING");
    }
    // eval_string_noresult()
    {
      stack.clear();
      stack.eval_string_noresult("'testString'.toUpperCase()");
      test_note("stack.eval_string_noresult(\"'testString'.toUpperCase()\");");
      test_expect( stack.top() == 0 );
    }
    // duk_call()
    {
      stack.clear();
      stack.push("2+3");
      stack.push("eval");
      stack.compile(duktape::api::compile_eval);
      stack.call(0);
      test_note("stack.call(0); <-- after push('eval 2+3'); and compile().");
      test_expect( stack.get<int>(0) == 2+3 );
    }
    // call_method()
    {
      stack.clear();
      stack.push("(function(x,y) { print('call_method() this:', this, 'x=', x, 'y=', y); return this+x+y; })");
      stack.eval();
      stack.push(1230);
      stack.push(2);
      stack.push(3);
      stack.call_method(2);
      test_expect( stack.get<int>(-1) == (1230+2+3));
    }
    // call_prop()
    {
      stack.clear();
      stack.eval_string("global.a_obj={ plus: function(a,b){return a+b} }");
      test_expect( stack.is_object(0) );
      test_expect( stack.has_prop_string(0, "plus") );
      stack.push("plus");
      stack.push(2);
      stack.push(3);
      stack.call_prop(0, 2);
      test_expect( stack.get<int>(-1) == (2+3));
    }
    // pcompile()
    {
      stack.clear();
      stack.push("print('program'); syntax error here=");
      stack.push("hello-with-syntax-error");
      test_expect(stack.pcompile(duktape::api::compile_default) != 0);
      stack.clear();
      stack.push("print('program without syntax error');");
      stack.push("hello-without-syntax-error");
      test_expect(stack.pcompile(duktape::api::compile_default) == 0);
    }
    // pcompile_string() / is_error()
    {
      stack.clear();
      test_expect((stack.pcompile_string(duktape::api::compile_default,  "print('program'); syntax error here=") != 0) && (stack.is_error(-1)));
      stack.clear();
      test_expect(stack.pcompile_string(duktape::api::compile_default, "print('program without syntax error');") == 0);
    }
    // pcompile_file()
    {
      stack.clear();
      stack.push("myFile.js");
      test_expect((stack.pcompile_file(duktape::api::compile_default,  "print('program'); syntax error here=") != 0) && (stack.is_error(-1)));
      stack.clear();
      stack.push("myFile.js");
      test_expect(stack.pcompile_file(duktape::api::compile_default, "print('program without syntax error');") == 0);
    }
    // peval()
    {
      stack.clear();
      stack.push("print('Hello world!'); syntax error here=");
      test_expect( (stack.peval() != 0) && (stack.is_error(-1)));
    }
    // peval_string()
    {
      stack.clear();
      test_expect( (stack.peval_string("123.toUpperCase()") != 0) && (stack.is_error(-1)));
      stack.clear();
      test_expect( (stack.peval_string("'testString'.toUpperCase()") == 0) && (stack.get<string>(-1) == "TESTSTRING"));
    }
    // peval_string()
    {
      stack.clear();
      test_expect( (stack.peval_string_noresult("E R R O R") != 0) && (stack.top()==0));
      stack.clear();
      test_expect( (stack.peval_string_noresult("'testString'.toUpperCase()") == 0) && (stack.top()==0));
    }
    // pcall()
    {
      stack.clear();
      stack.push("2+3");
      stack.push("eval");
      stack.compile(duktape::api::compile_eval);
      test_note("stack.pcall(0); <-- after push('eval 2+3'); and compile().");
      test_expect((stack.pcall(0) == 0) && (stack.get<int>(0) == 2+3) );
    }
    // pcall_method()
    {
      stack.clear();
      stack.push("(function(x,y) { print('call_method() this:', this, 'x=', x, 'y=', y); return this+x+y; })");
      stack.eval();
      stack.push(1230);
      stack.push(2);
      stack.push(3);
      test_expect((stack.pcall_method(2) == 0) && (stack.get<int>(-1) == (1230+2+3)));
    }
    // pcall_prop()
    {
      stack.clear();
      stack.eval_string("global.a_obj={ plus: function(a,b){return a+b} }");
      test_expect( stack.is_object(0) );
      test_expect( stack.has_prop_string(0, "plus") );
      stack.push("plus");
      stack.push(2);
      stack.push(3);
      test_expect( (stack.pcall_prop(0, 2) == 0) && (stack.get<int>(-1) == (2+3)) );
    }
    // safe_call()
    {
      auto args = my_safe_args();
      args.floor = 1;
      stack.push(10);
      stack.push(11);
      stack.push(12);
      test_expect(stack.safe_call(duktape_safe_call_compliant_function, (void *) &args, 3 /*nargs*/, 2 /*nrets*/) == 0);
      test_expect(stack.get<int>(-2) == 21);
      test_expect(stack.is_undefined(-1));
    }
    // check_type()
    {
      stack.clear();
      stack.push(1);
      test_expect( stack.check_type(-1, (DUK_TYPE_NUMBER)) );
      stack.push("1");
      test_expect( stack.check_type(-1, (DUK_TYPE_STRING)) );
    }
    // check_type_mask()
    {
      stack.clear();
      stack.push(1);
      test_expect( stack.check_type_mask(-1, (DUK_TYPE_MASK_NUMBER)) );
      stack.push("1");
      test_expect( stack.check_type_mask(-1, (DUK_TYPE_MASK_NUMBER|DUK_TYPE_MASK_STRING)) );
    }
    // get_top() / set_top() / get_top_index()
    {
      stack.clear();
      test_expect( stack.get_top() == 0 );
      test_expect( stack.get_top_index() == (DUK_INVALID_INDEX) );
      stack.push(42);
      test_expect( stack.get_top() == 1 );
      test_expect( stack.get_top_index() == 0 );
      stack.push(84);
      test_expect( stack.get_top() == 2 );
      test_expect( stack.get_top_index() == 1 );
      stack.set_top(1);
      test_expect( stack.get_top() == 1 );
      test_expect( stack.get_top_index() == 0 );
    }
    // dup() / dup_top()
    {
      stack.clear();
      stack.push(42);
      test_expect( (stack.top() == 1) && (stack.get<int>(0) == 42) );
      stack.dup(0);
      test_expect( (stack.top() == 2) && (stack.get<int>(1) == 42) );
      stack.dup_top();
      test_expect( (stack.top() == 3) && (stack.get<int>(2) == 42) );
    }
    // copy()
    {
      stack.clear();
      stack.push(42);
      stack.push(17);
      test_expect( (stack.get<int>(0)==42) && (stack.get<int>(1)==17) );
      stack.copy(0, 1);
      test_expect( (stack.get<int>(0)==42) && (stack.get<int>(1)==42) );
    }
    // remove()
    {
      stack.clear();
      stack.push(42);
      stack.push(17);
      test_expect( (stack.top()==2) && (stack.get<int>(0)==42) && (stack.get<int>(1)==17) );
      stack.remove(0);
      test_expect( (stack.top()==1) && (stack.get<int>(0)==17) );
      stack.remove(0);
      test_expect( stack.top()==0 );
    }
    // insert()
    {
      stack.clear();
      stack.push(42);
      stack.push(17);
      stack.insert(0);
      test_expect( (stack.top()==2) && (stack.get<int>(0)==17) && (stack.get<int>(1)==42) );
    }
    // replace()
    {
      stack.clear();
      stack.push(42);
      stack.push(17);
      stack.replace(0);
      test_expect( (stack.top()==1) && (stack.get<int>(0)==17) );
    }
    // swap() / swap_top()
    {
      stack.clear();
      stack.push(42);
      stack.push(17);
      stack.push(4);
      stack.swap(0, 2);
      test_expect( (stack.top()==3) && (stack.get<int>(0)==4) && (stack.get<int>(1)==17) && (stack.get<int>(2)==42) );
      stack.swap_top(0);
      test_expect( (stack.top()==3) && (stack.get<int>(0)==42) && (stack.get<int>(1)==17) && (stack.get<int>(2)==4) );
    }
    // check_stack() / check_stack_top()
    {
      stack.clear();
      test_expect( stack.check_stack(5) );
      test_expect( !stack.check_stack(std::numeric_limits<duktape::api::index_type>::max()) ); // That has to be seen. It is not explicitly defined in the duktape API, but I doubt that this will be possible.
      test_expect( stack.check_stack_top(5) );
      test_expect( !stack.check_stack_top(std::numeric_limits<duktape::api::index_type>::max()) ); // That has to be seen. It is not explicitly defined in the duktape API, but I doubt that this will be possible.
    }
    // pnew()
    {
      stack.clear();
      stack.eval_string("global.TestConstructor = function(n1,n2){this.n=n1+n2};");
      test_expect( stack.is_function(-1) );
      stack.push(42);
      stack.push(24);
      test_expect( stack.pnew(2) );
      test_expect( stack.is_object(-1) );
    }
    // next() / enumerator()
    {
      stack.clear();
      stack.eval_string("[11,12,13]");
      test_expect( stack.is_array(-1) );
      stack.enumerator(-1, duktape::api::enum_own_properties_only);
      const auto it = stack.top_index();
      test_expect( stack.next(it, 1) && stack.to<int>(-2)==0 && stack.get<int>(-1)==11 );
      test_expect( stack.next(it, 1) && stack.to<int>(-2)==1 && stack.get<int>(-1)==12 );
      test_expect( stack.next(it, 1) && stack.to<int>(-2)==2 && stack.get<int>(-1)==13 );
      test_expect( !stack.next(it, 1) );
    }
    // compact()
    {
      stack.clear();
      stack.push("The meaning of Liv");
      stack.compact(-1);
    }
    // del_prop() / del_prop_index() / del_prop_string()
    {
      stack.clear();
      stack.eval_string("obj = {a:4,b:5,c:6,'1':7}");
      test_expect( stack.is_object(-1) );
      test_expect( stack.has_prop_string(-1, "a") );
      test_expect( stack.has_prop_string(-1, "b") );
      test_expect( stack.has_prop_string(-1, "c") );
      test_expect( stack.has_prop_string(-1, "1") );
      stack.push("a");
      test_expect( stack.del_prop(-2) );
      test_expect( stack.top() == 1 );
      test_expect(!stack.has_prop_string(-1, "a") );
      test_expect( stack.has_prop_string(-1, "b") );
      test_expect( stack.has_prop_string(-1, "c") );
      test_expect( stack.has_prop_string(-1, "1") );
      test_expect( stack.del_prop_string(-1, "b") );
      test_expect(!stack.has_prop_string(-1, "a") );
      test_expect(!stack.has_prop_string(-1, "b") );
      test_expect( stack.has_prop_string(-1, "c") );
      test_expect( stack.has_prop_string(-1, "1") );
      test_expect( stack.del_prop_index(-1, 1) );
      test_expect(!stack.has_prop_string(-1, "a") );
      test_expect(!stack.has_prop_string(-1, "b") );
      test_expect( stack.has_prop_string(-1, "c") );
      test_expect(!stack.has_prop_string(-1, "1") );
      const auto property_name = "c";
      test_expect( stack.del_prop_string(-1, property_name) );
      test_expect(!stack.has_prop_string(-1, "a") );
      test_expect(!stack.has_prop_string(-1, "b") );
      test_expect(!stack.has_prop_string(-1, "c") );
      test_expect(!stack.has_prop_string(-1, "1") );
    }
    // get_prop() / push_global_object()
    {
      stack.clear();
      stack.push_global_object();
      stack.push("Math");
      test_expect( stack.get_prop(-2) );
      test_expect( stack.has_prop_string(-1, "PI") );
      stack.push("PI");
      test_expect( stack.get_prop(-2) );
      test_note( int(stack.get<double>(-1) * 10000) );
      test_expect( int(stack.get<double>(-1) * 10000) == 31415 );
    }
    // get_prop_index()
    {
      stack.clear();
      stack.eval_string("obj = [0x00,0x55,0xaa,0xff]");
      test_expect( stack.is_array(-1) );
      test_expect( stack.get_prop_index(-1, 0) );
      test_expect( stack.get<int>(-1) == 0x00 );
      test_expect( stack.get_prop_index(-2, 1) );
      test_expect( stack.get<int>(-1) == 0x55 );
      test_expect( stack.get_prop_index(-3, 2) );
      test_expect( stack.get<int>(-1) == 0xaa );
      test_expect( stack.get_prop_index(-4, 3) );
      test_expect( stack.get<int>(-1) == 0xff );
    }
    // get_prop_desc()
    {
      static constexpr unsigned get_prop_flags_0 = 0;
      stack.clear();
      stack.eval_string("obj = {a:5}");
      test_expect( stack.is_object(-1) );
      stack.push("a");
      test_expect_noexcept( stack.get_prop_desc(-2, get_prop_flags_0) );
      test_expect( stack.is_object(-1) );
    }
    // get_prototype() // set_prototype() / has_prop_string()
    {
      test_expect_noexcept( stack.eval_string("obj = {a:5}") );
      test_expect_noexcept( stack.eval_string("obj.__proto__ = Math") );
      test_expect_noexcept( stack.select("obj") );
      test_expect( stack.is_object(-1) );
      test_expect_noexcept( stack.get_prototype(-1) );
      test_expect( stack.has_prop_string(-1, "PI") );
      stack.clear();
      test_expect_noexcept( stack.eval_string("obj = {a:5}") );
      test_expect_noexcept( stack.select("Math") );
      test_expect_noexcept( stack.set_prototype(-2) );
      test_expect( stack.has_prop_string(-1, "PI") );
    }
    // get_global_string()
    {
      stack.clear();
      test_expect_noexcept( stack.eval_string("global.n1 = 123456") );
      test_expect_noexcept( stack.get_global_string("n1") );
      test_expect( stack.get<int>(-1) == 123456 );
      test_expect_noexcept( stack.get_global_string(string("n1")) );
      test_expect( stack.get<int>(-1) == 123456 );
    }
    // has_prop() / has_prop_index()
    {
      stack.clear();
      test_expect_noexcept( stack.eval_string("obj = {a:5}") );
      test_expect_noexcept( stack.push("a") );
      test_expect( stack.has_prop(-2) );
      stack.clear();
      test_expect_noexcept( stack.eval_string("obj = [1,2]") );
      test_expect( stack.has_prop_index(-1, 0) && stack.has_prop_index(-1, 1) );
    }
    // put_function_list() / put_prop_string()
    {
      stack.clear();
      const ::duk_function_list_entry module_functions[] = {
        { "tweak", duktape_c_function_argument_count, 0 /* no args */ },
        { "adjust", duktape_c_function_argument_count, 3 /* 3 args */ },
        { nullptr, nullptr, 0 }
      };
      stack.push_global_object();
      stack.push_object();
      stack.put_function_list(-1, module_functions);
      stack.put_prop_string(-2, "MyModule");
      stack.clear();
      stack.select("MyModule");
      test_expect( stack.is_object(-1) );
      test_expect( stack.has_prop_string(-1, "tweak") );
      test_expect( stack.has_prop_string(-1, "adjust") );
      stack.clear();
      stack.push_global_object();
      stack.push("string-value");
      stack.put_prop_string(-2, string("s"));
      stack.clear();
      stack.get_global_string(string("s"));
      test_expect( stack.get<string>(-1) == "string-value" );
      stack.clear();
    }
    // put_global_string()
    {
      stack.clear();
      stack.push("put_global_string");
      stack.put_global_string("a");
      stack.push_global_object();
      stack.get_prop_string(-1, "a");
      test_expect( stack.get<string>(-1) == "put_global_string" );
      stack.clear();
      stack.push("put_global_string");
      stack.put_global_string(string("a"));
      stack.push_global_object();
      stack.get_prop_string(-1, "a");
      test_expect( stack.get<string>(-1) == "put_global_string" );
      stack.clear();
      stack.push(string("put_global_string"));
      stack.put_global_string("b");
      stack.push_global_object();
      stack.get_prop_string(-1, "b");
      test_expect( stack.get<string>(-1) == "put_global_string" );
    }
    // put_number_list()
    {
      const ::duk_number_list_entry my_module_consts[] = { { "c0", 1.11 }, { "c1", 2.22 }, { nullptr, 0.0 } };
      stack.clear();
      stack.push_object();
      stack.put_number_list(-1, my_module_consts);
      test_expect( stack.is_object(-1) && (stack.top()==1) );
      test_expect( stack.property<double>("c0") == 1.11 );
      test_expect( stack.property<double>("c1") == 2.22 );
    }
    // put_prop() / get_prop()
    {
      stack.clear();
      stack.push_object();
      stack.push_string("key");
      stack.push_string("value");
      stack.put_prop(-3);
      test_expect( stack.is_object(-1) && (stack.top()==1) );
      stack.push_string("key");
      test_expect( stack.get_prop(-2) );
      test_expect( stack.get<string>(-1) == "value" );
    }
    // push_array() / put_prop_index() / get_prop_index()
    {
      stack.clear();
      stack.push_array();
      stack.push_string("val1");
      stack.put_prop_index(-2, 0);
      stack.push_string("val2");
      stack.put_prop_index(-2, 1);
      test_expect( stack.is_array(-1) && (stack.top()==1) );
      test_expect( stack.get_prop_index(-1, 0) );
      test_expect( stack.get<string>(-1) == "val1" );
      test_expect( stack.get_prop_index(-2, 1) );
      test_expect( stack.get<string>(-1) == "val2" );
    }
    // normalize_index()
    {
      stack.clear();
      stack.push("TEST1");
      stack.push("TEST2");
      stack.push("TEST4");
      test_expect( stack.normalize_index(1) == 1 );
    }
    // get_boolean() / push_true() / push_false()
    {
      stack.clear();
      stack.push(true);
      stack.push(false);
      stack.push_true();
      stack.push_false();
      test_expect( stack.get_boolean(0) && !stack.get_boolean(1) );
      test_expect( stack.get<bool>(2) && !stack.get<bool>(3) );
    }
    // get_buffer() / push_buffer() / get_buffer_data() / is_buffer()
    {
      ::duk_size_t sz=0;
      stack.clear();
      stack.push_buffer(size_t(10), false);
      stack.push_buffer(size_t(1), true);
      test_expect( stack.is_buffer(-2) );
      test_expect( stack.is_buffer(-1) );
      test_expect( stack.is_buffer_data(-2) );
      test_expect( stack.is_buffer_data(-1) );
      test_expect( stack.is_fixed_buffer(-2) );
      test_expect(!stack.is_fixed_buffer(-1) );
      test_expect(!stack.is_dynamic_buffer(-2) );
      test_expect( stack.is_dynamic_buffer(-1) );
      test_expect( (stack.get_buffer(-2, sz) != nullptr) && (sz==10) );
      test_expect( (stack.get_buffer(-1, sz) != nullptr) && (sz==1) );
      test_expect( (stack.get_buffer_data(-2, sz) != nullptr) && (sz==10) );
      test_expect( (stack.get_buffer_data(-1, sz) != nullptr) && (sz==1) );
      test_expect( (stack.to_buffer(-2, sz) != nullptr) && (sz==10) );
      test_expect( (stack.to_fixed_buffer(-2, sz) != nullptr) && (sz==10) );
      test_expect( (stack.to_dynamic_buffer(-2, sz) != nullptr) && (sz==10) );
    }
    // get_c_function() / push_c_function() / is_c_function() / is_callable()
    {
      stack.clear();
      test_expect( stack.push_c_function(duktape_c_function_argument_count, 2) == 0);
      test_expect( stack.is_c_function(0) );
      test_expect( stack.is_callable(0) );
      test_expect( stack.get_c_function(0) == duktape_c_function_argument_count );
      stack.push("arg1");
      stack.push("arg2");
      stack.call(2);
      test_expect( stack.get<int>(-1) == 2 );
    }
    // push_thread() / get_context() / push_thread_new_globalenv()
    {
      duktape::engine js; // Don't want an additional thread in the main test js engine.
      auto& stack = js.stack();
      ::duk_context *ctx1 = nullptr;
      stack.clear();
      test_expect( stack.push_thread() == 0 );
      test_expect( stack.is_thread(-1) );
      test_expect( (ctx1=stack.get_context(-1)) != nullptr );
      test_expect_noexcept( stack.push_thread_stash(ctx1) );
      stack.clear();
      test_expect( stack.push_thread_new_globalenv() == 0 );
      test_expect( (ctx1=stack.get_context(-1)) != nullptr );
      test_expect_noexcept( stack.push_thread_stash(ctx1) );
      js.clear();
    }
    // get_now()
    {
      stack.clear();
      using namespace std::chrono;
      test_expect( abs(long(stack.get_now()/1000)-long(duration_cast<seconds>(system_clock::now().time_since_epoch()).count())) < 2) ;
    }
    // push_int() / push_uint() / push_undefined() ... (push/get/is for standard types)
    {
      stack.clear();
      test_expect_noexcept( stack.push_int(-1111) );
      test_expect( stack.get_int(-1) == -1111 );
      test_expect( stack.is_number(-1) );
      test_expect_noexcept( stack.push_uint(1111) );
      test_expect( stack.is_number(-1) );
      test_expect( stack.get_uint(-1) == 1111 );
      test_expect_noexcept( stack.push_undefined() );
      test_expect( stack.is_undefined(-1) );
      test_expect( stack.is_null_or_undefined(-1) );
      test_expect_noexcept( stack.push_array() );
      test_expect( stack.get_length(-1) == 0 );
      test_expect_noexcept( stack.push_number(-1111.11) );
      test_expect( stack.is_number(-1) );
      test_expect( stack.get_number(-1) == -1111.11 );
      stack.clear();
      test_expect_noexcept( stack.push_null() );
      test_expect( stack.is_null(-1) );
      test_expect( stack.is_null_or_undefined(-1) );
      test_expect_noexcept( stack.push_string("A") );
      test_expect( stack.is_string(-1) );
      test_expect( stack.get_string(-1) == "A" );
      test_expect_noexcept( stack.push_pointer(duktape_c_array_to_test_data_pointers) );
      test_expect( stack.is_pointer(-1) );
      test_expect( stack.get_pointer(-1) == duktape_c_array_to_test_data_pointers );
      test_expect( stack.get_pointer(-1, nullptr) == duktape_c_array_to_test_data_pointers );
      stack.clear();
      test_expect_noexcept( stack.push_this() );
      test_expect( stack.is_undefined(-1) );
    }
    // push_proxy()
    {
      stack.clear();
      stack.push_object();
      stack.push_object();
      stack.push_c_function(duktape_c_function_argument_count, 3);
      stack.put_prop_string(-2, "get"); // getter trap of the proxy
      test_expect( stack.push_proxy() == 0 );
      test_expect( stack.is_object(0) );
    }
    // pop()
    {
      stack.clear();
      stack.push(1);
      stack.push(1);
      stack.push(1);
      stack.push(1);
      stack.push(1);
      test_expect( stack.top() == 5 );
      test_expect_noexcept( stack.pop() );
      test_expect( stack.top() == 4 );
      test_expect_noexcept( stack.pop(2) );
      test_expect( stack.top() == 2 );
    }
    // get_memory_functions()
    {
      stack.clear();
      auto mem_functions = duk_memory_functions();
      stack.get_memory_functions(mem_functions);
      test_expect( mem_functions.alloc_func != nullptr );
      test_expect( mem_functions.realloc_func != nullptr );
      test_expect( mem_functions.free_func != nullptr );
    }
    // is_false() / is_true()
    {
      stack.clear();
      test_expect_noexcept( stack.push(true, false) );
      test_expect( stack.is_true(-2) );
      test_expect( stack.is_false(-1) );
    }
    // is_instanceof()
    {
      stack.clear();
      stack.eval_string("err = new Error('test error');");
      stack.get_global_string("Error");
      test_expect( stack.is_instanceof(-2, -1) );
    }
    // is_bound_function()
    {
      stack.clear();
      stack.eval_string("function bindadd(a,b){return a+b}; o={a:10}; fn=bindadd.bind(o,1,2);");
      stack.get_global_string("fn");
      test_expect( stack.is_bound_function(-1) );
    }
    // is_bound_function()
    {
      stack.clear();
      stack.eval_string("fn = function(){return null}");
      test_expect( stack.is_ecmascript_function(-1) );
    }
    // is_eval_error()
    {
      stack.clear();
      test_expect( stack.peval_string("throw new EvalError()") != 0 );
      test_expect( stack.is_eval_error(-1) );
    }
    // is_object_coercible() / to_object()
    {
      stack.clear();
      stack.push(true);
      test_expect( stack.is_object_coercible(-1) );
      test_expect_noexcept( stack.to_object(-1) );
      test_expect( stack.is_object(-1) );
    }
    // is_***_error()
    {
      stack.clear();
      stack.peval_string("(function(){return o.a.b.c})()");
      test_expect( stack.is_error(-1) );
      test_expect( stack.is_type_error(-1) );
      test_expect(!stack.is_reference_error(-1) );
      test_expect(!stack.is_range_error(-1) );
      test_expect(!stack.is_syntax_error(-1) );
      test_expect(!stack.is_uri_error(-1) );
    }
    // is_constructort_call() / is_strict_call()
    {
      stack.clear();
      js.define("test_constructor", ctor_function);
      stack.eval_string("(function(){ o=new test_constructor() })()");
    }
    // is_constructort_call()
    {
      stack.clear();
      js.define("test_constructor", ctor_function);
      test_expect_noexcept( stack.eval_string("(function(){ o=new test_constructor() })()") );
    }
    // is_constructort_call()
    {
      stack.clear();
      test_expect(!stack.is_valid_index(0) );
      stack.push(1);
      test_expect( stack.is_valid_index(0) );
    }
    // push_current_thread()
    {
      stack.clear();
      test_expect_noexcept( stack.push_current_thread() );
      test_expect( !stack.is_thread(-1) ); // @todo implement coroutine
    }
    // push_global_stash()
    {
      stack.clear();
      test_expect_noexcept( stack.push_global_stash() );
      test_expect( stack.is_object(-1) );
    }
    // is_symbol()
    {
      stack.clear();
      ::duk_push_literal(stack.ctx(), DUK_HIDDEN_SYMBOL("mySymbol")); // we don't use symbols actively ...
      test_expect( stack.is_symbol(-1) ); // ...but a chech may be needed.
    }
    // to_***()
    {
      stack.clear();
      test_expect_noexcept( stack.push("111") );
      test_expect( stack.to_int(-1) == 111);
      test_expect( stack.to_int32(-1) == 111);
      test_expect( stack.to_uint(-1) == 111);
      test_expect( stack.to_uint16(-1) == 111);
      test_expect( stack.to_uint32(-1) == 111);
      test_expect( stack.to_string(-1) == "111" );
      test_expect_noexcept( stack.to_null(-1) );
      test_expect( stack.is_null(-1) );
      test_expect_noexcept( stack.to_undefined(-1) );
      test_expect( stack.is_undefined(-1) );
      test_expect_noexcept( stack.push("TEST") );
      test_expect( stack.to_pointer(-1) != nullptr);
      test_expect_noexcept( stack.eval_string("t = new Date()") );
      test_expect_noexcept( stack.to_primitive(-1, DUK_HINT_NUMBER) );
      test_expect( stack.is<long>(-1) );
      test_expect_noexcept( stack.eval_string("t = new Error('Test error')") );
      test_expect( !stack.to_stacktrace(-1).empty() );
      test_expect( !stack.safe_to_stacktrace(-1).empty() );
    }
    // samevalue() / concat() / join() / equals() / strict_equals()
    {
      stack.clear();
      stack.push("10");
      stack.push("10");
      test_expect( stack.samevalue(-1,-2) );
      test_expect( stack.equals(-1,-2) );
      test_expect( stack.strict_equals(-1,-2) );
      test_expect_noexcept( stack.concat(2) );
      test_expect( stack.get<string>(-1) == "1010" );
      stack.clear();
      stack.push(",");
      stack.push("10");
      stack.push(10);
      stack.push(true);
      test_expect_noexcept( stack.join(3) );
      test_expect( stack.get<string>(-1) == "10,10,true" );
    }
    // substring
    {
      stack.clear();
      stack.push("Hello World");
      test_expect_noexcept( stack.substring(-1, 6, 6+5) );
      test_note( stack.get<string>(-1)  );
      test_expect( stack.get<string>(-1) == "World" );
      stack.push(" Hello World   ");
      test_expect_noexcept( stack.trim(-1) );
      test_expect( stack.get<string>(-1) == "Hello World" );
      test_expect( stack.char_code_at(-1, 0) == 'H' );
    }
    // json_decode / json_encode
    {
      stack.clear();
      test_expect_noexcept( stack.push("{\"a\":1,\"b\":2}") );
      test_expect_noexcept( stack.json_decode(-1) );
      test_expect( stack.is_object(-1) && stack.has_prop_string(-1, "a") && stack.has_prop_string(-1, "b") );
      test_expect_noexcept( stack.del_prop_string(-1, "a") );
      test_expect_noexcept( stack.json_encode(-1) );
      test_expect( stack.get<string>(-1) == "{\"b\":2}" );
    }
    // base64_encode() / base64_decode()
    {
      auto sz = ::duk_size_t(0);
      stack.clear();
      uint8_t *buf = reinterpret_cast<uint8_t*>(stack.push_fixed_buffer(4));
      buf[0]=0x01; buf[1]=0x01; buf[2]=0x02; buf[3]=0x03; // dang it, no std::span<> available yet.
      test_expect( stack.base64_encode(-1) == "AQECAw==" );
      stack.push("AQECAw==");
      test_expect_noexcept( stack.base64_decode(-1) );
      test_expect( stack.is_buffer(-1) && reinterpret_cast<const uint8_t*>(stack.get_buffer_data(-1, sz))[0]==0x01 );
    }
    // hex_encode() / hex_decode()
    {
      stack.clear();
      test_expect_noexcept( stack.push("01020304") );
      test_expect_noexcept( stack.hex_decode(-1) );
      test_expect( stack.is_buffer(-1) );
      test_expect( stack.hex_encode(-1) == "01020304" );
    }
    // buffer_to_string
    {
      stack.clear();
      test_expect_noexcept( stack.push("32323200") );
      test_expect_noexcept( stack.hex_decode(-1) );
      test_expect( stack.is_buffer(-1) );
      test_expect( stack.buffer_to_string(-1) == "222" );
    }
    // map_string / decode_string
    {
      stack.clear();
      test_expect_noexcept( stack.push("1234567890") );
      test_expect_noexcept( stack.map_string(-1, map_char_function_all_to_a, nullptr) );
      test_expect( stack.get<string>(-1) == "AAAAAAAAAA" );
      test_expect_noexcept( stack.decode_string(-1, decode_char_function_all_to_a, nullptr) );
    }
    // alloc() / realloc() / free() / alloc_raw() / alrealloc_rawloc() / free_raw()
    {
      void* ptr = nullptr;
      stack.clear();
      test_expect( (ptr=stack.alloc(10)) != nullptr );
      test_expect( (ptr=stack.realloc(ptr, 5)) != nullptr );
      test_expect_noexcept( stack.free(ptr) );
      test_expect( (ptr=stack.alloc_raw(10)) != nullptr );
      test_expect( (ptr=stack.realloc_raw(ptr, 5)) != nullptr );
      test_expect_noexcept( stack.free_raw(ptr) );
    }
    // push_external_buffer()
    {
      auto sz = ::duk_size_t(0);
      auto arr = std::array<uint8_t, 128>();
      test_expect_noexcept( stack.push_external_buffer() );
      test_expect_noexcept( stack.config_buffer(-1, arr.data(), arr.max_size()) );
      test_expect( stack.is_buffer(-1) );
      test_expect( (reinterpret_cast<const uint8_t*>(stack.get_buffer(-1, sz)) == arr.data()) && (sz==arr.max_size()) );
      test_expect_noexcept( stack.push_external_buffer(arr.data(), arr.max_size()) );
      test_expect( stack.is_buffer(-1) );
      test_expect( (reinterpret_cast<const uint8_t*>(stack.get_buffer(-1, sz)) == arr.data()) && (sz==arr.max_size()) );
    }
    // def_prop()
    {
      stack.clear();
      stack.push_global_object();
      stack.push("a_key");
      stack.push("a_value");
      stack.def_prop(0);
      stack.clear();
      stack.select("a_key");
      test_expect( stack.get<string>(-1) == "a_value" );
    }
    // dump_function() / load_function()
    {
      stack.clear();
      test_expect_noexcept( stack.eval_string("(function helloWorld() { print('hello world'); })") );
      test_expect_noexcept( stack.dump_function() );
      test_expect( stack.is_buffer(-1) );
      test_expect_noexcept( stack.load_function() );
      test_expect( stack.is_callable(-1) );
    }
    // set_global_object()
    {
      stack.clear();
      test_expect_noexcept( stack.push_global_object() );
      test_expect_noexcept( stack.set_global_object() );
    }
    // set_length() / get_length()
    {
      stack.clear();
      test_expect_noexcept( stack.push_array() );
      test_expect_noexcept( stack.set_length(-1, 10) );
      test_expect( stack.get_length(-1) == 10 );
    }
    // suspend() / resume()
    {
      stack.clear();
      ::duk_thread_state trd;
      test_expect_noexcept( trd=stack.suspend() );
      test_expect_noexcept( stack.resume(trd) );
    }
    // steal_buffer()
    {
      stack.clear();
      auto sz = ::duk_size_t(0);
      uint8_t* ptr1 = nullptr;
      uint8_t* ptr2 = nullptr;
      test_expect( (ptr1=reinterpret_cast<uint8_t*>(stack.push_dynamic_buffer(10))) != nullptr );
      test_expect( (ptr2=reinterpret_cast<uint8_t*>(stack.steal_buffer(-1, sz))) != nullptr );
      test_expect( ptr1 == ptr2 );
      test_expect( sz == 10 );
      test_expect_noexcept( ::free(ptr2) );
    }
    // inspect_callstack_entry()
    {
      stack.clear();
      test_expect_noexcept( stack.inspect_callstack_entry(0) );
      test_expect( stack.is_undefined(-1) );
      test_expect_noexcept( stack.push(1) );
      test_expect_noexcept( stack.inspect_value(-1) );
      test_expect( stack.is_object(-1) );
    }
    // seal() / freeze()
    {
      stack.clear();
      test_expect_noexcept( stack.eval_string("global.o={a:1, b:2}") );
      test_expect( stack.is_object(-1) );
      test_expect_noexcept( stack.property("a", 100) );
      test_expect( stack.property<int>("a") == 100 );
      test_expect_noexcept( stack.seal(-1) );
      test_expect_noexcept( stack.freeze(-1) );
      test_note( stack.dump() );
      test_note( stack.json_encode(-1) );
      test_expect_noexcept( stack.eval_string("global.o.a=10;") );
      stack.top(0);
      test_expect_noexcept( stack.eval_string("global.o") );
      test_expect( stack.get_prop_string(-1, "a") );
      test_expect( stack.get<int>(-1) == 100 );
      test_note( stack.get<int>(-1) );
    }
    // push(Args...)
    {
      stack.clear();
      char nonconst_char[2] = {'A','\0'};
      long double lld = 1;
      string str2move = "moved string";
      test_expect_noexcept( stack.push(1, "a", "A", 1.1, vector<int>{1,2,3}, std::move(str2move), lld, nonconst_char) );
      test_expect( stack.top() == 8 );
      test_expect( stack.get<int>(0) == 1 );
      test_expect( stack.get<string>(1) == "a" );
      test_expect( stack.get<string>(2) == "A" );
      test_expect( stack.get<double>(3) == 1.1 );
      test_expect( stack.get<vector<int>>(4).size() == 3 );
      test_expect( stack.get<string>(5) == "moved string" );
      test_expect( stack.get<long double>(6) == lld );
      test_expect( stack.get<string>(7) == "A" );
    }
    // get(key, default)
    {
      stack.clear();
      test_expect_noexcept( stack.push_undefined() );
      test_expect( stack.get(1, string("AAA")) == "AAA" );
      test_expect( stack.get(2, 1.0) == 1.0 );
      test_expect( stack.get(3, 10) == 10 );
    }
    // push(T&&)
    {
      auto s = string("str");
      auto v = vector<int>{1,2,3};
      test_expect_noexcept( stack.push(std::move(s)) );
      test_expect( stack.is_string(-1) );
      test_expect_noexcept( stack.push(std::move(v)) );
      test_expect( stack.is_array(-1) );
    }
    // put_prop_string_hidden() / get_prop_string_hidden()
    {
      stack.clear();
      test_expect_noexcept( stack.push_global_object() );
      test_expect_noexcept( stack.push("hidden string") );
      test_expect_noexcept( stack.put_prop_string_hidden(0, "hidden_string") );
      test_expect_noexcept( stack.clear() );
      test_expect( js.eval<string>("global.hidden_string") == "undefined" );
      test_expect_noexcept( js.eval<void>("global.hidden_string = 'other value'") );
      test_expect_noexcept( stack.push_global_object() );
      test_expect_noexcept( stack.get_prop_string_hidden(0, "hidden_string") );
      test_expect( stack.get<string>(-1) == "hidden string" );
    }
    // error()
    {
      stack.clear();
      test_expect_except( stack.error(duktape::api::err_ecma_eval, "err_ecma_eval", "<file>", 1042) );
      stack.clear();
      stack.eval_string("o = new EvalError('test error')");
      test_expect( stack.get_error_code(-1) == duktape::api::err_ecma_eval );
    }
    // Reset.
    {
      stack.clear();
    }
  }

}

void test_stack_thread_use(duktape::engine& unused_js)
{
  (void)unused_js;
  auto js = duktape::engine();
  auto stack1 = js.stack();
  stack1.push(true);
  stack1.push(false);
  stack1.push(100);
  stack1.push_thread();
  auto stack2 = duktape::api(stack1.get_context(-1));
  test_expect_noexcept( stack1.xcopy_to_thread(stack2, 3) );
  test_expect_noexcept( stack2.xmove_to_thread(stack1, 3) );
  test_note( "stack1.top() == " << stack1.top() );
  test_note( "stack2.top() == " << stack2.top() );
  stack1.clear();
  stack2.clear();
}

/**
 * Test main.
 */
void test(duktape::engine& js)
{
  test_api_construction(js);
  test_typed_stack_getters_and_setters(js);
  test_stack_guard(js);
  test_native_class_wrapping_related(js);
  test_direct_stack_method_use(js);
  test_stack_thread_use(js);
}
