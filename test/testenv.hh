#ifndef DUKTAPE_HH_TESTING_ENVIRONMENT_HH
#define DUKTAPE_HH_TESTING_ENVIRONMENT_HH
#define WITH_MICROTEST_TMPFILE
#include "../duktape/duktape.hh"
#include "microtest.hh"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <locale>
#include <clocale>
#include <memory>

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
  #ifndef OS_WINDOWS
    #define OS_WINDOWS
  #endif
#endif

// Test debug macro used as temporary output to track problems.
// All uses must be removed from the code (the normal binary will
// intentionally not know the macro and won't compile).
#define TEST_DEBUG(X) {std::cerr << "[TEMP] " << X << std::endl;}


namespace testenv {

  #ifndef OS_WINDOWS

    int sysshellexec(std::string cmd) {
      int r=::system(cmd.c_str());
      return WEXITSTATUS(r);
    }

    bool exists(std::string file) {
      struct ::stat st;
      return (::stat(file.c_str(), &st)==0);
    }

    std::string test_path(std::string path="") {
      while(!path.empty() && path.front() == '/') path = path.substr(1);
      static auto tmp_dir =  test_make_tmpdir();
      return path.empty() ? tmp_dir.path() : (tmp_dir.path() + "/" + path);
    }

    void test_makesymlink(std::string src, std::string dst) {
      src = test_path(src);
      dst = test_path(dst);
      if(::symlink(src.c_str(), dst.c_str()) != 0) {
        test_fail(std::string("test_makesymlink(") + src + "," + dst + ") failed");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }

  #else

    int sysshellexec(std::string cmd) { return ::system(cmd.c_str()); }
    bool exists(std::string file) {
      for(auto& e:file) if(e=='/') e='\\';
      struct ::stat st; return (::stat(file.c_str(), &st)==0);
    }

    std::string test_path(std::string path="") {
      static auto tmp_dir =  test_make_tmpdir();
      while(!path.empty() && ((path.front() == '/') || (path.front() == '\\'))) path = path.substr(1);
      path = path.empty() ? tmp_dir.path() : (tmp_dir.path() + "\\" + path);
      for(auto& e:path) if(e=='/') e='\\';
      return path;
    }

    void test_makesymlink(std::string src, std::string dst) {
      (void) src;
      (void) dst;
    }

  #endif

  void test_makefile(std::string path) {
    std::ofstream fos(test_path(path));
    fos << path << "\n";
  }

  namespace {
    inline std::string to_string(std::string s) { return s; }
    inline std::string to_string(const char* c) { return std::string(c); }
    template<typename Head> inline std::string joined_to_string(Head&& h) { return to_string(h); }
    template<typename Head, typename ...Tail> inline std::string joined_to_string(Head&& h, Tail&& ...t) { return to_string(h) + joined_to_string(t...); }
  }
  template<typename ...Args>
  inline int sysexec(Args&&... args) { return sysshellexec(joined_to_string(args...)); }

  void test_rmfiletree()
  {
    using namespace std;
    char pwd[PATH_MAX+1];
    ::memset(pwd, 0, sizeof(pwd));
    if(!::getcwd(pwd, PATH_MAX)) {
      test_fail("test_rmfiletree() failed");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    if(::chdir(test_path().c_str()) == 0) {
      char s[PATH_MAX+2];
      ::memset(s, 0, sizeof(s));
      if(::getcwd(s, PATH_MAX) && (test_path() == s)) {
        sysexec(std::string("rm -rf -- *"));
      } else {
        if(::chdir(pwd) != 0) {}
        test_fail("test_rmfiletree() failed");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
    if(::chdir(pwd) != 0) {}
  }

  void test_makedir(std::string path, bool ignore_error=false)
  {
    #ifndef OS_WINDOWS
    if(::mkdir(test_path(path).c_str(), 0755) != 0) {
    #else
    if(::mkdir(test_path(path).c_str()) != 0) {
    #endif
      if(!ignore_error) {
        test_fail(std::string("test_makedir(") + test_path(path) + ") failed");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
  }

  void test_makefiletree()
  {
    using namespace std;
    test_info("(Re)building test temporary file tree: ", test_path());
    test_rmfiletree();
    test_makedir(test_path(), true);
    #ifndef OS_WINDOWS
    {
      if(::chdir("/tmp") != 0) {
        test_fail("Failed to chdir to /tmp");
        throw std::runtime_error("Aborted due to failed test assertion.");
      } else if(test_path().find("/tmp/") != 0) {
        test_fail("test_path() not in /tmp");
        throw std::runtime_error("Aborted due to failed test assertion.");
      }
    }
    #endif
    if(!exists(test_path()) || (::chdir(test_path().c_str()) != 0)) {
      test_fail("Test base temp directory does not exist or failed to chdir into it.");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    test_makedir("a");
    test_makedir("b");
    test_makefile("null");
    test_makefile("undefined");
    test_makefile("z");
    test_makefile("a/y");
    test_makefile("b/x");
    test_makefile("b/w with whitespace");
    test_makesymlink("a/y", "ly");
    test_makesymlink("a", "b/la");
    test_makesymlink("b/w with whitespace", "lw with whitespace");
  }
}

void test(duktape::engine& js);
std::string test_source_file;
std::vector<std::string> test_source_lines;
std::vector<std::string> test_cli_args;
std::vector<std::string> test_cli_env;

namespace {

  int callerline(duktape::api& stack)
  {
    std::string s = stack.callstack();
    auto p = s.find('\n');
    if(p != s.npos) s.resize(p);
    p = s.find(':');
    if(p == s.npos || p >= s.size()-1) return 0;
    s = s.substr(p+1);
    return ::atoi(s.c_str());
  }

  int ecma_callstack(duktape::api& stack)
  {
    stack.push(stack.callstack());
    return 1;
  }

  int ecma_garbage_collector(duktape::api& stack)
  {
    stack.gc();
    return 0;
  }

  // returns an absolute path to the given unix path (relative to test temp directory)
  int ecma_testabspath(duktape::api& stack)
  {
    if(!stack.top() || (!stack.is_string(0))) return 0;
    stack.push(testenv::test_path(stack.get<std::string>(0)));
    return 1;
  }

  // returns a relative path to the given unix path (relative to test temp directory)
  int ecma_testrelpath(duktape::api& stack)
  {
    if(!stack.top() || (!stack.is_string(0))) return 0;
    std::string path = stack.get<std::string>(0);
    while(!path.empty() && path.front() == '/') path = path.substr(1);
    #ifdef OS_WINDOWS
    for(auto& e:path) if(e=='/') e='\\';
    #endif
    stack.push(path);
    return 1;
  }

  int ecma_print(duktape::api& stack)
  {
    std::stringstream ss;
    int nargs = stack.top();
    if((nargs == 1) && stack.is_buffer(0)) {
      const char *buf = nullptr;
      duk_size_t sz = 0;
      if((buf = static_cast<const char*>(stack.get_buffer(0, sz))) && (sz > 0)) {
        ss.write(buf, sz);
      }
    } else if(nargs > 0) {
      ss << stack.to<std::string>(0);
      for(int i=1; i<nargs; i++) ss << " " << stack.to<std::string>(i);
    }
    std::string file = test_source_file;
    if(file.empty()) file = "test";
    ::sw::utest::test::comment(file, callerline(stack), ss.str());
    return 0;
  }

  int ecma_pass(duktape::api& stack)
  {
    int nargs = stack.top();
    std::string msg;
    if(nargs > 0) {
      msg = stack.to<std::string>(0);
      for(int i=1; i<nargs; i++) msg += std::string(" ") + stack.to<std::string>(i);
      ::sw::utest::test::pass(test_source_file, callerline(stack), msg);
    } else {
      ::sw::utest::test::pass(); // silent PASS.
    }
    stack.push(true);
    return 1;
  }

  int ecma_warn(duktape::api& stack)
  {
    std::stringstream ss;
    int nargs = stack.top();
    if(!nargs) return 0;
    ss << stack.to<std::string>(0);
    for(int i=1; i<nargs; i++) ss << " " << stack.to<std::string>(i);
    ::sw::utest::test::warning(test_source_file, callerline(stack), ss.str());
    return 0;
  }

  int ecma_fail(duktape::api& stack)
  {
    int nargs = stack.top();
    std::string msg;
    if(nargs > 0) {
      msg = stack.to<std::string>(0);
      for(int i=1; i<nargs; i++) msg += std::string(" ") + stack.to<std::string>(i);
    }
    ::sw::utest::test::fail(test_source_file, callerline(stack), msg);
    stack.push(false);
    return 1;
  }

  int ecma_comment(duktape::api& stack)
  {
    int nargs = stack.top();
    if(!nargs) return 0;
    std::string s(stack.to<std::string>(0));
    for(int i=1; i<nargs; i++) s += std::string(" ") + stack.to<std::string>(i);
    ::sw::utest::test::comment(test_source_file, callerline(stack), s);
    stack.push(true);
    return 1;
  }

  int ecma_expect(duktape::api& stack)
  {
    int nargs = stack.top();
    if(!nargs) return 0;
    bool passed = stack.to<bool>(0);
    if(nargs > 1) {
      std::string s(stack.to<std::string>(1));
      for(int i=2; i<nargs; i++) s += std::string(" ") + stack.to<std::string>(i);
      if(passed) {
        ::sw::utest::test::pass(test_source_file, callerline(stack), s);
      } else {
        ::sw::utest::test::fail(test_source_file, callerline(stack), s);
      }
    }
    stack.push(passed);
    return 1;
  }

  int ecma_eassert(duktape::api& stack)
  {
    std::string expr = stack.get<std::string>(0);
    stack.swap(0, 1);
    stack.top(1);
    if(stack.pcall(0) != 0) {
      std::string msg = stack.safe_to_string(-1);
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr + std::string(" | Unexpected exception: '") + msg + "', !assertion, aborting");
      throw duktape::exit_exception(1);
    } else if(stack.to<bool>(-1)) {
      ::sw::utest::test::pass(test_source_file, callerline(stack), expr);
    } else {
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr + " | !assertion, aborting");
      throw duktape::exit_exception(1);
    }
    stack.top(0);
    stack.push(true);
    return 1;
  }

  int ecma_eexpect(duktape::api& stack)
  {
    std::string expr = stack.get<std::string>(0);
    stack.swap(0, 1);
    stack.top(1);
    if(stack.pcall(0) != 0) {
      std::string msg = stack.safe_to_string(-1);
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr + std::string(" | Unexpected exception: '") + msg + "'");
      stack.push(false);
    } else if(stack.to<bool>(-1)) {
      ::sw::utest::test::pass(test_source_file, callerline(stack), expr);
      stack.push(true);
    } else {
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr);
      stack.push(false);
    }
    return 1;
  }

  int ecma_eexpect_except(duktape::api& stack)
  {
    std::string expr = stack.get<std::string>(0);
    stack.swap(0, 1);
    stack.top(1);
    if(stack.pcall(0) != 0) {
      std::string msg = stack.safe_to_string(-1);
      ::sw::utest::test::pass(test_source_file, callerline(stack), expr + std::string(" | Expected exception: '") + msg + "'");
      stack.push(true);
    } else {
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr + std::string(" | Exception was expected.") );
      stack.push(false);
    }
    return 1;
  }

  int ecma_eexpect_noexcept(duktape::api& stack)
  {
    std::string expr = stack.get<std::string>(0);
    stack.swap(0, 1);
    stack.top(1);
    if(stack.pcall(0) != 0) {
      std::string msg = stack.safe_to_string(-1);
      ::sw::utest::test::fail(test_source_file, callerline(stack), expr + std::string(" | Unexpected exception: '") + msg + "'");
      stack.top(0);
      return 0;
    } else {
      ::sw::utest::test::pass(test_source_file, callerline(stack), expr + std::string(" | No exception.") );
      return 1;
    }
  }

  int ecma_reset(duktape::api& stack)
  {
    ::sw::utest::test::reset(test_source_file.c_str(), callerline(stack));
    stack.push(true);
    return 1;
  }

}

namespace {

  std::string trim(const std::string& s)
  {
    const auto sz = s.size();
    auto si = size_t(0);
    while(si<sz && isspace(s[si])) ++si;
    if(si >= sz) return "";
    auto ei = size_t(sz-1);
    while(ei>si && isspace(s[ei])) --ei;
    return s.substr(si, ei-si+1);
  }

  // Note: The purpose of this function is to find the outer "test_expect()" locations and
  //       converting the arguments into a string to evaluate in the JS engine. The implementation
  //       is working but somewhat clumsy and will be replaced/optimised at a later date.
  std::string replace_expect_function_arguments(std::string&& code, std::string function_name, const std::string replace_function_name)
  {
    using namespace std;

    const auto escape = [](string arguments) {
      string s;
      for(auto c:arguments) { if((c == '"') || (c == '\\')) s += '\\'; s += c; }
      return string("\"") + s + "\"";
    };

    enum class chunk_span_types { instructions, block_comment, inline_comment, double_quoted_string, single_quoted_string, preprocessor };
    using chunk_type = std::pair<chunk_span_types, std::string>;
    std::vector<chunk_type> chunks;
    {
      regex re_start_all("/\\*|//|\\\"|'|[\\n][\\t ]*#", regex::ECMAScript|regex::optimize);
      regex re_end_block_comment("\\*/", regex::ECMAScript|regex::optimize);
      regex re_end_inline_comment("[\\n]", regex::ECMAScript|regex::optimize);
      regex re_end_dblquot_str("[^\\\\]\"", regex::ECMAScript|regex::optimize);
      regex re_end_sngquot_str("[^\\\\]'", regex::ECMAScript|regex::optimize);
      regex re_end_preproc("\\n", regex::ECMAScript|regex::optimize);
      auto it = code.cbegin();
      smatch m;
      auto search_next = [&](regex& re) -> bool {
        return regex_search(
          it, code.cend(), m, re,
          regex_constants::match_not_bol|regex_constants::match_not_eol
        );
      };
      auto last_position = distance(code.cbegin(), code.cbegin());
      while((it != code.end()) && search_next(re_start_all)) {
        string s = m.str(0);
        auto start_position = distance(code.cbegin(),it) + m.position(0);
        auto end_position = start_position;
        it = code.cbegin() + start_position; // not sure if the given match iterators are identical to the string to match
        auto advance = [&](int correction) -> string {
          end_position = distance(code.cbegin(),it) + m.position(0) + m.str(0).size() + correction;
          it = code.cbegin() + end_position;
          last_position = end_position;
          return code.substr(start_position, end_position-start_position);
        };
        {
          string cde = code.substr(last_position, start_position-last_position);
          if(!cde.empty()) {
            chunks.emplace_back(chunk_span_types::instructions, cde);
          }
        }
        if(s.back() == '*') {
          if(search_next(re_end_block_comment)) {
            chunks.emplace_back(chunk_span_types::block_comment, advance(0));
          }
        } else if(s.back() == '/') {
          if(search_next(re_end_inline_comment)) {
            chunks.emplace_back(chunk_span_types::inline_comment, advance(-1));
          }
        } else if(s.back() == '"') {
          if(search_next(re_end_dblquot_str)) {
            chunks.emplace_back(chunk_span_types::double_quoted_string, advance(0));
          }
        } else if(s.back() == '\'') {
          if(search_next(re_end_sngquot_str)) {
            chunks.emplace_back(chunk_span_types::single_quoted_string, advance(0));
          }
        } else if(s.back() == '#') {
          if(search_next(re_end_preproc)) {
            chunks.emplace_back(chunk_span_types::preprocessor, advance(-1));
          }
        } else {
          throw runtime_error(string("Unknown match '") + advance(0) + "'");
        }
      }
      if(size_t(last_position) < code.size()) {
        chunks.emplace_back(chunk_span_types::instructions, code.substr(last_position));
      }
    };

    code.clear();
    int parenthesis_level = 0;
    auto pos = code.npos;
    auto arg_argument_start_pos = code.npos;

    for(auto chunk:chunks) {
      if(chunk.first != chunk_span_types::instructions) {
        code += chunk.second;
      } else {
        auto s = chunk.second;
        while(!s.empty()) {
          if(parenthesis_level > 0) {
            if((pos = s.find_first_of(")(")) == s.npos) {
              code += s;
              s.clear();
            } else if(s[pos] == '(') {
              ++parenthesis_level;
              code += s.substr(0, pos+1);
              s = s.substr(pos+1);
            } else if(s[pos] == ')') {
              --parenthesis_level;
              code += s.substr(0, pos);
              s = s.substr(pos+1);
              if(parenthesis_level == 0) {
                string arguments = trim(code.substr(arg_argument_start_pos));
                code.resize(arg_argument_start_pos);
                auto arg_function_name_pos = code.rfind(function_name);
                string sep = code.substr(arg_function_name_pos+function_name.size());
                code.resize(arg_function_name_pos+function_name.size());
                string fnname = code.substr(arg_function_name_pos);
                code.resize(arg_function_name_pos);
                const auto wrapped_fn_body = "function(){return(" + arguments + ")}";
                arguments = escape(arguments);
                code += replace_function_name;
                code += sep;
                code += arguments;
                code += ",";
                code += wrapped_fn_body;
                code += ")";
              } else {
                code += ")";
              }
            } else {
              code += s.substr(0, pos+1);
              s = s.substr(pos+1);
            }
          } else if((pos=s.find(function_name)) != s.npos) {
            pos += function_name.size();
            for(; ::isspace(s[pos]) && (pos < s.size()); ++pos);
            char c = s[pos];
            code += s.substr(0, pos+1);
            s = s.substr(pos+1);
            if(c == '(') {
              parenthesis_level = 1;
              arg_argument_start_pos = code.size();
            }
          } else {
            code += s;
            s.clear();
          }
        }
      }
    }
    return std::move(code);
  }
}

void test_include_script(duktape::engine& js, const std::string source_file="test.js")
{
  using namespace std;
  // Redefine print and alert in case stdio was included in the test c++
  js.define("print", ecma_print); // may be overwritten by stdio
  js.define("alert", ecma_warn); // may be overwritten by stdio
  js.define("sys.script", source_file);

  // Actual include
  test_source_file = source_file;
  string code;
  {
    ifstream fis(test_source_file.c_str(), ios::in);
    string code0;
    code0.assign((istreambuf_iterator<char>(fis)), istreambuf_iterator<char>());
    string code1 = replace_expect_function_arguments(std::move(code0), "test_expect", "test_eval_expect");
    string code2 = replace_expect_function_arguments(std::move(code1), "test_assert", "test_eval_assert");
    string code3 = replace_expect_function_arguments(std::move(code2), "test_expect_noexcept", "test_eval_expect_noexcept");
    code = replace_expect_function_arguments(std::move(code3), "test_expect_except", "test_eval_expect_except");
  }
  js.eval(std::move(code), test_source_file);
}

void testenv_init(duktape::engine& js)
{
  js.define("print", ecma_print); // may be overwritten by stdio
  js.define("alert", ecma_warn); // may be overwritten by stdio
  js.define("test_fail", ecma_fail);
  js.define("test_pass", ecma_pass);
  js.define("test_warn", ecma_warn);
  js.define("test_expect", ecma_expect);
  js.define("test_eval_assert", ecma_eassert, 2);
  js.define("test_eval_expect", ecma_eexpect, 2);
  js.define("test_eval_expect_except", ecma_eexpect_except, 2);
  js.define("test_eval_expect_noexcept", ecma_eexpect_noexcept, 2);
  js.define("test_comment", ecma_comment);
  js.define("test_note", ecma_comment);
  js.define("test_reset", ecma_reset);
  js.define("test_abspath", ecma_testabspath, 1);
  js.define("test_relpath", ecma_testrelpath, 1);
  js.define("test_gc", ecma_garbage_collector, 0);
  js.define("callstack", ecma_callstack, 0);
  js.define("sys.args", test_cli_args);
  js.define("sys.envv", test_cli_env); // intentionally not sys.env object as in the main cli.
}

int main(int argc, char *argv[], const char** envv)
{
  try {
    std::locale::global(std::locale("C"));
    ::setlocale(LC_ALL, "C");
    test_initialize();
    for(int i=1; i<argc && argv[i]; ++i) test_cli_args.emplace_back(argv[i]);
    for(int i=0;   envv && envv[i]; ++i) test_cli_env.emplace_back(envv[i]);
    duktape::engine js;
    testenv_init(js);
    try {
      test(js);
    } catch(const duktape::exit_exception& e) {
      test_info(std::string("Exit with code ") + std::to_string(e.exit_code()) + "'.");
    } catch(const duktape::script_error& e) {
      test_fail("Unexpected script error: '", e.what(), "'", e.callstack());
    } catch(const duktape::engine_error& e) {
      test_fail("Unexpected engine error: ", e.what());
    } catch(const std::exception& e) {
      test_fail("Unexpected exception: '", e.what(), "'.");
    } catch (...) {
      test_fail("Unexpected exception.");
    }
  } catch(const std::exception& e) {
    test_fail("Unexpected init exception: '", e.what(), "'.");
  } catch (...) {
    test_fail("Unexpected init exception.");
  }
  return test_summary();
}

#endif
