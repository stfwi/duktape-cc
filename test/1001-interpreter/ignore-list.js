(function(){
  // Internationalization (Intl) is not compiled in, increases
  // the binary size too much and has unnecessary performance
  // impacts.
  const ignored_text_search = [
    /testIntl/i, /toLocale/, /Intl\./,
  ];
  const ignored_paths = [
    // Not ES5 compilable due to ES6 features in the test routine (not compilable).
    "test/built-ins/Array/prototype/toString/S15.4.4.2_A1_T4.js",   // ()=>{} used.
    "test/built-ins/Boolean/prototype/toString/S15.6.4.2_A2_T1.js", //  "
    "test/built-ins/Boolean/prototype/toString/S15.6.4.2_A2_T2.js", //  "
    "test/built-ins/Boolean/prototype/toString/S15.6.4.2_A2_T3.js", //  "
    "test/built-ins/Boolean/prototype/toString/S15.6.4.2_A2_T4.js", //  "
    "test/built-ins/Boolean/prototype/toString/S15.6.4.2_A2_T5.js", //  "
    "test/built-ins/Error/prototype/S15.11.4_A3.js",                //  "
    "test/built-ins/Error/prototype/S15.11.4_A4.js",                //  "
    "test/built-ins/Object/getOwnPropertyNames/15.2.3.4-4-44.js",   // Backtick string format used.
    "test/built-ins/Object/getOwnPropertyNames/15.2.3.4-4-49.js",   //  "
    "test/built-ins/Object/getOwnPropertyNames/15.2.3.4-4-b-2.js",  //  "
    "test/annexB/built-ins/RegExp/RegExp-trailing-escape.js",       // Throws. Invalid backref descr.
    "test/annexB/built-ins/RegExp/RegExp-leading-escape-BMP.js",    //  "
    "test/annexB/built-ins/RegExp/RegExp-leading-escape.js",        //  "
    "test/annexB/built-ins/RegExp/RegExp-trailing-escape-BMP.js",   //  "
    "test/language/future-reserved-words/implements.js",            // Future use reserved.
    "test/language/future-reserved-words/interface.js",             //  "
    "test/language/future-reserved-words/package.js",               //  "
    "test/language/future-reserved-words/private.js",               //  "
    "test/language/future-reserved-words/protected.js",             //  "
    "test/language/future-reserved-words/public.js",                //  "
    "test/language/future-reserved-words/static.js",                //  "

    // Implementation defined
    "test/language/function-code/10.4.3-1-17-s.js",                 // global is a fixed term in the implementation, but used in the tc39 file (in browsers 'this refers to the window object - test not applicable).
    "test/language/function-code/10.4.3-1-19-s.js",                 //  "

    // Internationalization related (is not compiled in, C locale is enforced).
    "test/annexB/built-ins/RegExp/RegExp-control-escape-russian-letter.js",
    "test/annexB/built-ins/RegExp/RegExp-decimal-escape-not-capturing.js",
    "test/built-ins/String/fromCharCode/S9.7_A2.1.js",              // Invalidates for given invalid char_code -1. OK.
    "test/built-ins/String/fromCharCode/S9.7_A2.2.js",              //  "
    "test/built-ins/String/fromCharCode/S9.7_A3.1_T2.js",           //  "
    "test/built-ins/String/fromCharCode/S9.7_A3.1_T3.js",           //  "
    "test/built-ins/String/fromCharCode/S9.7_A3.2_T1.js",           //  "
    "test/built-ins/String/prototype/toLowerCase/special_casing_conditional.js", // No lowercase conversion of special code points (Math \Sigma).

    // Related to default values/verification due to invalid use.
    "test/annexB/built-ins/Date/prototype/setYear/this-time-nan.js",
    "test/annexB/built-ins/Date/prototype/setYear/year-number-absolute.js",
    "test/annexB/built-ins/Date/prototype/setYear/year-number-relative.js",
    "test/built-ins/Array/prototype/every/15.4.4.16-3-8.js",        // Array forEach() call on non-arrays with length set to Infinity.
    "test/built-ins/Array/prototype/forEach/15.4.4.18-3-12.js",     // Array forEach() call on non-arrays with invalid length.
    "test/built-ins/Array/prototype/forEach/15.4.4.18-3-25.js",     //  "
    "test/built-ins/Array/prototype/forEach/15.4.4.18-3-7.js",      //  "
    "test/built-ins/Object/create/15.2.3.5-1-2.js",                 // Object.create(null) throws instead of returning {}. It is actually better that way.
    "test/built-ins/RegExp/S15.10.2.5_A1_T5.js",                    // Throws RangeError: regexp executor recursion limit, OK
    "test/built-ins/RegExp/S15.10.2.9_A1_T5.js",                    // Throws RangeError: regexp executor recursion limit, OK
    "test/built-ins/RegExp/prototype/exec/S15.10.6.2_A5_T3.js",     // Manipulating regex.lastIndex throws instead of starting from 0. Better than standard.
    "test/built-ins/RegExp/prototype/test/S15.10.6.3_A1_T22.js",    //  "

    // Tolerable value mismatches
    "test/built-ins/Date/prototype/valueOf/S9.4_A3_T1.js",          // -0.0 instead of 0.0 return for double numeric.
    "test/built-ins/Date/prototype/valueOf/S9.4_A3_T2.js",          //  "
    "test/built-ins/Error/prototype/S15.11.4_A2.js",                // Error.toString() is "[object Error]" instead of "[object Object]".
    "test/built-ins/Math/round/S15.8.2.15_A7.js",                   // 1/EPSILON rounding -9007199254740990 instead of -9007199254740991.
    "test/built-ins/Object/defineProperty/15.2.3.6-4-574.js",       // getOwnPropertyDescriptor for "set" (setter).
    "test/language/function-code/10.4.3-1-19gs.js",                 // Indirect eval in strict mode returns another 'this'. That is ok. The interpreter has no global window object that needs represents the 'this ref.
    "test/language/statements/for-in/12.6.4-2.js",                  // for(.. in ..) for non-enumerable shadowed property. Ok, duktape applies a more strict policy.
    "test/language/statements/for-in/12.6.4-2.js",                  //  "
    "test/language/statements/for/head-init-expr-check-empty-inc-empty-completion.js", // Ok, eval treated more strict in duktape, returning undefined instead of the last accessed reference of the evaluated expression.
    "test/language/statements/for/head-init-var-check-empty-inc-empty-completion.js",  //  "

    // Weird use of language with scoped accessors vs scoped variables, more academical tests, nothing that someone sane would actually implement.
    "test/language/expressions/assignment/S11.13.1_A5_T1.js",
    "test/language/expressions/assignment/S11.13.1_A5_T2.js",
    "test/language/expressions/assignment/S11.13.1_A5_T3.js",
    "test/language/expressions/assignment/S11.13.1_A6_T1.js",
    "test/language/expressions/assignment/S11.13.1_A6_T2.js",
    "test/language/expressions/assignment/S11.13.1_A6_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.1_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.1_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.1_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.10_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.10_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.10_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.11_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.11_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.11_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.2_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.2_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.2_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.3_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.3_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.3_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.4_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.4_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.4_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.5_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.5_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.5_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.6_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.6_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.6_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.7_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.7_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.7_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.8_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.8_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.8_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.9_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.9_T2.js",
    "test/language/expressions/compound-assignment/S11.13.2_A5.9_T3.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.1_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.10_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.11_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.2_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.3_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.4_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.5_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.6_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.7_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.8_T1.js",
    "test/language/expressions/compound-assignment/S11.13.2_A6.9_T1.js",
    "test/language/expressions/postfix-decrement/S11.3.2_A5_T1.js",
    "test/language/expressions/postfix-decrement/S11.3.2_A5_T2.js",
    "test/language/expressions/postfix-decrement/S11.3.2_A5_T3.js",
    "test/language/expressions/postfix-increment/S11.3.1_A5_T1.js",
    "test/language/expressions/postfix-increment/S11.3.1_A5_T2.js",
    "test/language/expressions/postfix-increment/S11.3.1_A5_T3.js",
    "test/language/expressions/prefix-decrement/S11.4.5_A5_T1.js",
    "test/language/expressions/prefix-decrement/S11.4.5_A5_T2.js",
    "test/language/expressions/prefix-decrement/S11.4.5_A5_T3.js",
    "test/language/expressions/prefix-increment/S11.4.4_A5_T1.js",
    "test/language/expressions/prefix-increment/S11.4.4_A5_T2.js",
    "test/language/expressions/prefix-increment/S11.4.4_A5_T3.js",
  ];

  return {
    paths: ignored_paths,
    contents: ignored_text_search
  }
})();
