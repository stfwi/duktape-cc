
# TC39 test232 harness for duktape-cc.

  The sections in this file contain the reference harness of the TC39 test232,
  modified to enable testing the interpreter:

    - Source: https://github.com/tc39/test262
    - License: https://github.com/tc39/test262/blob/main/LICENSE

  To run the TC39 tests, clone the `test262` somewhere and pass the full path to the
  Makefile:

    $ make test TEST=1001 TC39_262_ROOT=full/path/to/test262

  Only the ES5 applicable tests will be executed.

### sta

  // Copyright (c) 2012 Ecma International.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Provides both:

      - An error class to avoid false positives when testing for thrown exceptions
      - A function to explicitly throw an exception using the Test262Error class
  defines: [Test262Error, $DONOTEVALUATE]
  ---*/


  function Test262Error(message) {
    this.message = message || "";
  }

  Test262Error.prototype.toString = function () {
    return "Test262Error: " + this.message;
  };

  Test262Error.thrower = function(message) {
    throw new Test262Error(message);
  };

  function $DONOTEVALUATE() {
    throw "Test262: This statement should not be evaluated.";
  }

### asserts

  // Copyright (C) 2017 Ecma International.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Collection of assertion functions used throughout test262
  defines: [assert]
  ---*/


  function assert(mustBeTrue, message) {
    if (mustBeTrue === true) {
      return;
    }

    if (message === undefined) {
      message = 'Expected true but got ' + assert._toString(mustBeTrue);
    }
    throw new Test262Error(message);
  }

  assert._isSameValue = function (a, b) {
    if (a === b) {
      // Handle +/-0 vs. -/+0
      return a !== 0 || 1 / a === 1 / b;
    }

    // Handle NaN vs. NaN
    return a !== a && b !== b;
  };

  assert.sameValue = function (actual, expected, message) {
    try {
      if (assert._isSameValue(actual, expected)) {
        return;
      }
    } catch (error) {
      throw new Test262Error(message + ' (_isSameValue operation threw) ' + error);
      return;
    }

    if (message === undefined) {
      message = '';
    } else {
      message += ' ';
    }

    message += 'Expected SameValue(«' + assert._toString(actual) + '», «' + assert._toString(expected) + '») to be true';

    throw new Test262Error(message);
  };

  assert.notSameValue = function (actual, unexpected, message) {
    if (!assert._isSameValue(actual, unexpected)) {
      return;
    }

    if (message === undefined) {
      message = '';
    } else {
      message += ' ';
    }

    message += 'Expected SameValue(«' + assert._toString(actual) + '», «' + assert._toString(unexpected) + '») to be false';

    throw new Test262Error(message);
  };

  assert.throws = function (expectedErrorConstructor, func, message) {
    var expectedName, actualName;
    if (typeof func !== "function") {
      throw new Test262Error('assert.throws requires two arguments: the error constructor ' +
        'and a function to run');
      return;
    }
    if (message === undefined) {
      message = '';
    } else {
      message += ' ';
    }

    try {
      func();
    } catch (thrown) {
      if (typeof thrown !== 'object' || thrown === null) {
        message += 'Thrown value was not an object!';
        throw new Test262Error(message);
      } else if (thrown.constructor !== expectedErrorConstructor) {
        expectedName = expectedErrorConstructor.name;
        actualName = thrown.constructor.name;
        if (expectedName === actualName) {
          message += 'Expected a ' + expectedName + ' but got a different error constructor with the same name';
        } else {
          message += 'Expected a ' + expectedName + ' but got a ' + actualName;
        }
        throw new Test262Error(message);
      }
      return;
    }

    message += 'Expected a ' + expectedErrorConstructor.name + ' to be thrown but no exception was thrown at all';
    throw new Test262Error(message);
  };

  assert._toString = function (value) {
    try {
      if (value === 0 && 1 / value === -Infinity) {
        return '-0';
      }

      return String(value);
    } catch (err) {
      if (err.name === 'TypeError') {
        return Object.prototype.toString.call(value);
      }

      throw err;
    }
  };

### deepEqual

  // Copyright 2019 Ron Buckton. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: >
    Compare two values structurally
  defines: [assert.deepEqual]
  ---*/

  assert.deepEqual = function(actual, expected, message) {
    var format = assert.deepEqual.format;
    assert(
      assert.deepEqual._compare(actual, expected),
      `Expected ${format(actual)} to be structurally equal to ${format(expected)}. ${(message || '')}`
    );
  };

  assert.deepEqual.format = function(value, seen) {
    switch (typeof value) {
      case 'string':
        return typeof JSON !== "undefined" ? JSON.stringify(value) : `"${value}"`;
      case 'number':
      case 'boolean':
      case 'symbol':
      case 'bigint':
        return value.toString();
      case 'undefined':
        return 'undefined';
      case 'function':
        return `[Function${value.name ? `: ${value.name}` : ''}]`;
      case 'object':
        if (value === null) return 'null';
        if (value instanceof Date) return `Date "${value.toISOString()}"`;
        if (value instanceof RegExp) return value.toString();
        if (!seen) {
          seen = {
            counter: 0,
            map: new Map()
          };
        }

        let usage = seen.map.get(value);
        if (usage) {
          usage.used = true;
          return `[Ref: #${usage.id}]`;
        }

        usage = { id: ++seen.counter, used: false };
        seen.map.set(value, usage);

        if (typeof Set !== "undefined" && value instanceof Set) {
          return `Set {${Array.from(value).map(value => assert.deepEqual.format(value, seen)).join(', ')}}${usage.used ? ` as #${usage.id}` : ''}`;
        }
        if (typeof Map !== "undefined" && value instanceof Map) {
          return `Map {${Array.from(value).map(pair => `${assert.deepEqual.format(pair[0], seen)} => ${assert.deepEqual.format(pair[1], seen)}}`).join(', ')}}${usage.used ? ` as #${usage.id}` : ''}`;
        }
        if (Array.isArray ? Array.isArray(value) : value instanceof Array) {
          return `[${value.map(value => assert.deepEqual.format(value, seen)).join(', ')}]${usage.used ? ` as #${usage.id}` : ''}`;
        }
        let tag = Symbol.toStringTag in value ? value[Symbol.toStringTag] : 'Object';
        if (tag === 'Object' && Object.getPrototypeOf(value) === null) {
          tag = '[Object: null prototype]';
        }
        return `${tag ? `${tag} ` : ''}{ ${Object.keys(value).map(key => `${key.toString()}: ${assert.deepEqual.format(value[key], seen)}`).join(', ')} }${usage.used ? ` as #${usage.id}` : ''}`;
      default:
        return typeof value;
    }
  };

  assert.deepEqual._compare = (function () {
    var EQUAL = 1;
    var NOT_EQUAL = -1;
    var UNKNOWN = 0;

    function deepEqual(a, b) {
      return compareEquality(a, b) === EQUAL;
    }

    function compareEquality(a, b, cache) {
      return compareIf(a, b, isOptional, compareOptionality)
        || compareIf(a, b, isPrimitiveEquatable, comparePrimitiveEquality)
        || compareIf(a, b, isObjectEquatable, compareObjectEquality, cache)
        || NOT_EQUAL;
    }

    function compareIf(a, b, test, compare, cache) {
      return !test(a)
        ? !test(b) ? UNKNOWN : NOT_EQUAL
        : !test(b) ? NOT_EQUAL : cacheComparison(a, b, compare, cache);
    }

    function tryCompareStrictEquality(a, b) {
      return a === b ? EQUAL : UNKNOWN;
    }

    function tryCompareTypeOfEquality(a, b) {
      return typeof a !== typeof b ? NOT_EQUAL : UNKNOWN;
    }

    function tryCompareToStringTagEquality(a, b) {
      var aTag = Symbol.toStringTag in a ? a[Symbol.toStringTag] : undefined;
      var bTag = Symbol.toStringTag in b ? b[Symbol.toStringTag] : undefined;
      return aTag !== bTag ? NOT_EQUAL : UNKNOWN;
    }

    function isOptional(value) {
      return value === undefined
        || value === null;
    }

    function compareOptionality(a, b) {
      return tryCompareStrictEquality(a, b)
        || NOT_EQUAL;
    }

    function isPrimitiveEquatable(value) {
      switch (typeof value) {
        case 'string':
        case 'number':
        case 'bigint':
        case 'boolean':
        case 'symbol':
          return true;
        default:
          return isBoxed(value);
      }
    }

    function comparePrimitiveEquality(a, b) {
      if (isBoxed(a)) a = a.valueOf();
      if (isBoxed(b)) b = b.valueOf();
      return tryCompareStrictEquality(a, b)
        || tryCompareTypeOfEquality(a, b)
        || compareIf(a, b, isNaNEquatable, compareNaNEquality)
        || NOT_EQUAL;
    }

    function isNaNEquatable(value) {
      return typeof value === 'number';
    }

    function compareNaNEquality(a, b) {
      return isNaN(a) && isNaN(b) ? EQUAL : NOT_EQUAL;
    }

    function isObjectEquatable(value) {
      return typeof value === 'object';
    }

    function compareObjectEquality(a, b, cache) {
      if (!cache) cache = new Map();
      return getCache(cache, a, b)
        || setCache(cache, a, b, EQUAL) // consider equal for now
        || cacheComparison(a, b, tryCompareStrictEquality, cache)
        || cacheComparison(a, b, tryCompareToStringTagEquality, cache)
        || compareIf(a, b, isValueOfEquatable, compareValueOfEquality)
        || compareIf(a, b, isToStringEquatable, compareToStringEquality)
        || compareIf(a, b, isArrayLikeEquatable, compareArrayLikeEquality, cache)
        || compareIf(a, b, isStructurallyEquatable, compareStructuralEquality, cache)
        || compareIf(a, b, isIterableEquatable, compareIterableEquality, cache)
        || cacheComparison(a, b, fail, cache);
    }

    function isBoxed(value) {
      return value instanceof String
        || value instanceof Number
        || value instanceof Boolean
        || typeof Symbol === 'function' && value instanceof Symbol
        || typeof BigInt === 'function' && value instanceof BigInt;
    }

    function isValueOfEquatable(value) {
      return value instanceof Date;
    }

    function compareValueOfEquality(a, b) {
      return compareIf(a.valueOf(), b.valueOf(), isPrimitiveEquatable, comparePrimitiveEquality)
        || NOT_EQUAL;
    }

    function isToStringEquatable(value) {
      return value instanceof RegExp;
    }

    function compareToStringEquality(a, b) {
      return compareIf(a.toString(), b.toString(), isPrimitiveEquatable, comparePrimitiveEquality)
        || NOT_EQUAL;
    }

    function isArrayLikeEquatable(value) {
      return (Array.isArray ? Array.isArray(value) : value instanceof Array)
        || (typeof Uint8Array === 'function' && value instanceof Uint8Array)
        || (typeof Uint8ClampedArray === 'function' && value instanceof Uint8ClampedArray)
        || (typeof Uint16Array === 'function' && value instanceof Uint16Array)
        || (typeof Uint32Array === 'function' && value instanceof Uint32Array)
        || (typeof Int8Array === 'function' && value instanceof Int8Array)
        || (typeof Int16Array === 'function' && value instanceof Int16Array)
        || (typeof Int32Array === 'function' && value instanceof Int32Array)
        || (typeof Float32Array === 'function' && value instanceof Float32Array)
        || (typeof Float64Array === 'function' && value instanceof Float64Array)
        || (typeof BigUint64Array === 'function' && value instanceof BigUint64Array)
        || (typeof BigInt64Array === 'function' && value instanceof BigInt64Array);
    }

    function compareArrayLikeEquality(a, b, cache) {
      if (a.length !== b.length) return NOT_EQUAL;
      for (var i = 0; i < a.length; i++) {
        if (compareEquality(a[i], b[i], cache) === NOT_EQUAL) {
          return NOT_EQUAL;
        }
      }
      return EQUAL;
    }

    function isStructurallyEquatable(value) {
      return !(typeof Promise === 'function' && value instanceof Promise // only comparable by reference
        || typeof WeakMap === 'function' && value instanceof WeakMap // only comparable by reference
        || typeof WeakSet === 'function' && value instanceof WeakSet // only comparable by reference
        || typeof Map === 'function' && value instanceof Map // comparable via @@iterator
        || typeof Set === 'function' && value instanceof Set); // comparable via @@iterator
    }

    function compareStructuralEquality(a, b, cache) {
      var aKeys = [];
      for (var key in a) aKeys.push(key);

      var bKeys = [];
      for (var key in b) bKeys.push(key);

      if (aKeys.length !== bKeys.length) {
        return NOT_EQUAL;
      }

      aKeys.sort();
      bKeys.sort();

      for (var i = 0; i < aKeys.length; i++) {
        var aKey = aKeys[i];
        var bKey = bKeys[i];
        if (compareEquality(aKey, bKey, cache) === NOT_EQUAL) {
          return NOT_EQUAL;
        }
        if (compareEquality(a[aKey], b[bKey], cache) === NOT_EQUAL) {
          return NOT_EQUAL;
        }
      }

      return compareIf(a, b, isIterableEquatable, compareIterableEquality, cache)
        || EQUAL;
    }

    function isIterableEquatable(value) {
      return typeof Symbol === 'function'
        && typeof value[Symbol.iterator] === 'function';
    }

    function compareIteratorEquality(a, b, cache) {
      if (typeof Map === 'function' && a instanceof Map && b instanceof Map ||
        typeof Set === 'function' && a instanceof Set && b instanceof Set) {
        if (a.size !== b.size) return NOT_EQUAL; // exit early if we detect a difference in size
      }

      var ar, br;
      while (true) {
        ar = a.next();
        br = b.next();
        if (ar.done) {
          if (br.done) return EQUAL;
          if (b.return) b.return();
          return NOT_EQUAL;
        }
        if (br.done) {
          if (a.return) a.return();
          return NOT_EQUAL;
        }
        if (compareEquality(ar.value, br.value, cache) === NOT_EQUAL) {
          if (a.return) a.return();
          if (b.return) b.return();
          return NOT_EQUAL;
        }
      }
    }

    function compareIterableEquality(a, b, cache) {
      return compareIteratorEquality(a[Symbol.iterator](), b[Symbol.iterator](), cache);
    }

    function cacheComparison(a, b, compare, cache) {
      var result = compare(a, b, cache);
      if (cache && (result === EQUAL || result === NOT_EQUAL)) {
        setCache(cache, a, b, /** @type {EQUAL | NOT_EQUAL} */(result));
      }
      return result;
    }

    function fail() {
      return NOT_EQUAL;
    }

    function setCache(cache, left, right, result) {
      var otherCache;

      otherCache = cache.get(left);
      if (!otherCache) cache.set(left, otherCache = new Map());
      otherCache.set(right, result);

      otherCache = cache.get(right);
      if (!otherCache) cache.set(right, otherCache = new Map());
      otherCache.set(left, result);
    }

    function getCache(cache, left, right) {
      var otherCache;
      var result;

      otherCache = cache.get(left);
      result = otherCache && otherCache.get(right);
      if (result) return result;

      otherCache = cache.get(right);
      result = otherCache && otherCache.get(left);
      if (result) return result;

      return UNKNOWN;
    }

    return deepEqual;
  })();

### isConstructor

  // Copyright (C) 2017 André Bargull. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.

  /*---
  description: |
      Test if a given function is a constructor function.
  defines: [isConstructor]
  ---*/

  function isConstructor(f) {
      try {
          Reflect.construct(function(){}, [], f);
      } catch (e) {
          return false;
      }
      return true;
  }

### fnGlobalObject

  // Copyright (C) 2017 Ecma International.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Produce a reliable global object
  defines: [fnGlobalObject]
  ---*/

  var __globalObject = Function("return this;")();
  function fnGlobalObject() {
    return __globalObject;
  }

### tcoHelper

  // Copyright (C) 2016 the V8 project authors. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      This defines the number of consecutive recursive function calls that must be
      made in order to prove that stack frames are properly destroyed according to
      ES2015 tail call optimization semantics.
  defines: [$MAX_ITERATIONS]
  ---*/

  var $MAX_ITERATIONS = 100000;

### assertRelativeDateMs

  // Copyright (C) 2015 the V8 project authors. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Verify that the given date object's Number representation describes the
      correct number of milliseconds since the Unix epoch relative to the local
      time zone (as interpreted at the specified date).
  defines: [assertRelativeDateMs]
  ---*/

  /**
  * @param {Date} date
  * @param {Number} expectedMs
  */
  function assertRelativeDateMs(date, expectedMs) {
    var actualMs = date.valueOf();
    var localOffset = date.getTimezoneOffset() * 60000;

    if (actualMs - localOffset !== expectedMs) {
      throw new Test262Error(
        'Expected ' + date + ' to be ' + expectedMs +
        ' milliseconds from the Unix epoch'
      );
    }
  }

### compareArray

  // Copyright (C) 2017 Ecma International.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Compare the contents of two arrays
  defines: [compareArray]
  ---*/

  function compareArray(a, b) {
    if (b.length !== a.length) {
      return false;
    }

    for (var i = 0; i < a.length; i++) {
      if (!compareArray.isSameValue(b[i], a[i])) {
        return false;
      }
    }
    return true;
  }

  compareArray.isSameValue = function(a, b) {
    if (a === 0 && b === 0) return 1 / a === 1 / b;
    if (a !== a && b !== b) return true;

    return a === b;
  };

  compareArray.format = function(arrayLike) {
    return `[${[].map.call(arrayLike, String).join(', ')}]`;
  };

  assert.compareArray = function(actual, expected, message) {
    message  = message === undefined ? '' : message;

    if (typeof message === 'symbol') {
      message = message.toString();
    }

    assert(actual != null, `First argument shouldn't be nullish. ${message}`);
    assert(expected != null, `Second argument shouldn't be nullish. ${message}`);
    var format = compareArray.format;
    var result = compareArray(actual, expected);

    // The following prevents actual and expected from being iterated and evaluated
    // more than once unless absolutely necessary.
    if (!result) {
      assert(false, `Expected ${format(actual)} and ${format(expected)} to have the same contents. ${message}`);
    }
  };

### compareIterator

  // Copyright (C) 2018 Peter Wong.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: Compare the values of an iterator with an array of expected values
  defines: [assert.compareIterator]
  ---*/

  // Example:
  //
  //    function* numbers() {
  //      yield 1;
  //      yield 2;
  //      yield 3;
  //    }
  //
  //    assert.compareIterator(numbers(), [
  //      v => assert.sameValue(v, 1),
  //      v => assert.sameValue(v, 2),
  //      v => assert.sameValue(v, 3),
  //    ]);
  //
  assert.compareIterator = function(iter, validators, message) {
    message = message || '';

    var i, result;
    for (i = 0; i < validators.length; i++) {
      result = iter.next();
      assert(!result.done, 'Expected ' + i + ' values(s). Instead iterator only produced ' + (i - 1) + ' value(s). ' + message);
      validators[i](result.value);
    }

    result = iter.next();
    assert(result.done, 'Expected only ' + i + ' values(s). Instead iterator produced more. ' + message);
    assert.sameValue(result.value, undefined, 'Expected value of `undefined` when iterator completes. ' + message);
  }

### dateConstants

  // Copyright (C) 2009 the Sputnik authors.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Collection of date-centric values
  defines:
    - date_1899_end
    - date_1900_start
    - date_1969_end
    - date_1970_start
    - date_1999_end
    - date_2000_start
    - date_2099_end
    - date_2100_start
    - start_of_time
    - end_of_time
  ---*/

  var date_1899_end = -2208988800001;
  var date_1900_start = -2208988800000;
  var date_1969_end = -1;
  var date_1970_start = 0;
  var date_1999_end = 946684799999;
  var date_2000_start = 946684800000;
  var date_2099_end = 4102444799999;
  var date_2100_start = 4102444800000;

  var start_of_time = -8.64e15;
  var end_of_time = 8.64e15;

### nans

  // Copyright (C) 2016 the V8 project authors.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      A collection of NaN values produced from expressions that have been observed
      to create distinct bit representations on various platforms. These provide a
      weak basis for assertions regarding the consistent canonicalization of NaN
      values in Array buffers.
  defines: [NaNs]
  ---*/

  var NaNs = [
    NaN,
    Number.NaN,
    NaN * 0,
    0/0,
    Infinity/Infinity,
    -(0/0),
    Math.pow(-1, 0.5),
    -Math.pow(-1, 0.5),
    Number("Not-a-Number"),
  ];

### propertyHelper

  // Copyright (C) 2017 Ecma International.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Collection of functions used to safely verify the correctness of
      property descriptors.
  defines:
    - verifyProperty
    - verifyEqualTo # deprecated
    - verifyWritable # deprecated
    - verifyNotWritable # deprecated
    - verifyEnumerable # deprecated
    - verifyNotEnumerable # deprecated
    - verifyConfigurable # deprecated
    - verifyNotConfigurable # deprecated
  ---*/

  // @ts-check

  /**
  * @param {object} obj
  * @param {string|symbol} name
  * @param {PropertyDescriptor|undefined} desc
  * @param {object} [options]
  * @param {boolean} [options.restore]
  */
  function verifyProperty(obj, name, desc, options) {
    assert(
      arguments.length > 2,
      'verifyProperty should receive at least 3 arguments: obj, name, and descriptor'
    );

    var originalDesc = Object.getOwnPropertyDescriptor(obj, name);
    var nameStr = String(name);

    // Allows checking for undefined descriptor if it's explicitly given.
    if (desc === undefined) {
      assert.sameValue(
        originalDesc,
        undefined,
        "obj['" + nameStr + "'] descriptor should be undefined"
      );

      // desc and originalDesc are both undefined, problem solved;
      return true;
    }

    assert(
      Object.prototype.hasOwnProperty.call(obj, name),
      "obj should have an own property " + nameStr
    );

    assert.notSameValue(
      desc,
      null,
      "The desc argument should be an object or undefined, null"
    );

    assert.sameValue(
      typeof desc,
      "object",
      "The desc argument should be an object or undefined, " + String(desc)
    );

    var failures = [];

    if (Object.prototype.hasOwnProperty.call(desc, 'value')) {
      if (!isSameValue(desc.value, originalDesc.value)) {
        failures.push("descriptor value should be " + desc.value);
      }
    }

    if (Object.prototype.hasOwnProperty.call(desc, 'enumerable')) {
      if (desc.enumerable !== originalDesc.enumerable ||
          desc.enumerable !== isEnumerable(obj, name)) {
        failures.push('descriptor should ' + (desc.enumerable ? '' : 'not ') + 'be enumerable');
      }
    }

    if (Object.prototype.hasOwnProperty.call(desc, 'writable')) {
      if (desc.writable !== originalDesc.writable ||
          desc.writable !== isWritable(obj, name)) {
        failures.push('descriptor should ' + (desc.writable ? '' : 'not ') + 'be writable');
      }
    }

    if (Object.prototype.hasOwnProperty.call(desc, 'configurable')) {
      if (desc.configurable !== originalDesc.configurable ||
          desc.configurable !== isConfigurable(obj, name)) {
        failures.push('descriptor should ' + (desc.configurable ? '' : 'not ') + 'be configurable');
      }
    }

    assert(!failures.length, failures.join('; '));

    if (options && options.restore) {
      Object.defineProperty(obj, name, originalDesc);
    }

    return true;
  }

  function isConfigurable(obj, name) {
    var hasOwnProperty = Object.prototype.hasOwnProperty;
    try {
      delete obj[name];
    } catch (e) {
      if (!(e instanceof TypeError)) {
        throw new Test262Error("Expected TypeError, got " + e);
      }
    }
    return !hasOwnProperty.call(obj, name);
  }

  function isEnumerable(obj, name) {
    var stringCheck = false;

    if (typeof name === "string") {
      for (var x in obj) {
        if (x === name) {
          stringCheck = true;
          break;
        }
      }
    } else {
      // skip it if name is not string, works for Symbol names.
      stringCheck = true;
    }

    return stringCheck &&
      Object.prototype.hasOwnProperty.call(obj, name) &&
      Object.prototype.propertyIsEnumerable.call(obj, name);
  }

  function isSameValue(a, b) {
    if (a === 0 && b === 0) return 1 / a === 1 / b;
    if (a !== a && b !== b) return true;

    return a === b;
  }

  var __isArray = Array.isArray;
  function isWritable(obj, name, verifyProp, value) {
    var unlikelyValue = __isArray(obj) && name === "length" ?
      Math.pow(2, 32) - 1 :
      "unlikelyValue";
    var newValue = value || unlikelyValue;
    var hadValue = Object.prototype.hasOwnProperty.call(obj, name);
    var oldValue = obj[name];
    var writeSucceeded;

    try {
      obj[name] = newValue;
    } catch (e) {
      if (!(e instanceof TypeError)) {
        throw new Test262Error("Expected TypeError, got " + e);
      }
    }

    writeSucceeded = isSameValue(obj[verifyProp || name], newValue);

    // Revert the change only if it was successful (in other cases, reverting
    // is unnecessary and may trigger exceptions for certain property
    // configurations)
    if (writeSucceeded) {
      if (hadValue) {
        obj[name] = oldValue;
      } else {
        delete obj[name];
      }
    }

    return writeSucceeded;
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyEqualTo(obj, name, value) {
    if (!isSameValue(obj[name], value)) {
      throw new Test262Error("Expected obj[" + String(name) + "] to equal " + value +
            ", actually " + obj[name]);
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyWritable(obj, name, verifyProp, value) {
    if (!verifyProp) {
      assert(Object.getOwnPropertyDescriptor(obj, name).writable,
          "Expected obj[" + String(name) + "] to have writable:true.");
    }
    if (!isWritable(obj, name, verifyProp, value)) {
      throw new Test262Error("Expected obj[" + String(name) + "] to be writable, but was not.");
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyNotWritable(obj, name, verifyProp, value) {
    if (!verifyProp) {
      assert(!Object.getOwnPropertyDescriptor(obj, name).writable,
          "Expected obj[" + String(name) + "] to have writable:false.");
    }
    if (isWritable(obj, name, verifyProp)) {
      throw new Test262Error("Expected obj[" + String(name) + "] NOT to be writable, but was.");
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyEnumerable(obj, name) {
    assert(Object.getOwnPropertyDescriptor(obj, name).enumerable,
        "Expected obj[" + String(name) + "] to have enumerable:true.");
    if (!isEnumerable(obj, name)) {
      throw new Test262Error("Expected obj[" + String(name) + "] to be enumerable, but was not.");
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyNotEnumerable(obj, name) {
    assert(!Object.getOwnPropertyDescriptor(obj, name).enumerable,
        "Expected obj[" + String(name) + "] to have enumerable:false.");
    if (isEnumerable(obj, name)) {
      throw new Test262Error("Expected obj[" + String(name) + "] NOT to be enumerable, but was.");
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyConfigurable(obj, name) {
    assert(Object.getOwnPropertyDescriptor(obj, name).configurable,
        "Expected obj[" + String(name) + "] to have configurable:true.");
    if (!isConfigurable(obj, name)) {
      throw new Test262Error("Expected obj[" + String(name) + "] to be configurable, but was not.");
    }
  }

  /**
  * Deprecated; please use `verifyProperty` in new tests.
  */
  function verifyNotConfigurable(obj, name) {
    assert(!Object.getOwnPropertyDescriptor(obj, name).configurable,
        "Expected obj[" + String(name) + "] to have configurable:false.");
    if (isConfigurable(obj, name)) {
      throw new Test262Error("Expected obj[" + String(name) + "] NOT to be configurable, but was.");
    }
  }

### detachArrayBuffer

  // Copyright (C) 2016 the V8 project authors.  All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      A function used in the process of asserting correctness of TypedArray objects.

      $262.detachArrayBuffer is defined by a host.
  defines: [$DETACHBUFFER]
  ---*/

  function $DETACHBUFFER(buffer) {
    if (!$262 || typeof $262.detachArrayBuffer !== "function") {
      throw new Test262Error("No method available to detach an ArrayBuffer");
    }
    $262.detachArrayBuffer(buffer);
  }

### testTypedArray

  // Copyright (C) 2015 André Bargull. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Collection of functions used to assert the correctness of TypedArray objects.
  defines:
    - typedArrayConstructors
    - floatArrayConstructors
    - intArrayConstructors
    - TypedArray
    - testWithTypedArrayConstructors
    - testWithAtomicsFriendlyTypedArrayConstructors
    - testWithNonAtomicsFriendlyTypedArrayConstructors
    - testTypedArrayConversions
  ---*/

  /**
  * Array containing every typed array constructor.
  */
  var typedArrayConstructors = [
    Float64Array,
    Float32Array,
    Int32Array,
    Int16Array,
    Int8Array,
    Uint32Array,
    Uint16Array,
    Uint8Array,
    Uint8ClampedArray
  ];

  var floatArrayConstructors = typedArrayConstructors.slice(0, 2);
  var intArrayConstructors = typedArrayConstructors.slice(2, 7);

  /**
  * The %TypedArray% intrinsic constructor function.
  */
  var TypedArray = Object.getPrototypeOf(Int8Array);

  /**
  * Callback for testing a typed array constructor.
  *
  * @callback typedArrayConstructorCallback
  * @param {Function} Constructor the constructor object to test with.
  */

  /**
  * Calls the provided function for every typed array constructor.
  *
  * @param {typedArrayConstructorCallback} f - the function to call for each typed array constructor.
  * @param {Array} selected - An optional Array with filtered typed arrays
  */
  function testWithTypedArrayConstructors(f, selected) {
    var constructors = selected || typedArrayConstructors;
    for (var i = 0; i < constructors.length; ++i) {
      var constructor = constructors[i];
      try {
        f(constructor);
      } catch (e) {
        e.message += " (Testing with " + constructor.name + ".)";
        throw e;
      }
    }
  }

  /**
  * Calls the provided function for every non-"Atomics Friendly" typed array constructor.
  *
  * @param {typedArrayConstructorCallback} f - the function to call for each typed array constructor.
  * @param {Array} selected - An optional Array with filtered typed arrays
  */
  function testWithNonAtomicsFriendlyTypedArrayConstructors(f) {
    testWithTypedArrayConstructors(f, [
      Float64Array,
      Float32Array,
      Uint8ClampedArray
    ]);
  }

  /**
  * Calls the provided function for every "Atomics Friendly" typed array constructor.
  *
  * @param {typedArrayConstructorCallback} f - the function to call for each typed array constructor.
  * @param {Array} selected - An optional Array with filtered typed arrays
  */
  function testWithAtomicsFriendlyTypedArrayConstructors(f) {
    testWithTypedArrayConstructors(f, [
      Int32Array,
      Int16Array,
      Int8Array,
      Uint32Array,
      Uint16Array,
      Uint8Array,
    ]);
  }

  /**
  * Helper for conversion operations on TypedArrays, the expected values
  * properties are indexed in order to match the respective value for each
  * TypedArray constructor
  * @param  {Function} fn - the function to call for each constructor and value.
  *                         will be called with the constructor, value, expected
  *                         value, and a initial value that can be used to avoid
  *                         a false positive with an equivalent expected value.
  */
  function testTypedArrayConversions(byteConversionValues, fn) {
    var values = byteConversionValues.values;
    var expected = byteConversionValues.expected;

    testWithTypedArrayConstructors(function(TA) {
      var name = TA.name.slice(0, -5);

      return values.forEach(function(value, index) {
        var exp = expected[name][index];
        var initial = 0;
        if (exp === 0) {
          initial = 1;
        }
        fn(TA, value, exp, initial);
      });
    });
  }

### decimalToHexString

  // Copyright (C) 2017 André Bargull. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Collection of functions used to assert the correctness of various encoding operations.
  defines: [decimalToHexString, decimalToPercentHexString]
  ---*/

  function decimalToHexString(n) {
    var hex = "0123456789ABCDEF";
    n >>>= 0;
    var s = "";
    while (n) {
      s = hex[n & 0xf] + s;
      n >>>= 4;
    }
    while (s.length < 4) {
      s = "0" + s;
    }
    return s;
  }

  function decimalToPercentHexString(n) {
    var hex = "0123456789ABCDEF";
    return "%" + hex[(n >> 4) & 0xf] + hex[n & 0xf];
  }

### byteConversionValues

  // Copyright (C) 2016 the V8 project authors. All rights reserved.
  // This code is governed by the BSD license found in the LICENSE file.
  /*---
  description: |
      Provide a list for original and expected values for different byte
      conversions.
      This helper is mostly used on tests for TypedArray and DataView, and each
      array from the expected values must match the original values array on every
      index containing its original value.
  defines: [byteConversionValues]
  ---*/
  var byteConversionValues = {
    values: [
      127,         // 2 ** 7 - 1
      128,         // 2 ** 7
      32767,       // 2 ** 15 - 1
      32768,       // 2 ** 15
      2147483647,  // 2 ** 31 - 1
      2147483648,  // 2 ** 31
      255,         // 2 ** 8 - 1
      256,         // 2 ** 8
      65535,       // 2 ** 16 - 1
      65536,       // 2 ** 16
      4294967295,  // 2 ** 32 - 1
      4294967296,  // 2 ** 32
      9007199254740991, // 2 ** 53 - 1
      9007199254740992, // 2 ** 53
      1.1,
      0.1,
      0.5,
      0.50000001,
      0.6,
      0.7,
      undefined,
      -1,
      -0,
      -0.1,
      -1.1,
      NaN,
      -127,        // - ( 2 ** 7 - 1 )
      -128,        // - ( 2 ** 7 )
      -32767,      // - ( 2 ** 15 - 1 )
      -32768,      // - ( 2 ** 15 )
      -2147483647, // - ( 2 ** 31 - 1 )
      -2147483648, // - ( 2 ** 31 )
      -255,        // - ( 2 ** 8 - 1 )
      -256,        // - ( 2 ** 8 )
      -65535,      // - ( 2 ** 16 - 1 )
      -65536,      // - ( 2 ** 16 )
      -4294967295, // - ( 2 ** 32 - 1 )
      -4294967296, // - ( 2 ** 32 )
      Infinity,
      -Infinity,
      0
    ],

    expected: {
      Int8: [
        127,  // 127
        -128, // 128
        -1,   // 32767
        0,    // 32768
        -1,   // 2147483647
        0,    // 2147483648
        -1,   // 255
        0,    // 256
        -1,   // 65535
        0,    // 65536
        -1,   // 4294967295
        0,    // 4294967296
        -1,   // 9007199254740991
        0,    // 9007199254740992
        1,    // 1.1
        0,    // 0.1
        0,    // 0.5
        0,    // 0.50000001,
        0,    // 0.6
        0,    // 0.7
        0,    // undefined
        -1,   // -1
        0,    // -0
        0,    // -0.1
        -1,   // -1.1
        0,    // NaN
        -127, // -127
        -128, // -128
        1,    // -32767
        0,    // -32768
        1,    // -2147483647
        0,    // -2147483648
        1,    // -255
        0,    // -256
        1,    // -65535
        0,    // -65536
        1,    // -4294967295
        0,    // -4294967296
        0,    // Infinity
        0,    // -Infinity
        0
      ],
      Uint8: [
        127, // 127
        128, // 128
        255, // 32767
        0,   // 32768
        255, // 2147483647
        0,   // 2147483648
        255, // 255
        0,   // 256
        255, // 65535
        0,   // 65536
        255, // 4294967295
        0,   // 4294967296
        255, // 9007199254740991
        0,   // 9007199254740992
        1,   // 1.1
        0,   // 0.1
        0,   // 0.5
        0,   // 0.50000001,
        0,   // 0.6
        0,   // 0.7
        0,   // undefined
        255, // -1
        0,   // -0
        0,   // -0.1
        255, // -1.1
        0,   // NaN
        129, // -127
        128, // -128
        1,   // -32767
        0,   // -32768
        1,   // -2147483647
        0,   // -2147483648
        1,   // -255
        0,   // -256
        1,   // -65535
        0,   // -65536
        1,   // -4294967295
        0,   // -4294967296
        0,   // Infinity
        0,   // -Infinity
        0
      ],
      Uint8Clamped: [
        127, // 127
        128, // 128
        255, // 32767
        255, // 32768
        255, // 2147483647
        255, // 2147483648
        255, // 255
        255, // 256
        255, // 65535
        255, // 65536
        255, // 4294967295
        255, // 4294967296
        255, // 9007199254740991
        255, // 9007199254740992
        1,   // 1.1,
        0,   // 0.1
        0,   // 0.5
        1,   // 0.50000001,
        1,   // 0.6
        1,   // 0.7
        0,   // undefined
        0,   // -1
        0,   // -0
        0,   // -0.1
        0,   // -1.1
        0,   // NaN
        0,   // -127
        0,   // -128
        0,   // -32767
        0,   // -32768
        0,   // -2147483647
        0,   // -2147483648
        0,   // -255
        0,   // -256
        0,   // -65535
        0,   // -65536
        0,   // -4294967295
        0,   // -4294967296
        255, // Infinity
        0,   // -Infinity
        0
      ],
      Int16: [
        127,    // 127
        128,    // 128
        32767,  // 32767
        -32768, // 32768
        -1,     // 2147483647
        0,      // 2147483648
        255,    // 255
        256,    // 256
        -1,     // 65535
        0,      // 65536
        -1,     // 4294967295
        0,      // 4294967296
        -1,     // 9007199254740991
        0,      // 9007199254740992
        1,      // 1.1
        0,      // 0.1
        0,      // 0.5
        0,      // 0.50000001,
        0,      // 0.6
        0,      // 0.7
        0,      // undefined
        -1,     // -1
        0,      // -0
        0,      // -0.1
        -1,     // -1.1
        0,      // NaN
        -127,   // -127
        -128,   // -128
        -32767, // -32767
        -32768, // -32768
        1,      // -2147483647
        0,      // -2147483648
        -255,   // -255
        -256,   // -256
        1,      // -65535
        0,      // -65536
        1,      // -4294967295
        0,      // -4294967296
        0,      // Infinity
        0,      // -Infinity
        0
      ],
      Uint16: [
        127,   // 127
        128,   // 128
        32767, // 32767
        32768, // 32768
        65535, // 2147483647
        0,     // 2147483648
        255,   // 255
        256,   // 256
        65535, // 65535
        0,     // 65536
        65535, // 4294967295
        0,     // 4294967296
        65535, // 9007199254740991
        0,     // 9007199254740992
        1,     // 1.1
        0,     // 0.1
        0,     // 0.5
        0,     // 0.50000001,
        0,     // 0.6
        0,     // 0.7
        0,     // undefined
        65535, // -1
        0,     // -0
        0,     // -0.1
        65535, // -1.1
        0,     // NaN
        65409, // -127
        65408, // -128
        32769, // -32767
        32768, // -32768
        1,     // -2147483647
        0,     // -2147483648
        65281, // -255
        65280, // -256
        1,     // -65535
        0,     // -65536
        1,     // -4294967295
        0,     // -4294967296
        0,     // Infinity
        0,     // -Infinity
        0
      ],
      Int32: [
        127,         // 127
        128,         // 128
        32767,       // 32767
        32768,       // 32768
        2147483647,  // 2147483647
        -2147483648, // 2147483648
        255,         // 255
        256,         // 256
        65535,       // 65535
        65536,       // 65536
        -1,          // 4294967295
        0,           // 4294967296
        -1,          // 9007199254740991
        0,           // 9007199254740992
        1,           // 1.1
        0,           // 0.1
        0,           // 0.5
        0,           // 0.50000001,
        0,           // 0.6
        0,           // 0.7
        0,           // undefined
        -1,          // -1
        0,           // -0
        0,           // -0.1
        -1,          // -1.1
        0,           // NaN
        -127,        // -127
        -128,        // -128
        -32767,      // -32767
        -32768,      // -32768
        -2147483647, // -2147483647
        -2147483648, // -2147483648
        -255,        // -255
        -256,        // -256
        -65535,      // -65535
        -65536,      // -65536
        1,           // -4294967295
        0,           // -4294967296
        0,           // Infinity
        0,           // -Infinity
        0
      ],
      Uint32: [
        127,        // 127
        128,        // 128
        32767,      // 32767
        32768,      // 32768
        2147483647, // 2147483647
        2147483648, // 2147483648
        255,        // 255
        256,        // 256
        65535,      // 65535
        65536,      // 65536
        4294967295, // 4294967295
        0,          // 4294967296
        4294967295, // 9007199254740991
        0,          // 9007199254740992
        1,          // 1.1
        0,          // 0.1
        0,          // 0.5
        0,          // 0.50000001,
        0,          // 0.6
        0,          // 0.7
        0,          // undefined
        4294967295, // -1
        0,          // -0
        0,          // -0.1
        4294967295, // -1.1
        0,          // NaN
        4294967169, // -127
        4294967168, // -128
        4294934529, // -32767
        4294934528, // -32768
        2147483649, // -2147483647
        2147483648, // -2147483648
        4294967041, // -255
        4294967040, // -256
        4294901761, // -65535
        4294901760, // -65536
        1,          // -4294967295
        0,          // -4294967296
        0,          // Infinity
        0,          // -Infinity
        0
      ],
      Float32: [
        127,                  // 127
        128,                  // 128
        32767,                // 32767
        32768,                // 32768
        2147483648,           // 2147483647
        2147483648,           // 2147483648
        255,                  // 255
        256,                  // 256
        65535,                // 65535
        65536,                // 65536
        4294967296,           // 4294967295
        4294967296,           // 4294967296
        9007199254740992,     // 9007199254740991
        9007199254740992,     // 9007199254740992
        1.100000023841858,    // 1.1
        0.10000000149011612,  // 0.1
        0.5,                  // 0.5
        0.5,                  // 0.50000001,
        0.6000000238418579,   // 0.6
        0.699999988079071,    // 0.7
        NaN,                  // undefined
        -1,                   // -1
        -0,                   // -0
        -0.10000000149011612, // -0.1
        -1.100000023841858,   // -1.1
        NaN,                  // NaN
        -127,                 // -127
        -128,                 // -128
        -32767,               // -32767
        -32768,               // -32768
        -2147483648,          // -2147483647
        -2147483648,          // -2147483648
        -255,                 // -255
        -256,                 // -256
        -65535,               // -65535
        -65536,               // -65536
        -4294967296,          // -4294967295
        -4294967296,          // -4294967296
        Infinity,             // Infinity
        -Infinity,            // -Infinity
        0
      ],
      Float64: [
        127,         // 127
        128,         // 128
        32767,       // 32767
        32768,       // 32768
        2147483647,  // 2147483647
        2147483648,  // 2147483648
        255,         // 255
        256,         // 256
        65535,       // 65535
        65536,       // 65536
        4294967295,  // 4294967295
        4294967296,  // 4294967296
        9007199254740991, // 9007199254740991
        9007199254740992, // 9007199254740992
        1.1,         // 1.1
        0.1,         // 0.1
        0.5,         // 0.5
        0.50000001,  // 0.50000001,
        0.6,         // 0.6
        0.7,         // 0.7
        NaN,         // undefined
        -1,          // -1
        -0,          // -0
        -0.1,        // -0.1
        -1.1,        // -1.1
        NaN,         // NaN
        -127,        // -127
        -128,        // -128
        -32767,      // -32767
        -32768,      // -32768
        -2147483647, // -2147483647
        -2147483648, // -2147483648
        -255,        // -255
        -256,        // -256
        -65535,      // -65535
        -65536,      // -65536
        -4294967295, // -4294967295
        -4294967296, // -4294967296
        Infinity,    // Infinity
        -Infinity,   // -Infinity
        0
      ]
    }
  };
