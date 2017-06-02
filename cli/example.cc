/**
 * @file example.cc
 * C++ integration example for the  Duktape engine c++ interface.
 */
// This one is always needed
#include <duktape/duktape.hh>
// These are optional and used in this example
#include <duktape/mod/mod.stdio.hh>
#include <duktape/mod/mod.stdlib.hh>
// STL
#include <vector>
#include <string>

using namespace std;

/**
 * Native function to be used in the JS context, this one
 * expects a Number and returns a Boolean.
 * If the argument is not convertible to `unsigned long`
 * the wrapper will throw an exception in the JS runtime.
 *
 * @param unsigned long number
 * @return bool
 */
bool is_prime(unsigned long number)
{
  if(number == 0) return false;
  if(number == 1) return true;
  unsigned long end = static_cast<unsigned long>(sqrt(number));
  for(unsigned long i=2; i<=end; ++i) {
    if(number % i == 0) return false;
  }
  return true;
}

/**
 * This is the more flexible, but also longer way to
 * integrate C++ functionality. It uses the wrapper
 * around the Duktape API. The functions are named
 * like in Duktape, but some can be omitted (duplicates
 * or covered by the conversion, such as lstring functions
 * etc).
 */
int readfile(duktape::api& stack)
{
  if(stack.top() == 0) {
    // stack.top() is basically duk_get_top(ctx)
    return stack.throw_exception("No file given");
  } else if(
    // If a corresponding C++ type conversion exists
    // for the JS type, then the is<> template can be used.
    // `stack.is<string>(0)` is equivalent to `duk_is_string(ctx, index)`.
    stack.is<string>(0)
  ) {
    // Same with exceptions, `stack.throw_exception()` uses `duk_error_raw()`.
    return stack.throw_exception("A file path should be a String.");
  }

  // Get string, stack index 0
  string path = stack.get<string>(0);

  // Read file in c++
  string contents;
  std::ifstream fis(path.c_str(), std::ios::in);
  contents.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
  if(!fis.good() && !fis.eof()) {
    return stack.throw_exception("Failed to read file.");
  }

  // stack.push() is a template and uses the conversion trait from std::string
  // to JS String.
  stack.push(contents);

  // Duktape convention: 1 == we have a return value.
  return 1;
}

/**
 * And yes, the normally known Duktape `duk_c_function`s
 * can be used, too.
 */
::duk_ret_t argument_count(::duk_context *ctx)
{ ::duk_push_int(ctx, ::duk_get_top(ctx)); return 1; }


/**
 * Main
 *
 * @param int argc
 * @param const char** argv
 * @return int
 */
int main(int argc, const char** argv)
{
  try {
    //
    // Create Duktape context in js. This object
    // is also the main "access point" to the
    // functionality of the ECMA interpreter.
    // On destruction it also cleans up the
    // Duktape context/heap.
    //
    duktape::engine js;

    // We are lazy and use `print()`, `alert()` etc from
    // the stdio module. All modules should have a
    // `define_in()` function to simply say "add all
    // you have, and where it's right by default".
    duktape::mod::stdio::define_in(js);

    // Add a constant to the script context, here an
    // array of strings (vector is converter to Array).
    // E.g. the program arguments.
    {
      vector<string> args;
      for(int i=1; i<argc && argv[i]; ++i) args.push_back(argv[i]);
      js.define("sys.arguments", args);
    }

    // Define some other constants in the JS context
    js.define("my");                      // Plain object
    js.define("my.version_major", 1);     // Number
    js.define("my.version_minor", 2.0);   // Number
    js.define("my.builddate", __DATE__ " " __TIME__); // String

    //
    // Define native C/C++ functions in the JS context
    //
    js.define("my.isPrime", is_prime);        // from above (native c++)
    js.define("my.readfile", readfile);       // from above (Duktape API C++ wrapper function)
    js.define("my.argumentCount", argument_count);  // from above (Duktape C function)
    js.define("sys.exit", duktape::mod::stdlib::exit_js); // picked from a module

    //
    // Evaluate code. The result of the last operation/statement is
    // returned and converted to the C++ type of choice.
    //
    cout << "my.isPrime(10) = " << js.eval<bool>("my.isPrime(10)") << endl;   // Evaluate, convert to bool
    cout << "my.isPrime(11) = " << js.eval<string>("my.isPrime(11)") << endl; // Evaluate, convert to string

    //
    // Calling JS functions with arguments
    // Here our native function is called from c++ in the JS context
    // the result is represented in c++ as bool.
    //
    cout << "Prime numbers:";
    for(int i=0; i<100; ++i) {
      if(js.call<bool>("my.isPrime", i)) {
        cout << " " << i;
      }
    }
    cout << endl;

    //
    // Catching JS exeptions
    //
    try {
      // That will throw because we did not specify a file path.
      js.eval<void>("my.readfile()");
    } catch(const duktape::script_error& e) {
      cout << "Caught JS exception: '" << e.what() << "'" << endl;
      // --> Caught JS exception: 'Error: No file given'
    }

    //
    // Including JavaScript files. The result of the last statement
    // is returned.
    //
    js.include("example.js"); // void return (we don't care about the return here).
    cout << "js.include(\"example.js\") returned: '"
         << js.include<string>("example.js") << "'" << endl;

    //
    // Notes
    //
    // All constants you define() are by default not writable from JS code ...
    // To allow things you "define()" to be writable, change the define_flags.
    js.eval("my = 10");
    cout << "(my === 10) == " << js.eval<bool>("my === 10") << endl; // == false
    cout << "(typeof(my)) is '" << js.eval<string>("typeof(my)") << "'" << endl; // == still object
    js.define_flags(duktape::engine::defflags::writable); // Subsequent defines are writable
    js.define("my.nonconst", 10);
    cout << "original my.nonconst == " << js.eval<int>("my.nonconst") << endl; // == 10
    js.eval("my.nonconst = 100");
    cout << "altered  my.nonconst == " << js.eval<int>("my.nonconst") << endl; // == now 100

    return 0;
  } catch(const duktape::exit_exception& e) {
    // The very special case the exit(CODE) was called from js, by default
    // this function is disabled, and you can enable it e.g. by including
    // <mod/mod.stdlib.hh>, and say "duktape::mod::stdlib: :define_in(js);"
    return e.exit_code();
  } catch(const duktape::script_error& e) {
    //
    // Script exceptions are propagated to c++ as the `script_error` exception,
    // derived from `std::runtime_error`. The message is what the JS exception
    // says.
    //
    cerr << "Error: " << e.what() << endl;
    return 1;
  } catch(const duktape::engine_error& e) {
    //
    // Engine errors, also derived from `std::runtime_error`, have the meaning
    // that something really went wrong in the engine. Means search for bugs
    // and race conditions.
    //
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch(const std::exception& e) {
    //
    // STL containers may throw e.g. when they are out of memory, so that should
    // be caught, too.
    //
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch (...) {
    //
    // Maybe the c++ frame has an error message for that.
    //
    throw;
  }
  return 1;
}

