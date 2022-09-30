/**
 * Re-run tests with this build that are already verified
 * by the Duktape authors in the duktape github repository.
 *
 * The duktape sources have to be cloned into the sub-directory
 * `duktape-source` of this test.
 */
#include "../testenv.hh"
#include <string>
#include <string_view>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <regex>
#include <tuple>

#if (__cplusplus < 201700)
  void test(duktape::engine& js)
  {
    (void)js;
    test_info("Interpreter core test requires c++17 (std::filesystem), skipped.");
  }
#else
  #include <filesystem>

  using namespace std;

  namespace {

    int js_print_to(duktape::api& stack, string& str)
    {
      const auto nargs = stack.top();
      if(!nargs) return 0;
      str += stack.to<std::string>(0);
      for(int i=1; i<nargs; i++) { str += string(" "); str += stack.to<string>(i); }
      str += "\n";
      return 0;
    }

    auto sout = string();
    auto serr = string();

    int js_print(duktape::api& stack)
    { return js_print_to(stack, sout); }

    int js_alert(duktape::api& stack)
    { return js_print_to(stack, serr); }

    bool run_duktape_core_engine_test_script(string path, string include_dir_path)
    {
      try {
        cout.flush();
        cerr.flush();
        auto is = ifstream();
        is.open(path, ios::binary);
        auto code = string(istreambuf_iterator<char>(is), {});
        is.close();
        const auto expected_output = ([](const string& code) -> string {
          // ... -> regex aborts without exception or exit code != 0, maybe when newer compilers are roller out more in the wild ...
          // static const auto expected_output_re = regex("([\\/][\\*]+[=]+[\\s]*[\\n])([\\s\\S]*?)([\\n][=]+[\\*]+[\\/][\\n])", std::regex_constants::ECMAScript);
          // static const auto rit_end = std::sregex_iterator();
          // for(auto match=sregex_iterator(code.begin(), code.end(), expected_output_re); match!=rit_end; ++match) { expected_output += (*match)[2].str() + "\n"; }
          // ok, I don't want a library dependency because of this, so here goes the old long way...
          auto concatenated_result = string();
          const auto find_result_range = [](const string& code, string::size_type start) -> tuple<string, string::size_type> {
            static constexpr auto npos = string::npos;
            static const auto s_tag = string_view("\n/*==");
            static const auto e_tag = string_view("==*/");
            static const auto s_ignored = string_view("= \t\r\n");
            const auto len = code.size();
            const auto p0 = code.find("/*==", start);
            if(p0 == npos || (p0 > code.size()-(s_tag.size()+e_tag.size()-1))) return tuple(string(),npos);
            auto ps = p0 + s_tag.size();
            while(ps<len && s_ignored.find_first_of(code[ps])!=(s_ignored.npos)) ++ps;
            const auto p1 = code.find(e_tag, ps);
            if(p1 == npos) return tuple(string(),npos);
            string s = code.substr(ps, p1-1-ps);
            while(!s.empty() && s.back()!='\n') s.pop_back();
            if(!s.empty()) s.pop_back();
            if(!s.empty() && s.back()=='\r') s.pop_back();
            return tuple(s, p1+e_tag.size());
          };
          for(auto se=find_result_range(code,0); get<1>(se)!=string::npos; se=find_result_range(code, get<1>(se))) {
            concatenated_result += get<0>(se) + "\n";
          }
          return concatenated_result;
        })(code);
        const auto includes = ([](const string& code) -> vector<string> {
          static const auto s_tag = string_view("\n/*@include ");
          static const auto e_tag = string_view("@*/");
          auto inc = vector<string>();
          auto p0 = code.find(s_tag, 0);
          while(p0 != code.npos) {
            const auto p1 = code.find(e_tag, p0+1);
            if(p1==code.npos) throw runtime_error(string("No end tag '@*/' found for ") + code.substr(p0, 30) + "...");
            inc.push_back(code.substr(p0+s_tag.size(), p1-p0-s_tag.size()));
            p0 = code.find(s_tag, p1+1);
          }
          return inc;
        })(code);
        const auto remove_whitespaces = [](string s){
          s.erase(remove_if(s.begin(), s.end(), ::isspace));
          return s;
        };
        string().swap(sout);
        string().swap(serr);
        auto js = duktape::engine();
        js.define("print", js_print);
        js.define("alert", js_alert);
        test_info("Evaluating ...");
        cout.flush();
        for(const auto& fpath:includes) {
          test_info("Including '" + fpath + "' ...");
          js.include(include_dir_path+"/"+fpath);
        }
        js.eval(code);
        cout.flush();
        if(!serr.empty()) {
          test_fail(path + ": Test script stderr: " + serr);
        } else if(remove_whitespaces(sout) != remove_whitespaces(expected_output)) {
          test_fail(path);
          test_note("Result mismatch (>>>expected---result<<<):\n>>>\n" << expected_output << "---\n" << sout << "<<<");
        } else {
          test_pass(path);
          return true;
        }
      } catch(const regex_error& e) {
        test_fail("Unexpected regex error: ", e.what());
      } catch(const duktape::exit_exception& e) {
        test_note("Exit with code " << std::to_string(e.exit_code()) + "'.");
      } catch(const duktape::script_error& e) {
        test_fail("Unexpected script error: ", e.callstack());
      } catch(const duktape::engine_error& e) {
        test_fail("Unexpected engine error: ", e.what());
      } catch(const std::exception& e) {
        test_fail("Unexpected exception: '", e.what(), "'.");
      } catch (...) {
        test_fail("Unexpected exception.");
      }
      return false;
    }
  }

  void test(duktape::engine& js)
  {
    js.clear();
    auto failed_tests = vector<string>();
    const auto cli_arg = test_cli_args.empty() ? string() : test_cli_args.front();
    const auto getn = [](const string& s, int default_value){ try { return std::stoi(s); } catch(...) { return default_value; } };
    const auto cli_arg_start_index = getn(cli_arg, 0);
    const auto src_root = string("ecmascript/active");
    const auto inc_root = string("ecmascript/include");
    if(!filesystem::is_directory("ecmascript")) {
      test_note("Skipped Duktape ecmascript tests, missing directory 'ecmascript'");
      return;
    } else if(!filesystem::is_directory(src_root)) {
      test_fail("Tests to be executed (test-###.js) have to be placed in '", src_root, "'");
      return;
    } else if(!filesystem::is_directory(inc_root)) {
      test_fail("Test include files of the Duktape engine (e.g. util-###.js) have to be placed in '", inc_root, "'");
      return;
    }
    if(!cli_arg_start_index && !cli_arg.empty()) {
      try {
        const auto path_str = src_root + "/" + test_cli_args.front();
        test_note("Running '" << path_str << "' ...");
        run_duktape_core_engine_test_script(path_str, inc_root);
      } catch(const std::exception& e) {
        test_fail("Unexpected test RUN exception: '", e.what(), "'.");
      } catch (...) {
        test_fail("Unexpected test RUN exception.");
      }
    } else {
      auto paths = vector<string>();
      for(const auto& entry: std::filesystem::directory_iterator(src_root)) {
        if(!entry.is_regular_file() || entry.path().extension() != ".js") continue;
        if(entry.path().filename().string().find("test")!=0) continue;
        paths.push_back(entry.path().string());
      }
      sort(paths.begin(), paths.end());
      const auto n_tests = int(paths.size());
      auto test_index = 0;
      for(auto& path_str: paths) {
        if(test_index < cli_arg_start_index) {
          ++test_index;
          continue;
        }
        try {
          test_note("Running " << (++test_index) << "/" << n_tests << ": '" << path_str << "' ...");
          if(run_duktape_core_engine_test_script(path_str, inc_root)) continue;
        } catch(const std::exception& e) {
          test_fail("Unexpected test RUN exception: '", e.what(), "'.");
        } catch (...) {
          test_fail("Unexpected test RUN exception.");
        }
        failed_tests.push_back(path_str);
      }
    }
  }

#endif
