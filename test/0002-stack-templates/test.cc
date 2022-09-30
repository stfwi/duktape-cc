/**
 * Stack tests
 */
#include "../testenv.hh"
#include <limits>

using namespace std;

constexpr unsigned num_iterations = 1000;
constexpr duk_double_t duk_number_max_excact_value =  (numeric_limits<duk_double_t>::denorm_min()-1)/2;
constexpr duk_double_t duk_number_min_excact_value = -(duk_number_max_excact_value);

template <typename T>
static inline void check_typed_value(duktape::api& stack, const T& val)
{
  duktape::stack_guard sg(stack);
  stack.push(val);
  if(stack.top() != 1) test_fail("stack.top() == 1");
  if(!stack.is<T>(-1)) test_fail("stack.is<T>(-1)");
  T ret = stack.get<T>(-1);
  if(ret != val) {
    test_fail(ret, " != ", val);
  } else {
    test_pass(ret, " == ", val);
  }
}

template <typename T>
static void check_type(duktape::api& stack)
{
  test_comment("type: " << duktape::detail::conv<T>::cc_name());
  T min = numeric_limits<T>::min();
  T max = numeric_limits<T>::max();

  if(numeric_limits<T>::is_iec559) {
    if(sizeof(T) > sizeof(duk_double_t)) {
      if(max > T(numeric_limits<duk_double_t>::max())) max = T(numeric_limits<duk_double_t>::max());
      if(min < T(numeric_limits<duk_double_t>::min())) min = T(numeric_limits<duk_double_t>::min());
    }
  } else if(numeric_limits<T>::is_integer) {
    if(numeric_limits<T>::max() > T(duk_number_max_excact_value)) {
      max = T(duk_number_max_excact_value);
      if(std::numeric_limits<T>::is_signed) min = T(duk_number_min_excact_value);
    }
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
  for(unsigned i=0; i<num_iterations; ++i) check_typed_value(stack, sw::utest::random<T>(min, max));
}

template <>
void check_type<bool>(duktape::api& stack)
{
  check_typed_value(stack, true);
  check_typed_value(stack, false);
}

template <>
void check_type<long double>(duktape::api& stack)
{
  check_typed_value(stack, numeric_limits<double>::min());
  check_typed_value(stack, numeric_limits<double>::max());
  check_typed_value(stack, sw::utest::random<double>());
}

void test(duktape::engine& js)
{
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

  {
    const char* val = "TEST";
    string cmp(val);
    test_comment("type: 'const char*'");
    test_comment("value: " << val);
    duktape::stack_guard sg(stack);
    stack.push(val);
    test_expect(stack.top() == 1);
    test_expect(stack.is<const char*>(-1));
    test_expect(stack.is<string>(-1));
    test_expect(stack.get<string>(-1) == cmp);
  }

}
