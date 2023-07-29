/**
 * @test stdio
 *
 * Basic standard I/O functionality checks.
 */
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>
#include <stack>

namespace {
  #ifdef OS_WINDOWS
  static constexpr bool is_windows = true;
  #else
  static constexpr bool is_windows = false;
  #endif
}

using namespace std;

std::stringstream ssin, sout;

namespace {

  void test_basics(duktape::engine& js)
  {
    const auto reset_streams = [&](){ ssin.clear(); sout.str(""); };

    // confirm/prompt/console.read
    {
      reset_streams(); ssin.str("y\n");
      test_expect( js.eval<string>("confirm('Test confirm: ');") == "y");
      test_expect( sout.str() == "Test confirm: ");
      reset_streams(); ssin.str("y\n");
      test_expect( js.eval<string>("confirm();") == "y");
      test_expect( sout.str() == "[press ENTER to continue ...]");
      reset_streams(); ssin.str("prompt-line\n");
      test_expect( js.eval<string>("prompt('Test prompt: ');") == "prompt-line");
      reset_streams(); ssin.str("test line 1\ntest line 2\ntest line 3\n");
      test_expect( js.eval<string>("console.read();") == "test line 1\ntest line 2\ntest line 3\n");
      reset_streams(); ssin.str("1\n\n2\n3\n");
      test_expect(js.eval<string>("console.read(function(line){return line!=''})") == "1\n2\n3\n"); // filter function bool return
      reset_streams(); ssin.str("1\n2\n3");
      test_expect(js.eval<string>("console.read(function(line){return '#'+line})") == "#1\n#2\n#3\n"); // replacer function (string return)
      if(!is_windows) {
        // On windows the console has to be directly accessed, these data are not reproducible with iostreams.
        reset_streams(); ssin.str("");
        test_expect(js.eval<string>("JSON.stringify(console.read(1))") == "undefined");
      }
      reset_streams(); ssin.str("XYZ");
      test_expect(js.eval<string>("JSON.stringify(new Uint8Array(console.read(true)))") == "{\"0\":88,\"1\":89,\"2\":90}" ); // read as binary into a plain buffer, toJSON is a plain object.
      reset_streams(); ssin.str("test line 1\ntest line 2\ntest line 3\n");
      test_expect( js.eval<string>("console.readline();") == "test line 1");
    }
    // print/alert/console.log/console.write/console.readline/printf/sprintf
    {
      reset_streams();
      test_expect_noexcept(js.eval<void>("console.log('Test string', 1, false, 0x10, [11,12]);"));
      test_expect( sout.str() == "Test string 1 false 16 11,12\n");
      reset_streams();
      test_expect_noexcept(js.eval<void>("print('Test string', 1, false, 0x10, [11,12]);"));
      test_note( "stdout = " << js.call<string>("JSON.stringify", sout.str()));
      test_expect( sout.str() == "Test string 1 false 16 11,12\n");
      reset_streams();
      test_expect_noexcept(js.eval<void>("print(new Buffer([48,49,50]));")); // binary print, don't add newline.
      test_note( "stdout = " << js.call<string>("JSON.stringify", sout.str()));
      test_expect( sout.str() == "012");
      reset_streams();
      test_expect_noexcept(js.eval<void>("alert('Test string', 1, false, 0x10, [11,12]);"));
      test_note( "stdout = " << js.call<string>("JSON.stringify", sout.str()));
      test_expect( sout.str() == "Test string 1 false 16 11,12\n");
      reset_streams();
      test_expect_noexcept(js.eval<void>("console.write('Test string', 1, false, 0x10, [11,12]);"));
      test_note( "stdout = " << js.call<string>("JSON.stringify", sout.str()));
      test_expect( sout.str() == "Test string1false1611,12"); // no fillers, no newline.
      reset_streams();
      test_expect_noexcept(js.eval<void>("console.write('Test string', 1, false, 0x10, [11,12]);"));
      test_note( "stdout = " << js.call<string>("JSON.stringify", sout.str()));
      test_expect( sout.str() == "Test string1false1611,12"); // no fillers, no newline.
      reset_streams();
      test_expect_noexcept(js.eval<void>("console.write(new Buffer([48,49,50]));"));
      test_note( "stdout (ASCII 48='0') = " << sout.str());
      test_expect( sout.str() == "012"); // binary output, ascii(48)='0'
      reset_streams();
      test_expect_noexcept(js.eval<void>("console.write(new Buffer([48,49,50]).buffer);")); // plain buffer / array buffer
      test_note( "stdout = " << sout.str());
      test_expect( sout.str() == "012"); // binary output, ascii(48)='0'
      reset_streams();
      test_expect_noexcept(js.eval<void>("printf('%s|%10s|%d|%5d|%2x|%4x|%5.2f|%.1lf', 'string', 'string10', 42, 500, 0xff, 0xffff, 5.2, 1.0);"));
      test_note( "stdout = '" << sout.str() << "'");
      test_expect( sout.str() == "string|  string10|42|  500|ff|ffff| 5.20|1.0");
      reset_streams();
      test_expect_noexcept(js.eval<void>("printf('%s with implicit toString()', 10);")); // implicit to_string()
      test_note( "stdout = '" << sout.str() << "'");
      test_expect( sout.str() == "10 with implicit toString()");
      reset_streams();
      test_expect_except(js.eval<void>("printf('%lf', {});")); // no number
      test_expect_except(js.eval<void>("printf('%c', {});")); // no number (character) nor string
      test_expect_except(js.eval<void>("sprintf('%s', {});")); // not string compatible
      reset_streams();
      test_expect_noexcept(js.eval<void>("printf('%2s', '123');")); // at least 2 chars, if input is more extend.
      test_note( "stdout = '" << sout.str() << "'");
      test_expect( sout.str() == "123");
      reset_streams();
      test_expect(js.eval<string>("sprintf('%2s', '123');") == "123"); // implementation identical to printf, only using string stream.
      test_expect(js.eval<string>("sprintf('%c', 0x20);") == " "); // space character as number
      test_expect(js.eval<string>("sprintf('%c', ' ');") == " "); // space character as string
      test_expect(js.eval<string>("sprintf('%%%c', ' ');") == "% "); // Escaped %
    }
  }
}


namespace {

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

  void test_printf_valid(duktape::engine& js)
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

  void test_printf_invalid(duktape::engine& js)
  {
    test_expect_except( printf2str(js, "", 1) );
    test_expect_except( printf2str(js, "", 1,2,3,'a') );
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
    test_expect_except( printf2str(js, "%5000c", " ") ); // width field too large
  }

}

void test(duktape::engine& js)
{
  duktape::mod::stdio::in_stream = &ssin;
  duktape::mod::stdio::err_stream = &sout;
  duktape::mod::stdio::out_stream = &sout;
  duktape::mod::stdio::log_stream = &sout;
  duktape::mod::stdio::define_in(js);      // Note: That also overrides alert and print.
  test_basics(js);
  test_printf_valid(js);
  test_printf_invalid(js);
}
