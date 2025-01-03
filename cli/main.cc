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
#ifdef WITH_SOCKET
  // Special case socket: Normally opt'ed out, needs to be included
  // first under win32 (WSA header complains otherwise).
  #include <duktape/mod/mod.sys.socket.hh>
#endif
#include <duktape/duktape.hh>
#include <duktape/mod/mod.stdio.hh>
#include <duktape/mod/mod.stdlib.hh>
#include <duktape/mod/mod.fs.hh>
#include <duktape/mod/mod.fs.ext.hh>
#include <duktape/mod/mod.fs.file.hh>
#include <duktape/mod/mod.sys.hh>
#include <duktape/mod/mod.sys.exec.hh>
#include <duktape/mod/mod.sys.hash.hh>
#include <duktape/mod/mod.xlang.hh>
#include <duktape/mod/ext/mod.conv.hh>
#include <duktape/mod/ext/mod.ext.serial_port.hh>
#include <duktape/mod/ext/mod.ext.mmap.hh>
#ifdef WITH_APP_ATTACHMENT
  #include <duktape/mod/ext/app_attachment/mod.ext.app_attachment.hh>
#endif
#ifdef WITH_RESOURCE_IMPORT
  #include <duktape/mod/ext/mod.ext.resource_blob.hh>
#endif
#ifdef WITH_EXPERIMENTAL
  #include <duktape/mod/exp/mod.experimental.hh>
#endif
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

namespace {

  /**
   * Print text for `--help`
   */
  void print_help()
  {
    std::cerr
      << "NAME\n\n"
      << "  " << PROGRAM_NAME << "\n\n"
      << "SYNOPSIS" << "\n\n"
      << "  " << PROGRAM_NAME << " [ -h ] [ -e '<code>' | -s <script file> ] [--] [script arguments]\n\n"
      << "DESCRIPTION" << "\n\n"
      << "  Evaluate javascript code pass via -e argument, via script\n"
      << "  file, or via piping into stdin.\n\n"
      << "ARGUMENTS\n\n"
      << "       --help         : Print help and exit.\n"
      << "  -e | --eval <code>  : Evaluate code given as argument. Done after loading\n"
      << "                        a file (or stdin).\n"
      << "  -s | --script <file>: Optional explicit flag for <script file> shown below.\n"
      << "  <script file>       : (First positional argument). A javascript file to\n"
      << "                        load and run or - (dash) for piping in from stdin\n"
      << "  --                  : Optional separator between program options and\n"
      << "                        script options/arguments. Useful if e.g. '-e'\n"
      << "                        shall be passed to the script instead of evaluating.\n"
      << "  script arguments    : All arguments after '--' or the script file are passed\n"
      << "                        to the script and are there available as the 'sys.args'\n"
      << "                        array.\n\n"
      << "EXIT CODE\n\n"
      << "  0=success, other codes indicate an error, either from a script exception or\n"
      << "                       from binary program error.\n"
      << (PROGRAM_NAME) << " " << (PROGRAM_VERSION) << ", (CC) stfwi 2015-2020, lic: MIT\n"
      ;;
  }

  /**
   * Print text for `--version`
   */
  void print_version()
  {
    std::cout << "program: " << PROGRAM_NAME << "\nversion: " << PROGRAM_VERSION << "\n";
  }

}


/**
 * Application main().
 */
int main(int argc, const char** argv, const char** envv)
{
  using namespace duktape;
  using namespace std;

  string script_path, script_code, eval_code, lib_code;
  vector<string> args;
  unsigned arg_flags = 0;
  static constexpr unsigned arg_flag_verbose = 0x1;
  static constexpr unsigned arg_flag_version = 0x2;
  static constexpr unsigned arg_flag_help    = 0x4;

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
          args.push_back(std::move(arg));
        } else if(arg == "--") {
          was_last_opt = true;
        } else if((argc == 2) && (arg == "--help")) {
          arg_flags |= arg_flag_help;
        } else if((argc == 2) && ((arg == "--version") || (arg == "-v"))) {
          arg_flags |= arg_flag_version;
        } else if(arg == "-e" || arg == "--eval") {
          if((++i >= argc) || (!argv[i]) || ((arg=argv[i]) == "--")) {
            cerr << "No code after '-e/--eval'\n";
            return 1;
          } else {
            eval_code = arg;
          }
        } else if(arg == "-v" || arg == "--verbose") {
          arg_flags |= arg_flag_verbose;
        } else if(arg == "-s" || arg == "--script") {
          if((++i >= argc) || (!argv[i]) || ((arg=argv[i]) == "--")) {
            cerr << "No script file after '-s/--script'\n";
            return 1;
          } else {
            has_file_arg = true;
            script_path = arg;
          }
        } else if((!has_file_arg) && (arg.length() > 0) && (arg[0] != '-')) {
          #ifdef WITH_APP_ATTACHMENT
            // Positional script arg conflicts with built-in library script,
            // so specifying -s/--script is needed then.
            args.push_back(std::move(arg));
          #else
            has_file_arg = true;
            script_path = arg;
          #endif
        } else {
          args.push_back(std::move(arg));
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
    cerr << e.what() << "\n";
    return 1;
  }

  // Script init/run.
  try {
    duktape::engine js;
    // Module imports
    {
      #ifndef WITHOUT_STDLIB
        duktape::mod::stdlib::define_in(js);
      #endif
      #ifndef WITHOUT_STDIO
        duktape::mod::stdio::define_in(js);
      #endif
      #ifndef WITHOUT_FILESYSTEM
        duktape::mod::filesystem::generic::define_in(js);
        duktape::mod::filesystem::basic::define_in(js);
        duktape::mod::filesystem::extended::define_in(js);
        duktape::mod::filesystem::fileobject::define_in(js);
      #endif
      #ifndef WITHOUT_SYSTEM
        duktape::mod::system::define_in(js);
      #endif
      #ifndef WITHOUT_SYSTEM_EXEC
        duktape::mod::system::exec::define_in(js);
      #endif
      #ifndef WITHOUT_SYSTEM_HASH
        duktape::mod::system::hash::define_in(js);
      #endif
      #ifndef WITHOUT_XLANG
        duktape::mod::xlang::define_in(js);
      #endif
      #ifndef WITHOUT_CONVERSION
        duktape::mod::ext::conv::define_in(js);
      #endif
      #ifndef WITHOUT_SERIALPORT
        duktape::mod::ext::serial_port::define_in(js);
      #endif
      #ifndef WITHOUT_MMAP
        duktape::mod::ext::mmap::define_in(js);
      #endif
      #ifdef WITH_RESOURCE_IMPORT
        duktape::mod::ext::resource_blob::define_in(js);
      #endif
      #ifdef WITH_EXPERIMENTAL
        duktape::mod::experimental::define_in(js);
      #endif
      #ifdef WITH_SOCKET
        duktape::mod::system::socket::define_in(js);
      #endif
    }
    // Built-in constant definitions.
    {
      // Base application context constants.
      js.define("sys.app.name", PROGRAM_NAME);
      js.define("sys.app.version", PROGRAM_VERSION);
      js.define("sys.app.path", js.call<string>("fs.application"));
      js.define("sys.args", args);
      js.define("sys.script", script_path);
      js.define("sys.scriptdir", js.eval<string>("fs.dirname(sys.app.path)"));
      // Overwritable values:
      js.define_flags(duktape::engine::defflags::configurable|duktape::engine::defflags::writable|duktape::engine::defflags::enumerable);
      js.define("sys.app.verbose", bool(arg_flags & arg_flag_verbose));
      js.define("sys.env");
      #ifndef CONFIG_WITHOUT_ENVIRONMENT_VARIABLES
        duktape::mod::stdlib::define_env(js, envv);
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
      #ifdef WITH_APP_ATTACHMENT
      const bool has_lib = duktape::mod::ext::app_attachment::define_in(js);
      #else
      constexpr bool has_lib = false;
      #endif
      if((!has_lib) && (eval_code.empty()) && (script_code.empty())) {
        if(arg_flags & arg_flag_help) {
          print_help();
          return 1;
        } else if(arg_flags & arg_flag_version) {
          print_version();
          return 0;
        } else {
          cerr << "Error: No js file specified/piped in (-s <script>), and no code to evaluate passed (-e \"code\").\n";
          throw duktape::exit_exception(1);
        }
      } else {
        // Forward --help/--version to the built-in script. It is guaranteed
        // above that these are the only argument if specified.
        if(arg_flags & arg_flag_help) {
          args.push_back("--help");
        } else if(arg_flags & arg_flag_version) {
          args.push_back("--version");
        }
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
    if(e.callstack().empty()) {
      cerr << "Error: " << e.what() << "\n";
    } else {
      cerr << e.callstack() << "\n";
    }
    return 1;
  } catch(const duktape::engine_error& e) {
    cerr << "Fatal: " << e.what() << "\n";
    return 1;
  } catch(const std::exception& e) {
    cerr << "Fatal: " << e.what() << "\n";
    return 1;
  } catch(std::exception* e) {
    cerr << "Fatal: EXCEPTION THROWN BY POINTER FROM NATIVE CODE (REMOVE NEW):" << e->what() << "\n";
    // let the kernel clean after us this time.
    return 1;
  } catch (...) {
    throw; // maybe the c++ frame has an error message.
  }
  return 1;
}
