var backtrace = [];
function a() { b(); }
function b() { c(); }
function c() { backtrace = callstack().split("\n"); }
a();
//-------------------------------------------------------------------------------
test_comment(JSON.stringify(backtrace));

// Simple check if the functions are listed with correct line numbers
if(test_expect(backtrace !== undefined && backtrace.length !== undefined)) {
  test_expect(backtrace.length === 4);
  test_expect(backtrace[0].match(/^c/) && backtrace[0].match(/@test\.js:4/));
  test_expect(backtrace[1].match(/^b/) && backtrace[1].match(/@test\.js:3/));
  test_expect(backtrace[2].match(/^a/) && backtrace[2].match(/@test\.js:2/));
  test_expect(backtrace[3].match(/^eval/) && backtrace[3].match(/@test\.js:5/));
}
