#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>

using namespace std;

void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);

  test_comment( "sys.pid() = " << js.eval<long>("sys.pid()") );
  test_expect( js.eval<long>("sys.pid()") > 0 );

#if defined(__linux__) || defined(__linux)

  test_comment( "sys.uid() = " << js.eval<long>("sys.uid()") );
  test_expect( js.eval<long>("sys.uid()") > 0 );

  test_comment( "sys.gid() = " << js.eval<long>("sys.gid()") );
  test_expect( js.eval<long>("sys.gid()") > 0 );

  test_comment( "sys.user() = " << js.eval<string>("sys.user()") );
  test_expect( js.eval<bool>("typeof(sys.user()) === 'string'") );

  test_comment( "sys.group() = " << js.eval<string>("sys.group()") );
  test_expect( js.eval<bool>("typeof(sys.group()) === 'string'") );

  test_comment( "sys.user(0) = " << js.eval<string>("sys.user(0)") );
  test_expect( js.eval<bool>("sys.user(0) === 'root'") );

  test_comment( "sys.group(0) = " << js.eval<string>("sys.group(0)") );
  test_expect( js.eval<bool>("sys.group(0) === 'root'") );

  test_comment( "sys.uname() = " << js.eval<string>("JSON.stringify(sys.uname())") );
  test_expect( js.eval<bool>("sys.uname().sysname !== undefined") );
  test_expect( js.eval<bool>("sys.uname().release !== undefined") );
  test_expect( js.eval<bool>("sys.uname().machine !== undefined") );
  test_expect( js.eval<bool>("sys.uname().version !== undefined") );

#endif

}
