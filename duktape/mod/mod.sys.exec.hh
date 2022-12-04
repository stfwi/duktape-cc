/**
 * @file duktape/mod/mod.sys.exec.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++14
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++11 -W -Wall -Wextra -pedantic -fstrict-aliasing
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper.
 *
 * Optional basic process execution.
 *
 * -----------------------------------------------------------------------------
 * License: http://opensource.org/licenses/MIT
 * Copyright (c) 2014-2022, the authors (see the @authors tag in this file).
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions: The above copyright notice and
 * this permission notice shall be included in all copies or substantial portions
 * of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef DUKTAPE_MOD_BASIC_PROCESS_EXEC_HH
#define DUKTAPE_MOD_BASIC_PROCESS_EXEC_HH
#include "../duktape.hh"
#include "mod.sys.os.hh"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>


/**
 * @file-snipplet sw.process.hh
 * @package de.atwillys.cc.duktape
 * @version
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++14
 */
#ifndef SW_SYS_PROCESS_SNIP
#define SW_SYS_PROCESS_SNIP
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

  #if defined(OS_WINDOWS)
    #include <windows.h>
    #include <processenv.h>
  #else
    #include <unistd.h>
    #include <cstdio>
    #include <cstring>
    #include <cstdlib>
    #include <cerrno>
    #include <cassert>
    #include <unistd.h>
    #include <poll.h>
    #include <time.h>
    #include <limits.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #ifdef OS_LINUX
      #include <wait.h>
    #else
      #include <sys/wait.h>
    #endif
  #endif

  namespace duktape { namespace detail { namespace system { namespace exec {

    template <typename=void>
    class child_process
    {
    public:

      using string_type = std::string;
      using arguments_type = std::vector<string_type>;
      using timeout_type = int;
      using exitcode_type = int;
      using milliseconds_type = long;

    public:

      ~child_process() noexcept = default;
      child_process(const child_process&) = delete;
      child_process(child_process&&) = default;
      child_process& operator=(const child_process&) = delete;
      child_process& operator=(child_process&&) = default;

      explicit child_process() :
        program_(), arguments_(), environment_(), timeout_ms_(), ignore_stdout_(), ignore_stderr_(), redirect_stderr_to_stdout_(),
        without_path_search_(), dont_inherit_environment_(), no_argument_escaping_(), exit_code_(-1), was_timeout_(false),
        process_terminated_(true), start_time_(), proc_(), stdin_data_(), stdout_data_(), stderr_data_()
      {}

      child_process(
        string_type program,
        arguments_type arguments,
        arguments_type environment,
        string_type stdin_data,
        timeout_type timeout_ms = 0,
        bool ignore_stdout = false,
        bool ignore_stderr = false,
        bool redirect_stderr_to_stdout = false,
        bool without_path_search = false,
        bool dont_inherit_environment = false,
        bool no_argument_escaping = true
      ) :
        program_(), arguments_(), environment_(), timeout_ms_(timeout_ms),
        ignore_stdout_(ignore_stdout), ignore_stderr_(ignore_stderr), redirect_stderr_to_stdout_(redirect_stderr_to_stdout),
        without_path_search_(without_path_search), dont_inherit_environment_(dont_inherit_environment),
        no_argument_escaping_(no_argument_escaping), exit_code_(-1), was_timeout_(false), process_terminated_(false),
        start_time_(), proc_(), stdin_data_(std::move(stdin_data)),
        stdout_data_(), stderr_data_()
      {
        run(std::move(program), std::move(arguments), std::move(environment), timeout_ms);
      }

    public:

      const string_type& program() const noexcept
      { return program_; }

      const arguments_type& arguments() const noexcept
      { return arguments_; }

      const arguments_type& environment() const noexcept
      { return environment_; }

      timeout_type timeout() const noexcept
      { return timeout_ms_; }

      child_process& timeout(timeout_type ms) noexcept
      { timeout_ms_ = ms; return *this; }

      bool ignore_stdout() const noexcept
      { return ignore_stdout_; }

      child_process& ignore_stdout(bool ignore) noexcept
      { ignore_stdout_ = ignore; return *this; }

      bool ignore_stderr() const noexcept
      { return ignore_stderr_; }

      child_process& ignore_stderr(bool ignore) noexcept
      { ignore_stderr_ = ignore; return *this; }

      bool redirect_stderr_to_stdout() const noexcept
      { return redirect_stderr_to_stdout_; }

      child_process& redirect_stderr_to_stdout(bool redirect) noexcept
      { redirect_stderr_to_stdout_ = redirect; return *this; }

      bool no_path_search() const noexcept
      { return without_path_search_; }

      child_process& no_path_search(bool no_path) noexcept
      { without_path_search_ = no_path; return *this; }

      bool no_inherited_environment() const noexcept
      { return dont_inherit_environment_; }

      child_process& no_inherited_environment(bool no_env) noexcept
      { dont_inherit_environment_ = no_env; return *this; }

      bool no_arg_escaping() const noexcept
      { return no_argument_escaping_; }

      child_process& no_arg_escaping(bool no_esc) noexcept
      { no_argument_escaping_ = no_esc; return *this; }

      exitcode_type exit_code() const noexcept
      { return was_timeout_ ? exitcode_type(-1) : exitcode_type(exit_code_); }

      bool running() const noexcept
      { return !process_terminated_; }

      bool timed_out() const noexcept
      { return was_timeout_; }

      milliseconds_type time_running_ms() const noexcept
      { return milliseconds_type(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time_).count()); }

      const string_type& stdin_data() const noexcept
      { return stdin_data_; }

      child_process& stdin_data(const string_type& stdin_data) noexcept
      { stdin_data_ = stdin_data; }

      child_process& stdin_data(string_type&& stdin_data) noexcept
      { stdin_data_ = stdin_data; }

      const string_type& stdout_data() const noexcept
      { return stdout_data_; }

      child_process& stdout_data(string_type&& text) noexcept
      { stdout_data_ = text; return *this; }

      string_type& stdout_data() noexcept
      { return stdout_data_; }

      const string_type& stderr_data() const noexcept
      { return stderr_data_; }

      child_process& stderr_data(string_type&& text) noexcept
      { stderr_data_ = text; return *this; }

      string_type& stderr_data() noexcept
      { return stderr_data_; }

    public:

      static string_type escape(string_type arg)
      {
        #ifndef OS_WINDOWS
        string_type esc("'");
        for(auto e:arg) {
          if((e=='\\') || (e=='\'')) esc.push_back('\\');
          esc.push_back(e);
        }
        return esc + "'";
        #else
        if(arg.empty()) return string_type("\"\"");
        if(arg.find_first_of(" \t\n\v\"") == arg.npos) return arg;
        string_type s("\"");
        for(auto it=arg.begin();; ++it) {
          unsigned nbacksl = 0;
          while(it != arg.end() && *it == '\\') { ++it; ++nbacksl; }
          if(it == arg.end()) {
            s.append(nbacksl * 2, '\\');
            break;
          } else if(*it == '"') {
            s.append(nbacksl * 2 + 1, '\\');
            s.push_back(*it);
          } else {
            s.append(nbacksl, '\\');
            s.push_back(*it);
          }
        }
        s.push_back('"');
        return s;
        #endif
      }

  #ifndef OS_WINDOWS

    private:
      using fd_t = int;

    public:

      void run(string_type program, arguments_type arguments, arguments_type environment, timeout_type timeout_ms=0)
      {
        using namespace std;
        process_terminated_ = true;
        timeout_ms_ = timeout_ms;
        start_time_ = std::chrono::steady_clock::now();
        ::pid_t pid = -1;
        vector<const char*> argv, envv;
        argv.push_back(program.c_str());
        for(auto& e:arguments) argv.push_back(e.c_str());
        argv.push_back(nullptr);
        if(!dont_inherit_environment_) {
          for(auto& e:environment) envv.push_back(e.c_str());
          envv.push_back(nullptr);
          envv.push_back(nullptr);
        } else {
          if(!environment.empty()) {
            if(environment.size() & 1) environment.push_back("");
            vector<string_type> tenv;
            for(size_t i=0; i<environment.size()-1; i+=2) {
              tenv.emplace_back(environment[i] + "=" + environment[i+1]);
            }
            environment.swap(tenv);
            for(auto& e:environment) envv.push_back(e.c_str());
          }
          envv.push_back(nullptr);
        }
        program_ = program;
        arguments_.swap(arguments);
        environment_.swap(environment);
        int err = 0;
        fd_t pi[2] = {-1,-1}, po[2] = {-1,-1}, pe[2] = {-1,-1};
        if(::pipe(pi)) {
          err = errno; pi[0] = pi[1] = -1;
        } else if(::pipe(po)) {
          err = errno; po[0] = po[1] = -1;
        } else if((!redirect_stderr_to_stdout_) && ::pipe(pe)) {
          err = errno; pe[0] = pe[1] = -1;
        } else if(((pid = ::fork()) < 0)) {
          err = errno; pid = -1;
        }
        if(err) {
          close_pipe(pi[0]); close_pipe(pi[1]);
          close_pipe(po[0]); close_pipe(po[1]);
          close_pipe(pe[0]); close_pipe(pe[1]);
          throw runtime_error(string_type("Failed to execute (pipe or fork failed): ") + ::strerror(err));
        }
        if(pid == 0) {
          // Child
          if(1
            && (::dup2(pi[0], STDIN_FILENO) >= 0)
            && (::dup2(po[1], STDOUT_FILENO) >= 0)
            && (::dup2((pe[1]>=0)?(pe[1]):(po[1]), STDERR_FILENO) >= 0)
          ) {
            for(int i=3; i<1024; ++i) ::close(i); // @sw: check how to safely get the highest fd.
            if(ignore_stdout_) { ::close(STDOUT_FILENO); ::open("/dev/null", O_WRONLY); }
            if(ignore_stderr_) { ::close(STDERR_FILENO); ::open("/dev/null", O_WRONLY); }
            if(!dont_inherit_environment_) {
              // Set additional environment over the own process env and run
              for(size_t i=0; (i < envv.size()-2) && envv[i] && envv[i+1]; i+=2) ::setenv(envv[i], envv[i+1], 1);
              if(without_path_search_) {
                ::execv(program.c_str(), (char* const*)(&argv[0]));
              } else {
                ::execvp(program.c_str(), (char* const*)(&argv[0]));
              }
            } else {
              if(without_path_search_) {
                ::execve(program.c_str(), (char* const*)(&argv[0]), (char* const*)(&envv[0]));
              } else {
                ::execvpe(program.c_str(), (char* const*)(&argv[0]), (char* const*)(&envv[0]));
              }
            }
            cerr << "Failed to run ''" << program << "': " << ::strerror(errno) << "\n";
          }
          _exit(1);
        } else {
          // Parent, close unused fds and set variables used further on.
          const auto unblock = [](fd_t fd) { int o; if((fd >=0) && (o=::fcntl(fd, F_GETFL, 0)) >= 0) { ::fcntl(fd, F_SETFL, o|O_NONBLOCK); } };
          proc_.ifd = pi[1]; unblock(proc_.ifd); close_pipe(pi[0]);
          proc_.ofd = po[0]; unblock(proc_.ofd); close_pipe(po[1]);
          proc_.efd = pe[0]; unblock(proc_.efd); close_pipe(pe[1]);
          proc_.pid = pid;
          if(stdin_data_.empty()) close_pipe(proc_.ifd);
          if(ignore_stdout_) close_pipe(proc_.ofd);
          if(ignore_stderr_) close_pipe(proc_.efd);
        }
        process_terminated_ = false;
      }

      bool update(milliseconds_type poll_wait)
      {
        using namespace std;
        if((timeout_ms_ > 1) && (proc_.pid > 0)) {
          using namespace std::chrono;
          constexpr int force_kill_after_additional_ms = 2500;
          const auto dt = duration_cast<milliseconds>(steady_clock::now() - start_time_).count();
          if(dt > timeout_ms_) {
            if(!was_timeout_) {
              was_timeout_ = true;
              ::kill(proc_.pid, SIGINT);
              ::kill(proc_.pid, SIGQUIT);
            } else if(dt > (timeout_ms_ + force_kill_after_additional_ms)) {
              ::kill(proc_.pid, SIGKILL);
            }
          }
        }
        // Write to child stdin if bytes left
        if(proc_.ifd >= 0) {
          const size_t size = stdin_data_.length();
          const void* data = stdin_data_.data();
          int r;
          if(size <= 0) {
            close_pipe(proc_.ifd);
          } else if((r=::write(proc_.ifd, data, size)) < 0) {
            switch(errno) {
              case EINTR:
              case EAGAIN:
                break;
              default:
                string_type().swap(stdin_data_);
            }
          } else if((!r) || (((size_t)r) == stdin_data_.length())) {
            string_type().swap(stdin_data_);
          } else {
            stdin_data_ = stdin_data_.substr(r);
          }
          if(stdin_data_.empty()) {
            close_pipe(proc_.ifd);
          }
        }
        // Read/check stderr and stdout pipes
        {
          int rp = -1;
          struct ::pollfd pfd[2] = {{0,0,0},{0,0,0}};
          if(!process_terminated_) {
            pfd[0].fd = proc_.ofd; pfd[0].events = POLLIN|POLLPRI; pfd[0].revents = 0;
            pfd[1].fd = proc_.efd; pfd[1].events = POLLIN|POLLPRI; pfd[1].revents = 0;
            rp=::poll(pfd, sizeof(pfd)/sizeof(struct ::pollfd), poll_wait);
          }
          if((proc_.ofd >= 0) && (pfd[0].revents || (rp < 0))) { // r<0: force read to close broken pipes
            char data[4096];
            auto r = ::read(proc_.ofd, data, sizeof(data));
            while(r > 0) {
              stdout_data_.append(data, size_t(r));
              r = ::read(proc_.ofd, data, sizeof(data));
            }
            if((!r) || ((r<0) && (errno != EAGAIN) && (errno != EINTR))) {
              close_pipe(proc_.ofd);
            }
          }
          if((proc_.efd >= 0) && (pfd[1].revents || (rp < 0))) {
            char data[4096];
            auto r = ::read(proc_.efd, data, sizeof(data));
            while(r > 0) {
              stderr_data_.append(data, size_t(r));
              r = ::read(proc_.efd, data, sizeof(data));
            }
            if((!r) || ((r<0) && (errno != EAGAIN) && (errno != EINTR))) {
              close_pipe(proc_.efd);
            }
          }
        }
        if(proc_.pid < 0) {
          process_terminated_ = true;
        } else {
          // Check if the child process has terminated
          int r, status = 0;
          if((r=::waitpid(proc_.pid, &status, WNOHANG)) < 0) {
            switch(errno) {
              case EAGAIN:
              case EINTR:
                break;
              case ECHILD:
                r = proc_.pid;
                break;
              default:
                ;
            }
          }
          if(r == proc_.pid) {
            exit_code_ = WEXITSTATUS(status);
            proc_.pid = -1;
          }
        }
        return running();
      }

      void kill(bool dash9)
      {
        if(proc_.pid < 0) return;
        if(dash9) {
          ::kill(proc_.pid, SIGKILL);
        } else {
          ::kill(proc_.pid, SIGINT);
          ::kill(proc_.pid, SIGQUIT);
        }
        proc_.close();
        update(0);
        exit_code_ = -1;
      }

    private:

      void close_pipe(fd_t& fd) noexcept
      { if(fd >= 0) { ::close(fd); fd=-1; } };

      struct process_handles
      {
        explicit process_handles() noexcept : pid(-1), ifd(-1), ofd(-1), efd(-1) {}
        process_handles(::pid_t p, fd_t i, fd_t o, fd_t e) noexcept : pid(p), ifd(i), ofd(o), efd(e) {}
        process_handles(const process_handles&) = delete;
        process_handles(process_handles&&) = default;
        process_handles& operator=(const process_handles&) = delete;
        process_handles& operator=(process_handles&&) = default;
        ~process_handles() noexcept { close(); }

        void close() noexcept
        {
          if(ifd >= 0) ::close(ifd);
          if(ofd >= 0) ::close(ofd);
          if(efd >= 0) ::close(efd);
          ifd = ofd = efd = -1;
          if(pid > 0) {
            ::kill(pid, SIGTERM);
            ::sleep(0);
            int status = -1, r;
            if(((r=::waitpid((const ::pid_t) pid, &status, WNOHANG)) == 0) || (r<0 && (errno == ECHILD))) {
              ::kill(pid, SIGKILL);
            }
            r = ::waitpid((const ::pid_t) pid, &status, 0);
          } else {
            pid = -1;
          }
        }

        ::pid_t pid;
        fd_t ifd, ofd, efd;
      };

      struct pipe_handles
      {};

  #else

    public:

      void run(string_type program, arguments_type arguments, arguments_type environment, timeout_type timeout_ms=0)
      {
        program_ = std::move(program);
        arguments_ = std::move(arguments);
        environment_ = std::move(environment);
        timeout_ms_ = timeout_ms;
        start_time_ = std::chrono::steady_clock::now();
        if(process_terminated_) throw std::runtime_error("Attempt to double initialize a process.");
        process_terminated_ = true;
        if(program_.empty()) throw std::runtime_error("No program to execute given.");
        // Prepare pipes.
        if(
          !proc_.in.r || !proc_.in.w || !proc_.out.r || !proc_.out.w || !proc_.err.r || !proc_.err.w ||
          !::SetHandleInformation(proc_.in.w, HANDLE_FLAG_INHERIT, 0) || // ensures heathen end are not inherited by the child
          !::SetHandleInformation(proc_.out.r, HANDLE_FLAG_INHERIT, 0) ||
          !::SetHandleInformation(proc_.err.r, HANDLE_FLAG_INHERIT, 0)
          // No SetNamedPipeHandleState(in.w etc, &PIPE_NOWAIT,0,0), see MSDN SetNamedPipeHandleState->PIPE_NOWAIT.
        ) {
          throw std::runtime_error("Creating pipes failed.");
        }
        // argv  ; @todo: utf8 to wstring when mingw codecvt working.
        string_type argv;
        string_type program_file = program_;
        std::replace(program_file.begin(), program_file.end(), '/', '\\');
        program_file = escape(program_file);
        argv.append(program_file);
        for(auto& e:arguments_) {
          argv.append(" ");
          if(no_argument_escaping_) {
            argv.append(e);
          } else {
            argv.append(escape(e));
          }
        }

        // envv
        string_type envv;
        {
          if(!dont_inherit_environment_) {

            struct envstrings
            {
              envstrings() : cstr(getenv()) {}
              ~envstrings() { if(cstr) ::FreeEnvironmentStringsA(cstr); }
              char* cstr;

              static char* getenv() noexcept
              {
                // Fixes mingw issue that `GetEnvironmentStringsA()` is not defined for `-DUNICODE`,
                // and `GetEnvironmentStrings` is a macro override for `GetEnvironmentStringsW`.
                // Whatever, fine, it surely makes sense in a greater context, but we need bytes.
                #if defined(GetEnvironmentStrings) && !defined(GetEnvironmentStringsA)
                  #define GetEnvironmentStrings_TMP GetEnvironmentStrings
                  #undef GetEnvironmentStrings
                  return GetEnvironmentStrings();
                  #define GetEnvironmentStrings GetEnvironmentStrings_TMP
                #else
                  return GetEnvironmentStringsA();
                #endif
              }
            };

            envstrings userenv;
            if(userenv.cstr) {
              bool was0 = false;
              for(auto p=userenv.cstr; (!was0) || (*p!='\0') ; ++p) {
                envv.push_back(char(*p));
                was0 = (*p=='\0');
              }
            }
          }
          if(!environment_.empty()) {
            for(size_t i=0; i<environment_.size()-1; i+=2) {
              envv += environment_[i] + "=" + environment_[i+1];
              envv.push_back('\0');
            }
          }
          for(auto i=0; i<4; ++i) envv.push_back('\0');
        }
        {
          STARTUPINFOA si = STARTUPINFOA();
          si.cb = sizeof(STARTUPINFOA);
          si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
          si.hStdOutput = proc_.out.w;
          si.hStdError = redirect_stderr_to_stdout_ ? proc_.out.w : proc_.err.w;
          si.hStdInput = proc_.in.r;
          si.wShowWindow = SW_HIDE;
          if(!::CreateProcessA(
            without_path_search_ ? &program_file[0] : nullptr, &argv[0], nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE|CREATE_NO_WINDOW,
            &envv[0], nullptr, &si, &proc_.pi)
          ) {
            switch(::GetLastError()) {
              case ERROR_FILE_NOT_FOUND:
              case ERROR_PATH_NOT_FOUND: {
                proc_.in.close();
                proc_.err.close();
                proc_.out.close();
                exit_code_ = -1;
                return;
              }
              default:
                throw std::runtime_error(string_type("Running program failed: ") + errstr());
            }
          }
        }
        if(stdin_data_.empty()) {
          proc_.in.close();
        } else {
          ::CloseHandle(proc_.in.r);
          proc_.in.r = nullptr;
        }
        if(redirect_stderr_to_stdout_) {
          proc_.err.close();
        } else {
          ::CloseHandle(proc_.err.w);
          proc_.err.w = nullptr;
        }
        ::CloseHandle(proc_.out.w);
        proc_.out.w = nullptr;
        process_terminated_ = false;
      }

      bool update(milliseconds_type poll_wait)
      {
        int n_loops_left = 2;
        while(--n_loops_left > 0) {
          if(!process_terminated_) {
            using namespace std::chrono;
            if((!was_timeout_) && (timeout_ms_ > 1) && (duration_cast<milliseconds>(steady_clock::now() - start_time_).count() > timeout_ms_)) {
              was_timeout_ = true;
              ::TerminateProcess(proc_.pi.hProcess, 1);
            }
            std::vector<HANDLE> handles;
            handles.push_back(proc_.pi.hProcess);
            if(proc_.out.r) handles.push_back(proc_.out.r);
            if(proc_.err.r) handles.push_back(proc_.err.r);
            switch(::WaitForMultipleObjects(handles.size(), &handles[0], false, DWORD(poll_wait))) {
              case WAIT_OBJECT_0+1: // out or err
              case WAIT_OBJECT_0+2: // err
                n_loops_left = 2;
                break;
              case WAIT_OBJECT_0:
                process_terminated_ = true;
                break;
              case ERROR_INVALID_HANDLE:
                ::TerminateProcess(proc_.pi.hProcess, 1);
                n_loops_left = 0;
                break;
              case WAIT_TIMEOUT:
              case WAIT_FAILED:
              default:
                n_loops_left = 0;
            }
          }
          if(proc_.in.w && !stdin_data_.empty()) {
            bool keep_writing = true;
            while(proc_.in.w && !stdin_data_.empty() && keep_writing) {
              DWORD n_written = 0;
              DWORD n_towrite = stdin_data_.size() > 4096 ? 4096 : stdin_data_.size();
              if(!::WriteFile(proc_.in.w, stdin_data_.data(), n_towrite, &n_written, nullptr)) {
                switch(::GetLastError()) {
                  case ERROR_PIPE_BUSY:
                    keep_writing = false;
                    break;
                  case ERROR_BROKEN_PIPE:
                  case ERROR_NO_DATA:
                  case ERROR_PIPE_NOT_CONNECTED:
                    keep_writing = false;
                    proc_.in.close();
                    // break: intentionally no break.
                  case ERROR_INVALID_HANDLE:
                    keep_writing = false;
                    proc_.in.w = nullptr;
                    stdin_data_.clear();
                  default:
                    throw std::runtime_error(string_type("Failed to write to pipe: ") + errstr());
                }
              }
              if(n_written) {
                if(n_written >= stdin_data_.size()) {
                  stdin_data_.clear();
                  keep_writing = false;
                  proc_.in.close();
                } else {
                  stdin_data_ = stdin_data_.substr(n_written);
                }
              }
              if(n_written < n_towrite) {
                keep_writing = false;
              }
            }
          }
          {
            string_type data = readpipe(proc_.out.r, ignore_stdout_);
            if(!data.empty()) { n_loops_left = 0; stdout_data_.append(std::move(data)); }
          }
          {
            string_type data = readpipe(proc_.err.r, ignore_stderr_);
            if(!data.empty()) { n_loops_left = 0; stderr_data_.append(std::move(data)); }
          }
        }
        if(process_terminated_) {
          // @sw: Adaption for JS pipe closing: No need to wait for the destructor - here, when the GC kicks in.
          proc_.in.close();
          proc_.out.close();
          proc_.err.close();
          if(!proc_.pi.hProcess) {
            exit_code_ = -1;
          } else {
            DWORD ec = 0;
            if(::GetExitCodeProcess(proc_.pi.hProcess, &ec)) {
              exit_code_ = int(ec);
            } else {
              throw std::runtime_error(string_type("Failed to get child process exit code: ") + errstr());
            }
          }
        }
        return running();
      }

      void kill(bool dash9)
      {
        (void)dash9;
        ::TerminateProcess(proc_.pi.hProcess, 1);
        update(0);
        exit_code_ = -1;
      }

    private:

      struct process_handles
      {
        struct pipe_handles
        {
          HANDLE r, w;
          pipe_handles() noexcept : r(nullptr), w(nullptr)
          {
            SECURITY_ATTRIBUTES sattr = SECURITY_ATTRIBUTES();
            sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
            sattr.bInheritHandle = TRUE;
            sattr.lpSecurityDescriptor = nullptr;
            ::CreatePipe(&r, &w, &sattr, 0);
          }
          ~pipe_handles() noexcept { close(); }
          void close() noexcept { if(r){::CloseHandle(r);} if(w){::CloseHandle(w);} r=w=nullptr; }
        };

        process_handles() : pi(), in(), out(), err() {}
        ~process_handles() noexcept
        {
          if(pi.hProcess) {
            if(::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT) ::TerminateProcess(pi.hProcess, 1);
            ::CloseHandle(pi.hProcess);
          }
          if(pi.hThread) ::CloseHandle(pi.hThread);
        }

        PROCESS_INFORMATION pi;
        pipe_handles in, out, err;
      };

      static string_type errstr() noexcept
      {
        string_type s(256,0);
        size_t n = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &s[0], s.size()-1, nullptr);
        if(!n) return string_type();
        s.resize(n);
        return s;
      };

      static string_type readpipe(HANDLE& hpipe, bool ignored)
      {
        auto data = string_type();
        if(!hpipe) return data;
        for(;;) {
          char buffer[4096+1];
          DWORD n_read=0, size=0;
          if(!::PeekNamedPipe(hpipe, nullptr, 0, nullptr, &size, nullptr)) {
            switch(::GetLastError()) {
              case ERROR_MORE_DATA:
                break;
              case ERROR_PIPE_BUSY:
                return data;
              case ERROR_HANDLE_EOF:
              case ERROR_BROKEN_PIPE:
              case ERROR_NO_DATA:
              case ERROR_PIPE_NOT_CONNECTED:
                ::CloseHandle(hpipe);
                // break: intentionally no break.
              case ERROR_INVALID_HANDLE:
                hpipe = nullptr;
                return data;
              default:
                throw std::runtime_error(string_type("Failed to read from pipe: ") + errstr());
            }
          }
          if(!size) return data;
          if(size > sizeof(buffer)-1) size = sizeof(buffer)-1;
          if(!::ReadFile(hpipe, buffer, size, &n_read, nullptr)) {
            switch(::GetLastError()) {
              case ERROR_MORE_DATA:
                break;
              case ERROR_PIPE_BUSY:
                return data;
              case ERROR_HANDLE_EOF:
              case ERROR_BROKEN_PIPE:
              case ERROR_NO_DATA:
              case ERROR_PIPE_NOT_CONNECTED:
                ::CloseHandle(hpipe);
                // break: intentionally no break.
              case ERROR_INVALID_HANDLE:
                hpipe = nullptr;
                return data;
              default:
                throw std::runtime_error(string_type("Failed to read from pipe: ") + errstr());
            }
          } else if(!n_read) {
            return data;
          } else if(!ignored) {
            buffer[n_read] = 0; // should have no effect on string copy creation, only for safety in doubt.
            data += string_type(buffer, buffer+n_read);
          }
        }
      }

  #endif

    private:

      string_type program_;
      arguments_type arguments_;
      arguments_type environment_;
      timeout_type timeout_ms_;
      bool ignore_stdout_;
      bool ignore_stderr_;
      bool redirect_stderr_to_stdout_;
      bool without_path_search_;
      bool dont_inherit_environment_;
      bool no_argument_escaping_;

      int exit_code_;
      bool was_timeout_;
      bool process_terminated_;
      std::chrono::steady_clock::time_point start_time_;
      process_handles proc_;
      string_type stdin_data_;
      string_type stdout_data_;
      string_type stderr_data_;
    };

  }}}}

  #pragma GCC diagnostic pop
#endif

namespace duktape { namespace detail { namespace system { namespace exec {

  namespace {

    template <typename=void>
    static void read_callback(duktape::api& stack, const duktape::api::index_type funct, std::string& buf, std::string& out, const char* data, const size_t size)
    {
      using namespace std;
      if(size) { // data not checked because reference known
        buf.append(data, size);
      }
      do {
        string chunk;
        auto p = buf.find('\n');
        if((p == buf.npos) && (size > 0)) {
          return; // no full line contained
        } else if(p >= buf.length()-1) {
          // Last char is newline (newline flushed output, not unusual)
          chunk.swap(buf);
        } else {
          chunk = buf.substr(0, p+1);
          if(chunk.length() <= 1) {
            buf.clear();
          } else {
            buf = buf.substr(p+1);
          }
        }
        while((!chunk.empty()) && ((chunk.back()=='\n') || (chunk.back()=='\r'))) chunk.pop_back();
        stack.dup(funct);
        stack.push(chunk);
        stack.call(1);
        if(stack.is<string>(-1)) {
          // 1. string: Means a modified version of the path shall be added.
          out.append(stack.to<string>(-1));
        } else if(stack.is<bool>(-1)) {
          if(stack.get<bool>(-1)) {
            // 2. true: add.
            out.append(chunk);
          } else {
            // 3. returns false: don't add.
          }
        }
        // else if(stack.is_undefined(-1) || stack.is_null(-1)) ..
        // 4. returns undefined: Means, don't add, the callback has saved it somewhere.
        // Other return values: don't add.
        stack.pop();
      } while(!buf.empty());
    }

    template <typename=void>
    static std::string arguments_import(
      duktape::api& stack,
      int& timeout_ms,
      std::string& program,
      std::vector<std::string>& arguments,
      std::vector<std::string>& environment,
      std::string& stdin_data,
      bool& without_path_search,
      bool& noenv,
      bool& ignore_stdout,
      bool& ignore_stderr,
      bool& redirect_stderr_to_stdout,
      bool& no_exception,
      duktape::api::index_type& stdout_callback,
      duktape::api::index_type& stderr_callback
    )
    {
      using index_type = duktape::api::index_type;
      index_type optindex = -1;
      if(stack.is_object(0)) {
        optindex = 0;
        no_exception = stack.get_prop_string<bool>(optindex, "noexcept", false);
        if(stack.top() > 1) {
          return "exec(): When passing an object as first argument means that this must be the only argument containing all information.";
        }
      } else if(stack.is<std::string>(0)) {
        program = stack.to<std::string>(0);
      } else {
        return "exec(): First argument must be the program to execute (string) an object with all execution arguments.";
      }
      if(stack.top() > 1) {
        if(stack.is_array(1)) {
          arguments = stack.get<std::vector<std::string>>(1);
        } else if(stack.is_object(1)) {
          optindex = 1;
          no_exception = stack.get_prop_string<bool>(optindex, "noexcept", false);
          if(stack.top() > 2) {
            return "exec(): After the option object (here argument 2) no further arguments can follow.";
          }
        } else if(!stack.is_undefined(1)) {
          return "exec(): Program arguments must be passed as array (2nd argument invalid)";
        }
      }
      if(stack.top() > 2) {
        if(!stack.is_object(2)) {
          return "exec(): Program execution options must be passed as object";
        } else {
          optindex = 2;
          no_exception = stack.get_prop_string<bool>(optindex, "noexcept", false);
        }
      }
      if(optindex >= 0) {
        // flags
        without_path_search = stack.get_prop_string<bool>(optindex, "nopath", false);
        noenv = stack.get_prop_string<bool>(optindex, "noenv", false);
        timeout_ms = stack.get_prop_string<int>(optindex, "timeout", -1);
        // program path/name (in $PATH)
        if(stack.get_prop_string(optindex, "program")) {
          if(!stack.is<std::string>(-1)) {
            return "exec(): Program path/name to execute must be a string.";
          } else if(optindex > 0) {
            return "exec(): Program path/name already set as first argument.";
          } else {
            program = stack.to<std::string>(-1);
          }
        }
        stack.pop();
        // arguments
        if(stack.get_prop_string(optindex, "args")) {
          if(optindex > 1) {
            return "exec(): Program arguments already defined as 2nd argument.";
          } else {
            arguments = stack.get<std::vector<std::string>>(-1);
            int i=0;
            for(auto e:arguments) {
              for(auto c:e) {
                if(c == '\0') {
                  return (std::string("Argument ") + std::to_string(i) + " contains a null character.");
                }
              }
            }
          }
        }
        stack.pop();
        // stdout
        if(stack.get_prop_string(optindex, "stdout")) {
          if(stack.is_boolean(-1) || stack.is_null(-1)) {
            ignore_stdout = !stack.get<bool>(-1);
          } else if(stack.is_function(-1)) {
            ignore_stdout = false;
            stdout_callback = stack.top()-1;
            stack.push(0);
          } else {
            return "Invalid value for the 'stdout' exec option.";
          }
        }
        stack.pop();
        // stderr
        if(stack.get_prop_string(optindex, "stderr")) {
          if(stack.is_boolean(-1) || stack.is_null(-1)) {
            ignore_stderr = !stack.get<bool>(-1);
          } else if(stack.is_function(-1)) {
            ignore_stderr = false;
            stderr_callback = stack.top()-1;
            stack.push(0);
          } else if(stack.is_string(-1) && (stack.get<std::string>(-1) == "stdout")) {
            redirect_stderr_to_stdout = true;
            ignore_stdout = false;
            ignore_stderr = false;
          } else {
            return "Invalid value for the 'stderr' exec option.";
          }
        }
        stack.pop();
        // stdin
        if(stack.get_prop_string(optindex, "stdin")) {
          if(stack.is_string(-1)) {
            stdin_data = stack.get<std::string>(-1);
          } else if(stack.is_false(-1) || stack.is_null(-1) || stack.is_undefined(-1)) {
            stdin_data.clear();
          } else if(stack.is_buffer(-1) || stack.is_buffer_data(-1)) {
            stdin_data = stack.buffer<std::string>(-1);
          } else {
            return "Invalid value for the 'stdin' exec option.";
          }
        }
        stack.pop();
        // env
        if(stack.get_prop_string(optindex, "env")) {
          if(!stack.is_object(-1) || stack.is_array(-1) || stack.is_function(-1)) {
            return "exec(): Environment must be passed as plain object.";
          } else {
            stack.enumerator(-1, duktape::api::enum_own_properties_only);
            while(stack.next(-1, true)) {
              environment.push_back(stack.get<std::string>(-2));
              environment.push_back(stack.to<std::string>(-1));
              stack.pop(2);
            }
            stack.pop();
          }
          for(auto e:environment) {
            for(auto c:e) {
              if(c == '\0' || c == '=') {
                return "Environment contains invalid characters.";
              }
            }
          }
        }
        stack.pop();
      }
      if(program.empty()) {
        return "exec(): Empty argument passed as program to execute.";
      }
      return std::string();
    }
  }

  template <typename=void>
  std::string escape_shell_arg(std::string arg)
  { return ::duktape::detail::system::exec::child_process<>::escape(std::move(arg)); }

  template <typename=void>
  int execute(duktape::api& stack)
  {
    using index_type = duktape::api::index_type;
    int exit_code = 0;
    int timeout_ms = -1;
    std::string program;
    std::vector<std::string> arguments, environment;
    std::string stdin_data, stdout_data, stderr_data;
    bool without_path_search = false;
    bool noenv = false;
    bool ignore_stdout = true;
    bool ignore_stderr = true;
    bool redirect_stderr_to_stdout = false;
    bool no_exception = false;
    index_type stdout_callback = -1;
    index_type stderr_callback = -1;
    {
      const std::string error = arguments_import(
        stack, timeout_ms, program, arguments, environment, stdin_data, without_path_search,
        noenv, ignore_stdout, ignore_stderr, redirect_stderr_to_stdout, no_exception, stdout_callback,
        stderr_callback
      );
      if(!error.empty()) {
        if(!no_exception) stack.throw_exception(error); // else just return undefined.
        return 0;
      }
    }
    try {
      auto proc = child_process(
        std::move(program), std::move(arguments), std::move(environment), std::move(stdin_data),
        timeout_ms, ignore_stdout, ignore_stderr, redirect_stderr_to_stdout, without_path_search,
        noenv, false
      );
      std::string stdout_buffer, stderr_buffer;
      do {
        proc.update(10);
        if(!proc.stdout_data().empty()) {
          std::string& data = proc.stdout_data();
          if(stdout_callback >= 0) {
            read_callback(stack, stdout_callback, stdout_buffer, stdout_data, data.data(), data.size());
          } else {
            stdout_data.append(data);
          }
          data.clear(); // Keeps allocated RAM in proc stdin.
        }
        if(!proc.stderr_data().empty()) {
          std::string& data = proc.stderr_data();
          if(stderr_callback >= 0) {
            read_callback(stack, stderr_callback, stderr_buffer, stderr_data, data.data(), data.size());
          } else {
            stderr_data.append(data);
          }
          data.clear();
        }
      }
      while(proc.running());
      exit_code = proc.exit_code();
      // Flush buffers, only applies if std***_callback is actually not -1.
      if(!stdout_buffer.empty()) read_callback(stack, stdout_callback, stdout_buffer, stdout_data, "", 0);
      if(!stderr_buffer.empty()) read_callback(stack, stderr_callback, stderr_buffer, stderr_data, "", 0);
      if(proc.timed_out() && (!no_exception)) return stack.throw_exception("timeout");
    } catch(const std::exception& e) {
      // Explicitly no catch(...), those errors should pass through.
      // Free buffer memories, as allocation is a potential error source.
      std::string().swap(stdout_data);
      std::string().swap(stderr_data);
      if(!no_exception) return stack.throw_exception(std::string() + e.what());
    }
    {
      stack.top(0);
      if(ignore_stdout && ignore_stderr) {
        stack.push(exit_code);
      } else {
        stack.push_object();
        stack.set("exitcode", exit_code);
        stack.set("stdout", stdout_data);
        stack.set("stderr", stderr_data);
      }
      return 1;
    }
  }

  template <typename=void>
  int execute_shell(duktape::api& stack)
  {
    std::string command = (stack.top() > 0) ? stack.to<std::string>(0) : std::string();
    int timeout_ms = (stack.top() > 1) ? stack.to<int>(1) : -1;
    if(command.empty()) {
      stack.push(std::string());
      return 1;
    } else {
      std::vector<std::string> args;
      #ifndef OS_WINDOWS
      constexpr bool no_arg_escape = false;
      const std::string sh = "/bin/sh";
      args.push_back("-c");
      args.push_back(command);
      #else
      constexpr bool no_arg_escape = true;
      const char* p = getenv("ComSpec");
      const std::string sh = (!p) ? std::string("c:\\Windows\\system32\\cmd.exe") : std::string(p);
      args.push_back(sh);
      args.push_back("/C");
      args.push_back(escape_shell_arg(command));
      #endif
      try {
        auto proc = child_process(
          std::move(sh), std::move(args), std::vector<std::string>(), std::string(), timeout_ms,
          false, true, false, true, false, no_arg_escape
        );
        while(proc.running()) {
          proc.update(10);
        }
        if(proc.timed_out()) {
          return 0;
        } else {
          stack.push(proc.stdout_data());
          return 1;
        }
      } catch(const std::exception& e) {
        return 0;
      }
    }
  }

}}}}

namespace duktape { namespace mod { namespace system { namespace exec {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace ::duktape::detail::system::exec;
    using native_process = ::duktape::detail::system::exec::child_process<void>;

    #if(0 && JSDOC)
    /**
     * Execute a process, optionally fetch stdout, stderr or pass stdin data.
     *
     * - The `program` is the path to the executable.
     *
     * - The `arguments` (if not omitted/undefined) is an array with values, which
     *   are coercible to strings.
     *
     * - The `options` is a plain object with additional flags and options.
     *   All these options are optional and have sensible default values:
     *
     *    {
     *      // Plain object for environment variables to set.
     *      env     : {object}={},
     *
     *      // Optional text that is passed to the program via stdin piping.
     *      stdin   : {String}="",
     *
     *      // If true the output is an object containing the fetched output in the property `stdout`.
     *      // The exit code is then stored in the property `exitcode`.
     *      // If it is a function, see callbacks below.
     *      stdout  : {boolean|function}=false,
     *
     *      // If true the output is an object containing the fetched output in the property `stderr`.
     *      // The exit code is then stored in the property `exitcode`.
     *      // If the value is "stdout", then the stderr output is redirected to stdout, and the
     *      // option `stdout` is implicitly set to `true` if it was `false`.
     *      // If it is a function, see callbacks below.
     *      stderr  : {boolean|function|"stdout"}=false,
     *
     *      // Normally the user environment is also available for the executed child process. That
     *      // might cause issues, e.g. with security. To prevent passing through the current environment,
     *      // set this property to `true`.
     *      noenv   : {boolean}=false,
     *
     *      // Normally the execution also uses the search path variable ($PATH) to determine which
     *      // program to run - Means setting the `program` to `env` or `/usr/bin/env` is pretty much
     *      // the same. However, you might not want that programs are searched. By setting this option
     *      // to true, you must use `/usr/bin/env`.
     *      nopath  : {boolean}=false,
     *
     *      // Normally the function throws exceptions on execution errors.
     *      // If that is not desired, set this option to `true`, and the function will return
     *      // `undefined` on errors. However, it is possible that invalid arguments or script
     *      // engine errors still throw.
     *      noexcept: {boolean}=false,
     *
     *      // The function can be called like `fs.exec( {options} )` (options 1st argument). In this
     *      // case the program to execute can be specified using the `program` property.
     *      program : {string},
     *
     *      // The function can also be called with the options as first or second argument. In both
     *      // cases the command line arguments to pass on to the execution can be passed as the `args`
     *      // property.
     *      args    : {array},
     *
     *      // Process run timeout in ms, the process will be terminated (and SIGKILL killed later if
     *      // not terminating itself) if it runs longer than this timeout.
     *      timeout : {number}
     *
     *    }
     *
     * - The return value is:
     *
     *    - the exit code of the process, is no stdout nor stderr fetching is switched on,
     *
     *    - a plain object if any fetching is enabled:
     *
     *        {
     *          exitcode: {number},
     *          stdout  : {string},
     *          stderr  : {string}
     *        }
     *
     *    - `undefined` if exec exceptions are disabled and an error occurs.
     *
     * @throws {Error}
     * @param {string} program
     * @param {array} [arguments]
     * @param {object} [options]
     * @return {number|object}
     */
    sys.exec = function(program, arguments, options) {};
    #endif
    js.define("sys.exec", execute<>, -1);

    #if(0 && JSDOC)
    /**
     * Execute a shell command and return the STDOUT output. Does
     * not throw exceptions. Returns an empty string on error. Does
     * not redirect stderr to stdout, this can be done in the shell
     * command itself. The command passed to the shell is intentionally
     * NOT escaped (no "'" to "\'" and no "\" to "\\").
     *
     * @throws {Error}
     * @param {string} command
     * @return {string}
     */
    sys.shell = function(command) {};
    #endif
    js.define("sys.shell", execute_shell<>, -1);

    #if(0 && JSDOC)
    /**
     * Adds quotes and escapes conflicting characters,
     * so that the given text can be used as a shell
     * argument.
     * (This is not needed for `sys.exec()`, as this
     * method allows specifying program arguments as
     * array. `sys.shell()` may very well need escaping).
     *
     * @param {string} arg
     * @return {string}
     */
    sys.escapeshellarg = function(arg) {};
    #endif
    js.define("sys.escapeshellarg", escape_shell_arg<void>);

    #if(0 && JSDOC)
    /**
     * Starts a program as child process of this application,
     * throws on error. The arguments are identical to
     * `sys.exec()`, except that the callbacks for `stdout`
     * and `stderr` do not apply. Output and error pipes of
     * the child processes can be accessed via the `stdout`
     * and `stderr` properties of this object.
     * To check if the process is still running use the
     * boolean `running` property, which implicitly updates
     * the stdin/stderr/stdout pipes.
     *
     * @constructor
     * @throws {Error}
     * @param {string} program
     * @param {array} [arguments]
     * @param {object} [options]
     *
     * @property {string}  program      - The program executed.
     * @property {array}   arguments    - The program CLI arguments passed in.
     * @property {object}  environment  - The program environment variables passed in or deduced.
     * @property {boolean} running      - True if the child process has not terminated yet. Updates stdout/stderr/stdin/exitcode.
     * @property {number}  exitcode     - The exit code of the child process, or -1.
     * @property {string}  stdout       - Current output received from the child process.
     * @property {string}  stderr       - Current stderr output received from the child process.
     * @property {string}  stdin        - Current stdin left to transmit to the child process.
     * @property {number}  runtime      - Time in seconds the process is running for.
     * @property {number}  timeout      - The configured timeout in milliseconds. The process will be "killed" after exceeding.
     */
    sys.process = function(program, arguments, options) {};
    #endif
    js.define(
      // Wrapped class specification and type.
      ::duktape::native_object<native_process>("sys.process")
      // Native constructor from script arguments.
      .constructor([](duktape::api& stack) {
        using index_type = duktape::api::index_type;
        int timeout_ms = -1;
        std::string program;
        std::vector<std::string> arguments, environment;
        std::string stdin_data, stdout_data, stderr_data;
        bool without_path_search = false;
        bool noenv = false;
        bool ignore_stdout = true;
        bool ignore_stderr = true;
        bool redirect_stderr_to_stdout = false;
        bool no_exception = false;
        index_type stdout_callback = -1;
        index_type stderr_callback = -1;
        const std::string error = arguments_import(
          stack, timeout_ms, program, arguments, environment, stdin_data, without_path_search,
          noenv, ignore_stdout, ignore_stderr, redirect_stderr_to_stdout, no_exception, stdout_callback,
          stderr_callback
        );
        if(!error.empty()) throw std::runtime_error(error);
        return new native_process(
          std::move(program), std::move(arguments), std::move(environment), std::move(stdin_data),
          timeout_ms, ignore_stdout, ignore_stderr, redirect_stderr_to_stdout, without_path_search,
          noenv, false
        );
      })
      .getter("program", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.program());
      })
      .getter("arguments", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.arguments());
      })
      .getter("environment", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.environment());
      })
      .getter("exitcode", [](duktape::api& stack, native_process& instance) {
        stack.push(int(instance.exit_code()));
      })
      .getter("stdout", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.stdout_data());
      })
      .setter("stdout", [](duktape::api& stack, native_process& instance) {
        instance.stdout_data(stack.to<std::string>(0));
      })
      .getter("stdin", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.stdin_data());
      })
      .getter("stderr", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.stderr_data());
      })
      .setter("stderr", [](duktape::api& stack, native_process& instance) {
        instance.stderr_data(stack.to<std::string>(0));
      })
      .getter("running", [](duktape::api& stack, native_process& instance) {
        instance.update(0);
        stack.push(instance.running());
      })
      .getter("runtime", [](duktape::api& stack, native_process& instance) {
        stack.push(double(instance.time_running_ms())*1e-3);
      })
      .getter("timeout", [](duktape::api& stack, native_process& instance) {
        stack.push(int(instance.timeout()));
      })
      .setter("timeout", [](duktape::api& stack, native_process& instance) {
        if((!stack.is<int>(-1)) || (stack.get<int>(-1)<0)) {
          stack.throw_exception("sys.process.timeout must be set to an number in milliseconds >0.");
        } else {
          instance.timeout(stack.get<int>(-1));
        }
      })
      .method("update", [](duktape::api& stack, native_process& instance) {
        instance.update(5);
        stack.push_this();
        return 1;
      })
      #if(0 && JSDOC)
      /**
       * Sends a termination signal to the child process, normally
       * a graceful termination request (SIGTERM/SIGQUIT), when
       * `force` is `true`, the `SIGKILL` event is used.
       * On Windows this it applies a process termination, `force`
       * is ignored.
       *
       * @param {boolean} force
       */
      sys.process.prototype.kill = function(force) {};
      #endif
      .method("kill", [](duktape::api& stack, native_process& instance) {
        instance.kill(stack.is<bool>(0) && stack.get<bool>(0));
        stack.top(0);
        stack.push_this();
        return 1;
      })
      // undocumented testing properties.
      .getter("ignore_stdout", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.ignore_stdout());
      })
      .getter("ignore_stderr", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.ignore_stderr());
      })
      .getter("redirect_stderr_to_stdout", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.redirect_stderr_to_stdout());
      })
      .getter("no_path_search", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.no_path_search());
      })
      .getter("no_inherited_environment", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.no_inherited_environment());
      })
      .getter("no_arg_escaping", [](duktape::api& stack, native_process& instance) {
        stack.push(instance.no_arg_escaping());
      })
    );
  }

}}}}

#endif
