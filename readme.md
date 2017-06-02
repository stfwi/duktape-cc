# C++ wrapper templates around the Duktape JavaScript Engine (`duktape-cc`)

Duktape is, similar to LUA and TCL, a programming language implementation
optimised for resource restricted platforms, as well as embedding scripting
capabilities into native applications (for details please refer to
[duktape.org](http://duktape.org/) and the corresponding Github repository
[https://github.com/svaarala/duktape](https://github.com/svaarala/duktape)).


The set of templates in this repository facilitates embedding this ECMA
script engine in C++ applications by providing

  - a thin wrapper around the C-API of Duktape (`duktape::api` class),

  - type conversion traits from JavaScript types to C++ types and STL
    containers,

  - a `duktape::engine` class allowing to declare constants, variables,
    and native functions in the JS context, as well as evaluating script
    code, and including `js` files. The class also provides template based
    "automatic wrapping" of c++ functions and implicit selection of arguments
    passed to the ECMA context depending on the types in C++.

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
  omitted in this file tree). These files are not included in this
  C++ wrapper this repository, therefore the Makefile uses `wget` and
  `tar` to get and extract them first.
- On Windows machines you have `tar` and `wget` tools if `GIT` is installed
  globally. Make has to be downloaded (GNU make, version at least v4.0).
  On Linux/*nix the programs should be already there. If it does not work
  out download the package form `duktape.org` and put them in the right
  place.
- On BSD: `gmake` it is, not `pmake` (might work, too but no guarantees).

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

## Notes, development context, road map, todo's, etc ...

- This project does not intend to be a base for another nodejs
  or the like. The focus is set to fast integration. Main application
  have been up to now:

    - Implementing scriptable testing applications for libraries
      written in c++.

    - Adding programmatic configuration to an application.

    - Adding scripting hooks/callbacks to C++ applications.

- Duktape was a great help for me testing functionality in embedded
  systems. During the evaluation and of this class template set,
  especially the modules, it turned out that a sequential script
  execution and avoiding exceptions did lead to small and well
  understandable script codes. Therefore the module functions
  avoid to throw and indicate success or problems e.g. via boolean
  return (such as `if(!fs.chmod(file, mode)) handleit();`).

- According to conversions of types between JS and C++, the intension
  is to stick with the STL and adding traits for modules only where
  it is needed (but in the modules, not the main duktape.hh). Traits
  should be generally there for

    - [x] Numeric types
    - [x] `std::string`       -> String
    - [x] `std::vector<...>`  -> Array
    - [ ] `std::map<...>`,`std::unordered_map<...>`  -> Object

- Missing list

    - Maybe also `std::deque`. `map` is yet missing here, as it has to be
      crucially checked what to do with non-own-properties or `null` or
      unconvertible properties like ´function´ - and how recursion is dealt
      with.

    - [ ] Wrapping of C++ classes as JS objects: Maybe one day. It currently
      looks simpler and more straight forward to use the Duktape-API-way when
      defining constructors.

    - [ ] The test cases are yet somewhat rudimentary and cover only the main
      cases. Randomly generated inputs for better coverage are on the list.

    - [ ] With Visual Studio not tested at all yet.
