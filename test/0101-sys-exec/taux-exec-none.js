test_note("taux-exec-none ...");
const res = sys.exec(SCRIPT_TEST_BINARY, {stdout:true, stderr:'stdout'});
test_note( "res.exitcode ==" + res.exitcode );
test_note( "res.stdout == " + res.stdout.replace(/[\r\n]+/g,"\n") );
test_note( "res.stderr == " + res.stderr );
test_expect( res.exitcode == 0 );
test_expect( res.stdout.replace(/[\r\n]+/g,"\n") == "STDOUT text\nSTDERR text\n" );
test_expect( res.stderr == "" );
