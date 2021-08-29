/**
 * @file serial_port.hh
 * @author Stefan Wilhelm
 * @license MIT
 * @ccflags
 * @ldflags
 * @platform linux,(bsd),windows
 * @standard >= c++11
 * -----------------------------------------------------------------------------
 *
 * Serial port handling.
 *
 * -----------------------------------------------------------------------------
 * +++ MIT license +++
 * Copyright 2012-2017, Stefan Wilhelm (stfwi, <cerbero s@atwilly s.de>)
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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef SW_SERIAL_PORT_HH
#define	SW_SERIAL_PORT_HH
#if defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
  #ifndef __WINDOWS__
    #define __WINDOWS__
  #endif
  #include <windows.h>
  #if defined(__MINGW32__) || defined(__MINGW64__)
    #include <stdint.h>
  #else
    #error "no idea which stdint.h to use."
  #endif
#else
  #include <stdint.h>
  #include <cstring>
  #include <termios.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <sys/types.h>
  #include <ctype.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <time.h>
  #ifdef __linux__
    #include <sys/file.h>
    #include <sys/stat.h>
    #include <sys/poll.h>
    #include <dirent.h>
  #endif
#endif
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <thread>
#include <chrono>
#include <type_traits>
#include <unordered_map>


namespace sw { namespace com { namespace detail {

template <typename=void>
class basic_serial_port
{
public:

  using string_type = std::string;
  using port_type = string_type;
  using baudrate_type = int ;
  using buffer_size_type = std::size_t;
  using size_type = std::size_t;
  using ssize_type = int;
  using timeout_type = int;

  typedef enum { databits_7=7, databits_8=8 } databits_type;
  typedef enum { stopbits_1=0, stopbits_15=1, stopbits_2=2 } stopbits_type;
  typedef enum { parity_none=0, parity_odd=1, parity_even=2 } parity_type;
  typedef enum { flowcontrol_none=0, flowcontrol_xonxoff=1, flowcontrol_rtscts=2 } flowcontrol_type;
  static constexpr timeout_type default_timeout = 10;

  #ifdef __WINDOWS__
  typedef HANDLE descriptor_type;
  #define invalid_descriptor INVALID_HANDLE_VALUE /* int to pointer cast not allowed in constexpr */
  static constexpr timeout_type tx_timeout_multiplier_ms_ = 10;
  #else
  typedef int descriptor_type;
  static constexpr descriptor_type invalid_descriptor = -1;
  #endif

  typedef enum {
    e_ok=0,     /// No error
    e_inval,    /// Invalid argument
    e_io,       /// I/O error
    e_parity,   /// Parity error
    e_rxtx,     /// Communication error like buffer overun, frame error ...
  } error_type;

  enum class device_match { strict, nonstrict };

public:
  /**
   * Returns a list of available COM ports and the devices.
   * @return std::unordered_map<string_type, string_type>
   */
  static std::unordered_map<string_type, string_type> device_list()
  {
    std::unordered_map<string_type, string_type> map;
    #ifdef __WINDOWS__
    HKEY hkey;
    char class_name[MAX_PATH] = {0};
    DWORD class_size=MAX_PATH, num_subkeys=0, num_values=0;
    if(::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
      if(::RegQueryInfoKeyA(hkey, class_name, &class_size,0, &num_subkeys, 0,0, &num_values, 0,0,0,0) == ERROR_SUCCESS) {
        for(DWORD i=0; i<num_values; i++) {
          char name[16383]={0}, data[256]={0};
          DWORD type = 0, name_size=sizeof(name), data_size=sizeof(data);
          if(RegEnumValueA(hkey, i, name, &name_size, 0, &type, (LPBYTE)data, &data_size) == ERROR_SUCCESS) {
            if((type != REG_SZ) || (!name[0]) || (!data[0])) continue;
            string_type s = name;
            auto pos = s.find_last_of("\\");
            if((pos != s.npos) && (pos < s.length()-1)) s = s.substr(pos+1);
            map[s.c_str()] = (const char*)data;
          }
        }
      }
      ::RegCloseKey(hkey);
    }
    #else
    // List /dev/ttyWHATEVER tty filtered or driver filtered.
    #endif
    #ifdef __linux__
    struct dir_gaurd {
      ::DIR *d;
      dir_gaurd() : d(nullptr) {};
      ~dir_gaurd() { if(d) ::closedir(d); }
    };
    dir_gaurd dg;
    const std::string basepath = "/dev";
    if((dg.d=::opendir(basepath.c_str())) != nullptr) {
      struct ::dirent *dir;
      while((dir=readdir(dg.d)) != nullptr) {
        std::string file = dir->d_name;
        if(file.find("ttyS") == 0 || file.find("ttyUSB") == 0) {
          map[file] = basepath + "/" + file;
        }
      }
      ::closedir(dg.d);
      dg.d = nullptr;
    }
    #endif
    return map;
  }

  /**
   * Tries to match a given device path or name (e.g. passed by the user)
   * to a correct, full path (or name like COM#).
   * @param path_or_name
   * @return string_type
   */
  static string_type nonstrict_device_match(string_type path_or_name)
  {
    if(path_or_name.empty()) return string_type();
    auto list = device_list();
    // 1st case: A correct device identifier was already given.
    for(auto& e:list) if(e.second == path_or_name) return e.second;
    // 2nd case: A correct name was already given.
    for(auto& e:list) if(e.first == path_or_name) return e.second;
    // 3nd case: Case mismatch.
    auto tolower = [](string_type s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; };
    path_or_name = tolower(path_or_name);
    for(auto& e:list) {
      if(tolower(e.first).find(path_or_name) != string_type::npos) return e.second;
      if(tolower(e.second) == path_or_name) return e.second;
    }
    return string_type();
  }

public:

  /**
   * c'tor
   */
  explicit inline basic_serial_port(
    port_type port = port_type(),
    baudrate_type baud = 115200,
    parity_type parity = parity_none,
    databits_type data_bits = databits_8,
    stopbits_type stop_bits = stopbits_1,
    flowcontrol_type flow_control = flowcontrol_none,
    timeout_type timeout_ms = default_timeout
  ) noexcept :
    d_(invalid_descriptor), port_(port), baudrate_(baud), databits_(data_bits), stopbits_(stop_bits),
    parity_(parity), flowcontrol_(flow_control), timeout_ms_(timeout_ms), error_(e_ok), error_message_()
    #ifndef _WINDOWS_
    , attr_orig_(), mdlns_orig_(0)
    #endif

  { ; }

  basic_serial_port(const basic_serial_port&) = delete;
  basic_serial_port(basic_serial_port&&) = default;
  basic_serial_port& operator=(const basic_serial_port&) = delete;
  basic_serial_port& operator=(basic_serial_port&&) = default;

  virtual ~basic_serial_port() noexcept
  {
    #ifdef __WINDOWS__
    if(d_ != invalid_descriptor) ::CloseHandle(d_);
    #else
    if(d_ >= 0) ::close(d_);
    #endif
  }

public:
  inline error_type error() const noexcept
  { return error_; }

  inline const string_type& error_message() const noexcept
  { return error_message_; }

  inline const port_type& port() const noexcept
  { return port_; }

  inline void port(port_type p) noexcept
  { port_.swap(p); }

  inline baudrate_type baudrate() const noexcept
  { return baudrate_; }

  inline void baudrate(baudrate_type baud) noexcept
  { baudrate_ = baud; }

  inline parity_type parity() const noexcept
  { return parity_; }

  inline void parity(parity_type par) noexcept
  { parity_ = par; }

  inline databits_type databits() const noexcept
  { return databits_; }

  inline void databits(databits_type data_bits) noexcept
  { databits_ = data_bits; }

  inline stopbits_type stopbits() const noexcept
  { return stopbits_; }

  inline void stopbits(stopbits_type stop_bits) noexcept
  { stopbits_ = stop_bits; }

  inline flowcontrol_type flowcontrol() const noexcept
  { return flowcontrol_; }

  inline void flowcontrol(flowcontrol_type flow_control) noexcept
  { flowcontrol_ = flow_control; }

  inline timeout_type timeout() const noexcept
  { return timeout_ms_; }

  inline void timeout(timeout_type timeout) noexcept
  { timeout_ms_ = timeout < 0 ? 0 : timeout; }

  /**
   * Returns true if the port is closed, false
   * if it is open.
   *
   * @return bool
   */
  inline bool closed() const noexcept
  { return d_ == invalid_descriptor; }

  inline descriptor_type descriptor() const noexcept
  { return d_; }

public:

  /**
   * Open the port, defining the port path/name to open and leaving
   * all port settings as they are. On error the port is closed()
   * and an error is set accordingly.
   * @param port_type port
   * @return bool
   */
  bool open(port_type port)
  { close(); this->port(port); return open(); }

  /**
   * Open the port, specifying the port name/path and settings.
   * All setting that are omitted are implicitly set to the
   * default values. On error, the port is closed()  and an
   * error is set accordingly.
   * @param port_type port
   * @param string_type settings
   * @return bool
   */
  bool open(port_type port, string_type settings)
  { close(); return this->settings(port + "," + settings) && open(); }

  /**
   * Open the port, specifying the parameters in detail.
   * @param port_type port
   * @param baudrate_type baudrate
   * @param databits_type databits
   * @param stopbits_type stopbits
   * @param parity_type parity
   * @return bool
   */
  bool open(
    port_type port,
    baudrate_type baudrate,
    databits_type databits=databits_8,
    stopbits_type stopbits=stopbits_1,
    parity_type parity=parity_none,
    device_match strict=device_match::nonstrict
  ) {
    close();
    this->settings(port, strict);
    this->baudrate(baudrate);
    this->databits(databits);
    this->stopbits(stopbits);
    this->parity(parity);
    return open();
  }

  /**
   * Open the port. On error, the port is closed()
   * and an error is set accordingly.
   * @return bool
   */
  bool open()
  {
    close();
    error_ = e_ok;
    error_message_.clear();

    if(port_.empty()) {
      error_ = e_inval;
      error_message_ = "No port specified.";
      return false;
    }

    #ifdef __WINDOWS__
    string_type port_path = string_type("\\\\.\\") + port_;
    d_ = ::CreateFileA(
      port_path.c_str(),
      GENERIC_READ|GENERIC_WRITE, 0, 0,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0
    );
    if((!d_) || (d_ == invalid_descriptor)) {
      d_ = invalid_descriptor;
      set_error();
      string_type s = error_message_;
      error_message_ = string_type("Failed to open ") + port_path + " [" + s +  "]";
      error_ = e_io;
      return false;
    }
    ::SetCommMask(d_, 0); // No events
    DCB dcb;
    dcb.DCBlength = sizeof(dcb);
    if(!(::GetCommState(d_, &dcb))) {
      error_ = e_io;
      error_message_ = "Failed to get serial device parameters.";
      close();
      return false;
    }
    dcb.BaudRate = baudrate_;
    dcb.fBinary = TRUE; // must be according to MSDN
    dcb.fNull = FALSE; // don't discard \0 bytes on RX
    dcb.fAbortOnError = FALSE; // We abort on error, means the port has to be reset after an error.

    switch(databits_) {
      case databits_7:
        dcb.ByteSize = 7;
        break;
      case databits_8:
        dcb.ByteSize = 8;
        break;
      default:
        error_ = e_inval;
        error_message_ = "Invalid number of data bits.";
        close();
        return false;
    }
    switch(stopbits_) {
      case stopbits_1:
        dcb.StopBits = ONESTOPBIT;
        break;
      case stopbits_15:
        dcb.StopBits = ONE5STOPBITS;
        break;
      case stopbits_2:
        dcb.StopBits = TWOSTOPBITS;
        break;
      default:
        error_ = e_inval;
        error_message_ = "Invalid number of stop bits.";
        close();
        return false;
    }
    switch(parity_) {
      case parity_none:
        dcb.fParity = FALSE;
        dcb.Parity = NOPARITY;
        break;
      case parity_odd:
        dcb.fParity = TRUE;
        dcb.Parity = ODDPARITY;
        break;
      case parity_even:
        dcb.fParity = TRUE;
        dcb.Parity = EVENPARITY;
        break;
      default:
        // we don't accept mark, space
        error_ = e_inval;
        error_message_ = "Invalid parity.";
        close();
        return false;
    }
    switch(flowcontrol_) {
      case flowcontrol_none:
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        break;
      case flowcontrol_xonxoff:
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
        dcb.XoffChar = 0x13;
        dcb.XonChar = 0x11;
        dcb.XoffLim = 1;
        dcb.XonLim = 0;
        break;
      case flowcontrol_rtscts:
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_TOGGLE;
        dcb.fOutxCtsFlow = TRUE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        break;
      default:
        error_ = e_inval;
        error_message_ = "Invalid flow control setting.";
        close();
        return false;
    }
    if(!(::SetCommState(d_, &dcb))) {
      error_ = e_inval;
      error_message_ = "Failed to set serial device parameters.";
      close();
      return false;
    }
    {
      auto to = COMMTIMEOUTS();
      to.ReadIntervalTimeout = MAXDWORD;
      to.ReadTotalTimeoutMultiplier = MAXDWORD;
      to.ReadTotalTimeoutConstant = 0;
      to.WriteTotalTimeoutConstant = timeout_ms_ < 50 ? 50 : timeout_ms_;
      to.WriteTotalTimeoutMultiplier = unsigned(tx_timeout_multiplier_ms_ > 10 ? tx_timeout_multiplier_ms_ : 10);
      if(!(::SetCommTimeouts(d_, &to))) {
        error_ = e_inval;
        error_message_ = "Failed to set serial device timeouts.";
        close();
      }
    }
    #else
    ::memset(&attr_orig_, 0, sizeof(attr_orig_));
    mdlns_orig_ = 0;
    int fd = -1;

    int n;
    if((fd=::open(port_.c_str(), O_RDWR|O_NDELAY|O_NOCTTY|O_CLOEXEC|O_EXCL|O_NONBLOCK)) < 0) {
      error_ = e_io;
      if(string_type(strerror(errno)).find("denied") != string_type::npos) {
        error_message_ = (string_type("Failed to open '") + port_ + "' (are you in group 'dialout'?): " + ::strerror(errno));
      } else {
        error_message_ = (string_type("Failed to open '") + port_ + "': " + ::strerror(errno));
        if(::access(port_.c_str(), 0) != 0) {
          std::string prt = port_;
          std::transform(prt.begin(), prt.end(), prt.begin(), ::tolower);
          auto list = device_list();
          for(auto& e:list) {
            std::string s = e.first;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            if(s.find(prt) == 0) {
              if((fd=::open(e.second.c_str(), O_RDWR|O_NDELAY|O_NOCTTY|O_CLOEXEC|O_EXCL|O_NONBLOCK)) >= 0) {
                error_ = e_ok;
                error_message_.clear();
                port_ = e.second;
                break;
              }
            }
          }
        }
      }
      if(fd < 0) return false;
    } else if(::flock(fd, LOCK_EX|LOCK_NB) < 0) {
      error_ = e_io;
      error_message_ = (string_type("Port is already in use (didn't get exclusive lock): '") + port_ + "' (error: '" + ::strerror(errno) + "').");
      ::close(fd);
      return false;
    } else if(!::isatty(fd)) {
      error_ = e_io;
      error_message_ = (string_type("Port '") + port_ + "' is no TTY.");
      ::close(fd);
      return false;
    } else if((timeout_ms_ >=0) && (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0)) {
      error_ = e_io;
      error_message_ = (string_type("Failed to set port nonblocking: ") + ::strerror(errno));
      ::close(fd);
    } else if(::tcgetattr(fd, &attr_orig_) < 0) {
      error_ = e_io;
      error_message_ = (string_type("Failed to get port terminal attributes of '") + port_ + "': " + ::strerror(errno) + ".");
      ::close(fd);
      return false;
    } else if(::ioctl(fd, TIOCMGET, &mdlns_orig_) < 0) {
      error_ = e_io;
      error_message_ = (string_type("Failed to get port io control of '") + port_ + "': " + ::strerror(errno) + ".");
      ::close(fd);
      return false;
    }
    struct ::termios attr = attr_orig_;
    ::cfmakeraw(&attr);
    attr.c_iflag &= ~(IGNBRK|BRKINT|ICRNL|INLCR|PARMRK|ISTRIP|IXON|IXOFF);
    attr.c_cflag &= ~(ECHO|ECHONL|ICANON|IEXTEN|ISIG|ISTRIP|CRTSCTS|CMSPAR);
    attr.c_oflag &= ~(OCRNL|ONLCR|ONLRET|ONOCR|OFILL|OPOST);
    attr.c_iflag = (attr.c_iflag & ~(IGNPAR|INPCK)) | (parity_ == parity_none ? IGNPAR : INPCK);
    attr.c_cflag = (attr.c_cflag & ~CSTOPB) | (stopbits_ == stopbits_1 ? 0 : CSTOPB); // @sw double check differing 1.5 and 2 stop bits
    attr.c_cflag = (attr.c_cflag & ~(HUPCL)) | CREAD | CLOCAL;
    attr.c_cflag = (attr.c_cflag & ~(PARENB))| (parity_ != parity_none ? PARENB : 0);
    attr.c_cflag = (attr.c_cflag & ~(PARODD))| (parity_ == parity_odd ? PARODD : 0);
    if(flowcontrol_ == flowcontrol_xonxoff) {
      attr.c_iflag |= IXON|IXOFF;
    } else if(flowcontrol_ == flowcontrol_rtscts) {
      attr.c_cflag |= CRTSCTS;
    }
    switch(static_cast<int>(databits_)) {
      case 5: n = CS5; break;
      case 6: n = CS6; break;
      case 7: n = CS7; break;
      case 8: n = CS8; break;
      default: n = CS8;
    }
    attr.c_cflag = (attr.c_cflag & ~(CSIZE)) | n;
    attr.c_cc[VTIME] = (timeout_ms_ / 100) + 1;
    attr.c_cc[VMIN]  = 0;
    ::speed_t br = B9600;
    switch(baudrate_) {
      case 50: br = B50; break;
      case 75: br = B75; break;
      case 110: br = B110; break;
      case 134: br = B134; break;
      case 150: br = B150; break;
      case 200: br = B200; break;
      case 300: br = B300; break;
      case 600: br = B600; break;
      case 1200: br = B1200; break;
      case 1800: br = B1800; break;
      case 2400: br = B2400; break;
      case 4800: br = B4800; break;
      #ifdef B7200
      case 7200: br = B7200; break;
      #endif
      case 9600: br = B9600; break;
      case 19200: br = B19200; break;
      case 38400: br = B38400; break;
      #ifdef B14400
      case 14400: br = B14400; break;
      #endif
      #ifdef B28800
      case 28800: br = B28800; break;
      #endif
      case 57600: br = B57600; break;
      #ifdef B76800
      case 76800: br = B76800; break;
      #endif
      case 115200: br = B115200; break;
      case 230400: br = B230400; break;
      #ifdef B921600
      case 460800: br = B460800; break;
      case 500000: br = B500000; break;
      case 576000: br = B576000; break;
      case 921600: br = B921600; break;
      #endif
      #ifdef B4000000
      case 1000000: br = B1000000; break;
      case 1152000: br = B1152000; break;
      case 1500000: br = B1500000; break;
      case 2000000: br = B2000000; break;
      case 2500000: br = B2500000; break;
      case 3000000: br = B3000000; break;
      case 3500000: br = B3500000; break;
      case 4000000: br = B4000000; break;
      #endif
      case 0:
      default:
        error_ = e_inval;
        error_message_ = (string_type("Invalid baudrate: ") + std::to_string(baudrate_));
    }
    ::cfsetispeed(&attr, br);
    ::cfsetospeed(&attr, br);
    ::tcflush(fd, TCIOFLUSH);
    if(::tcsetattr(fd, TCSANOW, &attr) < 0) {
      error_ = e_io;
      error_message_ = (string_type("Failed to set port terminal attributes: ") + ::strerror(errno));
      ::close(fd);
    }
    d_ = fd;
    if(flowcontrol_ == flowcontrol_rtscts) rts(true);
    #endif
    return (error_ == e_ok);
  }

  /**
   * Closes the port
   */
  inline void close() noexcept
  {
    #ifdef __WINDOWS__
    if(d_ != invalid_descriptor) ::CloseHandle(d_);
    d_ = invalid_descriptor;
    #else
    ::tcflush(d_, TCIOFLUSH);
    if(d_ >= 0) {
      for(size_t i=0; i<sizeof((attr_orig_)); i++) {
        if(((unsigned char*)&(attr_orig_))[i]) {
          ::tcsetattr(d_, TCSANOW, &(attr_orig_));
          ::ioctl(d_, TIOCMSET, &mdlns_orig_);
          break;
        }
      }
      ::close(d_);
      d_ = -1;
    }
    #endif
  }

  /**
   * Purge buffers, abort transmissions.
   */
  inline void purge() noexcept
  {
    #ifdef __WINDOWS__
    if(!closed()) ::PurgeComm(d_, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR);
    #else
    close(); open();
    #endif
  }

  /**
   * Reads from the port RX buffer, success, the number of bytes
   * read is placed into n_read. This operation is blocking until
   * `timeout_ms`. If the read operation is interrupted e.g. by
   * a signal, then the method will return `true`, and `n_read`
   * will be zero: further, the method will not restart reading
   * when receiving a signal, even if there is time left until
   * the `timeout_ms` expired (this allows actually reacting to
   * signals).
   *
   * @param void *data
   * @param size_type sz
   * @param size_type& n_read
   * @param timeout_type timeout_ms
   * @return bool
   */
  bool read(void *data, size_type sz, size_type& n_read, timeout_type timeout_ms)
  {
    n_read = 0;
    if(closed()) return false;
    if(sz > std::numeric_limits<size_type>::max()) {
      // Even if this is very likely an error, it is not up to us to decide
      // if this is unintended, but more than max ssize_t cannot be received.
      sz = static_cast<size_type>(std::numeric_limits<size_type>::max());
    }
    error_ = e_ok;
    error_message_.clear();
    #ifdef __WINDOWS__
      DWORD n = 0;
      bool ok = true;
      const auto deadline = milliclock() + timeout_ms;
      while(ok && (!n)) {
        ok = ::ReadFile(d_, data, sz, &n, NULL);
        if((timeout_ms<=0) || (milliclock() >= deadline)) break;
      }
      if(!ok) {
        set_error();
        return false;
      } else {
        DWORD errors;
        COMSTAT st;
        if(!::ClearCommError(d_, &errors, &st)) {
          set_error();
          close();
          return false;
        } else if(errors) {
          error_ = e_rxtx;
          error_message_ = "Receive errors: ";
          if(errors & CE_BREAK)    { error_message_ += "break-condition "; }
          if(errors & CE_FRAME)    { error_message_ += "frame-error "; }
          if(errors & CE_OVERRUN)  { error_message_ += "buffer-overrun "; }
          if(errors & CE_RXOVER)   { error_message_ += "rx-overrun "; }
          if(errors & CE_RXPARITY) { error_ = e_parity; error_message_ += "parity-error"; }
          return false;
        } else {
          n_read = size_type(n);
          return true;
        }
      }
    #else
      if(!data) {
        error_ = e_inval;
        error_message_ = string_type("read: data nullptr passed.");
      } else if(d_ < 0) {
        error_ = e_io;
        error_message_ = string_type("Port not open.");
      } else if(!sz) {
        return true;
      } else {
        ssize_t n = 0;
        memset(data, 0, sz);
        if(timeout_ms != 0) {
          struct ::pollfd pfd = { d_, POLLIN|POLLPRI, 0 };
          struct ::timespec to, ts;
          if(::clock_gettime(CLOCK_MONOTONIC, &to) < 0) {
            error_message_ = string_type("Getting timeout initial time failed: ") + ::strerror(errno);
            error_ = e_io;
            return false;
          } else {
            to.tv_nsec += timeout_ms * 1000000;
            to.tv_sec += timeout_ms / 1000;
            if(to.tv_nsec >= 1000000000) {
              to.tv_sec += to.tv_nsec / 1000000000;
              to.tv_nsec %= 1000000000;
            }
            int r;
            while(((r=::poll(&pfd, 1, (int)(timeout_ms))) < 0) && (errno != EAGAIN)) {
              if(::clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
                error_message_ = string_type("Getting timeout initial time failed: ") + ::strerror(errno);
                error_ = e_io;
                return false;
              } else if((ts.tv_nsec > to.tv_sec) || ((ts.tv_nsec == to.tv_sec) && (ts.tv_nsec > to.tv_nsec))) {
                break;
              }
            }
            if(r < 0) {
              switch(errno) {
                case EAGAIN:
                case EINTR:
                case ETIMEDOUT:
                  // Note: These are not port errors.
                  return true;
                default:
                  error_message_ = string_type("Waiting for bytes to be received failed: ") + ::strerror(errno);
                  error_ = e_io;
                  return false;
              }
            }
            // no == 0 check, read nonblocking anyway to check for errors.
          }
        }
        if((n=::read(d_, data, sz)) >= 0) {
          n_read = size_type(n);
          return true;
        } else if((errno == EAGAIN) || (errno == EINTR)) {
          return true;
        } else {
          error_ = e_io;
          error_message_ = string_type("Reading port failed: ") + ::strerror(errno);
        }
      }
      return false;
    #endif
  }

  /**
   * Reads from the port RX buffer, returns success.
   * This operation is blocking until the current
   * value of `timeout()`.
   *
   * @param void *data
   * @param size_type sz
   * @param size_type& n_read
   * @return bool
   */
  inline bool read(void *data, size_type sz, size_type& n_read)
  { return read(data, sz, n_read, timeout()); }

  /**
   * Reads from the port RX buffer, returns success.
   * This operation is blocking until the current
   * value of `timeout`. If the read process is
   * interrupted or times out, then the return
   * value will be `true` and data contain the
   * received characters until then.
   *
   * @param std::string& data
   * @param timeout_type timeout_ms
   * @return bool
   */
  inline bool read(std::string& data, timeout_type timeout_ms)
  {
    const auto deadline = milliclock() + timeout_ms;
    data.clear();
    size_type n_read = 0;
    bool ok = false;
    char buffer[4096];
    while((ok=read(buffer, sizeof(buffer), n_read, timeout_ms)) && n_read) {
      data.append(buffer, n_read);
      const auto now = milliclock();
      if(now >= deadline) break;
      timeout_ms -= timeout_type(deadline-now);
    }
    return ok;
  }

  /**
   * Reads from the port RX buffer, returns success.
   * This operation is blocking until the current
   * value of `timeout()`. If the read process is
   * interrupted or times out, then the return value
   * will be `true` and data contain the received
   * characters until then.
   *
   * @param std::string& data
   * @return bool
   */
  inline bool read(std::string& data)
  { return read(data, timeout()); }

  /**
   * Writes into the port TX buffer, returns success,
   * the number of bytes written is placed in n_written.
   * This operation is blocking.
   * If the operation is interrupted due to signal
   * (e.g. EINTR), then the method will return
   * true and set n_written to 0 (no port error, only
   * nothing written).
   *
   * @param const void *data
   * @param size_type sz
   * @param size_type& n_written
   * @return bool
   */
  bool write(const void *data, size_type sz, size_type& n_written)
  {
    n_written = 0;
    if(closed()) return false;
    if(sz > std::numeric_limits<int>::max()) {
      error_ = e_inval;
      error_message_ = "write(): number of bytes to write too big.";
      return false;
    }
    error_ = e_ok;
    error_message_.clear();
    #ifdef __WINDOWS__
    DWORD n;
    if(!(::WriteFile(d_, data, sz, &n, nullptr))) {
      set_error();
      return false;
    } else {
      DWORD errors;
      COMSTAT comstat;
      if(!::ClearCommError(d_, &errors, &comstat)) {
        set_error();
        close();
        return false;
      } else if(errors) {
        error_ = e_rxtx;
        error_message_ = "Transmit errors: ";
        if(errors & CE_BREAK)    { error_message_ += "break-condition "; }
        if(errors & CE_FRAME)    { error_message_ += "frame-error "; }
        if(errors & CE_OVERRUN)  { error_message_ += "buffer-overrun "; }
        if(errors & CE_RXOVER)   { error_message_ += "rx-overrun "; }
        if(errors & CE_RXPARITY) { error_ = e_parity; error_message_ += "parity-error"; }
        return false;
      } else {
        n_written = size_type(n);
        return true;
      }
    }
    #else
    if(!data) {
      error_ = e_inval;
      error_message_ = "write: data nullptr passed.";
      return false;
    }
    ssize_t n = -1;
    while(n < 0) {
      if((n =::write(d_, data, sz)) < 0) {
        switch(errno) {
          case EAGAIN:
            std::this_thread::sleep_for(std::chrono::milliseconds(eagain_sleep_time_ms()));
            continue;
          case EINTR:
            return true; // n_written is 0.
          default:
            error_ = e_io;
            error_message_ = string_type("Writing to port failed: ") + ::strerror(errno);
            return false;
        }
      }
    }
    n_written = size_type(n);
    return true;
    #endif
  }

  /**
   * Writes into the port TX buffer, returns success.
   * This operation is blocking.
   *
   * @param const void *data
   * @param size_type sz
   * @return bool
   */
  inline bool write(const void *data, size_type sz)
  { size_type n; return write(data, sz, n); (void)n; }

  /**
   * Writes into the port TX buffer, returns success.
   * This operation is blocking.
   *
   * @param const char
   * @param size_type sz
   * @return bool
   */
  inline bool write(const char data)
  { return write(&data, sizeof(char)); }

  /**
   * Writes a character string into the port TX buffer,
   * returns true if all characters were written, false
   * on error. This operation is blocking.
   * If the operation is interrupted due to signal
   * (EINTR), then the method will also return false.
   *
   * @param const std::string& data
   * @param size_type& n_written
   * @return bool
   */
  inline bool write(const std::string& data, size_type& n_written)
  {
    size_type n=1;
    n_written=0;
    size_type n_left = data.size();
    while((n_left > 0) && (n > 0)) {
      if(!write(&(data[n_written]), n_left, n)) return false;
      n_left -= n;
      n_written += n;
    }
    return true;
  }

  /**
   * Writes a character string into the port TX buffer,
   * returns true if all characters were written, false
   * otherwise. This operation is blocking.
   *
   *
   * @param const std::string& data
   * @param size_type& n_written
   * @return bool
   */
  inline bool write(const std::string& data)
  { size_type n = 0; return write(data, n) && (n == data.size()); }

  /**
   * Writes a character string into the port TX buffer,
   * returns true if all characters were written, false
   * on error. This operation is blocking.
   * If the operation is interrupted due to signal
   * (EINTR), then the method will also return false.
   *
   * @param const std::string&& data
   * @param size_type& n_written
   * @return bool
   */
  inline bool write(const std::string&& data)
  { size_type n = 0; return write(data, n) && (n == data.size()); }

  /**
   * Returns the settings as string
   *
   * @param bool all
   * @return string_type
   */
  string_type settings() const
  {
    std::stringstream ss;
    ss << port_ << "," << ((long) baudrate_)
       << ((parity_ == parity_none) ? "N" : ((parity_ == parity_even) ? "E"
            : ((parity_ == parity_odd) ? "O" : "?")))
       << ((long) databits_)
       << (stopbits_==stopbits_1 ? "1" : ((stopbits_==stopbits_15 ? "1.5"
            : (((stopbits_==stopbits_2 ? "2" : "?"))))))
       ;

    switch(flowcontrol_) {
      case flowcontrol_none:
        break;
      case flowcontrol_xonxoff:
        ss << ",xonxoff";
        break;
      case flowcontrol_rtscts:
        ss << ",rtscts";
        break;
      default:
        ss << ",(unknown-flow-control)";
    }
    ss << ",timeout:" << ((long) timeout_ms_) << "ms";
    return ss.str();
  }

  /**
   * Parses the setting from string. On error, it sets the
   * error() and error_message() accordingly and returns false.
   * On success, it returns true (without changing the
   * error state).
   *
   * @param const string_type::value_type *str
   * @param device_match strict
   * @return bool
   */
  bool settings(const typename string_type::value_type *str, device_match strict=device_match::strict)
  { return settings(string_type(str)); }

  /**
   * Parses the setting from string. On error, it sets the
   * error() and error_message() accordingly and returns false.
   * On success, it returns true (without changing the
   * error state).
   *
   * @param string_type str
   * @param device_match strict=device_match::strict
   * @return bool
   */
  bool settings(string_type str, device_match strict=device_match::nonstrict)
  {
    string_type prt;
    string_type::size_type p = str.find(',');
    error_ = e_ok;
    error_message_.clear();
    if(p == string_type::npos) {
      prt.swap(str);
    } else if((p == 0) || (p >= str.length()-1)) {
      error_ = e_inval;
      error_message_ = "Invalid settings.";
      return false;
    } else {
      prt = str.substr(0, p);
      str = str.substr(p+1);
      std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }
    if(strict == device_match::nonstrict) {
      string_type matched_port = nonstrict_device_match(prt);
      if(!matched_port.empty()) prt.swap(matched_port);
    }
    #ifdef _WINDOWS_
      // Do some port name convention fixes: We
      // Simply use the last digits and add
      // "COM" before.
      if(!prt.empty()) {
        int i = (int) prt.length() - 1;
        while((i >= 0) && ::isdigit(prt[i])) --i;
        if(i < 0) {
          error_ = e_inval;
          error_message_ = "Invalid port name.";
          return false;
        } else {
          port_ = string_type("COM") + prt.substr(i+1);
        }
      }
    #else
      if(!prt.empty()) {
        struct ::stat st;
        if((::stat(prt.c_str(), &st) == 0) && (st.st_mode & S_IFCHR)) {
          port_ = prt;
        } else {
          error_ = e_inval;
          error_message_ = string_type("Not a character device: ") + prt;
          return false;
        }
      }
    #endif

    // Baud
    if(!str.empty()) {
      // ... the simple way.
      baudrate_type br = 0;
      string_type::size_type i=0;
      for(; i<str.length(); ++i) {
        if(!::isdigit(str[i])) break;
        br *= 10;
        br += str[i] - '0';
      }
      if(i >= str.length()-1) {
        str.clear();
      } else {
        str = str.substr(i);
      }
      baudrate_ = br;
    }

    // Parity
    if((strict == device_match::nonstrict) && (!str.empty() && (str[0] == ','))) {
      str = str.substr(1);
    }
    if(!str.empty() && (str[0] != ',')) {
      switch(str[0]) {
        case 'n':
          parity_ = parity_none;
          break;
        case 'o':
          parity_ = parity_odd;
          break;
        case 'e':
          parity_ = parity_even;
          break;
        default:
          error_ = e_inval;
          error_message_ = "Invalid parity.";
          return false;
      }
      str = str.substr(1);
    }

    // Databits
    if((strict == device_match::nonstrict) && (!str.empty() && (str[0] == ','))) {
      str = str.substr(1);
    }
    if(!str.empty() && (str[0] != ',')) {
      switch(str[0]) {
        case '7':
          databits_ = databits_7;
          break;
        case '8':
          databits_ = databits_8;
          break;
        default:
          error_ = e_inval;
          error_message_ = "Invalid number of databits.";
          return false;
      }
      str = str.substr(1);
    }

    // Stopbits
    if((strict == device_match::nonstrict) && (!str.empty() && (str[0] == ','))) {
      str = str.substr(1);
    }
    if(!str.empty() && (str[0] != ',')) {
      if(str[0] == '2') {
        stopbits_ = stopbits_2;
        if(str.length() > 1) {
          str = str.substr(1);
        } else {
          str.clear();
        }
      } else if(str.find("1.5") == 0) {
        stopbits_ = stopbits_15;
        if(str.length() > 3) {
          str = str.substr(3);
        } else {
          str.clear();
        }
      } else if(str[0] == '1') {
        stopbits_ = stopbits_1;
        if(str.length() > 1) {
          str = str.substr(1);
        } else {
          str.clear();
        }
      } else {
        error_ = e_inval;
        error_message_ = "Invalid number of stopbits.";
        return false;
      }
    }

    // Other settings
    flowcontrol_ = flowcontrol_none;
    std::stringstream ss(str);
    str.clear();
    while(std::getline(ss, str, ',')) {
      if(str.find("xon") == 0) {
        flowcontrol_ = flowcontrol_xonxoff;
      } else if(str.find("rts") == 0) {
        flowcontrol_ = flowcontrol_rtscts;
      } else if(str.find("timeout:") == 0) {
        const auto tos = str.substr(std::string("timeout:").size());
        timeout_ms_ = timeout_type((tos.empty() || (!::isdigit(tos[0]))) ? (-1) : (::atoi(tos.c_str())));
        if(timeout_ms_ < 0) {
          timeout_ms_ = default_timeout;
          error_ = e_inval;
          error_message_ = "Invalid timeout.";
          return false;
        }
      }
    }
    return true;
  }

  /**
   * Returns if RTS (output) is currently set
   * @return bool
   */
  bool rts() noexcept
  {
    #if defined(TIOCMGET)
    return tiocm_get(TIOCM_RTS);
    #else
    error_ = e_inval;
    return false;
    #endif
  }

  /**
   * Returns if CTS (input) is currently set.
   * @return bool
   */
  bool cts() noexcept
  {
    #if defined(TIOCMGET)
    return tiocm_get(TIOCM_CTS);
    #else
    error_ = e_inval;
    return false;
    #endif
  }

  /**
   * Returns if DTR (output) is currently set
   * @return bool
   */
  bool dtr() noexcept
  {
    #if defined(TIOCMGET)
    return tiocm_get(TIOCM_DTR);
    #else
    error_ = e_inval;
    return false;
    #endif
  }

  /**
   * Returns if DSR (input) is currently set.
   * @return bool
   */
  bool dsr() noexcept
  {
    #if defined(TIOCMGET)
    return tiocm_get(TIOCM_DSR);
    #else
    error_ = e_inval;
    return false;
    #endif
  }

  /**
   * Set RTS to the specified argument.
   * @param bool set
   */
  void rts(bool set) noexcept
  {
    #ifdef _WINDOWS_
    error_ = e_inval;
    (void)set;
    #elif defined(TIOCMGET)
    tiocm_set(TIOCM_RTS, set);
    #else
    error_ = e_inval;
    #endif
  }

  /**
   * Set DTR to the specified argument.
   * @param bool set
   */
  void dtr(bool set) noexcept
  {
    #ifdef _WINDOWS_
    error_ = e_inval;
    (void)set;
    #elif defined(TIOCMGET)
    tiocm_set(TIOCM_DTR, set);
    #else
    error_ = e_inval;
    #endif
  }

protected:

  static inline int64_t milliclock() noexcept
  { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

private:

  /**
   * Sets the error text to the system formatted message of
   * the last error code.
   */
  void set_error() noexcept
  {
    error_ = e_io;
    #ifdef __WINDOWS__
      char s[256];
      memset(s,0,sizeof(s));
      s[sizeof(s)-1] = '\0';
      ::FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), s, sizeof(s)-1, NULL
      );
      char* p = &s[strlen(s)-1];
      while(p>=s && (::isspace(*p) || (*p=='.'))) *p-- = '\0';
      error_message_ = s;
    #else
      error_message_ = ::strerror(errno);
    #endif
  }

  #if !defined(_WINDOWS_) && defined(TIOCMGET) && defined(TIOCMSET)

    template <typename T>
    bool tiocm_get(T what) noexcept
    {
      error_ = e_ok;
      error_message_.clear();
      int r;
      if(::ioctl(d_, TIOCMGET, &r) < 0) { set_error(); return false; }
      return (r & what) != 0;
    }

    template <typename T>
    void tiocm_set(T what, bool val) noexcept
    {
      error_ = e_ok;
      error_message_.clear();
      int r;
      if(::ioctl(d_, TIOCMGET, &r) < 0) { set_error(); return; }
      if(val) r |= what; else r &= ~(what);
      if(::ioctl(d_, TIOCMSET, &r) < 0) set_error();
    }

  #endif

private:
  // Definitely enough for 115200 baud and less and a UART buffer of about 1024 and more.
  static constexpr unsigned eagain_sleep_time_ms() { return 10; }
  descriptor_type d_;
  port_type port_;
  baudrate_type baudrate_;
  databits_type databits_;
  stopbits_type stopbits_;
  parity_type parity_;
  flowcontrol_type flowcontrol_;
  timeout_type timeout_ms_;
  error_type error_;
  string_type error_message_;
  #ifndef _WINDOWS_
  struct ::termios attr_orig_; // to restore settings when closing
  int mdlns_orig_;
  #else
  #undef invalid_descriptor
  #endif
};

}}}


namespace sw { namespace com { namespace detail {

  /**
   * Wrapper with text send/receiving
   */
  template <typename BaseType>
  class serial_tty : public BaseType
  {
  public:

    using base_type = BaseType;
    using string_type = typename base_type::string_type;

  public:

    /**
     * Args see BaseType, constructors must match, (std>=c++11, no concepts possible).
     */
    explicit inline serial_tty(
      typename base_type::port_type port = typename base_type::port_type(),
      typename base_type::baudrate_type baud = 115200,
      typename base_type::parity_type parity = base_type::parity_none,
      typename base_type::databits_type data_bits = base_type::databits_8,
      typename base_type::stopbits_type stop_bits = base_type::stopbits_1,
      typename base_type::flowcontrol_type flow_control = base_type::flowcontrol_none,
      typename base_type::timeout_type timeout_ms = base_type::default_timeout
    ) noexcept
      : base_type(port, baud, parity, data_bits, stop_bits, flow_control, timeout_ms),
      rx_newline_(),
      tx_newline_("\n"),
      rx_sanitizer_()
    {}

    serial_tty(const serial_tty&) = delete;
    serial_tty(serial_tty&&) = default;
    serial_tty& operator=(const serial_tty&) = delete;
    serial_tty& operator=(serial_tty&&) = default;
    virtual ~serial_tty() noexcept = default;

  public:

    /**
     * Returns the newline definition sequence for receiving,
     * e.g. "\r", "\n", "\r\n", or empty. If empty, both
     * '\r' and '\n' are valid newline characters.
     * @return const string_type&
     */
    inline const string_type& rx_newline() const noexcept
    { return rx_newline_; }

    /**
     * Sets the newline definition sequence for receiving,
     * @see `rx_newline() const`.
     * @param const string_type& nl
     * @return serial_tty&
     */
    inline serial_tty& rx_newline(const string_type& nl)
    { rx_newline_ = nl; return *this; }

    /**
     * Sets the newline definition sequence for receiving,
     * @see `rx_newline() const`.
     * @param string_type&& nl
     * @return serial_tty&
     */
    inline serial_tty& rx_newline(string_type&& nl) noexcept
    { rx_newline_.swap(nl); return *this; }

    /**
     * Returns the newline definition sequence for transmission,
     * e.g. "\r", "\n", "\r\n", or empty. The newline will be
     * appended when invoking `writeln()`.
     * @return const string_type&
     */
    inline const string_type& tx_newline() const noexcept
    { return tx_newline_; }

    /**
     * Sets the newline definition sequence for transmission,
     * @see `tx_newline() const`.
     * @param const string_type& nl
     * @return serial_tty&
     */
    inline serial_tty& tx_newline(const string_type& nl)
    { tx_newline_ = nl; return *this; }

    /**
     * Sets the newline definition sequence for transmission,
     * @see `tx_newline() const`.
     * @param string_type&& nl
     * @return serial_tty&
     */
    inline serial_tty& tx_newline(string_type&& nl) noexcept
    { tx_newline_.swap(nl); return *this; }

    /**
     * Assigns a character sanitizing function for the reception.
     * This function can replace e.g. invalid characters in the
     * string returned byref from `read(string&)`.
     * @param const FunctionType& fn
     * @return serial_tty&
     */
    template<typename FunctionType> // let it always match with any fntype, but bail on at the actual assignment.
    inline serial_tty& rx_sanitizer(const FunctionType& fn) noexcept
    { rx_sanitizer_ = fn; }

    /**
     * Returns the current input buffer contents of `readln()`.
     * @return const string_type&
     */
    inline const string_type& rx_buffer() const noexcept
    { return rx_buffer_; }

  public:

    /**
     * Closes the port and purges the `readln()` receive buffer.
     */
    inline void close() noexcept
    { base_type::close(); rx_buffer_.clear(); }

    /**
     * Purges the port and the `readln()` receive buffer.
     */
    inline void purge() noexcept
    { base_type::purge(); rx_buffer_.clear(); }

    /**
     * Reads from the port RX buffer, returns success.
     * This operation is blocking until the current
     * value of `timeout`. If the read process is
     * interrupted or times out, then the return
     * value will be `true` and data contain the
     * received characters until then.
     *
     * @param std::string& data
     * @param timeout_type timeout_ms
     * @return bool
     */
    inline bool read(std::string& data, typename base_type::timeout_type timeout_ms)
    {
      if(!serial_tty::read(data, timeout_ms)) return false;
      if(!!rx_sanitizer_) rx_sanitizer_(data);
      return true;
    }

    /**
     * Reads from the port RX buffer, returns success.
     * This operation is blocking until the current
     * value of `timeout()`. If the read process is
     * interrupted or times out, then the return value
     * will be `true` and data contain the received
     * characters until then.
     *
     * @param std::string& data
     * @return bool
     */
    inline bool read(std::string& data)
    { return serial_tty::read(data, base_type::timeout()); }

    /**
     * Write a line by appending the configured newline to
     * the given argument string. Writes blocking until all
     * bytes are written or an error has occurred. Returns
     * boolean success (on false check `error()`).
     * @param string_type tx
     * @return bool
     */
    inline bool writeln(string_type tx)
    {
      // todo: This can be improved according to performance
      //       and memory.
      tx.append(tx_newline());
      while(!tx.empty()) {
        auto n = typename base_type::size_type(0);
        if(!base_type::write(tx, n)) return false;
        if(n >= tx.size()) break;
        tx = tx.substr(n);
      }
      return true;
    }

    /**
     * Dedicated return type for `readln()`.
     */
    enum class readln_result { nothing=0, received=1, error=2, };

    /**
     * Reads a line (terminated with the specified `rx_newline()`) with
     * a given timeout. The result is stored in the `received` argument.
     * If `ignore_empty_lines==true`, the function continues reading if
     * the return value is an empty line.
     * Returns
     *  - `readln_result::nothing`  if no full line was received (timeout),
     *  - `readln_result::received` if a line was received
     *  - `readln_result::error`    if the underlaying port reported an error,
     *                              check `error()` and `error_message()`.
     *
     * @param string_type& received
     * @param const typename base_type::timeout_type timeout
     * @param bool ignore_empty_lines =false
     * @return readln_result
     */
    inline readln_result readln(std::vector<string_type>& received, const typename base_type::timeout_type timeout, bool ignore_empty_lines=false)
    {
      using namespace std;
      received.clear();
      auto to = ((timeout < 0) ? base_type::timeout() : timeout);
      const auto deadline = base_type::milliclock() + to + 1;
      do {
        {
          string rx;
          if(!serial_tty::read(rx, to)) return readln_result::error;
          if(!rx.empty()) rx_buffer_.append(rx);
        }
        if(rx_newline().empty()) {
          auto p = typename string_type::size_type(rx_buffer_.find_first_of("\r\n"));
          auto ps = typename string_type::size_type(0);
          while(p != rx_buffer_.npos) {
            const string rx = rx_buffer_.substr(ps, p-ps);
            if(!rx.empty() || !ignore_empty_lines) received.push_back(std::move(rx));
            ps = p + 1;
            if((ps < rx_buffer_.size()) && (rx_buffer_[p]=='\r') && (rx_buffer_[ps]=='\n')) ++ps; // auto \r\n
            p = rx_buffer_.find_first_of("\r\n", ps);
          }
          rx_buffer_ = rx_buffer_.substr(ps, p-ps);
        } else {
          auto p = typename string_type::size_type(rx_buffer_.find(rx_newline()));
          auto ps = typename string_type::size_type(0);
          while(p != rx_buffer_.npos) {
            const string rx = rx_buffer_.substr(ps, p-ps);
            if(!rx.empty() || !ignore_empty_lines) received.push_back(std::move(rx));
            ps = p + 1;
            p = rx_buffer_.find(rx_newline(), ps);
          }
          rx_buffer_ = rx_buffer_.substr(ps, p-ps);
        }
      } while(base_type::milliclock() <= deadline);
      return received.empty() ? readln_result::nothing : readln_result::received;
    }

  private:

    string_type rx_buffer_;   // Line read buffer.
    string_type rx_newline_;  // Reception line termination.
    string_type tx_newline_;  // Transmission line termination.
    std::function<void(string_type&)> rx_sanitizer_;
  };

}}}


namespace sw { namespace com {
  using serial_port = detail::basic_serial_port<>;
  using serial_tty  = detail::serial_tty<serial_port>;
}}

#endif
