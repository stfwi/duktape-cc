
function test_number_limit(timeout_s, max_iterations)
{
  if(Number.prototype.limit === undefined) {
    test_note("Skipping Number.prototype.limit test, feature not compiled.");
    return;
  }

  const check_rnd = function(range) {
    const value = (Math.random()-0.5) * 2.0 * range;
    const min = (Math.random()-0.5) * 2.0 * range;
    const max = (Math.random()-0.5) * 2.0 * range;
    const v1 = Math.max(min, Math.min(max, value));
    const v2 = value.limit(min, max);
    if(v1 === v2) {
      test_pass("(num="+value+").limit("+min+","+max+")=="+v2);
    } else {
      test_fail("(num="+value+").limit("+min+","+max+")=="+v2+" differs from Math:"+v1);
    }
  };

  max_iterations = max_iterations || 100000;
  timeout_s = timeout_s || 1000;
  const t0 = sys.clock() + timeout_s;
  while(--max_iterations > 0) {
    if(sys.clock() > t0) break;
    check_rnd(1e5);
    check_rnd(1e9);
    check_rnd(1e18);
  }
}


test_number_limit();
