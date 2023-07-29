#include "../duktape/duktape.hh"
#include "../duktape/mod/mod.sys.hh"
#include "../duktape/mod/mod.stdio.hh"
#include "../duktape/mod/mod.stdlib.hh"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <locale>
#include <clocale>
#include <memory>


int main(int argc, char *argv[], const char* envv[])
{
  using namespace std;
  auto args = vector<string>();
  string function_selection = "none";
  try {
    locale::global(locale("C"));
    ::setlocale(LC_ALL, "C");
    duktape::engine js;
    duktape::mod::stdlib::define_in(js);
    duktape::mod::stdio::define_in(js);
    duktape::mod::system::define_in(js);
    if(argv) {
      for(int i=1; i<argc && argv[i]; ++i) args.emplace_back(argv[i]);
      js.define("sys.args", args);
      if(args.size() > 0) {
        function_selection = args.front();
        replace(function_selection.begin(), function_selection.end(), '-', '_');
      }
    }
    if(envv) {
      js.define("sys.env");
      auto& stack = js.stack();
      duktape::stack_guard sg(stack, true);
      stack.select("sys.env");
      for(int i=0; envv[i]; ++i) {
        const auto e = string(envv[i]);
        const auto pos = e.find('=');
        if((pos != e.npos) && (pos > 0)) {
          string key = e.substr(0, pos);
          string val = e.substr(pos+1);
          stack.set(std::move(key), std::move(val));
        }
      }
    }
    try {
      js.include("taux.js");
      return js.call<int>("command_" + function_selection, args); // invoke `function command_####(...)`
    } catch(duktape::exit_exception& e) {
      cout << string("Exit with code ") << to_string(e.exit_code()) << "'.\n";
    } catch(duktape::script_error& e) {
      cout << "Unexpected script error: " << e.callstack() << "\n";
    } catch(duktape::engine_error& e) {
      cout << "Unexpected engine error: " << e.what() << "\n";
    } catch(exception& e) {
      cout << "Unexpected exception: '" << e.what() << "'.\n";
    } catch (...) {
      cout << "Unexpected exception.\n";
    }
  } catch(exception& e) {
    cout << "Unexpected init exception: '" << e.what() << "'.\n";
  } catch (...) {
    cout << "Unexpected init exception.\n";
  }
  return 1;
}