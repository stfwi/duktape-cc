#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.sys.exec.hh>

using namespace std;

#ifndef OS_WINDOWS
  static const string SCRIPT_TEST_BINARY = "./taux.elf";
#else
  static const string SCRIPT_TEST_BINARY = "./taux.exe";
#endif

namespace {

  void test_exec(duktape::engine& js)
  {
    // Parameter position/format validity checks
    {
      test_expect_except( js.eval<int>("sys.exec()") );
      test_expect_except( js.eval<int>("sys.exec('')") );
      test_expect_except( js.eval<int>("sys.exec({})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo'}, {invalid:true})") ); // 1st arg object -> only arg.
      test_expect_except( js.eval<int>("sys.exec({'program':['invalid-not-string']})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo', args:{invalid:'not-array'}})") );
      test_expect_except( js.eval<int>("sys.exec('echo', ['1','2','3'], {args:['invalid','already','specified']})") );
      test_expect_except( js.eval<int>("sys.exec([])") );
      test_expect_except( js.eval<int>("sys.exec('',[])") );
      test_expect_except( js.eval<int>("sys.exec('echo','invalid-not-an-arg-array')") );
      test_expect_except( js.eval<int>("sys.exec('echo',['test-output'], 'invalid-not-an-arg-object')") );
      test_expect_except( js.eval<int>("sys.exec('echo',{program:'echo-double-program-spec'})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo', stdout:{invalid:'no-string-or-bool'}})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo', stderr:{invalid:'no-string-or-bool'}})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo', env:'invalid-no-plain-object'})") );
      test_expect_except( js.eval<int>("sys.exec({'program':'echo', env:{inval:'with=equals'}})") );
      test_expect_except( js.eval<int>("sys.exec('',[],{})") );
      test_expect_except( js.eval<int>("sys.exec('echo', {}, {})") );
      test_expect_except( js.eval<int>("sys.exec('echo', {}, [])") );
      test_expect_noexcept( js.eval<int>("sys.exec('echo', {})") ); // no arguments, options directly
      test_expect_noexcept( js.eval<int>("sys.exec('cat',{stdin:false})") );
      test_expect_noexcept( js.eval<int>("sys.exec('cat',{stdin:null})") );
      test_expect_noexcept( js.eval<int>("sys.exec('cat',{stdin:undefined})") );
      test_expect_noexcept( js.eval<int>("sys.exec('cat',{stdin:new Buffer(3)})") );
      test_expect_except( js.eval<int>("sys.exec('cat',{stdin:{invalid:'no-object-type-allowed'}})") );

      test_note( js.eval<string>("JSON.stringify( sys.exec('###notthere') )") );
      test_expect( js.eval<int>("sys.exec('###notthere')") != 0 ); // no arg error, shell exit code

      #ifndef OS_WINDOWS
      {
        test_note( js.call<string>("sys.escapeshellarg", "no-specials") );
        test_expect( js.call<string>("sys.escapeshellarg", "no-specials") == "'no-specials'" );
        test_note( js.call<string>("sys.escapeshellarg", "\"double-quotes\"") );
        test_expect( js.call<string>("sys.escapeshellarg", "\"double-quotes\"") == "'\"double-quotes\"'" );
        test_note( js.call<string>("sys.escapeshellarg", "'single-quotes'") );
        test_expect( js.call<string>("sys.escapeshellarg", "'single-quotes'") == "'\\'single-quotes\\''" );
      }
      #else
      {
        test_note( js.call<string>("sys.escapeshellarg", "no-specials") );
        test_expect( js.call<string>("sys.escapeshellarg", "no-specials") == "no-specials" );
        test_note( js.call<string>("sys.escapeshellarg", "\"double-quotes\"") );
        test_expect( js.call<string>("sys.escapeshellarg", "\"double-quotes\"") == "\"\\\"double-quotes\\\"\"" );
        test_note( js.call<string>("sys.escapeshellarg", "'single-quotes'") );
        test_expect( js.call<string>("sys.escapeshellarg", "'single-quotes'") == "'single-quotes'" );
      }
      #endif
    }

    #ifndef OS_WINDOWS
    {
      test_expect( js.eval<int>("sys.exec('/bin/ls')") == 0);
      test_expect( js.eval<int>("sys.exec('ls')") == 0);
      test_expect( js.eval<int>("sys.exec('echo')") == 0);
      test_expect( js.eval<int>("sys.exec({ program:'echo' })") == 0);
      test_expect( js.eval<int>("sys.exec({ program:'echo', args:[1] })") == 0);
      test_expect( js.eval<int>("sys.exec('echo', ['a'])") == 0);
      test_expect( js.eval<int>("sys.exec('echo', ['a']), {}") == 0);
      test_expect( js.eval<int>("sys.exec('/bin/true')") == 0);
      test_expect( js.eval<int>("sys.exec('/bin/false')") == 1);

      // stdout, stderr
      test_comment( js.eval<string>("JSON.stringify( sys.exec('echo', ['-n','a',1], {stdout:true}) )") );
      test_comment( js.eval<string>("JSON.stringify(sys.exec('/bin/sh', ['-c','/bin/echo -n test >/dev/stderr'], {stdout:true,stderr:true}))") );
      test_expect( js.eval<string>("JSON.stringify(sys.exec('/bin/sh', ['-c','/bin/echo -n test >/dev/stderr'], {stdout:true,stderr:true}))") == "{\"exitcode\":0,\"stdout\":\"\",\"stderr\":\"test\"}");

      // stdin
      test_expect( js.eval<string>("sys.exec('/bin/cat', {stdout:true, stdin:'TEST'}).stdout") == "TEST");
      test_expect( js.eval<string>("sys.exec('/bin/cat', [], {stdout:true, stdin:'TEST'}).stdout") == "TEST");
      test_expect( js.eval<string>("sys.exec({program:'/bin/cat', stdout:true, stdin:'TEST'}).stdout") == "TEST");

      // redirect stderr to stdout
      test_comment( js.eval<string>("JSON.stringify( sys.exec({program:'/bin/sh', args:['-c','/bin/echo -n test >/dev/stderr'], stdout:true, stderr:'stdout'}) )") );
      test_expect( js.eval<string>("JSON.stringify( sys.exec({program:'/bin/sh', args:['-c','/bin/echo -n test >/dev/stderr'], stdout:true, stderr:'stdout'}) )") == "{\"exitcode\":0,\"stdout\":\"test\",\"stderr\":\"\"}" );

      // nopath option (execution does not include $PATH environment search)
      test_expect( js.eval<int>("sys.exec('echo')") == 0);
      test_expect( js.eval<int>("sys.exec('echo', {nopath:false})") == 0);
      test_expect( js.eval<int>("sys.exec('echo', {nopath:true})") == 1);

      // noenv option (do not pass on the current user environment to the child process)
      test_comment( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true}).stdout") );
      test_comment( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, noenv:true}).stdout") );
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, noenv:false}).stdout") != "" );
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, noenv:true}).stdout") == "" );
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true}).stdout") != "" );
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, noenv:true, nopath:true}).stdout") == "" );

      // setting environment
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, env:{TEST_ENVVAR:123456}}).stdout").find("TEST_ENVVAR=123456") != string::npos );
      test_expect( js.eval<string>("sys.exec('/usr/bin/env', {stdout:true, noenv:true, env:{TEST_ENVVAR:123456}}).stdout").find("TEST_ENVVAR=123456") == 0);

      // stdout callback: return replaced string to fetch
      test_expect( js.eval<string>( "sys.exec('cat', { stdin:'a\\nb\\nc\\n', stdout:function(s){ return s.replace(/^a$/mig, 'AAA').replace(/[\\n]/mig,''); } }).stdout") == "AAAbc" );

      // stdout callback: return true to add, false to omit
      test_expect( js.eval<string>( "sys.exec('cat', { stdin:'TEST', stdout:function(s){ return true; } }).stdout") == "TEST" );
      test_expect( js.eval<string>( "sys.exec('cat', { stdin:'TEST', stdout:function(s){ return false; } }).stdout") == "" );
      test_expect( js.eval<string>( "sys.exec('cat', { stdin:'TEST', stdout:function(s){} }).stdout") == "" );
      test_expect( js.eval<string>( "sys.exec('/bin/cat', ['--not-existing-option'], {stderr:function(s){return s.replace(/^/mig,'[e] ');} }).stderr").find("[e]") == 0 );
      test_comment( js.eval<string>( "sys.exec('/bin/cat', ['--not-existing-option'], {stderr:function(s){return s.replace(/^/mig,'[e] ');} }).stderr")  );
    }
    #else
    {
      test_expect( js.eval<int>("sys.exec('cat.exe')") == 0);
      test_expect( js.eval<int>("sys.exec('ls.exe')") == 0);
      test_expect( js.eval<int>("sys.exec('echo')") == 0);
      test_expect( js.eval<int>("sys.exec({ program:'echo' })") == 0);
      test_expect( js.eval<int>("sys.exec({ program:'echo', args:[1] })") == 0);
      test_expect( js.eval<int>("sys.exec('echo', ['a'])") == 0);
      test_expect( js.eval<int>("sys.exec('echo', ['a']), {}") == 0);
      test_expect( js.eval<int>("sys.exec('true.exe')") == 0);
      test_expect( js.eval<int>("sys.exec('false.exe')") == 1);

      // stdout, stderr
      #ifndef OS_WINDOWS
      // echo is not a program under win32.
      test_comment( js.eval<string>("JSON.stringify(sys.exec('echo', ['a',1], {stdout:true}))") );
      test_expect ( js.eval<string>("JSON.stringify(sys.exec('echo', ['a',1], {stdout:true}))") == "{\"exitcode\":0,\"stdout\":\"a 1\\n\",\"stderr\":\"\"}");
      #endif
      test_comment( js.eval<string>("JSON.stringify(sys.exec('cmd.exe', ['/C', 'echo test 1>&2'], {stdout:true,stderr:true}))") );
      test_expect ( js.eval<string>("JSON.stringify(sys.exec('cmd.exe', ['/C', 'echo test 1>&2'], {stdout:true,stderr:true}))") == "{\"exitcode\":0,\"stdout\":\"\",\"stderr\":\"test \\r\\n\"}");

      // stdin
      test_expect( js.eval<string>("sys.exec('cat.exe', {stdout:true, stdin:'TEST'}).stdout") == "TEST");
      test_expect( js.eval<string>("sys.exec('cat.exe', [], {stdout:true, stdin:'TEST'}).stdout") == "TEST");
      test_expect( js.eval<string>("sys.exec({program:'cat.exe', stdout:true, stdin:'TEST'}).stdout") == "TEST");

      // redirect stderr to stdout
      test_comment( js.eval<string>("JSON.stringify(sys.exec({program:'cmd.exe', args:['/C','echo test 2>&1'], stdout:true, stderr:'stdout'}))") );
      test_expect( js.eval<string>("JSON.stringify(sys.exec({program:'cmd.exe', args:['/C','echo test 2>&1'] , stdout:true, stderr:'stdout'}))") == "{\"exitcode\":0,\"stdout\":\"test \\r\\n\",\"stderr\":\"\"}" );

      // nopath option (execution does not include $PATH environment search)
      test_expect( js.eval<int>("sys.exec('cmd.exe', ['/C'])") == 0);
      test_expect( js.eval<int>("sys.exec('cmd.exe', {args:['/C'], nopath:false})") == 0);
      test_comment( js.eval<string>(" JSON.stringify( sys.exec('cmd.exe', {args:['/C'], nopath:true}) )") );
      test_expect( js.eval<int>("sys.exec('cmd.exe', {args:['/C'], nopath:true})") != 0);

      // noenv option (do not pass on the current user environment to the child process)
      test_comment( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env-keys'], stdout:true}).stdout") );
      test_comment( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env-keys'], stdout:true, noenv:true}).stdout") );
      test_expect ( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env'], stdout:true, noenv:false}).stdout") != "" );

      // setting environment
      test_note  ( "sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env-keys'], stdout:true, env:{TEST_ENVVAR:123456}}).stdout = ...");
      test_note  ( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env-keys'], stdout:true, env:{TEST_ENVVAR:123456}}).stdout.replace(/[\\r\\n]+/g,'\\n')"));
      test_expect( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env'], stdout:true, env:{TEST_ENVVAR:123456}}).stdout").find("TEST_ENVVAR=123456") != string::npos );
      test_expect( js.eval<string>("sys.exec(SCRIPT_TEST_BINARY, {args: ['print-env'], stdout:true, noenv:true, env:{TEST_ENVVAR:123456}}).stdout").find("TEST_ENVVAR=123456") != string::npos );
    }
    #endif
  }

  void test_shell(duktape::engine& js)
  {
    test_expect( js.eval<string>("sys.shell()") == "" );
    test_expect( js.eval<string>("sys.shell('')") == "" );
    test_expect( js.eval<string>("sys.shell({})") == "" );
    test_expect( js.eval<string>("sys.shell([])") == "" );

    #ifndef OS_WINDOWS
    {
      test_note  ( "sys.shell('/usr/bin/env') = '" << js.eval<string>("sys.shell('/usr/bin/env')") << "'");
      test_expect( js.eval<string>("sys.shell('/usr/bin/env')") != "" );
      test_expect( js.eval<bool>("sys.shell('sleep 0.1', 500) === ''") );
      test_expect( js.eval<bool>("sys.shell('sleep 0.5', 10) === undefined") );
    }
    #else
    {
      test_expect( js.eval<string>("sys.shell('set')") != "" );
    }
    #endif
  }

  void test_taux_scripts(duktape::engine& js)
  {
    test_include_script(js, "taux-exec-none.js");
    test_include_script(js, "taux-exec-echo-stdin.js");
    test_include_script(js, "taux-check-process.js");
  }

}

void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);
  duktape::mod::system::exec::define_in<>(js);
  js.define("SCRIPT_TEST_BINARY", SCRIPT_TEST_BINARY);
  test_exec(js);
  test_shell(js);
  test_taux_scripts(js);
}
