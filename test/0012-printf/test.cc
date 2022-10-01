/**
 * Include, function definitions
 */
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>
#include <stack>

using namespace std;
std::stringstream ssin, sout;

template <typename ...Args>
std::string printf2str(duktape::engine& js, std::string fmt, Args... args)
{
  std::stringstream localout;
  duktape::api& stack = js.stack();
  try {
    duktape::mod::stdio::out_stream = &localout;
    stack.top(0);
    stack.get_global_string("printf");
    stack.push(fmt);
    stack.push(args...);
    stack.pcall(stack.top()-1);
    if(stack.is_error(-1)) {
      throw duktape::script_error(stack.to<string>(-1));
    }
    stack.top(0);
    duktape::mod::stdio::out_stream = &sout;
    string s = localout.str();
    test_note(string("output: '") + s + "'");
    return s;
  } catch(...) {
    duktape::mod::stdio::out_stream = &sout;
    stack.top(0);
    throw;
  }
}

void test_valid(duktape::engine& js)
{
  test_expect( printf2str(js, "%d", 1) == "1" );
  test_expect( printf2str(js, "%d %d %d", 1,2,3) == "1 2 3" );
  test_expect( printf2str(js, "%%%d%%", 1) == "%1%" );
  test_expect( printf2str(js, "%%X.$+~%d~+$.Y%%", 1) == "%X.$+~1~+$.Y%" );
  test_expect( printf2str(js, "%%ABC DEF GHI%d~+$.Y%%", 1) == "%ABC DEF GHI1~+$.Y%" );

  test_expect( printf2str(js, "%4d", 111) == " 111" );
  test_expect( printf2str(js, "%04d", 111) == "0111" );
  test_expect( printf2str(js, "%+4d", 11) == " +11" );
  test_expect( printf2str(js, "%+04d", 11) == "+011" );
  test_expect( printf2str(js, "%+04d", -11) == "-011" );
  test_expect( printf2str(js, "%4d", -111) == "-111" );

  test_expect( printf2str(js, "%ld", 1) == "1" );
  test_expect( printf2str(js, "%10ld", 11) == "        11" );
  test_expect( printf2str(js, "%+10ld", 11) == "       +11" );
  test_expect( printf2str(js, "%ld", 2000000000) == "2000000000" ); // note: for 64bit %ld and %lld are equivalent
  test_expect( printf2str(js, "%ld", 2147483647) == "2147483647" );
  test_expect( printf2str(js, "%ld", -2147483648) == "-2147483648" );

  test_expect( printf2str(js, "%lld", 1) == "1" );
  test_expect( printf2str(js, "%10lld", 11) == "        11" );
  test_expect( printf2str(js, "%+10lld", 11) == "       +11" );
  test_expect( printf2str(js, "%lld", 2000000000000000) == "2000000000000000" );

  test_expect( printf2str(js, "%x", 255) == "ff");
  test_expect( printf2str(js, "%X", 255) == "FF");
  test_expect( printf2str(js, "%4x", 255) == "  ff");
  test_expect( printf2str(js, "%04x", 255) == "00ff");

  test_expect( printf2str(js, "%o", 255) == "377");
  test_expect( printf2str(js, "%4o", 255) == " 377");
  test_expect( printf2str(js, "%04o", 255) == "0377");

  test_expect( printf2str(js, "%f", 1).find_first_not_of("1.0") == std::string::npos );
  test_expect( printf2str(js, "%.3f", 1) == "1.000");
  test_expect( printf2str(js, "%5.3f", 1) == "1.000" );
  test_expect( printf2str(js, "%6.3f", 1) == " 1.000" );
  test_expect( printf2str(js, "%06.3f", 1) == "01.000" );
  test_expect( printf2str(js, "%+7.3f", 1) == " +1.000" );
  test_expect( printf2str(js, "%+08.3f", 1) == "+001.000" );

  test_expect( printf2str(js, "%lf", 1).find_first_not_of("1.0") == std::string::npos );
  test_expect( printf2str(js, "%.3lf", 1) == "1.000");
  test_expect( printf2str(js, "%5.3lf", 1) == "1.000" );
  test_expect( printf2str(js, "%6.3lf", 1) == " 1.000" );
  test_expect( printf2str(js, "%06.3lf", 1) == "01.000" );
  test_expect( printf2str(js, "%+7.3lf", 1) == " +1.000" );
  test_expect( printf2str(js, "%+08.3lf", 1) == "+001.000" );

  test_expect( (printf2str(js, "%12.4e", 1) == "  1.0000e+00") || (printf2str(js, "%12.4e", 1) == " 1.0000e+000") );
  test_expect( (printf2str(js, "%12.4E", 1) == "  1.0000E+00") || (printf2str(js, "%12.4E", 1) == " 1.0000E+000") );
  test_expect( (printf2str(js, "%g", 1) == "1") );
  test_expect( (printf2str(js, "%g", 1.1234) == "1.1234") );
  test_expect( (printf2str(js, "%g", 0.000000001) == "1e-09") || (printf2str(js, "%g", 0.000000001) == "1e-009") );

  test_expect( printf2str(js, "%c", 65) == "A" );
  test_expect( printf2str(js, "%2c", 65) == " A" );
  test_expect( printf2str(js, "%02c", 65) == " A" );
  test_expect( printf2str(js, "%c", "ABC") == "A" );
  test_expect( printf2str(js, "%c", "CBA") == "C" );
  test_expect( printf2str(js, "%100c", " ") == string(100,' ') );

  test_expect( printf2str(js, "%s", "") == "" );
  test_expect( printf2str(js, "%s", "CBA") == "CBA" );
  test_expect( printf2str(js, "%3s", "abcde") == "abcde" );
  test_expect( printf2str(js, "%s", 1000) == "1000" );
  test_expect( printf2str(js, "%s", -1000) == "-1000" );
  test_expect( printf2str(js, "%6s", -1000) == " -1000" );
  test_expect( printf2str(js, "%06s", -1000) == " -1000" );
  test_expect( printf2str(js, "%100s", " ") == string(100, ' ') );
  test_expect( printf2str(js, "%1000s", " ") == string(1000, ' ') );
  test_expect( printf2str(js, "test\tvalue:%05d,a=%.2f%%,set:%c,hex:0x%04x", 123, 98, "A", 4095) == "test\tvalue:00123,a=98.00%,set:A,hex:0x0fff" );
}

void test_invalid(duktape::engine& js)
{
  test_expect_except( printf2str(js, "", 1) );
  test_expect_except( printf2str(js, "", 1,2,3,'a') == "" );
  test_expect_except( printf2str(js, "%d", "A") );
  test_expect_except( printf2str(js, "%d", "") );
  test_expect_except( printf2str(js, "%ld", "") );
  test_expect_except( printf2str(js, "%u", 1) );
  test_expect_except( printf2str(js, "%n") );
  test_expect_except( printf2str(js, "%2$4d", 1,1) );
  test_expect_except( printf2str(js, "%*d", 5,1) );
  test_expect_except( printf2str(js, "%*x", 5, 0xff) );
  test_expect_except( printf2str(js, "%*s", 5, "abc") );
  test_expect_except( printf2str(js, "%s %s", "abc") );
  test_expect_except( printf2str(js, "%d") );
  test_expect_except( printf2str(js, "%d %d", 1) );
  test_expect_except( printf2str(js, "%d %d %d", 1,2) );
  test_expect_except( printf2str(js, "%2049s", " ") ); // width field too large
  test_expect_except( printf2str(js, "%5000c", " ") == string(5000,' ') ); // width field too large
}

void test_random(duktape::engine& js)
{
  (void) js; // @todo: add randomly generated patterns/values
}

void test(duktape::engine& js)
{
  duktape::mod::stdio::in_stream = &ssin;
  duktape::mod::stdio::err_stream = &sout;
  duktape::mod::stdio::out_stream = &sout;
  duktape::mod::stdio::log_stream = &sout;
  duktape::mod::stdio::define_in(js);
  test_invalid(js);
  test_valid(js);
  test_random(js);
}
