test_note("taux-check-process ...");
var proc = null;

const timeout_guarded = function(proc, timeout_ms) {
  const timeout = sys.clock() + (timeout_ms || (proc.timeout+500)) * 1e-3;
  while(proc.running) {
    proc.update();
    if(sys.clock() > timeout) {
      proc.kill(9);
      proc = null;
      break;
    }
  }
  if(!proc) {
    test_fail("Process did not terminate in "+timeout_ms+"ms");
  } else {
    test_note("proc.program == " + proc.program);
    test_note("proc.environment == " + proc.environment);
    test_note("proc.arguments == " + proc.arguments);
    test_note("proc.ignore_stdout == " + proc.ignore_stdout);
    test_note("proc.ignore_stderr == " + proc.ignore_stderr);
    test_note("proc.redirect_stderr_to_stdout == " + proc.redirect_stderr_to_stdout);
    test_note("proc.no_path_search == " + proc.no_path_search);
    test_note("proc.no_inherited_environment == " + proc.no_inherited_environment);
    test_note("proc.no_arg_escaping == " + proc.no_arg_escaping);
    test_note( "proc.running == " + proc.running );
    test_note( "proc.timeout == " + proc.timeout );
    test_note( "proc.runtime == " + proc.runtime );
    test_note( "proc.exitcode == " + proc.exitcode );
    test_note( "proc.stdout == " + JSON.stringify(proc.stdout) );
    test_note( "proc.stderr == " + JSON.stringify(proc.stderr) );
  }
  return proc;
}

//----------------------------------------------------------------------------------------------------------

test_note("sys.process: Process prints to stderr/stdout and exits. Manual timeout check.");
if((proc=timeout_guarded(new sys.process(SCRIPT_TEST_BINARY, {stdout:true, stderr:true, timeout:200}))) != null) {
  test_expect( !proc.running );
  test_expect( !proc.ignore_stdout );
  test_expect( !proc.ignore_stderr );
  test_expect( proc.program == SCRIPT_TEST_BINARY);
  test_expect( proc.timeout == 200 );
  test_expect( proc.runtime < proc.timeout );
  test_expect( proc.runtime >= 0 );
  test_expect( proc.exitcode == 0 );
  test_expect( proc.stdout.replace(/[\r\n]+/g,"\n") == "STDOUT text\n" );
  test_expect( proc.stderr.replace(/[\r\n]+/g,"\n") == "STDERR text\n" );
}

//----------------------------------------------------------------------------------------------------------

test_note("sys.process: Timeout check: Process prints 0..9 with 50ms delay - shall be killed after 50ms.");
if((proc=timeout_guarded(new sys.process(SCRIPT_TEST_BINARY, {args:["child-process-print10x50ms"], timeout:50, stdout:true, stderr:true}))) != null, 500) {
  test_expect( !proc.running );
  test_expect( proc.timeout == 50 );
  test_expect( proc.runtime < (proc.timeout+25) );
  test_expect( proc.runtime >= 50e-3 );
  test_expect( proc.exitcode != 0 );
  test_expect( proc.stdout !== undefined );
  test_expect( proc.stderr !== undefined );
}

//----------------------------------------------------------------------------------------------------------

test_note("sys.process: Capture process stdin/stdout.");
if((proc=timeout_guarded(new sys.process(SCRIPT_TEST_BINARY, {args:["child-process-print10x50ms"], timeout:1000, stdout:true, stderr:true}))) != null) {
  test_expect( !proc.running );
  test_expect( proc.timeout == 1000 );
  test_expect( proc.runtime < (proc.timeout+25) );
  test_expect( proc.runtime >= 50e-3 );
  test_expect( proc.exitcode == 0 );
  test_expect( proc.stdout.replace(/[\r\n]+/g,"\n") == "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n" );
  test_expect( proc.stderr.replace(/[\r\n]+/g,"\n") == "" );
}

//----------------------------------------------------------------------------------------------------------

test_note("sys.process: Check stdin -> stdout/stderr reflection.");
proc = timeout_guarded(new sys.process(SCRIPT_TEST_BINARY, {
  args:["child-process-reflect-stdin"],
  timeout:100,
  stdout:true,
  stderr:true,
  stdin: ((("[###########----------##########----------]") + "\n").repeat(25) +"exit\n")
}));
test_expect( !proc.running );
test_expect( proc.timeout == 100 );
test_expect( proc.runtime < (proc.timeout+25) );
test_expect( proc.runtime >= 0 );
test_expect( proc.exitcode == 0 );
test_expect( proc.stdout.replace(/[\r]/g,"").search(/[\n]exit[\n]$/) > 0 );
test_expect( proc.stderr.replace(/[\r]/g,"").search(/[\n]exit[\n]$/) > 0 );
test_expect( proc.stderr == proc.stdout );
test_note  ( proc.stdout = "" );
test_note  ( proc.stderr = "" );
test_expect( proc.stdin  == "" );
test_expect( proc.stdout == "" );
test_expect( proc.stderr == "" );

//----------------------------------------------------------------------------------------------------------

test_note("sys.process: Check stdin -> process kill.");
proc = new sys.process(SCRIPT_TEST_BINARY, { args:["child-process-reflect-stdin"], timeout:100 });
sys.sleep(5e-3)
proc.kill();
for(var i=0; i<10 && proc.running; ++i) sys.sleep(5e-3);
test_expect( !proc.running );
proc.kill(9);

test_note("sys.process: Check stdin -> process kill 9.");
proc = new sys.process(SCRIPT_TEST_BINARY, { args:["child-process-reflect-stdin"], timeout:1000 });
proc.timeout = 100; // dynamic timeout adaption.
sys.sleep(5e-3);
proc.kill(9);
for(var i=0; i<10 && proc.running; ++i) sys.sleep(5e-3);
test_expect( !proc.running );
test_expect( proc.timeout == 100 );

proc = null;
