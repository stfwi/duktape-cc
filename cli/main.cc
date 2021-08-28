/**
 * @file main.cc
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.5
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++17 -W -Wall -Wextra -pedantic -fstrict-aliasing
 */
#include <duktape/duktape.hh>
#include <duktape/mod/mod.stdio.hh>
#include <duktape/mod/mod.stdlib.hh>
#include <duktape/mod/mod.fs.hh>
#include <duktape/mod/mod.fs.ext.hh>
#include <duktape/mod/mod.fs.file.hh>
#include <duktape/mod/mod.sys.hh>
#include <duktape/mod/mod.sys.exec.hh>
#include <duktape/mod/mod.sys.hash.hh>
#include <duktape/mod/ext/app_attachment/mod.ext.app_attachment.hh>
#include <duktape/mod/ext/serial_port/mod.ext.serial_port.hh>
#include <duktape/mod/ext/conv/mod.conv.hh>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>
#include <locale>
#include <clocale>

#ifndef PROGRAM_NAME
  #define PROGRAM_NAME "djs"
#endif

#ifndef PROGRAM_VERSION
  #define PROGRAM_VERSION "v1.0"
#endif

/**
 * Application main().
 */
int main(int argc, const char** argv, const char** envv)
{
  using namespace duktape;
  using namespace std;

  string script_path, script_code, eval_code, lib_code;
  vector<string> args;
  bool has_verbose = false;

  // Application input processing
  try {
    locale::global(locale("C"));
    ::setlocale(LC_ALL, "C");
    // Command line arguments
    {
      bool was_last_opt = false;
      bool has_file_arg = false;
      // Rudimentary handling of application args before passing on (without library deps).
      for(int i=1; i<argc && argv[i]; ++i) {
        string arg = argv[i];
        if(was_last_opt || has_file_arg) {
          args.emplace_back(move(arg));
        } else if(arg == "--") {
          was_last_opt = true;
        } else if(arg == "--help") {
          cerr << "NAME" << endl << endl
               << "  " << PROGRAM_NAME << endl << endl
               << "SYNOPSIS" << endl << endl
               << "  " << PROGRAM_NAME << " [ -h ] [ -e '<code>' | -s <script file> ] [--] [script arguments]" << endl << endl
               << "DESCRIPTION" << endl << endl
               << "  Evaluate javascript code pass via -e argument, via script " << endl
               << "  file, or via piping into stdin." << endl << endl
               << "ARGUMENTS" << endl << endl
               << "       --help         : Print help and exit." << endl
               << "  -e | --eval <code>  : Evaluate code given as argument. Done after loading" << endl
               << "                        a file (or stdin)." << endl
               << "  -s | --script <file>: Optional explicit flag for <script file> shown below." << endl
               << "  <script file>       : (First positional argument). A javascript file to" << endl
               << "                        load and run or - (dash) for piping in from stdin" << endl
               << "  --                  : Optional separator between program options and" << endl
               << "                        script options/arguments. Useful if e.g. '-e'" << endl
               << "                        shall be passed to the script instead of evaluating." << endl
               << "  script arguments    : All arguments after '--' or the script file are passed" << endl
               << "                        to the script and are there available as the 'sys.args'" << endl
               << "                        array." << endl << endl
               << "EXIT CODE" << endl << endl
               << "  0=success, other codes indicate an error, either from a script exception or " << endl
               << "                       from binary program error." << endl << endl
               << (PROGRAM_NAME) << " " << (PROGRAM_VERSION) << ", (CC) stfwi 2015-2020, lic: MIT" << endl
               ;;
          return 1;
        } else if((argc == 2) && ((arg == "--version") || (arg == "-v"))) {
          cout << "program: " << PROGRAM_NAME << endl << "version: " << PROGRAM_VERSION << endl;
          return 0;
        } else if(arg == "-e" || arg == "--eval") {
          if((++i >= argc) || (!argv[i]) || ((arg=argv[i]) == "--")) {
            cerr << "No code after '-e/--eval'" << endl;
            return 1;
          } else {
            eval_code = arg;
          }
        } else if(arg == "-v" || arg == "--verbose") {
          has_verbose = true;
        } else if(arg == "-s" || arg == "--script") {
          if((++i >= argc) || (!argv[i]) || ((arg=argv[i]) == "--")) {
            cerr << "No script file after '-s/--script'" << endl;
            return 1;
          } else {
            has_file_arg = true;
            script_path = arg;
          }
        } else if((!has_file_arg) && (arg.length() > 0) && (arg[0] != '-')) {
          has_file_arg = true;
          script_path = arg;
        } else {
          args.emplace_back(move(arg));
        }
      }
    }
    // Command line specified code
    if(script_path == "-") {
      script_path = "(piped code)";
      script_code.assign((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
      if(!cin.good() && !cin.eof()) throw std::runtime_error("Failed to read script from stdin.");
      if(script_code.empty()) throw std::runtime_error("Script to execute is empty.");
    } else if(!script_path.empty()) {
      if(script_path[0] == '-') throw std::runtime_error(std::string("Expected script file as positional argument, found option '") + script_path + "'");
      std::ifstream fis(script_path);
      script_code.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
      if(!fis.good() && !fis.eof()) throw std::runtime_error(std::string("Failed to read script '") + script_path + "'");
      if(script_code.empty()) throw std::runtime_error("Script to execute is empty");
    }
    // Remove leading #!/bin/whatever
    if((script_code.length() > 2) && (script_code[0] == '#') && (script_code[1] == '!')) {
      for(auto& e:script_code) {
        if((e == '\n') || (e == '\r')) break;
        e = ' ';
      }
    }
  } catch(const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  }

  // Script init/run.
  try {
    duktape::engine js;
    // Module imports
    {
      duktape::mod::stdlib::define_in(js);
      duktape::mod::stdio::define_in(js);
      duktape::mod::filesystem::generic::define_in(js);
      duktape::mod::filesystem::basic::define_in(js);
      duktape::mod::filesystem::extended::define_in(js);
      duktape::mod::filesystem::fileobject::define_in(js);
      duktape::mod::system::define_in(js);
      duktape::mod::system::exec::define_in(js);
      duktape::mod::system::hash::define_in(js);
      duktape::mod::ext::conv::define_in(js);
      duktape::mod::ext::serial_port::define_in(js);
    }
    // Built-in constant definitions.
    {
      // Base application context constants.
      js.define("sys.app.name", PROGRAM_NAME);
      js.define("sys.app.version", PROGRAM_VERSION);
      js.define("sys.app.path", js.call<string>("fs.dirname", js.call<string>("fs.realpath", string(argv[0]))));
      js.define("sys.args", args);
      js.define("sys.script", script_path);
      js.define("sys.scriptdir", js.eval<string>("sys.app.path"));
      // Overwritable values:
      js.define_flags(duktape::engine::defflags::configurable|duktape::engine::defflags::writable|duktape::engine::defflags::enumerable);
      js.define("sys.app.verbose", has_verbose);
      js.define("sys.env");
      #ifndef CONFIG_WITHOUT_ENVIRONMENT_VARIABLES
      {
        // Environment can be opt'ed out using the compiler switch.
        const auto add_environment_variables = [](duktape::engine& js, const char** envv) {
          if(envv) {
            auto& stack = js.stack();
            duktape::stack_guard sg(stack, true);
            stack.select("sys.env");
            for(int i=0; envv[i]; ++i) {
              const auto e = string(envv[i]);
              const auto pos = e.find('=');
              if((pos != e.npos) && (pos > 0)) {
                string key = e.substr(0, pos);
                string val = e.substr(pos+1);
                stack.set(move(key), move(val));
              }
            }
          }
        };
        if(envv) {
          auto& stack = js.stack();
          duktape::stack_guard sg(stack, true);
          stack.select("sys.env");
          for(int i=0; envv[i]; ++i) {
            const auto e = string(envv[i]);
            const auto pos = e.find('=');
            if((pos != e.npos) && (pos > 0)) {
              string key = e.substr(0, pos);
              string val = e.substr(pos+1);
              stack.set(move(key), move(val));
            }
          }
        }
        add_environment_variables(js, envv);
      }
      #endif
    }
    // Externally specified modules (e.g. via `-include` compiler option)
    {
      #ifdef CONFIG_ADDITIONAL_FEATURE_INITIALIZATION
        CONFIG_ADDITIONAL_FEATURE_INITIALIZATION(js);
      #endif
    }
    // Script execution
    {
      // Built-in library, if available, is executed first.
      #ifdef CONFIG_WITH_APP_ATTACHMENT
      const bool has_lib = duktape::mod::ext::app_attachment::define_in(js);
      #else
      constexpr bool has_lib = false;
      #endif
      if((!has_lib) && eval_code.empty() && (script_code.empty())) {
        throw std::runtime_error("No js file specified/piped in, and no code to evaluate passed. Nothing to do");
      }
      // -s/--script (or positional) script file executed 2nd.
      js.eval<void>(script_code, script_path);
      string().swap(script_code);
      // If the script or built-in library has a function main(args){}, it is invoked next.
      int exit_code = 0;
      if(js.stack().select("main") && js.stack().is_callable(-1)) {
        js.stack().top(0);
        exit_code = js.call<int>("main", args);
      }
      // -e/--eval inline code runs after everything else.
      if(!eval_code.empty()) {
        js.eval<void>(eval_code, "(inline eval code)");
      }
      return exit_code;
    }
  } catch(const duktape::exit_exception& e) {
    return e.exit_code();
  } catch(const duktape::script_error& e) {
    cerr << e.callstack() << endl;
    return 1;
  } catch(const duktape::engine_error& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch(const std::exception& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch(std::exception* e) {
    cerr << "Fatal: EXCEPTION THROWN BY POINTER FROM NATIVE CODE (REMOVE NEW):" << e->what() << endl;
    // let the kernel clean after us this time.
    return 1;
  } catch (...) {
    throw; // maybe the c++ frame has an error message.
  }
  return 1;
}
