#include <duktape/duktape.hh>
#include <duktape/mod/mod.stdio.hh>
#include <duktape/mod/mod.stdlib.hh>
#include <duktape/mod/mod.fs.hh>
#include <duktape/mod/mod.fs.file.hh>
#include <duktape/mod/mod.sys.hh>
#include <duktape/mod/mod.sys.exec.hh>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>

#ifndef PROGRAM_NAME
  #define PROGRAM_NAME "js"
#endif

#ifndef PROGRAM_VERSION
  #define PROGRAM_VERSION "v1.0"
#endif

using namespace std;
using namespace duktape;

int main(int argc, const char** argv)
{
  try {

    // Command line arguments
    string script_path, script_code, eval_code;
    vector<string> args;
    {
      bool was_last_opt = false;
      bool has_file_arg = false;
      // rudimentary handling of application args before passing on
      // (without library deps).
      for(int i=1; i<argc && argv[i]; ++i) {
        string arg = argv[i];
        if(was_last_opt || has_file_arg) {
          args.emplace_back(move(arg));
        } else if(arg == "--") {
          was_last_opt = true;
        } else if(arg == "-h" || arg == "--help") {
          cerr << "NAME" << endl << endl
               << "  " << PROGRAM_NAME << endl << endl
               << "SYNOPSIS" << endl << endl
               << "  " << PROGRAM_NAME << " [ -h ] [ -e '<code>' | <script file> ] [--] [script arguments]" << endl << endl
               << "DESCRIPTION" << endl << endl
               << "  Evaluate javascript code pass via -e argument, via script " << endl
               << "  file or via piping stdin." << endl << endl
               << "ARGUMENTS" << endl << endl
               << "  -h | --help        : Print help and exit." << endl
               << "  -e | --eval <code> : Evaluate code given as argument. Done after loading" << endl
               << "                       a file (or stdin)." << endl
               << "  <script file>      : (First positional argument). A javascript file to" << endl
               << "                       load and run or - (dash) for piping in from stdin" << endl
               << "  --                 : Optional separator between program options and" << endl
               << "                       script options/arguments. Useful if e.g. '-e'" << endl
               << "                       shall be passed to the script instead of evaluating." << endl
               << "  script arguments   : All arguments after '--' or the script file are passed" << endl
               << "                       to the script and are there available as the 'sys.args'" << endl
               << "                       array." << endl << endl
               << "EXIT CODE" << endl << endl
               << "  0=success, other codes indicate an error, either from a script exception or " << endl
               << "                       from binary program error." << endl << endl
               << (PROGRAM_NAME) << " " << (PROGRAM_VERSION) << ", (CC) stfwi 2015-2017, lic: MIT" << endl
               ;;
          return 1;
        } else if(arg == "-V" || arg == "--version") {
          cout << "program: " << PROGRAM_NAME << endl << "version: " << PROGRAM_VERSION << endl;
          return 0;
        } else if(arg == "-e" || arg == "--eval") {
          if((++i >= argc) || (!argv[i]) || ((arg=argv[i]) == "--")) {
            cerr << "No code after '-e/--eval'" << endl;
            return 1;
          } else {
            eval_code = arg;
          }
        } else if(!has_file_arg) {
          has_file_arg = true;
          script_path = arg;
        } else {
          args.emplace_back(move(arg));
        }
      }
    }

    duktape::engine js;
    duktape::mod::stdlib::define_in(js);
    duktape::mod::stdio::define_in(js);
    duktape::mod::filesystem::generic::define_in(js);
    duktape::mod::filesystem::basic::define_in(js);
    duktape::mod::filesystem::enhanced::define_in(js);
    duktape::mod::filesystem::fileobject::define_in(js);
    duktape::mod::system::define_in(js);
    duktape::mod::system::exec::define_in(js);
    js.define("sys.args", args);
    vector<string>().swap(args);

    if(script_path == "-") {
      script_path = "(piped code)";
      script_code.assign((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
      if(!cin.good() && !cin.eof()) throw std::runtime_error("Failed to read script from stdin.");
      if(script_code.empty()) throw std::runtime_error("Script to execute is empty.");
    } else if(!script_path.empty()) {
      std::ifstream fis(script_path);
      script_code.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
      if(!fis.good() && !fis.eof()) throw std::runtime_error(std::string("Failed to read script '") + script_path + "'.");
      if(script_code.empty()) throw std::runtime_error("Script to execute is empty.");
    } else if(eval_code.empty()) {
      throw std::runtime_error("No js file specified/piped in, and no code to evaluate passed. Nothing to do.");
    }
    // remove leading #!/bin/whatever
    if((script_code.length() > 2) && (script_code[0] == '#') && (script_code[1] == '!')) {
      for(auto& e:script_code) {
        if((e == '\n') || (e == '\r')) break;
        e = ' ';
      }
    }
    js.eval<void>(std::move(script_code), script_path);
    if(!eval_code.empty()) js.eval<void>(std::move(eval_code), "(inline eval code)");
    return 0;
  } catch(duktape::exit_exception& e) {
    return e.exit_code();
  } catch(duktape::script_error& e) {
    cerr << e.callstack() << endl;
    return 1;
  } catch(duktape::engine_error& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch(std::exception& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch (...) {
    throw; // maybe the c++ frame has an error message.
  }
  return 1;
}
