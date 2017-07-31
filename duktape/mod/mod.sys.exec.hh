/**
 * @file duktape/mod/mod.sys.exec.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++11
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
 * Copyright (c) 2014-2017, the authors (see the @authors tag in this file).
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
#ifndef DUKTAPE_MOD_BASIC_PROCESS_EXEC_UNISTD_HH
#define DUKTAPE_MOD_BASIC_PROCESS_EXEC_UNISTD_HH

// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "../duktape.hh"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#if defined(_MSCVER) || defined(__MINGW32__) || defined(__MINGW64__)
  #include <windows.h>
  #include <processenv.h>
  #ifndef WINDOWS
    #define WINDOWS
  #endif
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
  #ifdef __linux__
    #include <wait.h>
  #else
    #include <sys/wait.h>
  #endif
#endif

#ifdef DUKTAPE_MOD_BASIC_PROCESS_EXEC_UNISTD_WITH_DEBUG
  #define clog__(X) { std::cerr << "[sys.exec()] " << X << std::endl; }
#else
  #define clog__(X) { ; }
#endif
// </editor-fold>

namespace duktape { namespace detail { namespace system { namespace exec {

  // <editor-fold desc="escape_shell_arg" defaultstate="collapsed">
  template <typename=void>
  std::string escape_shell_arg(std::string arg)
  {
    #ifndef WINDOWS
    std::string esc("'");
    for(auto e:arg) {
      if((e=='\\') || (e=='\'')) esc.push_back('\\');
      esc.push_back(e);
    }
    return esc + "'";
    #else
    if(arg.empty()) return std::string("\"\"");
    if(arg.find_first_of(" \t\n\v\"") == arg.npos) return arg;
    std::string s("\"");
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
  // </editor-fold>

  #ifndef WINDOWS
  // <editor-fold desc="execute_backend: linux" defaultstate="collapsed">
  template <typename StdOutCallback, typename StdErrCallback>
  void execute_backend(
    std::string program,
    std::vector<std::string> arguments,
    std::vector<std::string> environment,
    int& exit_code,
    StdOutCallback stdout_proc,
    StdErrCallback stderr_proc,
    std::string stdin_data,
    bool ignore_stdout,
    bool ignore_stderr,
    bool redirect_stderr_to_stdout,
    bool without_path_search,
    bool dont_inherit_environment,
    int timeout_ms,
    bool& was_timeout,
    bool no_argument_escaping=false
  )
  {
    (void) no_argument_escaping;
    using fd_t = int;
    auto start_time = std::chrono::steady_clock::now();

    auto unblock = [](fd_t fd) {
      int o;
      if((fd >=0) && (o=::fcntl(fd, F_GETFL, 0)) >= 0) {
        ::fcntl(fd, F_SETFL, o|O_NONBLOCK);
      }
    };

    auto close_pipe = [](fd_t& fd) {
      if(fd >= 0) {
        clog__("close_pipe(" << fd << ")");
        ::close(fd); fd = -1;
      }
    };

    struct proc_guard
    {
      ::pid_t& pid;
      fd_t& ifd;
      fd_t& ofd;
      fd_t& efd;

      explicit proc_guard(::pid_t& pid__, fd_t& ifd__, fd_t& ofd__, fd_t& efd__) noexcept :
        pid(pid__), ifd(ifd__), ofd(ofd__), efd(efd__)
      { }

      ~proc_guard() noexcept
      {
        clog__("~proc_guard() p=" << pid << ", i=" << ifd << ", o=" << ofd << ", e=" << efd);
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
    };

    ::pid_t pid = -1;
    fd_t ifd=-1, ofd=-1, efd=-1;

    {
      std::vector<const char*> argv, envv;
      argv.push_back(program.c_str());
      for(auto& e:arguments) argv.push_back(e.c_str());
      argv.push_back(nullptr);

      // note: we do this composition before forking.
      if(!dont_inherit_environment) {
        // for ::setenv()
        for(auto& e:environment) envv.push_back(e.c_str());
        envv.push_back(nullptr);
        envv.push_back(nullptr);
      } else {
        // for execve()/execvpe()
        if(!environment.empty()) {
          {
            if(environment.size() & 1) environment.push_back("");
            std::vector<std::string> tenv;
            for(size_t i=0; i<environment.size()-1; i+=2) {
              tenv.emplace_back(environment[i] + "=" + environment[i+1]);
            }
            environment.swap(tenv);
          }
          for(auto& e:environment) envv.push_back(e.c_str());
        }
        envv.push_back(nullptr);
      }

      // for(auto e:argv) std::cerr << (e ? e : "<nullptr>") << ", "; cerr << std::endl;
      // for(auto e:envv) std::cerr << (e ? e : "<nullptr>") << ", "; cerr << std::endl;
      int err = 0;
      fd_t pi[2] = {-1,-1}, po[2] = {-1,-1}, pe[2] = {-1,-1};

      if(::pipe(pi)) {
        err = errno; pi[0] = pi[1] = -1;
      } else if(::pipe(po)) {
        err = errno; po[0] = po[1] = -1;
      } else if((!redirect_stderr_to_stdout) && ::pipe(pe)) {
        err = errno; pe[0] = pe[1] = -1;
      } else if(((pid = ::fork()) < 0)) {
        err = errno; pid = -1;
      }

      if(err) {
        close_pipe(pi[0]); close_pipe(pi[1]);
        close_pipe(po[0]); close_pipe(po[1]);
        close_pipe(pe[0]); close_pipe(pe[1]);
        throw std::runtime_error(std::string("Failed to execute (pipe or fork failed): ") + ::strerror(err));
      }

      if(pid == 0) {
        // Child
        if(1
          && (::dup2(pi[0], STDIN_FILENO) >= 0)
          && (::dup2(po[1], STDOUT_FILENO) >= 0)
          && (::dup2((pe[1]>=0)?(pe[1]):(po[1]), STDERR_FILENO) >= 0)
        ) {
          for(int i=3; i<1024; ++i) ::close(i); // @sw: check how to safely get the highest fd.
          if(ignore_stdout) { ::close(STDOUT_FILENO); ::open("/dev/null", O_WRONLY); }
          if(ignore_stderr) { ::close(STDERR_FILENO); ::open("/dev/null", O_WRONLY); }
          if(!dont_inherit_environment) {
            // Set additional environment over the own process env and run
            for(size_t i=0; (i < envv.size()-2) && envv[i] && envv[i+1]; i+=2) ::setenv(envv[i], envv[i+1], 1);
            if(without_path_search) {
              ::execv(program.c_str(), (char* const*)(&argv[0]));
            } else {
              ::execvp(program.c_str(), (char* const*)(&argv[0]));
            }
          } else {
            if(without_path_search) {
              ::execve(program.c_str(), (char* const*)(&argv[0]), (char* const*)(&envv[0]));
            } else {
              ::execvpe(program.c_str(), (char* const*)(&argv[0]), (char* const*)(&envv[0]));
            }
          }
          std::cerr << "Failed to run ''" << program << "': " << ::strerror(errno) << std::endl;
        }
        _exit(1);
      } else {
        // Parent, close unused fds and set variables used further on.
        ifd = pi[1]; unblock(ifd); close_pipe(pi[0]);
        ofd = po[0]; unblock(ofd); close_pipe(po[1]);
        efd = pe[0]; unblock(efd); close_pipe(pe[1]);
        if(stdin_data.empty()) close_pipe(ifd);
        if(ignore_stdout) close_pipe(ofd);
        if(ignore_stderr) close_pipe(efd);
      }
    }

    proc_guard proc(pid, ifd, ofd, efd);
    bool done = false;
    constexpr int force_kill_after_additional_ms = 2500;
    while((pid > 0) || (ofd >= 0) || (efd >= 0)) {
      using namespace std::chrono;
      if((timeout_ms > 1) && (pid > 0)) {
        auto dt = int(duration_cast<milliseconds>(steady_clock::now() - start_time).count());
        clog__("timeout left == " << timeout_ms-dt);
        if(dt > timeout_ms) {
          if(!was_timeout) {
            was_timeout = true;
            clog__("timeout: " << dt <<  " --> kill(child pid=" << pid << ", SIGINT)");
            ::kill(pid, SIGINT);
            ::kill(pid, SIGQUIT);
          } else if(dt > (timeout_ms + force_kill_after_additional_ms)) {
            clog__("timeout: " << dt <<  " --> kill(child pid=" << pid << ", KILL)");
            ::kill(pid, SIGKILL);
            break;
          }
        }
      }

      // Write to child stdin if bytes left
      if(ifd >= 0) {
        size_t size = stdin_data.length();
        const void* data = stdin_data.data();
        int r;
        if(size <= 0) {
          close_pipe(ifd);
        } else if((r=::write(ifd, data, size)) < 0) {
          switch(errno) {
            case EINTR:
            case EAGAIN:
              break;
            default:
              clog__("Failed to write n=" << std::dec << size << " bytes to child stdin: " << ::strerror(errno));
              std::string().swap(stdin_data);
          }
        } else if((!r) || (((size_t)r) == stdin_data.length())) {
          std::string().swap(stdin_data);
        } else {
          stdin_data = stdin_data.substr(r);
        }
        if(stdin_data.empty()) {
          close_pipe(ifd);
        }
      }

      // Read/check stderr and stdout pipes
      {
        int r = -1;
        struct ::pollfd pfd[2] = {{0,0,0},{0,0,0}};
        if(!done) {
          pfd[0].fd = ofd; pfd[0].events = POLLIN|POLLPRI; pfd[0].revents = 0;
          pfd[1].fd = efd; pfd[1].events = POLLIN|POLLPRI; pfd[1].revents = 0;
          r=::poll(pfd, sizeof(pfd)/sizeof(struct ::pollfd), 100);
          if(!r || (r < 0 && (errno == EAGAIN || errno == EINTR))) {
            // continue;
          }
        }
        if((ofd >= 0) && (pfd[0].revents || r < 0)) { // r<0: force read to close broken pipes
          char data[4096];
          ssize_t r = ::read(ofd, data, sizeof(data));
          while(r > 0) {
            if(!stdout_proc(std::string(data, data+size_t(r)))) {
              r = 0;
            } else {
              r = ::read(ofd, data, sizeof(data));
            }
          }
          if((!r) || ((r<0) && (errno != EAGAIN) && (errno != EINTR))) {
            close_pipe(ofd);
          }
        }
        if((efd >= 0) && (pfd[1].revents || r < 0)) {
          char data[4096];
          ssize_t r = ::read(efd, data, sizeof(data));
          while(r > 0) {
            if(!stderr_proc(std::string(data, data+size_t(r)))) {
              r = 0;
            } else {
              r = ::read(efd, data, sizeof(data));
            }
          }
          if((!r) || ((r<0) && (errno != EAGAIN) && (errno != EINTR))) {
            close_pipe(efd);
          }
        }
      }

      if(pid < 0) {
        //
        // That delays exiting the loop one iteration, so that pending
        // data can be
        //
        // Important: while loop break point here.
        if(done) break;
        done = true;
      } else {
        // Check if the child process has terminated
        int r, status = 0;
        if((r=::waitpid((const ::pid_t)pid, &status, WNOHANG)) < 0) {
          switch(errno) {
            case EAGAIN:
            case EINTR:
              break;
            case ECHILD:
              r = pid;
              break;
            default:
              clog__("waitpid() error '" << ::strerror(errno) << "'");
          }
        }
        if(r == pid) {
          exit_code = WEXITSTATUS(status);
          pid = -1;
        }
      }
    }
  }
  // </editor-fold>
  #else
  // <editor-fold desc="execute_backend: windows" defaultstate="collapsed">
  template <typename StdOutCallback, typename StdErrCallback>
  void execute_backend(
    std::string program,
    std::vector<std::string> arguments,
    std::vector<std::string> environment,
    int& exit_code,
    StdOutCallback stdout_proc,
    StdErrCallback stderr_proc,
    std::string stdin_data,
    bool ignore_stdout,
    bool ignore_stderr,
    bool redirect_stderr_to_stdout,
    bool without_path_search,
    bool dont_inherit_environment,
    int timeout_ms,
    bool& was_timeout,
    bool no_argument_escaping=false
  )
  {
    struct pipe_handles
    {
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
      HANDLE r, w;
    };

    struct process_handles
    {
      process_handles() : pi() { }

      ~process_handles() noexcept
      {
        if(pi.hProcess) {
          if(::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT) {
            ::TerminateProcess(pi.hProcess, 1);
          }
          ::CloseHandle(pi.hProcess);
        }
        if(pi.hThread) {
          ::CloseHandle(pi.hThread);
        }
      }

      PROCESS_INFORMATION pi;
    };

    auto errstr = []() -> std::string {
      std::string s(256,0);
      size_t n = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &s[0], s.size()-1, NULL);
      if(!n) return std::string();
      s.resize(n);
      return s;
    };

    auto start_time = std::chrono::steady_clock::now();
    exit_code = -1;
    if(program.empty()) throw std::runtime_error("No program to execute given.");
    pipe_handles in, out, err;
    process_handles proc;
    {
      if(
        !in.r || !in.w || !out.r || !out.w || !err.r || !err.w ||
        !::SetHandleInformation(in.w, HANDLE_FLAG_INHERIT, 0) || // ensures heathen end are not inherited by the child
        !::SetHandleInformation(out.r, HANDLE_FLAG_INHERIT, 0) ||
        !::SetHandleInformation(err.r, HANDLE_FLAG_INHERIT, 0)
        // No SetNamedPipeHandleState(in.w etc, &PIPE_NOWAIT,0,0), see MSDN SetNamedPipeHandleState->PIPE_NOWAIT.
      ) {
        throw std::runtime_error("Creating pipes failed.");
      }
    }

    //@todo: utf8 to wstring when mingw codecvt working.
    {
      // argv
      std::string argv;
      program = escape_shell_arg(program);
      argv.append(program);
      for(auto& e:arguments) {
        argv.append(" ");
        if(no_argument_escaping) {
          argv.append(e);
        } else {
          argv.append(escape_shell_arg<>(e));
        }
      }

      // envv
      std::string envv;
      {
        if(!dont_inherit_environment) {
          struct envstrings {
            envstrings() : cstr(nullptr) {
              cstr = ::GetEnvironmentStrings();
            }
            ~envstrings() {
              if(cstr) ::FreeEnvironmentStrings(cstr);
            }
            #ifdef UNICODE
            wchar_t* cstr;
            #else
            char* cstr;
            #endif
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
        if(!environment.empty()) {
          if(environment.size() & 1) environment.push_back("");
          for(size_t i=0; i<environment.size()-1; i+=2) {
            envv += environment[i] + "=" + environment[i+1];
            envv.push_back('\0');
          }
        }
        for(auto i=0; i<4; ++i) envv.push_back('\0');
      }

      STARTUPINFOA si = STARTUPINFOA();
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
      si.hStdOutput = out.w;
      si.hStdError = redirect_stderr_to_stdout ? out.w : err.w;
      si.hStdInput = in.r;
      si.wShowWindow = SW_HIDE;
      if(!::CreateProcessA(
        without_path_search ? &program[0] : nullptr, &argv[0], nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE|CREATE_NO_WINDOW,
        &envv[0], nullptr, &si, &proc.pi)
      ) {
        switch(::GetLastError()) {
          case ERROR_FILE_NOT_FOUND:
          case ERROR_PATH_NOT_FOUND:
            exit_code = 1;
            return;
          default:
            throw std::runtime_error(std::string("Running program failed: ") + errstr());
        }
      }
      if(stdin_data.empty()) {
        in.close();
      } else {
        ::CloseHandle(in.r);
        in.r = nullptr;
      }
      if(redirect_stderr_to_stdout) {
        err.close();
      } else {
        ::CloseHandle(err.w);
        err.w = nullptr;
      }
      ::CloseHandle(out.w);
      out.w = nullptr;
    }

    auto readpipe = [errstr](HANDLE& hpipe, std::string& data, bool ignored) {
      if(!hpipe) return;
      for(;;) {
        char buffer[4096+1];
        DWORD n_read=0, size=0;
        if(!::PeekNamedPipe(hpipe, nullptr, 0, nullptr, &size, nullptr)) {
          switch(::GetLastError()) {
            case ERROR_MORE_DATA:
              break;
            case ERROR_PIPE_BUSY:
              return;
            case ERROR_HANDLE_EOF:
            case ERROR_BROKEN_PIPE:
            case ERROR_NO_DATA:
            case ERROR_PIPE_NOT_CONNECTED:
              ::CloseHandle(hpipe);
              // break: intentionally no break.
            case ERROR_INVALID_HANDLE:
              hpipe = nullptr;
              return;
            default:
              throw std::runtime_error(std::string("Failed to read from pipe: ") + errstr());
          }
        }
        if(!size) return;
        if(size > sizeof(buffer)-1) size = sizeof(buffer)-1;
        if(!::ReadFile(hpipe, buffer, size, &n_read, nullptr)) {
          switch(::GetLastError()) {
            case ERROR_MORE_DATA:
              break;
            case ERROR_PIPE_BUSY:
              return;
            case ERROR_HANDLE_EOF:
            case ERROR_BROKEN_PIPE:
            case ERROR_NO_DATA:
            case ERROR_PIPE_NOT_CONNECTED:
              ::CloseHandle(hpipe);
              // break: intentionally no break.
            case ERROR_INVALID_HANDLE:
              hpipe = nullptr;
              return;
            default:
              throw std::runtime_error(std::string("Failed to read from pipe: ") + errstr());
          }
        } else if(!n_read) {
          return;
        } else if(!ignored) {
          buffer[n_read] = 0; // should have no effect on string copy creation, only for safety in doubt.
          data += std::string(buffer, buffer+n_read);
        }
      }
    };

    bool process_terminated = false;
    int n_loops_left = 2;
    while(--n_loops_left > 0) {
      if(!process_terminated) {
        using namespace std::chrono;
        if((!was_timeout) && (timeout_ms > 1) && (duration_cast<milliseconds>(steady_clock::now() - start_time).count() > timeout_ms)) {
          was_timeout = true;
          ::TerminateProcess(proc.pi.hProcess, 1);
        }
        std::vector<HANDLE> handles;
        handles.push_back(proc.pi.hProcess);
        if(out.r) handles.push_back(out.r);
        if(err.r) handles.push_back(err.r);
        // switch(WaitForSingleObject(proc.pi.hProcess, 10)) {
        switch(WaitForMultipleObjects(handles.size(), &handles[0], false, 10)) {
          case WAIT_TIMEOUT:
          case WAIT_OBJECT_0+1: // out or err
          case WAIT_OBJECT_0+2: // err
            n_loops_left = 2;
            break;
          case WAIT_OBJECT_0:
            process_terminated = true;
            break;
          case WAIT_FAILED:
          default:
            n_loops_left = 0;
        }
      }
      if(in.w && !stdin_data.empty()) {
        bool keep_writing = true;
        while(in.w && !stdin_data.empty() && keep_writing) {
          DWORD n_written = 0;
          DWORD n_towrite = stdin_data.size() > 4096 ? 4096 : stdin_data.size();
          if(!::WriteFile(in.w, stdin_data.data(), n_towrite, &n_written, nullptr)) {
            switch(::GetLastError()) {
              case ERROR_PIPE_BUSY:
                keep_writing = false;
                break;
              case ERROR_BROKEN_PIPE:
              case ERROR_NO_DATA:
              case ERROR_PIPE_NOT_CONNECTED:
                in.close();
                // break: intentionally no break.
              case ERROR_INVALID_HANDLE:
                in.w = nullptr;
                stdin_data.clear();
              default:
                throw std::runtime_error(std::string("Failed to write to pipe: ") + errstr());
            }
          }
          if(n_written) {
            if(n_written >= stdin_data.size()) {
              stdin_data.clear();
              keep_writing = false;
              in.close();
            } else {
              stdin_data = stdin_data.substr(n_written);
            }
          }
          if(n_written < n_towrite) {
            keep_writing = false;
          }
        }
      }
      {
        std::string data;
        readpipe(out.r, data, ignore_stdout);
        if((!data.empty()) && (!stdout_proc(std::move(data)))) n_loops_left = 0;
      }
      {
        std::string data;
        readpipe(err.r, data, ignore_stderr);
        if((!data.empty()) && (!stderr_proc(std::move(data)))) n_loops_left = 0;
      }
    }
    if(process_terminated) {
      DWORD ec = 0;
      if(::GetExitCodeProcess(proc.pi.hProcess, &ec)) {
        exit_code = int(ec);
      } else {
        throw std::runtime_error(std::string("Failed to get child process exit code: ") + errstr());
      }
    } else {
      ::TerminateProcess(proc.pi.hProcess, 1);
    }
  }
  // </editor-fold>
  #endif

  // <editor-fold desc="execute" defaultstate="collapsed">
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
   *      env     : [Object]={},
   *
   *      // Optional text that is passed to the program via stdin piping.
   *      stdin   : [String]="",
   *
   *      // If true the output is an object containing the fetched output in the property `stdout`.
   *      // The exit code is then stored in the property `exitcode`.
   *      // If it is a function, see callbacks below.
   *      stdout  : [Boolean|Function]=false,
   *
   *      // If true the output is an object containing the fetched output in the property `stderr`.
   *      // The exit code is then stored in the property `exitcode`.
   *      // If the value is "stdout", then the stderr output is redirected to stdout, and the
   *      // option `stdout` is implicitly set to `true` if it was `false`.
   *      // If it is a function, see callbacks below.
   *      stderr  : [Boolean|Function|"stdout"]=false,
   *
   *      // Normally the user environment is also available for the executed child process. That
   *      // might cause issues, e.g. with security. To prevent passing through the current environment,
   *      // set this property to `true`.
   *      noenv   : [Boolean]=false,
   *
   *      // Normally the execution also uses the search path variable ($PATH) to determine which
   *      // program to run - Means setting the `program` to `env` or `/usr/bin/env` is pretty much
   *      // the same. However, you might not want that programs are searched. By setting this option
   *      // to true, you must use `/usr/bin/env`.
   *      nopath  : [Boolean]=false,
   *
   *      // Normally the function throws exceptions on execution errors.
   *      // If that is not desired, set this option to `true`, and the function will return
   *      // `undefined` on errors. However, it is possible that invalid arguments or script
   *      // engine errors still throw.
   *      noexcept: [Boolean]=false,
   *
   *      // The function can be called like `fs.exec( {options} )` (options 1st argument). In this
   *      // case the program to execute can be specified using the `program` property.
   *      program : [String],
   *
   *      // The function can also be called with the options as first or second argument. In both
   *      // cases the command line arguments to pass on to the execution can be passed as the `args`
   *      // property.
   *      args    : [Array],
   *
   *      // Process run timeout in ms, the process will be terminated (and SIGKILL killed later if
   *      // not terminating itself) if it runs longer than this timeout.
   *      timeout : [Number]
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
   *          exitcode: [Number],
   *          stdout: [String],
   *          stderr: [String]
   *        }
   *
   *    - `undefined` if exec exceptions are disabled and an error occurs.
   *
   * @throw Error
   * @param String program
   * @param undefined|Array arguments
   * @param undefined|Object options
   * @return Number|Object
   */
  sys.exec = function(program, arguments, options) {};
  #endif
  template <typename=void>
  int execute(duktape::api& stack)
  {
    // <editor-fold desc="types, nested functions" defaultstate="collapsed">
    using index_t = duktape::api::index_t;

    const auto read_callback = [&](const index_t funct, std::string& buf, std::string& out, const char* data, const size_t size) {
      if(size) { // data not checked because reference known
        buf.append(data, size);
      }
      do {
        std::string chunk;
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
        stack.dup(funct);
        stack.push(chunk);
        stack.call(1);

        if(stack.is<std::string>(-1)) {
          // 1. string: Means a modified version of the path shall be added.
          out.append(stack.to<std::string>(-1));
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
    };

    // </editor-fold>

    // <editor-fold desc="variables" defaultstate="collapsed">
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
    index_t stdout_callback = -1;
    index_t stderr_callback = -1;
    // </editor-fold>

    // <editor-fold desc="arguments" defaultstate="collapsed">
    {
      index_t optindex = -1;
      if(stack.is_object(0)) {
        optindex = 0;
        no_exception = stack.get_prop_string<bool>(optindex, "noexcept", false);
        if(stack.top() > 1) {
          if(!no_exception) stack.throw_exception("exec(): When passing an object as first argument means that this must be the only argument containing all information.");
          return 0;
        }
      } else if(stack.is<std::string>(0)) {
        program = stack.to<std::string>(0);
      } else {
        if(!no_exception) stack.throw_exception("exec(): First argument must be the program to execute (string) an object with all execution arguments.");
        return 0;
      }

      if(stack.top() > 1) {
        if(stack.is_array(1)) {
          arguments = stack.req<std::vector<std::string>>(1);
        } else if(stack.is_object(1)) {
          optindex = 1;
          no_exception = stack.get_prop_string<bool>(optindex, "noexcept", false);
          if(stack.top() > 2) {
            if(!no_exception) stack.throw_exception("exec(): After the option object (here argument 2) no further arguments can follow.");
            return 0;
          }
        } else if(!stack.is_undefined(1)) {
          if(!no_exception) stack.throw_exception("exec(): Program arguments must be passed as array (2nd argument invalid).");
          return 0;
        }
      }

      if(stack.top() > 2) {
        if(!stack.is_object(2)) {
          if(!no_exception) stack.throw_exception(std::string("exec(): Program execution options must be passed as object."));
          return 0;
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
            if(!no_exception) stack.throw_exception("exec(): Program path/name to execute must be a string.");
            return 0;
          } else if(optindex > 0) {
            if(!no_exception) stack.throw_exception("exec(): Program path/name already set as first argument.");
            return 0;
          } else {
            program = stack.to<std::string>(-1);
          }
        }
        stack.pop();

        // arguments
        if(stack.get_prop_string(optindex, "args")) {
          if(optindex > 1) {
            if(!no_exception) stack.throw_exception("exec(): Program arguments already defined as 2nd argument.");
            return 0;
          } else {
            arguments = stack.req<std::vector<std::string>>(-1);
            int i=0;
            for(auto e:arguments) {
              for(auto c:e) {
                if(c == '\0') {
                  if(!no_exception) stack.throw_exception(std::string("Argument " + std::to_string(i) + " contains a null character."));
                  return 0;
                }
              }
            }
          }
        }
        stack.pop();

        // stdout
        if(stack.get_prop_string(optindex, "stdout")) {
          if(stack.is_boolean(-1) || stack.is_null(-1)) {
            clog__("opts.stdout bool or null");
            ignore_stdout = !stack.get<bool>(-1);
          } else if(stack.is_function(-1)) {
            clog__("opts.stdout function");
            ignore_stdout = false;
            stdout_callback = stack.top()-1;
            stack.push(0);
          } else {
            if(!no_exception) stack.throw_exception(std::string("Invalid value for the 'stdout' exec option."));
            return 0;
          }
        }
        stack.pop();

        // stderr
        if(stack.get_prop_string(optindex, "stderr")) {
          if(stack.is_boolean(-1) || stack.is_null(-1)) {
            clog__("opts.stderr bool or null");
            ignore_stderr = !stack.get<bool>(-1);
          } else if(stack.is_function(-1)) {
            clog__("opts.stderr function");
            ignore_stderr = false;
            stderr_callback = stack.top()-1;
            stack.push(0);
          } else if(stack.is_string(-1) && (stack.get<std::string>(-1) == "stdout")) {
            clog__("opts.stderr === stdout");
            redirect_stderr_to_stdout = true;
            ignore_stdout = false;
            ignore_stderr = false;
          } else {
            if(!no_exception) stack.throw_exception(std::string("Invalid value for the 'stderr' exec option."));
            return 0;
          }
        }
        stack.pop();

        // stdin
        if(stack.get_prop_string(optindex, "stdin")) {
          if(stack.is_string(-1)) {
            stdin_data = stack.get<std::string>(-1);
            clog__("opts.stdin === string(" << stdin_data.length() << ")");
          } else if(stack.is_false(-1) || stack.is_null(-1) || stack.is_undefined(-1)) {
            clog__("opts.stdin === false/null/undefined");
            stdin_data.clear();
          } else if(stack.is_buffer(-1)) {
            clog__("opts.stdin === buffer");
            stdin_data = stack.get_buffer<std::string>(-1);
          } else {
            if(!no_exception) stack.throw_exception(std::string("Invalid value for the 'stdin' exec option."));
            return 0;
          }
        }
        stack.pop();

        // env
        if(stack.get_prop_string(optindex, "env")) {
          if(!stack.is_object(-1) || stack.is_array(-1) || stack.is_function(-1)) {
            if(!no_exception) stack.throw_exception(std::string("exec(): Environment must be passed as plain object."));
            return 0;
          } else {
            stack.enumerator(-1, duktape::api::enum_own_properties_only);
            while(stack.next(-1, true)) {
              environment.push_back(stack.req<std::string>(-2));
              environment.push_back(stack.to<std::string>(-1));
              stack.pop(2);
            }
            stack.pop();
          }
          for(auto e:environment) {
            for(auto c:e) {
              if(c == '\0' || c == '=') {
                if(!no_exception) stack.throw_exception("Environment contains invalid characters.");
                return 0;
              }
            }
          }
        }
        stack.pop();
      }

      if(program.empty()) {
        if(!no_exception) stack.throw_exception(std::string("exec(): Empty string passed as program to execute."));
        return 0;
      }

      #ifdef DUKTAPE_MOD_BASIC_PROCESS_EXEC_UNISTD_WITH_DEBUG
      {
        std::stringstream ss_args, ss_env;
        for(auto e:arguments) ss_args << " '" << e << "'";
        for(auto e:environment) ss_env << " '" << e << "'";
        clog__("program = '" << program << "'");
        clog__("arguments =" << ss_args.str());
        clog__("environment =" << ss_env.str());
        clog__("without_path_search = " << without_path_search);
        clog__("noenv = " << noenv);
        clog__("redirect_stderr_to_stdout = " << redirect_stderr_to_stdout);
        clog__("ignore_stderr = " << ignore_stderr);
        clog__("ignore_stdout = " << ignore_stdout);
        clog__("stdin_buffer.length() = " << stdin_data.length());
      }
      #endif
    }
    // </editor-fold>

    // <editor-fold desc="run" defaultstate="collapsed">
    try {
      std::string stdout_buffer, stderr_buffer;
      bool was_timeout = false;
      execute_backend(program, arguments, environment, exit_code,
        [&](std::string&& data){
          if(stdout_callback >= 0) {
            read_callback(stdout_callback, stdout_buffer, stdout_data, data.data(), data.size());
          } else {
            stdout_data.append(data);
          }
          return true;
        },
        [&](std::string&& data){
          if(stderr_callback >= 0) {
            read_callback(stderr_callback, stderr_buffer, stderr_data, data.data(), data.size());
          } else {
            stderr_data.append(data);
          }
          return true;
        },
        stdin_data,
        ignore_stdout, ignore_stderr, redirect_stderr_to_stdout, without_path_search, noenv, timeout_ms, was_timeout
      );
      // Flush buffers, note: only applies if std***_callback is actually not -1
      if(!stdout_buffer.empty()) read_callback(stdout_callback, stdout_buffer, stdout_data, "", 0);
      if(!stderr_buffer.empty()) read_callback(stderr_callback, stderr_buffer, stderr_data, "", 0);
      if(was_timeout && (!no_exception)) return stack.throw_exception("timeout");
    } catch(const std::exception& e) {
      // Explicitly no catch(...), those errors should pass through.
      // Free buffer memories, as allocation is a potential error source.
      std::string().swap(stdout_data);
      std::string().swap(stderr_data);
      if(!no_exception) return stack.throw_exception(std::string() + e.what());
    }
    // </editor-fold>

    // <editor-fold desc="return value composition" defaultstate="collapsed">
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
    // </editor-fold>
  }
  // </editor-fold>

  // <editor-fold desc="execute_shell" defaultstate="collapsed">
  #if(0 && JSDOC)
  /**
   * Execute a shell command and return the STDOUT output. Does
   * not throw exceptions. Returns an empty string on error. Does
   * not redirect stderr to stdout, this can be done in the shell
   * command itself. The command passed to the shell is intentionally
   * NOT escaped (no "'" to "\'" and no "\" to "\\").
   *
   * @throw Error
   * @param String command
   * @return String
   */
  sys.shell = function(command) {};
  #endif
  template <typename=void>
  int execute_shell(duktape::api& stack)
  {
    constexpr bool no_exception = true; // check what to do later
    std::string command = (stack.top() > 0) ? stack.to<std::string>(0) : std::string();
    int timeout_ms = (stack.top() > 1) ? stack.to<int>(1) : -1;
    std::string stdout_data;
    bool was_timeout = false;
    bool no_arg_escape = false;
    if(command.empty()) {
      stack.push(std::string());
      return 1;
    } else {
      std::vector<std::string> args;
      #ifndef WINDOWS
      const std::string sh = "/bin/sh";
      args.push_back("-c");
      args.push_back(command);
      #else
      const char* p = getenv("ComSpec");
      // @todo: That default backup path is not really a nice thing. Fortunately, comspec is quite
      //        always part of the env.
      const std::string sh = (!p) ? std::string("c:\\Windows\\system32\\cmd.exe") : std::string(p);
      args.push_back(std::string("/c \"") + command + "\"");
      no_arg_escape = true;
      #endif
      try {
        int exit_code = -1;
        execute_backend(sh, args, std::vector<std::string>(), exit_code,
          [&](std::string&& data){ stdout_data.append(data); return true; }, // stdout data callback
          [&](std::string&&){ return true; }, // stdout data callback
          std::string(), // stdin_data
          false,  // ignore_stdout,
          true,   // ignore_stderr
          false,  // redirect_stderr_to_stdout,
          true,   // without_path_search,
          false,  // noenv
          timeout_ms,
          was_timeout,
          no_arg_escape
        );
        if(was_timeout && (!no_exception)) {
          return stack.throw_exception("timeout");
        } else {
          stack.push(stdout_data);
          return 1;
        }
      } catch(const std::exception& e) {
        std::string().swap(stdout_data);
        if(!no_exception) stack.throw_exception(std::string() + e.what());
        return 0;
      }
    }
  }
  // </editor-fold>

}}}}

namespace duktape { namespace mod { namespace system { namespace exec {

  // <editor-fold desc="js decls" defaultstate="collapsed">
  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace ::duktape::detail::system::exec;
    js.define("sys.exec", execute<>, -1);
    js.define("sys.shell", execute_shell<>, -1);
    js.define("sys.escapeshellarg", escape_shell_arg<void>);
  }
  // </editor-fold>

}}}}

// <editor-fold desc="undefs" defaultstate="collapsed">
#undef clog__
// </editor-fold>

#endif
