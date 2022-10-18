#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>
#include <time.h>
#include <sstream>
#include <vector>

using namespace std;

/**
 * Conversion Array <--> std::vector<>
 */
namespace {

  void test_conv_std_vector(duktape::engine& js)
  {
    duktape::api stack(js);
    {
      duktape::stack_guard sg(stack);
      std::vector<int> values{ 1,2,3,4,5,10,100,1000 };
      js.define("array_values", values);

      string s = js.eval<string>("array_values");
      test_comment("Array values: " << s);

      std::vector<int> readback = js.eval<std::vector<int>>("array_values");

      {
        stringstream sss;
        sss << "{";
        if(!readback.empty()) sss << readback.front();
        for(size_t i=1; i<readback.size(); ++i) sss << "," << std::dec << readback[i];
        sss << "}";
        string s = sss.str();
        test_comment("Readback is: " << s);
      }

      test_expect( readback.size() == values.size() );
      if(readback.size() == values.size()) {
        test_expect( std::equal(values.begin(), values.end(), readback.begin()) );
      }
    }
  }

}

/**
 * Conversion Date <--> ::timespec
 */
namespace {

  #ifdef __linux__

  using timespec_type = ::timespec;

  timespec_type native_time_change_100_200(timespec_type ts)
  {
    ts.tv_sec = 100;
    ts.tv_nsec = 200000000;
    return ts;
  }

  int native_time_add_components(duktape::api& stack)
  {
    test_expect(stack.top() == 3);
    test_expect(stack.is<timespec_type>(0));
    test_expect(stack.is<long>(1));
    test_expect(stack.is<long>(2));
    const auto ts = stack.get<timespec_type>(0);
    const auto secs = stack.get<long>(1);
    const auto nsecs = stack.get<long>(2);
    stack.top(0);
    stack.push(timespec_type{ts.tv_sec+secs, ts.tv_nsec+nsecs});
    return 1;
  }

  int check_timespec_conv(duktape::api& stack)
  {
    using ts_conv_type = duktape::detail::conv<timespec_type>;
    stack.top(0);
    stack.push(timespec_type());
    test_expect( ts_conv_type::nret() == 1 );
    test_expect( string(ts_conv_type::cc_name()) == "timespec" );
    test_expect( string(ts_conv_type::ecma_name()) == "Date" );
    test_expect( ts_conv_type::is(stack.ctx(), 0) );
    test_expect_noexcept( ts_conv_type::req(stack.ctx(), 0) );
    test_expect_noexcept( ts_conv_type::to(stack.ctx(), 0) );
    return 0;
  }

  void test_timespec(duktape::engine& js)
  {
    js.define("native_time_change_100_200", native_time_change_100_200);
    js.define("native_time_add_components", native_time_add_components, 3);
    js.define("check_timespec_conv", check_timespec_conv, 0);
    auto ts = timespec_type();

    test_expect( js.eval<long>("var d = new Date('1970-01-01T00:05:00'); d;") == 300000 );
    test_expect( js.eval<string>("typeof(d)") == "object");
    test_expect( js.eval<bool>("d instanceof Date") );

    ts.tv_sec = 3600; ts.tv_nsec = 987000000;
    test_expect_noexcept( js.define("thetime", ts) );
    test_info( "::timespec{3600,987000000} === " << js.eval<string>("thetime.valueOf();") );
    test_info( "::timespec{3600,987000000} === " << js.eval<string>("thetime;") );
    test_expect( js.eval<long>("thetime.valueOf();") == 3600987 );

    test_expect_noexcept( ts=js.eval<timespec_type>("new Date('1970-01-01T00:05:00.1')") );
    test_expect( ts.tv_sec == 300 && ts.tv_nsec == 100000000 );

    test_expect_noexcept( ts=js.eval<timespec_type>("native_time_change_100_200(new Date('1970-01-01T00:05:00.1'))") );
    test_note( "ts.tv_sec=" << int64_t(ts.tv_sec) << ", ts.tv_nsec=" << int64_t(ts.tv_nsec) );
    test_expect( ts.tv_sec==100 && ts.tv_nsec==200000000 );

    test_expect_noexcept( ts=js.eval<timespec_type>("native_time_add_components(new Date('1970-01-01T00:05:00.1'), 1000, 500000000)") );
    test_note( "ts.tv_sec=" << int64_t(ts.tv_sec) << ", ts.tv_nsec=" << int64_t(ts.tv_nsec) );
    test_expect( ts.tv_sec==1300 && ts.tv_nsec==600000000 );

    test_expect_noexcept(js.call("check_timespec_conv"));
  }

  #endif
}

/**
 * Conversion Date <--> unix_timestamp
 */
namespace {

  int check_unix_timestamp_conv(duktape::api& stack)
  {
    using unix_timestamp = duktape::detail::unix_timestamp;
    using ts_conv_type = duktape::detail::conv<unix_timestamp>;
    test_expect_noexcept( stack.top(0) );
    test_expect_noexcept( stack.push(unix_timestamp()) );
    test_expect( ts_conv_type::nret() == 1 );
    test_expect( string(ts_conv_type::cc_name()) == "unix_timestamp" );
    test_expect( string(ts_conv_type::ecma_name()) == "Date" );
    test_expect( ts_conv_type::is(stack.ctx(), 0) );
    test_expect( ts_conv_type::req(stack.ctx(), 0).t == 0 );
    test_expect( ts_conv_type::get(stack.ctx(), 0).t == 0 );
    test_expect( ts_conv_type::to(stack.ctx(), 0).t == 0 );
    test_expect_noexcept( stack.top(0) );
    test_expect_noexcept( stack.push(unix_timestamp(1000)) );
    test_expect( ts_conv_type::req(stack.ctx(), 0).t == 1000 );
    test_expect_noexcept( stack.top(0) );
    test_expect_noexcept( stack.push(unix_timestamp(1000)) );
    test_expect( ts_conv_type::get(stack.ctx(), 0).t == 1000 );
    test_expect_noexcept( stack.top(0) );
    test_expect_noexcept( stack.push(unix_timestamp(1000)) );
    test_expect( ts_conv_type::to(stack.ctx(), 0).t == 1000 );
    return 0;
  }

  void test_unix_timestamp(duktape::engine& js)
  {
    js.define("check_unix_timestamp_conv", check_unix_timestamp_conv, 0);
    test_expect_noexcept(js.call<void>("check_unix_timestamp_conv"));
  }

}

void test(duktape::engine& js)
{
  test_conv_std_vector(js);
  #ifndef __linux__
    test_note("Skipped Date() to :timespec longer1ace for non-linux OS.");
  #else
    test_timespec(js);
  #endif
  test_unix_timestamp(js);
}
