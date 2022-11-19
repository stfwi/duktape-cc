
(function(){
  const o = {a:1,b:2}
  o.fn = function() {return "text"};
  o.fn2 = function() { throw "THROWN" };
  // These should fail
  test_note( "----- The following tests fail intentionally ---" );
  test_expect( o.fn2() == "text" );
  test_expect( o.fn() == "TEXT" );
  test_fail( "Fail message" );
  test_warn( "Warning message");
  test_reset();
  test_comment( "-----" )
  test_pass( "Pass message" );
  test_expect( (function(){ return o.a })() == o.b-1 );
  test_expect( o.fn().toLowerCase() == "text" );
  test_assert( o.fn() == "text" );
  test_expect_noexcept( o.fn() == "text" );
  test_expect_except( o.fn2() == "text" );
  test_note( "TEST NOTE", "with", "multiple", "arguments" );
  print( "TEST NOTE", "using", "print()" )
  alert( "TEST NOTE", "using", "alert()" )
  test_note( "sys.args=" + JSON.stringify(sys.args) );
  test_note( "callstack() = " + callstack());
  test_note( "test_abspath('.') = '" + test_abspath('.') + "'");
  test_note( "test_relpath('.') = '" + test_relpath('.') + "'");
})();
