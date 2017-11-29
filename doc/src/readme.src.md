# C++ wrapper templates around the Duktape JavaScript Engine (`duktape-cc`)

Duktape is, similar to LUA and TCL, a programming language implementation
optimised for resource restricted platforms, as well as embedding scripting
capabilities into native applications (for details please refer to
[duktape.org](http://duktape.org/) and the corresponding Github repository
[https://github.com/svaarala/duktape](https://github.com/svaarala/duktape)).


The set of c++ templates in this repository facilitates embedding the Duktape
ECMA script engine into C++ applications by providing

  - a thin wrapper around the C-API of Duktape (`duktape::api` class),

  - type conversion traits from JavaScript types to C++ types and STL
    containers,

  - a `duktape::engine` class allowing to declare constants, variables,
    and native functions in the JS context, as well as evaluating script
    code, and including `js` files. The class also provides template based
    "automatic wrapping" of c++ functions and implicit selection of arguments
    passed to the ECMA context depending on the types in c++.

  - optional modules for Linux/BSD/Windows for `stdio`, file system
    operations, executing programs with `stdout`/`stderr`/`stdin` piping,
    basic user and system information, etc.
    All module features, including basic functionality like `exit()`,
    `print()`, `alert()` and the like, are optional and selectable either
    individually or as whole module.


The features of the optional (native c++) modules are, in contrast to the
way JavaScript is mostly used, *NOT* implemented promise/event based. Instead
the functions and methods are blocking until they return. Callbacks are only
used for filtering while reading larger files or the `stdout` of an executed
process (please see the "notes" section for details).


## Building

A good starting point is to take a look at the Makefile, the CLI application
`main.cc` and the tests. The directory structure looks like:

      duktape-cc/
      ├── cli
      │   ├── example.cc
      │   └── main.cc
      ├── doc
      │   ├── readme.md
      │   └── stdmods.js
      ├── duktape
      │   ├── duk_config.h
      │   ├── duktape.c
      │   ├── duktape.h
      │   ├── duktape.hh
      │   └── mod
      │       ├── mod.fs.hh
      │       ├── [...]
      ├── Makefile
      ├── readme.md
      └── test
          ├── 0001-include
          │   ├── test.cc
          │   └── test.js
          └── [...]

Build notes:

- `duk_config.h`, `duktape.h` and `duktape.c` are the required release
  source code files of the Duktape project (other files in the package
  omitted in this file tree). These files are now included in this
  repository to facilitate building under Windows (without the need of
  missing UNIX tools like wget and xz).

Invoking `make` produces the CLI application as default target:

      $ make
      [c++ ] cli/main.cc  cli/main.o
      [c++ ] duktape/duktape.c  duktape/duktape.o
      [ld  ] cli/main.o duktape/duktape.o  cli/js
      [note] binary is cli/js

      # Quick test using inline eval:
      $ ./cli/js -e 'var a=10; print("-> a is " + a + ".");'
      -> a is 10.

`duktape.c` is compiled with the c++ compiler and `DUK_USE_CPP_EXCEPTIONS` must
be defined (use `throw` instead of `setlongjmp`). This is important because
otherwise the support for c++ exceptions is missing. Compiling manually looks
like

      g++ -c -o cli/main.o cli/main.cc -std=c++11 -Iduktape -I.
      g++ -c -o duktape/duktape.o duktape/duktape.c -std=c++11 -DDUK_USE_CPP_EXCEPTIONS
      g++ -o cli/js cli/main.o duktape/duktape.o -lm -lrt

(omitted flags that are also used in the Makefile: `-W` `-Wall` `-Wextra`
`-pedantic` `-Os` `-fomit-frame-pointer` `-fdata-sections` `-ffunction-sections`).

## Integrating JS into the C++ application

The `example.cc` shows the main features in detail, at this point
only the "basic implementation style" is briefly depicted.

```c++
#include <duktape/duktape.hh>

using namespace std;

// A native c++ wrapped function
bool native_cpp_function(int a, int b) { return a == b; }

// A duktape c++ API wrapped function
int argument_count1(duktape::api& stack)
{ stack.push(stack.top()); return 1; }

// A Duktape C function
::duk_ret_t argument_count2(::duk_context *ctx)
{ ::duk_push_int(ctx, ::duk_get_top(ctx)); return 1; }

int main(int argc, const char** argv)
{
  // The JS engine with the Duktape heap.
  duktape::engine js;

  // Defining constants and functions in the JS context
  js.define("my");                      // Plain object
  js.define("my.version_major", 1);     // Number
  js.define("my.version_minor", 2.0);   // Number
  js.define("my.builddate", __DATE__ " " __TIME__); // String
  js.define("isEqual", native_cpp_function);
  js.define("my.numArgs1", argument_count1);
  js.define("my.numArgs2", argument_count2);

  // Evaluating code
  cout << js.eval<int>("my.numArgs1(1,2,3,4)") << endl; // --> 4
  cout << js.eval<bool>("isEqual(1,2)") << endl; // --> false

  // Call functions in JS from c++
  cout << js.call<string>("isEqual", 100, 100) << endl; // --> "true"

  // Include script file and return last statement result
  string result = js.include<string>("myscript.js");

  // Exception handling from jJS in C++
  try {
    js.eval<void>("throw new Error('not good');");
  } catch(const duktape::script_error& e) {
    cout << "Caught '" << e.what() << "'" << endl; // --> Caught 'Error: not good'
  }
  return 0;
}
```

## Notes, development context, road map, etc ...

- This project does not intend to be a base for another node.js
  or the like. The focus is set on fast integration. Main application
  fields have been up to now:

    - Implementing scriptable testing applications for libraries
      written in c++.

    - Adding programmatic configuration to an application.

    - Adding scripting hooks/callbacks to C++ applications.

- Duktape was a great help for me when testing functionality in
  embedded systems. During the development and evaluation of this
  class template set (especially the modules) it turned out that
  sequential script execution (which stands in big contrast to e.g.
  node.js and browser ECMA) did lead to small and well readable
  script codes - which is for "tool control language" use cases
  ideal. Also colleagues in the C/C++/Java world like it because
  the basic "code layout" (curly braces, brackets etc) looks similar
  to the latter languages - hence, "it has become our TCL or LUA".

- About conversions of types between JS and C++: The intension is
  to stick with the STL and adding traits for modules only where it
  is needed (but in the modules, not the main duktape.hh). Traits
  are always provided for

    - Numeric types
    - `std::string`       -> String
    - `std::vector<...>`  -> Array

- Duktape release files

    - Initially the release files were separately downloaded
      and extracted from the `tar.xz` archive when invoking
      `make`, but unfortunately people keep having trouble
      with wget, tar and xz on Windows. So they are now included
      as the licenses of the projects are identical.


## List of available builtin functions and objects

[](!include doc/src/function-list.md)
