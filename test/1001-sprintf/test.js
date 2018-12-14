alert("warning (alert)");
print("note (print)");
test_fail("called test_fail");
test_pass("called test_pass");
test_expect( "a" !== 'b' );
test_expect( eval("sprintf('aa')") === "aa" );
test_expect_except( notexisting() );
test_reset();
