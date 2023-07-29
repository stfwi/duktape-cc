// (@todo: Extend container op tests, fuzz types).

/**
 * Number.prototype.limit()
 */
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

  max_iterations = max_iterations || 1000;
  timeout_s = timeout_s || 1000;
  const t0 = sys.clock() + timeout_s;
  while(--max_iterations > 0) {
    if(sys.clock() > t0) break;
    check_rnd(1e5);
    check_rnd(1e9);
    check_rnd(1e18);
  }
}

/**
 * - Object.each(fn) / Object.forEach(fn) for arrays AND enumerable own object keys.
 * - Object.prototype.any(fn), Object.prototype.none(fn), Object.prototype.all(fn).
 */
function test_container_operations()
{
  function test_foreach()
  {
    if(Object.prototype.each === undefined) {
      test_note("Skipping Object.prototype.each test, feature not compiled.");
      return;
    }
    const check_object = function(data) {
      const indexed = Array.isArray(data);
      const ref = [];
      if(!indexed) {
        // Enumerable properties only.
        Object.keys(data).filter(function(it){ ref.push([it, data[it]]); });
      } else {
        // Explicit old-school straight array indexing as reference.
        for(var i=0; i<data.length; ++i) { ref.push([i, data[i]]); }
      }
      test_note("data reference length = " + ref.length);
      const res_each = [];
      data.each(function(val, key, me) {
        if(me !== data) test_fail("Unexpected this reference mismatch.");
        res_each.push([key, val]);
      });
      test_note("Object.each length = " + ref.length);
      const res_foreach = [];
      data.forEach(function(val, key, me) {
        if(me !== data) test_fail("Unexpected this reference mismatch.");
        res_foreach.push([key, val]);
      });
      test_note("Checking for identical value contents ...");
      test_note("Object.foreach length = " + ref.length);
      test_note("ref     = " + JSON.stringify(ref));
      test_note("each    = " + JSON.stringify(res_each));
      test_note("foreach = " + JSON.stringify(res_foreach));
      test_expect(ref.length == res_each.length);
      test_expect(ref.length == res_foreach.length);
      test_expect(JSON.stringify(ref) == JSON.stringify(res_each));
      test_expect(JSON.stringify(ref) == JSON.stringify(res_foreach));

      // Element check.
      for(var i=0; i<ref.length; ++i) {
        if(res_each[i][0] !== ref[i][0]) {
          test_fail("Unexpected key mismatch for res_each element " + i);
          continue;
        } else if(res_foreach[i][0] !== ref[i][0]) {
          test_fail("Unexpected key mismatch for res_foreach element " + i);
          continue;
        }
        const v1 = ref[i][1];
        const v2 = res_each[i][1];
        const v3 = res_foreach[i][1];
        if(v1 !== v2) {
          test_fail("Type mismatch of primitive for res_each element " + i + "(="+JSON.stringify(v1)+"!=="+JSON.stringify(v2)+")");
        } else if(v1 !== v3) {
          test_fail("Type mismatch of primitive for res_foreach element " + i + "(="+JSON.stringify(v1)+"!=="+JSON.stringify(v3)+")");
        } else{
          test_pass();
        }
      }
    }

    // Known value checks.
    check_object([]);
    check_object({});
    check_object([1,2,3,4,5]);
    check_object([1,"-",{a:1}]);
    check_object({a:1,b:2,c:3,d:4,e:5,f:[{a:"-",b:[1.0],}]});
    check_object([function(){return 0}, function(){return 1}]); // Note that JSON will convert functions to null, so the serialized string shall still match as well.
    // Null is an object, interpreter shall throw.
    try {
      null.each(function(){});
      test_fail("Expected en exception for null.each()");
    } catch(ex) {
      test_pass("Ok, null.each() did throw");
    }
    try {
      null.forEach(function(){});
      test_fail("Expected en exception for null.forEach()");
    } catch(ex) {
      test_pass("Ok, null.forEach() did throw");
    }

    // Protoype properties check, these shall not be included,
    // as with Object.keys().
    const derived = {a:1,b:2,c:3,d:4,e:5,f:[{a:"-",b:[1.0]}]};
    const base = {x:1, y:2, z:3};
    Object.setPrototypeOf(derived, base);
    test_expect((derived.x == 1) && (derived.y == 2) && (derived.z == 3));
    check_object(derived);
  }

  function test_any_none_all()
  {
    if((Object.prototype.any === undefined) || (Object.prototype.all === undefined) || (Object.prototype.none === undefined)) {
      test_note("Skipping Object.prototype.any/all/none test, feature not compiled.");
    }

    // Array
    test_expect(!([].any(function(v){return !!v})) );
    test_expect( [1,2,3,4].any(function(v){return v==3}) );
    test_expect( [1,2,3,4].any(function(v){return v!=3}) );
    test_expect(![1,2,3,4].any(function(v){return v==0}) );

    test_expect(!([].some(function(v){return !!v})) );
    test_expect( [1,2,3,4].some(function(v){return v==3}) );
    test_expect( [1,2,3,4].some(function(v){return v!=3}) );
    test_expect(![1,2,3,4].some(function(v){return v==0}) );

    test_expect( ([].none(function(v){return !!v})) );
    test_expect( [1,2,3,4].none(function(v){return v==0}) );
    test_expect(![1,2,3,4].none(function(v){return v==1}) );
    test_expect(![1,2,3,4].none(function(v){return v!=1}) );

    test_expect( ([].all(function(v){return !!v})) );
    test_expect( [1,2,3,4].all(function(v){return v<5}) );
    test_expect(![1,2,3,4].all(function(v){return v<4}) );
    test_expect(![1,2,3,4].all(function(v){return v==1}) );

    test_expect( ([].every(function(v){return !!v})) );
    test_expect( [1,2,3,4].every(function(v){return v<5}) );
    test_expect(![1,2,3,4].every(function(v){return v<4}) );
    test_expect(![1,2,3,4].every(function(v){return v==1}) );

    // General plain object
    test_expect(!({}.any(function(v){return !!v})) );
    test_expect( {a:1,b:2,c:3,d:4}.any(function(v){return v==3}) );
    test_expect( {a:1,b:2,c:3,d:4}.any(function(v){return v!=3}) );
    test_expect(!{a:1,b:2,c:3,d:4}.any(function(v){return v==0}) );

    test_expect(!({}.some(function(v){return !!v})) );
    test_expect( {a:1,b:2,c:3,d:4}.some(function(v){return v==3}) );
    test_expect( {a:1,b:2,c:3,d:4}.some(function(v){return v!=3}) );
    test_expect(!{a:1,b:2,c:3,d:4}.some(function(v){return v==0}) );

    test_expect( ({}.none(function(v){return !!v})) );
    test_expect( {a:1,b:2,c:3,d:4}.none(function(v){return v==0}) );
    test_expect(!{a:1,b:2,c:3,d:4}.none(function(v){return v==1}) );
    test_expect(!{a:1,b:2,c:3,d:4}.none(function(v){return v!=1}) );

    test_expect( ({}.all(function(v){return !!v})) );
    test_expect( {a:1,b:2,c:3,d:4}.all(function(v){return v<5}) );
    test_expect(!{a:1,b:2,c:3,d:4}.all(function(v){return v<4}) );
    test_expect(!{a:1,b:2,c:3,d:4}.all(function(v){return v==1}) );

    test_expect( ({}.every(function(v){return !!v})) );
    test_expect( {a:1,b:2,c:3,d:4}.every(function(v){return v<5}) );
    test_expect(!{a:1,b:2,c:3,d:4}.every(function(v){return v<4}) );
    test_expect(!{a:1,b:2,c:3,d:4}.every(function(v){return v==1}) );

    // Key/this
    const obj = {a:1,b:2,c:3,d:4};
    test_expect( obj.every(function(v,k,o) {return (v>=1) && (v<=4) && ("abcd".indexOf(k)>=0) && (o===obj)}));
    test_expect( obj.all(function(v,k,o) {return (v>=1) && (v<=4) && ("abcd".indexOf(k)>=0) && (o===obj)}));
    test_expect( obj.any(function(v,k,o) {return (v>=1) && (v<=4) && ("abcd".indexOf(k)>=0) && (o===obj)}));
    test_expect( obj.some(function(v,k,o) {return (v>=1) && (v<=4) && ("abcd".indexOf(k)>=0) && (o===obj)}));
    test_expect( obj.none(function(v,k,o) {return (v<1) && (v>4) && ("abcd".indexOf(k)<0) && (o!==obj)}));
  }

  test_foreach();
  test_any_none_all();
}

/**
 * - Object.each(fn) / Object.forEach(fn) for arrays AND enumerable own object keys.
 * - Object.prototype.any(fn), Object.prototype.none(fn), Object.prototype.all(fn).
 */
function test_string_trim()
{
  if(String.prototype.trim === undefined) {
    test_note("Skipping String.prototype.trim test, feature not compiled.");
    return;
  }

  test_expect(" abc ".trim() == "abc" );
  test_expect(" abc".trim() == "abc" );
  test_expect("abc ".trim() == "abc" );
  test_expect("abc".trim() == "abc" );
  test_expect("".trim() == "" );
  test_expect("äöü".trim() == "äöü" );
}

function test_math_linear_regression()
{
  // Against known values.
  const check_data = function(xyso) {
    const round3 = function(x) { return Math.round(x*1e3) * 1e-3 };
    const coeffs = Math.linfit(xyso.x, xyso.y);
    const slope  = round3(coeffs.slope);
    const offset = round3(coeffs.offset);
    test_note("Coeffs = " + JSON.stringify(coeffs));
    test_note("Slope = " + slope);
    test_note("Offset = " + offset);
    test_expect( Math.abs(offset-round3(xyso.offset)) < 1e-9 );
    test_expect( Math.abs(slope-round3(xyso.slope)) < 1e-9 );
  }

  check_data({
    x: [  0, 2,  5,  7 ],
    y: [ -1, 5, 12, 20 ],
    offset:-1.138, slope:2.897
  });
  check_data({
    "offset":-0.10934782608695647,
    "slope":0.13102766798418967,
    "x":[0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2,2.1,2.2],
    "y":[0.26,-0.83,2.08,-0.01,-1.58,-1.1,0.53,0.96,-0.53,-0.58,1.07,-0.99,-0.34,-1.51,0.6,1.53,0.16,0.45,-0.31,0.99,-0.47,0.59,-0.17]
  });
  check_data({
    "slope":0.21161764705882364,
    "offset":-0.4055882352941179,
    "x":[0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1,1.1,1.2,1.3,1.4,1.5],
    "y":[0.32,0.03,0.17,-1.44,-1.32,-0.61,0.47,1.35,-1.14,-0.89,-0.45,-1.14,-1.03,0.43,0.98,0.32]
  });

  // Check against octave
  for(var i=0; i<10; ++i) {
    var data = sys.shell("octave-cl --norc --no-gui --silent linfit.m");
    if(!data) { test_note("Skipping linfit checks against octave (not installed or not in the PATH, or not executable)."); break; }
    data = data.replace(/[\n\r\s]+$/,"");
    test_note("Octave data:" + data);
    check_data(JSON.parse(data));
  }
}

test_number_limit();
test_container_operations();
test_string_trim();
test_math_linear_regression();
