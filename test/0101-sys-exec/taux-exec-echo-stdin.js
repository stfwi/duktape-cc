
test_note("taux-exec-echo-stdin ...");
const res = sys.exec(SCRIPT_TEST_BINARY, {args:["echo-stdin"], stdout:true, stderr:'stdout', stdin:'TEST STDIN\nTEST STDIN\nTEST STDIN'});
test_note( "res.exitcode ==" + res.exitcode );
test_note( "res.stdout == " + res.stdout.replace(/[\r\n]+/g,"\n") );
test_note( "res.stderr == " + res.stderr );
test_expect( res.exitcode == 0 );
test_expect( res.stdout.replace(/[\r\n]+/g,"\n") == "TEST STDIN\nTEST STDIN\nTEST STDIN\n" );
test_expect( res.stderr == "" );
