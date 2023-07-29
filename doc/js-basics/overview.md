
# JavaScript Basics (ES5) using `Duktape`/`duktape-cc`

## Running the CLI application

Checking out the `duktape-cc` repository and running (GNU) `make`,
the command line application `djs` (or `djs.exe` on win32) will
usable. We presume here that you put it in the executable search
`PATH`.

```shell
# Run script file
$ djs -s script_file.djs

# Run script with input arguments
$ djs -s script_file.djs  ARG1 "CLI Argument 2" 3 4 5

# Run script with verbose exception tracing (`-v` means "verbose")
$ djs -v -s script_file.djs
```

## ECMA ES5 in Pistachio

### Basic input and output

I/O is not actually defined in the ECMA standard, but by the
application c/c++ code that executes the JS interpreter.

The `duktape-cc` mod.stdio.hh module provides standard input
and output to the typical application I/O channels `STDIN`,
`STDOUT`, and `STDERR`.

Another well known (often preferred) method to feed input data
into the CLI application are command line arguments and system
(or user) environment variables. These are handled in the CLI
main application and passed into the interpreter as variables.

```js
  // @example basic-io
  // Output to console. `print()` is like `console.log()`
  // known from browsers.
  print("This is sent to STDOUT");

  // `alert("Message")` is a very old feature and opens a message
  // box in browsers. In the CLI application, it simply prints
  // to `STDERR`.
  alert("This is sent to STDERR");

  // Blocking input from console. Formerly this opened a small
  // input window with a text box (and "OK-Abort" buttons). In
  // djs, it reads an input line from `STDIN` (the console).
  // For compliancy, also `confirm()` reads from `STDIN`.
  var line = prompt("Type something: ");
  print("Input was '" + line + "'.");

  // Fetching command line arguments (e.g. djs -s script.djs ARG1 ARG2 ...)
  // Prints "ARG1,ARG2,...". sys.args is an array.
  print("sys.args = ", sys.args);

  // Fetching environment variables from the system/shell.
  // Prints user temporary directory (type `env` (unix) `set` (win32)
  // to see your ENV variables independent of the djs application).
  print("sys.env.TMP = '" + sys.env.TMP + "'");
```

### Variables and Types

There are only very few types in ECMA, and all of them have a literal. All
more complex data types are derived from `object`. Even functions are first
class objects that are callable.

  - `undefined`: Means "not there" (variable assigned, field in an object does not exist).
  - `boolean`: `true` and `false`.
  - `number`: Numbers, integers up to 32bit, or 64bit (`double`) floating point.
  - `string`: Text data. JS uses ASCII or (if unicode preferred utf8).
  - `object`: General `struct` or `class` like type, contains `key-value` assignments.
  - `function`: Sub-routines with optional return value. Technically callable objects.
  - (`null`): Explicit non-value object. (use as seldom as possible).
  - (`symbol`): Unique identifiers that only equal themselves (very rarely used).

  - `@see`: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Language_Overview.

Like most scripting languages, JS types are not fixed for variables or function returns,
but runtime dynamic. You can assign a string to a variable, and later a number. This is
called *"duck typing": "Looks like a duck, sounds like a duck, is a duck"*. This is a
blessing, but also a curse, because types sometimes have to be checked at runtime.

```js
  // @example types
  // Numbers
  var number1 = -1;
  var number2 = 1.0;
  var number3 = 50e-3;
  var not_a_number = Number.NaN;
  print("typeof(number1) == " + typeof(number1));

  // Booleans
  var holding_yes = true;
  var holding_no = false;
  print("typeof(holding_yes) == " + typeof(holding_yes));

  // Strings
  var empty_string = "";
  var a_string = "String literal";
  var another_string = 'Also string literal, not only single character';
  print("typeof(empty_string) == " + typeof(empty_string));

  // Objects
  var empty_object = {};
  var an_object = { prop1:10, prop2:"20", prop3:[1,2,3], prop4:{a:1} };
  var null_object = null;
  print("typeof(empty_object) == " + typeof(empty_object));
  print("typeof(null_object) == " + typeof(null_object));

  // Arrays
  var empty_array = [];
  var an_array = [1,2,3];
  var another_array = [1, 2, "s1", "s2", ["sub", "array"] ];
  print("typeof(empty_array) == " + typeof(empty_array));
  print("Array.isArray(empty_array) == " + Array.isArray(empty_array));
  print("Array.isArray(empty_object) == " + Array.isArray(empty_object));

  // Functions
  var func1 = function() { return 42; }
  var func2 = function(arg1) { return arg1; }
  print("typeof(func1) == " + typeof(func1));
  print("func1() == " + func1());
  print("func2('argument') == " + func2('argument'));

  // Symbols
  var symbol1 = Symbol();
  var symbol2 = Symbol();
  print("typeof(symbol1) == " + typeof(symbol1));
  print("(symbol1==symbol1) == " + (symbol1==symbol1));
  print("(symbol1==symbol2) == " + (symbol1==symbol2));
```

Variables are defined, as in the example, using the `var` keyword, followed by an
assignment: `var name = value`. Things to note about variables (especially when coming
from strong typed languages):

  - The type is automatically assigned to variable with the value.

  - Defined variables (with `var`) are *function scoped*, so they can be seen everywhere in
    the function where they are defined, also in nested functions (functions: see below, it
    is possible to write functions in functions, pretty useful).

  - If you forget the `var` keyword, the interpreter assumes the global object (in browsers
    the "window" object). So take care *not* to forget it. A protection is to add
    `"use strict"` at the top of a file.

  - If you define a variable twice in a function using the `var` keyword, the interpreter,
    the variable is the same (aka it's overwritten). This applies not to nested functions,
    which each have their own scope (by definition).

  - Primitive types (numbers, strings, boolean) are copied, but objects are referenced, see
    example below. Technically, all data are assigned by value - only that the value may be
    a reference. The concept of this is simple and saves a lot of RAM and performance, but
    comes with on pitfall: You may unintentionally change data in objects. The solution, where
    needed, is to make a so called "clone" or "deep copy" of the object.

  - The Duktape interpreter provides the `const` keyword from ES6, which can be used instead
    of `var`. Variables defined with `const` have immutable *values* - Note the point above:
    If the variable "points to" an object, you cannot change variable (aka, the reference to
    the object), but you can still change data in the object itself. That is substantially
    different from the `const` keyword in e.g. c++. Knowing that allows using `const` to an
    advantage, but it's definitely a pitfall when getting started with JS.

  - A word about type coercion to `boolean`, which important to know for conditions (`if`,
    `for`, `while`, as well as logical operators (`!`, `&&`, `||`): Coming e.g. from C, we
    know `0` is `false` and everything else is `true`. JS extends this a bit when checking
    for truth values: `0`, `undefined`, `null`, `NaN`, and `""` are seen as `false`, all other
    values as `true` (even empty arrays or all kind of objects).


```js
  // @example global-variables
  // Without var the "global root scope" is assumed. Rule of thumb: Don't do it, always
  // use `var` or the `global` object.
  global_variable = "GLOBAL";

  // Verbose access to global variables using `global`
  print('global.global_variable == "' + global.global_variable + '"');

  function func_changing_global()
  {
    print('global_variable in function == "' + global_variable + '"');
    global_variable = "CHANGED GLOBAL"; // prefer: global.variable_name = ...
  }

  // Call func1
  func_changing_global();
  print('global_variable after function call == "' + global_variable + '"');
```

```js
  // @example local-variables
  // Scope of variables in functions. Function arguments are like local variables
  // defined with `var`:
  function func2(arg1, arg2)
  {
    var local_variable1 = "ONLY VISIBLE IN FUNC2";

    // func3 is a nested function. `arg1` has the same name, "the most local one" will
    // be used. So, `arg1` is the one passed into `func3()`, and `arg2` is the argument
    // variable from `func2(arg1, arg2)`, which is also visible here:
    function func3(arg1)
    {
      var local_variable2 = "ONLY VISIBLE IN FUNC3";
      print("func3() arg1 == " + arg1); // func3(arg1)
      print("func3() arg2 == " + arg2); // func2(####, arg2)
      print("func3() local_variable1 == " + local_variable1);
      print("func3() local_variable2 == " + local_variable2);
    }

    func3(arg1 + arg1); // call func3 to print that, too.

    // We don't see the `arg1` from `func3(arg1)` because we are outside `func3`.
    // But we see our own argument `arg1`, because we are inside `func2`.
    print("func2() arg1 == " + arg1);
    print("func2() arg2 == " + arg2);
    print("func2() local_variable1 == " + local_variable1);
    // We're outside func3 and cannot see it, a ReferenceError would be thrown.
    print("func2() typeof(local_variable2) == " + typeof(local_variable2));
  }

  func2(128, 1024); // and finally call func2
```

```js
  // @example object-references
  // Object references, and the "const" pitfall: The variable `elements` is const,
  // we cannot re-assign it. But the underlying array object is not, so we can add,
  // remove, or change elements.
  const elements = [];
  elements.push(1);
  elements.push(2);
  print("elements ==", elements); // -> 1,2
  print("elements.length ==", elements.length); // -> 2 (array has 2 elements).

  // Object references, and the "copy" pitfall:
  const o1 = { value:10 };
  const o2 = o1;
  o1.a = 11; // modify object 1
  print("o1 ==", JSON.stringify(o1)); // {"value":11}
  print("o2 ==", JSON.stringify(o2)); // {"value":11}, **not** 10, -> ref to same object.

  // Simplest solution for deep copy, but also lacking performance:
  const clone = function(o) { return JSON.parse(JSON.stringify(o)) };
  const o3 = { value: 10};
  const o4 = clone(o3);
  o3.value = 11;
  print("o3 ==", JSON.stringify(o3)); // {"value":11}
  print("o4 ==", JSON.stringify(o4)); // {"value":10}, unchanged, different objects now.
```

### Operators

Operators and their precedences are projected from the common convention known from `C`.
@see: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Expressions_and_Operators.

A short list of operators in-code:

```js
  // @example operators
  const c=1, d=2;
  var x;

  // --- Assignments
  x = c       // Basic Assignment: c is written into x.
  x += c      // Addition assignment x = x + c
  x -= c      // Subtraction assignment: x = x - c
  x *= c      // Multiplication assignment: x = x * c
  x /= c      // Division assignment: x = x / c
  x %= c      // Remainder assignment: x = x % c
  x <<= c     // Left shift assignment: x = x << c
  x >>= c     // Signed right shift assignment: x = x >> c
  x >>>= c    // Unsigned right shift assignment: x = x >>> c
  x &= c      // Bitwise AND assignment x = x & c
  x ^= c      // Bitwise XOR assignment x = x ^ c
  x |= c      // Bitwise OR assignment x = x | c

  // --- Comparison operators (evaluates to `true` if ...)
  x == c      // x and c are equal.
  x != c      // x and c are not equal.
  x === c     // x and c are equal, and have the same type (strict equal).
  x !== c     // x and c differ or do not have the same type (strict not equal).
  x > c       // x greater than c.
  x < c       // x less than c.
  x >= c      // x greater than or equal to c.
  x <= c      // x less than or equal to c.

  // --- Arithmetic operators (evaluate to `numbers`, with one exception)
  x + c       // Sum x + y. Careful: String concatenation: "4"+2 === "42".
  x - c       // Difference x - y
  x * c       // Product x * y
  x / c       // Division x / y
  x % c       // Remainder of integral division x/c ("modulo").
  ++x         // Pre-increment: Calculates x=x+1, and then evaluates to that value.
  x++         // Post-increment: Evaluates to x, and increases x by 1 after that.
  --x         // Pre-decrement: Calculates x=x-1, and then evaluates to that value.
  x--         // Post-decrement: Evaluates to x, and decreases x by 1 after that.
  -x          // Unary minus. Evaluates to negative x.
  +x          // Unary plus. Implicitly coerces to `number` (e.g. +true==1, +"3"==3).

  // Logical operators
  !x          // Logical NOT: `false` if x is (or coercible to) `true`.
  x && c      // Logical AND: The first value from left to right that coerces to `false`.
  x || c      // Logical OR:. The first value from left to right that coerces to `true`.

  // --- Bitwise operators (32bit, everything higher is cut off, evaluate to `number`s).
  x & c       // Bitwise AND (for each bit of x with each corresponding bit of c).
  x | c       // Bitwise OR (for each bit of x with each corresponding bit of c).
  x ^ c       // Bitwise XOR (for each bit of x with each corresponding bit of c).
  ~x          // Bitwise complement: Inverts each bit of x.
  x << c      // Left shift x, c times. Zeros are shifted in from the right.
  x >> c      // Right shift x, c times. Arithmetic shift (sign bit is preserved).
  x >>> c     // Right shift x, c times. Zeros are shifted in from the left.

  // Ternary operator
  x ? c : d   // Returns c if x coerces to `true`, d otherwise.

  // Unary keyword based operators
  void x      // Evaluate x while discarding the return value.
  typeof x    // Returns the type name of the value stored in x.
  delete x    // Delete a variable or property of an object (x is `undefined` after deleting).
```

Although also operands these are described later: `instanceof`, `in`, `new`.

Because of the dynamic typing, minor but notable differences apply (aka pitfalls
from other languages):

  - *Bit operations are 32bit*: Numbers are converted from floating-point to
    integers before bitwise operations take place. All bits higher than 2^31
    are cut off (forced to 0). *There is no `uint64_t`, `uint128_t` bit op
    with the type `number`*.

  - *Plus on strings*: Yields the concatenated string, `"15"+"15"` is `"1515"`,
    not `30`. Because evaluating from left to right, `"15"+15` is also `"1515"`,
    because the number is coerced to string.

  - Logical NOT (`!`) is often used to homogenize everything that evaluates to
    `false`, it matches well for `null`, `undefined`, `""`, `0`, and `NaN`.

  - Logical OR (`||`) returns the first *value* that is convertible to `boolean`
    `true`, or the last element. Hence, `arg = arg || "alternative";` is often
    used for default function arguments, as `arg1` is `undefined` if it is not
    set. The difference between JS and e.g. C is that JS does not return `0`
    (`c++ false`), but the actual value `"alternative"`, which is still of type
    `string`. It is yet encouraged to explicitly check for `undefined` for safety,
    but if there are any more detailed argument checks down the line, this method
    can be a help. If you are familiar with `shell` programming,

  - Logical AND (`&&`) works similar to OR, except that it returns the first
    value that is coercible to `false`, or the last element. Hence `null && false`
    evaluates to `null`, not `false`. Similar: `"cat" && "squirrel" && "dog"`
    evaluates to `"dog"`, because neither of the strings is empty, and the last
    element is `dog`.

```js
  // @example operators-pitfalls
  // Pitfall 1: 32it integer range.
  const x30 = Math.pow(2,30);
  const x31 = Math.pow(2,31);
  const x32 = Math.pow(2,32);
  print("2^30 == x30 == " + x30 ); // -> 1073741824
  print("2^31 == x31 == " + x31 ); // -> 2147483648 -> double value, positive.
  print("2^32 == x32 == " + x32 ); // -> 4294967296 -> out of 32bit range.
  print("x30 & x30 == " + (x30 & x30).toString() ); // -> 1073741824
  print("x31 & x31 == " + (x31 & x31).toString() ); // -> -2147483648 (int32 value)
  print("x32 & x32 == " + (x32 & x32).toString() ); // -> 0 (cut off)

  // Pitfall 2: String concat
  print('"1"+"2" == ', "1"+"2");

  // Pitfall 3: AND / OR
  print("null && false ==", null && false ); // -> null
  print("'' && undefined ==", '' && undefined ); // -> ""
  print("'0' && false ==", '0' && false ); // -> false, string "0" is not empty.
  print('"0" || true ==', "0" || true ); // -> "0"
  print('undefined || "" ==', undefined || "" ); // -> "", which is last in the list.
```

As a "common access" operator, brackets `[]` are worth mentioning separately.
The work like the array access in C, but can also be used on common objects (means
with strings), not only on arrays with numeric indices:

```js
  // @example array-access

  // Numeric access to arrays and Array `length`.
  const my_array = [1,2,3,4,5];
  const first_element = my_array[0];
  const last_element = my_array[my_array.length-1];
  print("array first/last ==", first_element, "/", last_element);

  // Iterating index through an array is C-like:
  for(var i=0; i<my_array.length; ++i) {
    print("my_array["+i+"] =", my_array[i]);
  }

  // Similar, but not identical, objects can be indexed with
  // strings. Using numbers will implicitly coerce that number
  // to a string. The same is true for the direct definition:
  const my_object = { x:1, y:2, 10:11 };
  const x = my_object.x;
  const also_x = my_object["x"];
  print("my_object.x ==", x, "==", also_x);
  print("my_object['10'] ==", my_object['10']);
  print("my_object[10] ==", my_object[10]);

  // Iterating through objects: We don't know the keys yet,
  // the `in` operator can help us:
  for(var key in my_object) {
    print("my_object['"+key+"'] =", my_object[key]);
  }
```

### Flow Control

The statements used for flow control are very similar to the commonly known from `c`,`c++`,
`java`, `c#`, etc. The difference is often writing `var` instead of `int` and the like:

```js
  // @example flow-control
  // If-then-else
  const sys_name = sys.uname().sysname.toLowerCase();
  if(sys_name == "linux") {
    print("Welcome Linus.");
  } else if(sys_name == "macos") {
    print("Welcome Tim.");
  } else if(sys_name == "windows") {
    print("Welcome Bill, no, Steve ... Satya.");
  } else {
    print("Welcome, you there.");
  }

  // Switch-case (also works on strings or symbols)
  switch(sys_name) {
    case "windows":
      print("Use backslash.");
      break;
    case "linux":
    case "macos":
      print("Use slash.");
      break;
    default:
      print("Use probably slash.");
  }

  // For loop
  var acc = 0;
  for(var i=0; i<10; ++i) {
    acc += i;
  }
  print("Accumulated values 0 to 10 ==", acc);

  // While loop
  var acc = 1; // note: knowingly overwriting `acc` above.
  while(acc <= 10000) {
    acc *= 2;
  }
  print("First 2^N value greater than 10000 ==", acc);

  // Do-While loop
  var acc = 1;
  do {
    acc *= 2;
  } while(acc <= 10000);
  print("First 2^N value greater than 10000 ==", acc);

  // Continue and break in loops. Lets accumulate all even numbers,
  // and stop after the accumulator reached e.g. the value 15000.
  var acc = 0;
  for(var i=0; i<10000; ++i) {
    if(i % 2 != 0) continue;  // not even number, directly do next iteration.
    acc += i; // accumulate.
    if(acc >= 15000) break; // abort the loop if threshold reached.
  }
  print("First even cumulative sum > 15000 ==", acc);

  // Key-value iteration loop: You can use `for` to index objects:
  const data = { a:10, b:11, c:12 };
  for(var key in data) {
    const value = data[key]; // objects values are accessed like array values.
    print("data: " + key + "=" + value);
  }
```

JS also supports labels, which can be used with `break`. This is not described here.

### Exceptions

For conventional OOP like Error handling, JS provides the usual exceptions with `try-catch`.
As exception object, although not strictly needed, an `Error` is normally instantiated:

```js
  // @example exceptions
  // Something that throws first:
  function failing(do_fail) {
    if(do_fail) {
      throw new Error("Nope, that failed.");
    } else {
      return "ok return value";
    }
  }

  // Call and catch exceptions:
  try {
    print("failing(false) ==", failing(false)); // is printed.
    print("failing(true) ==", failing(true));   // is not printed, catch block instead.
    print("Not reached");
  } catch(ex) {
    print("failing(true) did throw: '" + ex.message + "'");
  }

  // Call and let the exception through, but do some cleanup using `finally`:
  try {
    print("failing(false) ==", failing(false)); // is printed.
    print("failing(true) ==", failing(true));   // is not printed, catch block instead.
    print("Not reached");
  } finally {
    print("failing(true) did throw. Cleanup before the exception is escalated up.");
  }
  print("Not reached, the script terminates with a stack trace");
```

### Functions/Methods

Like in most programming languages, functions are sub-routines, which can receive input
arguments and return a value. The arguments are passed in using parentheses `()` when calling
a function, and the return value is present at the position of the function after function
has returned. If no return value was set, it is implicitly `undefined`.

Technically, function names and function bodies are separate things. A function is only a
piece of callable code, and a function name simply a variable that holds a reference to that
code. That means these variables can also be re-assigned or deleted, and sometimes it is not
even needed to name a function because it is only used once in-place. Out in the wild there
are two commonly seen ways to define functions, which are both identical to the interpreter:

```js
  // @example function-basic
  // Traditional definition: Variable name for is `vector_length`, and the
  // value is the function that takes three arguments. Use the `return` keyword
  // to exit a function with a value.
  function vector_length(x, y, z)
  {
    return Math.sqrt(x*x + y*y + z*z);
  }

  print("vector_length(1, 1, 1) == " + vector_length(1, 1, 1));

  //
  // Assignment definition: This is identical to the above, but let's return a
  // rounded value for verification.
  //
  global.vector_length = function(x, y, z) {
    return Math.round(Math.sqrt(x*x + y*y + z*z));
  }

  print("Overwritten: vector_length(1, 1, 1) == " + vector_length(1, 1, 1));

  //
  // Scoped assignment: We can also do this properly scoped. Like `var`, `const`
  // only visible in the current scope.
  //
  const vector_length = function(x, y, z) {
    return Math.round(Math.sqrt(x*x + y*y + z*z) * 1000) / 1000;
  }
  print("const: vector_length(1, 1, 1) == " + vector_length(1, 1, 1));
```

Functions can be called in the context of objects (accessing object properties using `this`).
Typically functions are referred to as `methods` in this case. For "normal" functions, `this`
is simply the global object, which means functions are really always methods in JS.

```js
  // @example function-method
  // Plain object with some data:
  var vec1 = {x:1, y:2, z:3};
  print("vec1 = [" + vec1.x + "," + vec1.y + "," + vec1.z + "]");

  // We add a function to that, again the vector length. The key here is to use `this` to
  // access the properties ("fields") of the object on which this function will be called.
  vec1.abs = function() { return Math.sqrt(this.x*this.x + this.y*this.y + this.z*this.z); }

  // If we call that like a method with the typical OOP "dot" notation. This calls the
  // function in the object context of `vec1`:
  print("vec1.abs() == " + vec1.abs());

  //
  // To show that `this` is the global object for normal functions:
  //
  a_global_var = "that is also a field of the `global` object";

  function a_normal_function() {
    print("a_normal_function(): global.a_global_var == '" + global.a_global_var + "'");
    print("a_normal_function(): this.a_global_var == '" + this.a_global_var + "'");
  }
  a_normal_function(); //<- Not called like `obj.method()`, `this` will be the `global` object.
  global.a_normal_function(); // That is the same as the call above.
```

The dynamic usage of functions is an essential part of the language, especially for flexible
processing of data. Therefore, you also see functions without names all over the place, often
named "anonymous functions" or "lambda functions" (careful, ES6 has a short syntax for lambda
expressions, see example comment).

```js
  // @example function-anonymous
  // This very often used to properly encapsulate the scope of variables
  // to the current file. An anonymous function is defined and called
  // directly. To make the interpreter accepting the syntax, parenthesis
  // have to be added around the function. The trailing parenthesis are
  // the actual function call without arguments:
  (function(){
    var this_is_only_here = true;
  })();

  // Data processing example. Simple array from 1 to 10, we want
  // every even number from that. Array has a method called `filter`.
  // It expects a function that returns `true` or `false`. All `true`
  // values are kept, all `false` values dropped. All we need to do
  // is checking for "even", aka: value modulo 2 is zero:
  var all_values = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
  var only_even = all_values.filter(function(val){ return val % 2 == 0 });
  print("all_values = " + all_values);
  print("only_even = " + only_even);

  // Similarly, `fs.read()` can read a file, and in-place filter lines
  // while reading:
  const markdown_header2 = fs.read(
    // argument 1: file to read
    "../js-tutorial.md",
    // argument 2: object with special (documented) settings. `filter`
    // is used for the line filter function:
    {
      filter: function(line) {
        // The regex looks for "##" at the start of the line, but not more "#".
        // It returns the index where the match was found. That can be only 0 here,
        // otherwise it would not be the start of the line. String.search() returns
        // -1 if there was no match at all:
        return line.search(/^##[^#]/) == 0;
      }
    }
  );
  print("markdown_header2 =", JSON.stringify(markdown_header2) );

  // ES6 syntax for lambda expressions would be:
  //  (x)=>{return x+x;}
  // ... or even shorter as expression:
  //  (x)=>x+x
  // ... but these do not have the same variable scoping
  //     like `function(x){return x+x;}`.
```

### Objects and Inheritance

JS provides object oriented programming with dynamic polymorphism. This inheritance design is
very small and simple, and called "prototype based inheritance". (Aside: Later in ES6 the `class`
keyword was added to make the language more looking like the usual OOP suspects - ES5 has only
prototype based inheritance).

In short, instead of separating classes and objects, JS uses only objects, and you can simply
"assign" a parent object to a child object. If you try to access a property in the child that is
not there, then the interpreter will look in the parent object for that property (also the grand
parent etc). It stops if parent is `null`, and then it complains that the property is undefined.
This is called going up the "prototype chain". The parent of an object can be checked with the
`__proto__` property.

To mark the point why it is desireable to have classes/prototypes. Presume we have a simple
storage box with a maximum capacity and the number of items stored in it. Additionally we like
some methods to check if it's full or empty.

```js
  // @example prototype-inheritance1

  const storage1 = {
    amount: 10,
    capacity: 20,
    isEmpty: function() { return this.amount <= 0; },
    isFull: function() { return this.amount >= this.capacity; },
    toString: function() { return "(storage:" + this.amount + "/" + this.capacity + ")"; }
  };

  print( "Initial storage1 = " + storage1.toString() );
  print( "Initial storage1.isEmpty() = " + storage1.isEmpty() );
  print( "Initial storage1.isFull() = " + storage1.isFull() );
  storage1.amount = 20;
  print( "Changed storage1 = " + storage1.toString() );
  print( "Changed storage1.isEmpty() = " + storage1.isEmpty() );
  print( "Changed storage1.isFull() = " + storage1.isFull() );
```

We can see room for improvement, especially if more than one of these boxes are used. Let's shift
out the common stuff in a parent object, and then use that as prototype for a few actual boxes.

```js
  // @example prototype-inheritance2

  // Commonly used elements into the prototype:
  const storage_proto = {
    capacity: 20,
    isEmpty:  function() { return this.amount <= 0; },
    isFull:   function() { return this.amount >= this.capacity; },
    toString: function() { return "(storage:" + this.amount + "/" + this.capacity + ")"; }
  };

  // Make some boxes:
  const box0 = { amount:  0 };
  const box1 = { amount: 10 };
  const box2 = { amount: 20 };

  // Set the parent object.
  box0.__proto__ = storage_proto;
  box1.__proto__ = storage_proto;
  box2.__proto__ = storage_proto;

  // Print, note that `string + box` will automatically look for `toString()`
  print("box0 == " + box0);
  print("box1 == " + box1);
  print("box2 == " + box2);
```

Although marking the point, this is still tedious, and rarely used. A common way to create objects
is to use *functions as constructors*. JS also has the `new` keyword like other OOP languages.
But instead of instantiating a class, an empty object is created, and then the constructor called
in the object context of this empty object. We use the `this` keyword to access (and define) the
properties of our object:

```js
  // @example constructor-function

  function StorageBox(amount)
  {
    // Data
    this.amount = amount || 0; // default for undefined, no detailed error checks.
    this.capacity = 20;

    // Methods
    this.toString = function() {
      return "(storage:" + this.amount + "/" + this.capacity + ")";
    }

    // The function implicitly returns `this`.
  }

  const box0 = new StorageBox();
  const box1 = new StorageBox(10);
  const box2 = new StorageBox(20, "ignored");
  print("box0 == " + box0);
  print("box1 == " + box1);
  print("box2 == " + box2);
```

And again, let's shift out the common parts we like to have the same capacity and methods for all
boxes. Instead of directly using `__proto__`, JS allows to lookup a special property of the
constructor function object, called `prototype`. (Remember, functions are objects? So you can also
add properties to them as you like).

```js
  // @example constructor-prototype

  function StorageBox(amount)
  {
    this.amount = amount || 0;
  }

  StorageBox.prototype.capacity = 20;

  StorageBox.prototype.toString = function() {
    var s = "(";
    s += "" + this.amount + "/" + this.capacity;
    if(this.isFull()) {
      s += "|full"
    } else if(this.isEmpty()) {
      s += "|empty"
    }
    s += ")";
    return s
  };

  StorageBox.prototype.isEmpty = function() {
    return this.amount <= 0;
  };

  StorageBox.prototype.isFull = function() {
    return this.amount >= this.capacity;
  };

  // Instantiate a bunch:
  const boxes = [
    new StorageBox(),
    new StorageBox(10)
  ];

  print("boxes ==", boxes); // -> (0/20|empty),(10/20)

  // Changing the capacity for box 0. This does **not** change the
  // prototype capacity. Instead, the object gets its own capacity.
  boxes[0].capacity = 10;
  print("boxes ==", boxes); // -> (0/10|empty),(10/20)

  // Changing the proto capacity does not affect the box0 anymore,
  // it still has the individual `capacity` assigned above, so the
  // interpreter does not need to look down the prototype chain.
  // For box1 it has to, so we see the `5` there.
  StorageBox.prototype.capacity = 5;
  print("boxes ==", boxes); // -> (0/10|empty),(10/5|full)
```

Finally, to emphasize that the `Function.prototype` is no magic, but simply an *object* that is
used as default prototype for objects created with `new Function();`. You can any kind of "static"
functions to function objects. As long as you don't put it in the `prototype`, they are not used as
methods of the objects. This can be quite useful e.g. for factories:

```js
  // @example factories

  // Presume the same as above, but shorten that a bit:
  function StorageBox(amount) {
    this.amount = amount || 0;
  }

  StorageBox.prototype.capacity = 20;

  StorageBox.prototype.toString = function() {
    return ""+this.amount;
  }

  // This is like a "static" function of a class StorageBox:
  StorageBox.createRandom = function() {
    const cap = StorageBox.prototype.capacity;
    const rnd = Math.min(cap, Math.floor(Math.random() * (cap+1)));
    return new StorageBox(rnd);
  }

  // Check:
  const boxes = [];
  for(var i=0; i<10; ++i) {
    boxes.push( StorageBox.createRandom() );
  }
  print("boxes=", boxes);
```

### Language Built-In Objects

#### Math

Math contains a set of standard numerical functions known from most languages. A quick reference
of the most important functions is shown in the listing below.

See https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Math/random.

```js
  // @example math
  const x = 0;

  // Value clamping/limiting:
  Math.min(-4, 2);    // -> -4
  Math.max(-4, 2);    // -> 2

  // Rounding and integer conversion
  Math.round(3.5);    // -> 4, rounds towards the nearest integer.
  Math.floor(3.99);   // -> 3, rounds down.
  Math.floor(-3.3);   // -> -4 (also for negative numbers).
  Math.ceil(3.1);     // -> 4, rounds up, also for negative numbers.

  // Random double between 0.0 and almost 1.0.
  Math.random();      // -> 0 .. 0.999999...
  Math.floor(Math.random() * 6) +1; // -> 1..6 (dice).

  // Absolute value (removes sign)
  Math.abs(-5);       // -> 5
  Math.abs( 5);       // -> 5, already positive.

  // Root
  Math.sqrt(9);       // -> 3, square root, 3*3 == 9.

  // Sign check
  Math.sign(1);       // -> 1
  Math.sign(0);       // -> 0
  Math.sign(-1);      // -> -1

  // Exponential
  Math.pow(2, 10);    // -> 1024, 2^10
  Math.exp(1);        // -> 2.71828182 === e
  Math.log(1);        // -> 0, natural logarithm (base e).
  Math.log2(1024);    // -> 10, base 2 logarithm, 2^10==1024.
  Math.log10(100);    // -> 2, base 10 logarithm, 10^2==100.

  // Trigonometric
  Math.sin(x)         // -> 0, x is an angle in radiants.
  Math.cos(x)         // -> 1, x is an angle in radiants.
  Math.tan(x)         // -> 0, x is an angle in radiants.
  Math.asin(0)        // -> 0
  Math.acos(1)        // -> pi/2
  Math.atan(0)        // -> 0
```

#### Date

Date is an accessor object to parse, compose, get components of calendar date/time information.
Internally, JS stores dates as 64bit number that represent UNIX timestamps *in milliseconds*.

See https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date.

```js
  // @example date

  // Date with actual time: Either using `new`, or `now()`.
  const date1 = new Date();           // Constructed with the current date/time.
  const date2 = Date.now();           // Same, date2 >= date1 (millisecond differences).

  // Date by a set value: Parse or constructor.
  const date3 = new Date("2011-11-11T11:11:11.111"); // using ISO string.

  // Get unix timestamp
  const ts = date3.valueOf() / 1000;  // uts with millisecond resolution.
  const iso = date3.toISOString();    // ISO 8601.

  // Check
  print("date3 ==", date3);           // -> 2011-11-11 12:11:11.111+01:00
  print("ts ==", ts);                 // -> 1321009871.111
  print("iso ==", iso);               // -> 2011-11-11T11:11:11.111Z

  // Components, also setters available.
  print("year    ==", date3.getFullYear());
  print("month   ==", date3.getMonth());
  print("day     ==", date3.getDate());
  print("weekday ==", date3.getDay());
  print("hours   ==", date3.getHours());
  print("minutes ==", date3.getMinutes());
  print("seconds ==", date3.getSeconds());
  print("ms      ==", date3.getMilliseconds());
```

#### RegExp

Regular expressions are a central part of text processing in JS. RegExp look very similar to
the ones known from Pearl, also with a dedicated literal (`/pattern/flags`). Exhaustively going
into the details would be beyond the scope of this brief introduction, we only cover some
example usages. Regular expressions are normally used in `String` methods.

See https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp.

```js
  // @example regex
  const str1 = " 1: A line\n2: The_second-line~with~win32*newline \r\n 3: End of\ttext \n";

  // Get an array with all lines of a string. Unix and windows line endings accepted.
  // The `?` means "match \r one time or 0 times.".
  const lines = str1.split(/\r?\n/);
  print("lines ==", JSON.stringify(lines));
  // -> [" 1: A line","2: The_second-line~with~win32*newline "," 3: End of\ttext ",""]


  // Remove all non-word characters from a string. The `/g` at the end means "global"
  // (do not stop at the first match). The `\w` is a short for all word characters, which
  // are '0'..'9', 'a'..'z', 'A'..'Z', and '_' (everything that can occur in variable names).
  // The `[^  ]` means "all characters that do NOT match.". So we replace globally all chars,
  // which are not word characters, with nothing ("").
  const only_wc = str1.replace(/[^\w]/g, "");
  print("only_wc ==", only_wc);
  // -> 1Aline2The_secondlinewithwin32newline3Endoftext


  // String `search()` returns the position of the first match (0 to string.length-1), or
  // `-1` if there is no match. The `/i` flag means "ignore case". So, this will return the
  // position of "text\n" in line 3.
  const index_of_text = str1.search(/TEXT/i);
  print("index_of_text ==", index_of_text);
  // -> 62


  // Replace trim leading and trailing whitespaces in lines. Parentheses `()` group patterns
  // and make them "fetchable" using `$1` later. The flags `/mg` mean "multiline and global".
  const trimmed = str1.replace(/(^|\n)[\t ]+/mg, "$1").replace(/[\t ]+(\r|\n|$)/mg, "$1");
  print("trimmed ==", JSON.stringify(trimmed));
  // -> "1: A line\n2: The_second-line~with~win32*newline\r\n3: End of\ttext\n"
```

#### JSON

JSON parsing and composing utility. Frequently used.

```js
  // @example json
  const object1 = { num:1, str:"A string", arr:[1,2,3] };
  // Serialize:
  const serialized = JSON.stringify(object1);
  // Unserialize:
  const unserialized = JSON.parse(serialized);

  print("serialized =", serialized);    // -> {"num":1,"str":"A string","arr":[1,2,3]}
  print("object1.str =", object1.str);
  print("unserialized.str =", unserialized.str);
```

#### ---
