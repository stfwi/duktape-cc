//
// Number.fromHex##()/Number.ToHex##() extensions.
//

function check_cmp(val, hex, to) {
  if((val>>>0) !== (to>>>0)) {
    test_fail("  "+val+" -> '"+hex+"' -> "+ to);
  } else {
    test_pass("  "+val+" -> '"+hex+"' -> "+ to);
  }
};

function check_with(bits, val) {
  bits = ""+bits;
  test_note("------------------------------------------------------------");
  test_note("With val=" + val + ", bits="+bits);

  const signed_from = eval("Number.fromHexS"+bits);
  const signed_from_le = eval("Number.fromHexS"+bits+"LE");
  const signed_from_be = eval("Number.fromHexS"+bits+"BE");
  const signed_to = eval("Number.toHexS"+bits);
  const signed_to_le = eval("Number.toHexS"+bits+"LE");
  const signed_to_be = eval("Number.toHexS"+bits+"BE");

  const hex_s = signed_to(val);
  const hex_s_le = signed_to_le(val);
  const hex_s_be = signed_to_be(val);
  const val_s = signed_from(hex_s);
  const val_s_le = signed_from_le(hex_s_le);
  const val_s_be = signed_from_be(hex_s_be);

  test_note( " -toHexS"+bits+"=" + hex_s );
  test_note( " -fromHexS"+bits+"=" + val_s );
  check_cmp(val, hex_s, val_s);

  test_note( " -toHexS"+bits+"LE=" + hex_s_le );
  test_note( " -fromHexS"+bits+"LE=" + val_s_le );
  check_cmp(val, hex_s_le, val_s_le);

  test_note( " -toHexS"+bits+"BE=" + hex_s_be );
  test_note( " -fromHexS"+bits+"BE=" + val_s_be );
  check_cmp(val, hex_s_be, val_s_be);

  if(val < 0) return;

  const unsigned_from = eval("Number.fromHexU"+bits);
  const unsigned_from_le = eval("Number.fromHexU"+bits+"LE");
  const unsigned_from_be = eval("Number.fromHexU"+bits+"BE");
  const unsigned_to = eval("Number.toHexU"+bits);
  const unsigned_to_le = eval("Number.toHexU"+bits+"LE");
  const unsigned_to_be = eval("Number.toHexU"+bits+"BE");

  const hex_u = unsigned_to(val);
  const hex_u_le = unsigned_to_le(val);
  const hex_u_be = unsigned_to_be(val);
  const val_u = unsigned_from(hex_u);
  const val_u_le = unsigned_from_le(hex_u_le);
  const val_u_be = unsigned_from_be(hex_u_be);

  test_note( " -toHexU"+bits+"=" + hex_u );
  test_note( " -fromHexU"+bits+"=" + val_u );
  check_cmp(val, hex_u, val_u);

  test_note( " -toHexU"+bits+"LE=" + hex_u_le );
  test_note( " -fromHexU"+bits+"LE=" + val_u_le );
  check_cmp(val, hex_u_le, val_u_le);

  test_note( " -toHexU"+bits+"BE=" + hex_u_be );
  test_note( " -fromHexU"+bits+"BE=" + val_u_be );
  check_cmp(val, hex_u_be, val_u_be);
}

function check_eval_except(term) {
  try {
    eval(term);
    test_fail("Exception was expected for '"+term+"'");
  } catch(ex) {
    test_pass("Expected exception for '"+term+"': " + ex.message);
  }
}

function check_eval_noexcept(term) {
  try {
    eval(term);
  } catch(ex) {
    test_fail("Unexpected exception for '"+term+"'");
  }
}

test_note( "Number.machineEndianess() ==" + Number.machineEndianess() );
test_expect( Number.machineEndianess()=="big" || Number.machineEndianess()=="little" || Number.machineEndianess()=="middle" );

check_eval_except('Number.fromHexS32("0123WXYZ")');
check_eval_except('Number.fromHexS32("")');
check_eval_except('Number.fromHexS32()');
check_eval_except('Number.toHexU32("123")');
check_eval_except('Number.toHexU32(-123)');
check_eval_except('Number.toHexU16(-123)');
check_eval_except('Number.toHexU8(-123)');

check_eval_noexcept('check_with(32,  0)');
check_eval_noexcept('check_with(16,  0)');
check_eval_noexcept('check_with( 8,  0)');
check_eval_noexcept('check_with(32,  1)');
check_eval_noexcept('check_with(16,  1)');
check_eval_noexcept('check_with( 8,  1)');
check_eval_noexcept('check_with(32, -1)');
check_eval_noexcept('check_with(16, -1)');
check_eval_noexcept('check_with( 8, -1)');

// @todo: fuzzing
