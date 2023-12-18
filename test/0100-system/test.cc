#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>

using namespace std;

void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);

  test_note( "sys.pid() = " << js.eval<long>("sys.pid()") );
  test_expect( js.eval<long>("sys.pid()") > 0 );

#ifdef __linux__

  test_note( "sys.uid() = " << js.eval<long>("sys.uid()") );
  test_expect( js.eval<long>("sys.uid()") > 0 );

  test_note( "sys.gid() = " << js.eval<long>("sys.gid()") );
  test_expect( js.eval<long>("sys.gid()") > 0 );

  test_note( "sys.user() = " << js.eval<string>("sys.user()") );
  test_expect( js.eval<bool>("typeof(sys.user()) === 'string'") );
  test_note( "sys.user(0) = " << js.eval<string>("sys.user(0)") );
  test_expect( js.eval<bool>("sys.user(0) === 'root'") );
  test_expect( js.eval<bool>("sys.user('invalid-arg') === undefined") );

  test_note( "sys.group() = " << js.eval<string>("sys.group()") );
  test_expect( js.eval<bool>("typeof(sys.group()) === 'string'") );
  test_note( "sys.group(0) = " << js.eval<string>("sys.group(0)") );
  test_expect( js.eval<bool>("sys.group(0) === 'root'") );
  test_expect( js.eval<bool>("sys.group('invalid-arg') === undefined") );

  test_note( "sys.uname() = " << js.eval<string>("JSON.stringify(sys.uname())") );
  test_expect( js.eval<bool>("sys.uname().sysname !== undefined") );
  test_expect( js.eval<bool>("sys.uname().release !== undefined") );
  test_expect( js.eval<bool>("sys.uname().machine !== undefined") );
  test_expect( js.eval<bool>("sys.uname().version !== undefined") );

  test_expect( js.eval<string>("sys.executable()") != "" );

  test_expect( js.eval<bool>("sys.sleep(1e-3) === true") );
  test_expect( js.eval<bool>("sys.sleep(Number.NaN) === false") );
  test_expect( js.eval<bool>("sys.sleep('also nan') === false") );

  test_expect( js.eval<bool>("sys.clock() >= 0") ); // default monotonic
  test_expect( js.eval<bool>("sys.clock('m') >= 0") ); // monotonic
  test_expect( js.eval<bool>("sys.clock('b') >= 0") ); // monotonic boot time
  test_expect( js.eval<bool>("sys.clock('r') >= 0") ); // monotonic real time, if available

  const auto t0 = js.call<double>("sys.clock");
  test_expect( js.eval<bool>("sys.sleep(50e-3) === true") );
  const auto t1 = js.call<double>("sys.clock");
  test_note("t0 before sleep = " << t0 << ", t1 after sleep = " << t1);
  test_expect( ((t1-t0) >= 50e-3) && ((t1-t0) <= 1.0) );

  test_expect( js.eval<bool>("sys.isatty('stdin') !== undefined") );  // isatty(STDIN_FILENO)
  test_expect( js.eval<bool>("sys.isatty('stdout') !== undefined") ); // isatty(STDOUT_FILENO)
  test_expect( js.eval<bool>("sys.isatty('stderr') !== undefined") ); // isatty(STDERR_FILENO)
  test_expect( js.eval<bool>("sys.isatty('what') === undefined") ); // isatty(???)

  test_note( "sys.beep() = " << js.eval<bool>("sys.beep()") ); // default frequency+duration
  test_note( "sys.beep(440) = " << js.eval<bool>("sys.beep(440)") ); // set frequency, default+duration
  test_note( "sys.beep(440, 500) = " << js.eval<bool>("sys.beep(440, 500)") ); // set frequency+duration

#endif

}
