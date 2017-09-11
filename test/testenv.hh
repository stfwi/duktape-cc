#ifndef DUKTAPE_HH_TESTING_ENVIRONMENT_HH
#define	DUKTAPE_HH_TESTING_ENVIRONMENT_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
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

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
#endif
// </editor-fold>

// <editor-fold desc="auxiliary fs test functions" defaultstate="collapsed">
namespace testenv {
  #ifndef WINDOWS
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
    return path.empty() ? ::sw::utest::tmpdir::path() : (::sw::utest::tmpdir::path() + "/" + path);
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
    while(!path.empty() && path.front() == '/') path = path.substr(1);
    path = path.empty() ? ::sw::utest::tmpdir::path() : (::sw::utest::tmpdir::path() + "\\" + path);
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
    fos << path << std::endl;
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
    #ifndef WINDOWS
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
    test_comment("(Re)building test temporary file tree '" << test_path() << "'");
    test_rmfiletree();
    test_makedir(test_path(), true);
    #ifndef WINDOWS
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
// </editor-fold>

// <editor-fold desc="js testing functions" defaultstate="collapsed">
void test(duktape::engine& js);
std::string test_source_file;
std::vector<std::string> test_source_lines;

int callerline(duk_context *ctx)
{
  duktape::api stack(ctx);
  std::string s = stack.callstack();
  auto p = s.find('\n');
  if(p != s.npos) s.resize(p);
  p = s.find(':');
  if(p == s.npos || p >= s.size()-1) return 0;
  s = s.substr(p+1);
  return ::atoi(s.c_str());
}

// returns an absolute path to the given unix path (relative to test temp directory)
int ecma_testabspath(duk_context *ctx)
{
  duktape::api stack(ctx);
  if(!stack.top() || (!stack.is_string(0))) return 0;
  stack.push(testenv::test_path(stack.get<std::string>(0)));
  return 1;
}

// returns a relative path to the given unix path (relative to test temp directory)
int ecma_testrelpath(duk_context *ctx)
{
  duktape::api stack(ctx);
  if(!stack.top() || (!stack.is_string(0))) return 0;
  std::string path = stack.get<std::string>(0);
  while(!path.empty() && path.front() == '/') path = path.substr(1);
  #ifdef WINDOWS
  for(auto& e:path) if(e=='/') e='\\';
  #endif
  stack.push(path);
  return 1;
}

int ecma_assert(duk_context *ctx)
{
  (void)ctx; return 1;
}

int ecma_print(duk_context *ctx)
{
  duktape::api stack(ctx);
  std::stringstream ss;
  int nargs = stack.top();
  if((nargs == 1) && stack.is_buffer(0)) {
    const char *buf = NULL;
    duk_size_t sz = 0;
    if((buf = (const char *) stack.get_buffer(0, sz)) && sz > 0) {
      ss.write(buf, sz);
    }
  } else if(nargs > 0) {
    ss << stack.to<std::string>(0);
    for(int i=1; i<nargs; i++) ss << " " << stack.to<std::string>(i);
  }
  std::string file = test_source_file;
  if(file.empty()) file = "test";
  ::sw::utest::test::comment(file, callerline(ctx), ss.str());
  return 0;
}

int ecma_pass(duk_context *ctx)
{
  duktape::api stack(ctx);
  int nargs = stack.top();
  std::string msg;
  if(nargs > 0) {
    msg = stack.to<std::string>(0);
    for(int i=1; i<nargs; i++) msg += std::string(" ") + stack.to<std::string>(i);
  }
  ::sw::utest::test::pass(test_source_file, callerline(ctx), msg);
  stack.push(true);
  return 1;
}

int ecma_warn(duk_context *ctx)
{
  duktape::api stack(ctx);
  std::stringstream ss;
  int nargs = stack.top();
  if(!nargs) return 0;
  ss << stack.to<std::string>(0);
  for(int i=1; i<nargs; i++) ss << " " << stack.to<std::string>(i);
  ::sw::utest::test::warn(test_source_file, callerline(ctx), ss.str());
  return 0;
}

int ecma_fail(duk_context *ctx)
{
  duktape::api stack(ctx);
  int nargs = stack.top();
  std::string msg;
  if(nargs > 0) {
    msg = stack.to<std::string>(0);
    for(int i=1; i<nargs; i++) msg += std::string(" ") + stack.to<std::string>(i);
  }
  ::sw::utest::test::fail(test_source_file, callerline(ctx), msg);
  stack.push(false);
  return 1;
}

int ecma_comment(duk_context *ctx)
{
  duktape::api stack(ctx);
  int nargs = stack.top();
  if(!nargs) return 0;
  std::string s(stack.to<std::string>(0));
  for(int i=1; i<nargs; i++) s += std::string(" ") + stack.to<std::string>(i);
  ::sw::utest::test::comment(test_source_file, callerline(ctx), s);
  stack.push(true);
  return 1;
}

int ecma_expect(duk_context *ctx)
{
  duktape::api stack(ctx);
  int nargs = stack.top();
  if(!nargs) return 0;
  bool passed = stack.to<bool>(0);
  if(nargs > 1) {
    std::string s(stack.to<std::string>(1));
    for(int i=2; i<nargs; i++) s += std::string(" ") + stack.to<std::string>(i);
    if(passed) {
      ::sw::utest::test::pass(test_source_file, callerline(ctx), s);
    } else {
      ::sw::utest::test::fail(test_source_file, callerline(ctx), s);
    }
  }
  stack.push(passed);
  return 1;
}

int ecma_eexpect(duk_context *ctx)
{
  // eval string, expect result to be true
  duktape::api stack(ctx);
  std::string expr = stack.get<std::string>(0);
  stack.top(0);
  while(!expr.empty() && ::isspace(expr.back())) expr.pop_back();
  while(!expr.empty() && ::isspace(expr.front())) expr = expr.substr(1);
  stack.peval_string(expr);
  if(stack.is_error(-1)) {
    std::string msg = stack.safe_to_string(-1);
    ::sw::utest::test::fail(test_source_file, callerline(ctx), expr + std::string(" | Unexpected exception: '") + msg + "'");
    stack.push(false);
  } else if(stack.to<bool>(-1)) {
    ::sw::utest::test::pass(test_source_file, callerline(ctx), expr);
    stack.push(true);
  } else {
    ::sw::utest::test::fail(test_source_file, callerline(ctx), expr);
    stack.push(false);
  }
  return 1;
}

int ecma_eexpect_except(duk_context *ctx)
{
  // eval string, expect result to be true
  duktape::api stack(ctx);
  std::string expr = stack.get<std::string>(0);
  stack.top(0);
  while(!expr.empty() && ::isspace(expr.back())) expr.pop_back();
  while(!expr.empty() && ::isspace(expr.front())) expr = expr.substr(1);
  stack.peval_string(expr);
  if(stack.is_error(-1)) {
    std::string msg = stack.safe_to_string(-1);
    ::sw::utest::test::pass(test_source_file, callerline(ctx), expr + std::string(" | Expected exception: '") + msg + "'");
    stack.push(false);
  } else {
    ::sw::utest::test::fail(test_source_file, callerline(ctx), expr + std::string(" | Exception was expected.") );
    stack.push(false);
  }
  return 1;
}

int ecma_reset(duk_context *ctx)
{
  duktape::api stack(ctx);
  ::sw::utest::test::reset(test_source_file, callerline(ctx));
  stack.push(true);
  return 1;
}
// </editor-fold>

// <editor-fold desc="main()" defaultstate="collapsed">

// <editor-fold desc="test functions argument try/catch enclosing" defaultstate="collapsed">
// Note: The purpose of this function is to find the outer "test_expect()" locations and
//       converting the arguments into a string to evaluate in the JS engine. The implementation
//       is working but somewhat clumsy and will be replaced/optimised at a later date.
template <typename Callback>
std::string replace_expect_function_arguments(std::string&& code, std::string function_name, Callback&& callback)
{
  using namespace std;
  enum class chunk_span_types { instructions, block_comment, inline_comment,
    double_quoted_string, single_quoted_string, preprocessor };
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
              string arguments = code.substr(arg_argument_start_pos);
              code.resize(arg_argument_start_pos);
              auto arg_function_name_pos = code.rfind(function_name);
              string sep = code.substr(arg_function_name_pos+function_name.size());
              code.resize(arg_function_name_pos+function_name.size());
              string fnname = code.substr(arg_function_name_pos);
              code.resize(arg_function_name_pos);
              callback(fnname, arguments);
              code += fnname;
              code += sep;
              code += arguments;
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
  return code;
}
// </editor-fold>

void test_include_script(duktape::engine& js)
{
  using namespace std;


  // Actual include
  {
    test_source_file = "test.js";
    string code;
    {
      string contents;
      std::ifstream fis(test_source_file.c_str(), std::ios::in);
      contents.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
      string contents2 = replace_expect_function_arguments(
        std::move(contents), "test_expect", [](string& function_name, string& arguments) {
          function_name = "test_eval_expect";
          string s; for(auto c:arguments) { if((c == '"') || (c == '\\')) s += '\\'; s += c; }
          arguments = string("\"") + s + "\"";
        }
      );
      code = replace_expect_function_arguments(
        std::move(contents), "test_expect_except", [](string& function_name, string& arguments) {
          function_name = "test_eval_expect_except";
          string s; for(auto c:arguments) { if((c == '"') || (c == '\\')) s += '\\'; s += c; }
          arguments = string("\"") + s + "\"";
        }
      );
    }
    js.eval(std::move(code), test_source_file);
  }
}

int main(int argc, const char **argv)
{
  try {
    std::locale::global(std::locale("C"));
    ::setlocale(LC_ALL, "C");
    duktape::engine js;
    js.define("print", ecma_print); // may be overwritten by stdio
    js.define("alert", ecma_warn); // may be overwritten by stdio
    js.define("test_fail", ecma_fail);
    js.define("test_pass", ecma_pass);
    js.define("test_warn", ecma_warn);
    js.define("test_expect", ecma_expect);
    js.define("test_eval_expect", ecma_eexpect, 1);
    js.define("test_eval_expect_except", ecma_eexpect_except, 1);
    js.define("test_comment", ecma_comment);
    js.define("test_reset", ecma_reset);
    js.define("test_abspath", ecma_testabspath, 1);
    js.define("test_relpath", ecma_testrelpath, 1);
    {
      std::vector<std::string> args;
      for(int i=1; i<argc && argv[i]; ++i) {
        args.emplace_back(argv[i]);
        js.define("sys.args", args);
      }
    }
    try {
      test(js);
    } catch(duktape::exit_exception& e) {
      test_note(std::string("Exit with code ") + std::to_string(e.exit_code()) + "'.");
    } catch(duktape::script_error& e) {
      test_fail("Unexpected script error: ", e.callstack());
    } catch(duktape::engine_error& e) {
      test_fail("Unexpected engine error: ", e.what());
    } catch(std::exception& e) {
      test_fail("Unexpected exception: '", e.what(), "'.");
    } catch (...) {
      test_fail("Unexpected exception.");
    }
  } catch(std::exception& e) {
    test_fail("Unexpected init exception: '", e.what(), "'.");
  } catch (...) {
    test_fail("Unexpected init exception.");
  }
  ::sw::utest::tmpdir::remove();
  return sw::utest::test::summary();
}
// </editor-fold>

#endif
