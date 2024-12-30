/**
 * @file duktape/mod/mod.sys.socket.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++14
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 * @requires WIN32 LDFLAGS -lws2_32
 * @cxxflags -std=c++14 -W -Wall -Wextra -pedantic -fstrict-aliasing
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper, basic socket handling.
 *
 * Windows: Link with -lws2_32 .
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
#ifndef DUKTAPE_MOD_SYS_SOCKET_HH
#define DUKTAPE_MOD_SYS_SOCKET_HH
#include "./mod.sys.os.hh"
#ifdef OS_WINDOWS
  // ... please include winsock2.h before windows.h
  #include <winsock2.h>
#endif

#include "../duktape.hh"

#ifdef OS_WINDOWS
  #include <windows.h>
  #include <ws2tcpip.h>
  using sa_family_t = unsigned long;
  using socklen_t = int;
  using in_port_t = USHORT;
  struct sockaddr_un { ::sa_family_t sun_family; char sun_path[108]; }; // No AF_UNIX (unix domain socket) in win32, but the type is needed. Socket creation will fail at runtime.
  #define SCK_IDTYPE SOCKET
  #define SCK_INVALIDID (INVALID_SOCKET)
  #define SCK_ERROR (SOCKET_ERROR)
  #define SCK_ERRNO ((long)(::WSAGetLastError()))
  #define SCK_EINTR (WSAEINTR)
  #define SCK_ENEXIST (WSAENOTSOCK)
  #define SCK_ETIMEOUT (WSAETIMEDOUT)
  #define SCK_EAGAIN (WSAEWOULDBLOCK)
  #define SCK_EINVAL (WSAEINVAL)
  #define SCK_DEFAULT_TIMEOUT (1000)
  #define MSG_DONTWAIT 0
  namespace sw { namespace detail {
    template <typename=void>
    struct winsock_initializer
    {
      ~winsock_initializer() noexcept { if(initialized_) ::WSACleanup(); }
      explicit winsock_initializer() noexcept = default;
      winsock_initializer(const winsock_initializer&) = delete;
      winsock_initializer(winsock_initializer&&) = delete;
      winsock_initializer& operator=(const winsock_initializer&) = delete;
      winsock_initializer& operator=(winsock_initializer&&) = delete;
      void init() noexcept { if(!initialized_) { initialized_=true; err_=::WSAStartup(MAKEWORD(2,2), &wsadata_); } }
      bool initialized_;
      int err_;
      WSADATA wsadata_;
    };

    winsock_initializer<> wsinit_;
    #define SCK_INIT_SYSTEM() {::sw::detail::wsinit_.init();}
  }}
#else
  #include <cstring>
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <netdb.h>
  #include <poll.h>
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <signal.h>
  #include <errno.h>
  #include <unistd.h>
  #include <cstdlib>
  #define SCK_IDTYPE int
  #define SCK_INVALIDID (-1)
  #define SCK_ERROR (-1)
  #define SCK_ERRNO (errno)
  #define SCK_EINTR (EINTR)
  #define SCK_EINVAL (EINVAL)
  #define SCK_ENEXIST (ENOTSOCK)
  #define SCK_ETIMEOUT (ETIMEDOUT)
  #define SCK_EAGAIN (EWOULDBLOCK)
  #define SCK_DEFAULT_TIMEOUT (100)
  #define SCK_INIT_SYSTEM()
#endif
#include <string>

#ifndef SCK_LTRACE
  // I know, I know, marco. For temporary trouble shooting purposes it's ok.
  #define SCK_LTRACE(...) {void(0);}
#endif

namespace duktape { namespace detail { namespace system { namespace sw {

  template <typename=void>
  class ip_address
  {
  public:

    using address_sas_type = ::sockaddr_storage;  // socket storage
    using address_len_type = ::socklen_t;      // socket data length
    using port_type = ::in_port_t;            // inet port type
    using string_type = std::string;          // string type

    using family_type = ::sa_family_t;
    static constexpr family_type family_any  = AF_UNSPEC;
    static constexpr family_type family_ipv4 = AF_INET;
    static constexpr family_type family_ipv6 = AF_INET6;
    static constexpr family_type family_unix = AF_UNIX;

    using styp_type = int;
    static constexpr styp_type styp_any = 0;
    static constexpr styp_type styp_tcp = SOCK_STREAM;
    static constexpr styp_type styp_udp = SOCK_DGRAM;

    struct hostlookup_data_type { string_type saddr, shost; family_type family; address_len_type size; address_sas_type adr; };
    using hostlookup_list_type = std::vector<hostlookup_data_type>; // Intermediate list type for address lookups.

  public:

    explicit ip_address() noexcept = default;
    explicit ip_address(address_sas_type adr) noexcept : address_(adr) {}
    ip_address(const ip_address&) = default;
    ip_address(ip_address&&) = default;
    ip_address& operator=(const ip_address&) = default;
    ip_address& operator=(ip_address&&) = default;
    ~ip_address() = default;

    /**
     * Constructor from dotted string, optional with port ( "127.0.0.1:21" or "127.0.0.1")
     * If no port is defined, the port is unchanged = 0
     */
    ip_address(const string_type& s)
    { str(s); }

    /**
     * String assignment
     * @param const string_type& ip
     */
    ip_address& operator=(string_type ip)
    { str(std::move(ip)); }

  public:

    /**
     * Invalidates the address
     */
    void clear() noexcept
    { address_ = address_sas_type(); }

    /**
     * Returns if the address is set
     */
    bool valid() const noexcept
    { return address_.ss_family != 0; }

    /**
     * Returns if the currently stored address is IP V4
     * @return bool
     */
    bool is_ipv4() const noexcept
    { return address_.ss_family == AF_INET; }

    /**
     * Returns if the currently stored address is IP V6
     * @return bool
     */
    bool is_ipv6() const noexcept
    { return address_.ss_family == AF_INET6; }

    /**
     * Returns the address (storage type)
     */
    const address_sas_type& address() const noexcept
    { return address_; }

    /**
     * Clears the object and sets the address (storage type). You have to set the data size
     * yourself if the family is not `family_ipv4` or `family_ipv6`, otherwise the object is invalid.
     */
    void address(const address_sas_type& adr) noexcept
    { address_ = adr; }

    /**
     * Rreturns the address family
     */
    family_type family() const noexcept
    { return address_.ss_family; }

    /**
     * Sets the address family. Most assignment methods of this object set the family implicitly.
     * This function is required when defining addresses that are not AF_INET or AF_INET6.
     */
    ip_address& family(family_type family__) noexcept
    { address_.ss_family = family__; return *this; }

    /**
     * Returns the data size of the address structure dependent on the family, 0 for
     * unsupported socket family.
     * @return socklen_t
     */
    socklen_t size() const noexcept
    {
      switch(family()) {
        case family_ipv4: return sizeof(sockaddr_in);
        case family_ipv6: return sizeof(sockaddr_in6);
        case family_unix: return sizeof(sockaddr_un);
        default: return 0;
      }
    }

    /**
     * Returns the port, or 0 if not set or not applicable for the address type.
     * @return port_type
     */
    port_type port() const noexcept
    {
      switch(family()) {
        case family_ipv4: return ::ntohs(((sockaddr_in*)&address_)->sin_port);
        case family_ipv6: return ::ntohs(((sockaddr_in6*)&address_)->sin6_port);
        default: return 0;
      }
    }

    /**
     * Sets the port, ignored if not applicable for the address type.
     * For other families you must set the
     * port directly in the corresponding sockaddr_### structure and assign the storage type
     * to this address.
     */
    bool port(port_type port__) noexcept
    {
      switch(family()) {
        case family_ipv4: { reinterpret_cast<sockaddr_in&>(address_).sin_port = ::htons(port__); return true; }
        case family_ipv6: { reinterpret_cast<sockaddr_in6&>(address_).sin6_port = ::htons(port__); return true; }
        default: return false;
      }
    }

    /**
     * String representation of the address (without port, service etc - only the plain address)
     * @return string_type
     */
    string_type str() const
    {
      switch(address_.ss_family) {
        case AF_INET6: {
          const auto& adr = reinterpret_cast<const ::sockaddr_in6&>(address_);
          char s[INET6_ADDRSTRLEN+2];
          if(!::inet_ntop(AF_INET6, &adr.sin6_addr, s, sizeof(s))) return string_type();
          s[sizeof(s)-1] = '\0';
          return string_type("[") + s + "]:" + std::to_string(port());
        }
        case AF_INET: {
          const auto& adr = reinterpret_cast<const ::sockaddr_in&>(address_);
          char s[INET_ADDRSTRLEN+2];
          if(!::inet_ntop(AF_INET, &adr.sin_addr, s, sizeof(s))) return string_type();
          s[sizeof(s)-1] = '\0';
          return string_type(s) + ":" + std::to_string(port());
        }
        default: {
          return string_type();
        }
      }
    }

    /**
     * Set resource path with ip, port and path from string.
     * This method does not perform host lookups.
     * - 192.168.0.1    (v4 address)
     * - a::bcd0        (v6 address)
     * - 127.0.0.1:1234 (v4 address : port)
     * - [::1]:1234     (v6 address : port)
     */
    bool str(string_type adr)
    {
      clear();
      if(adr.size() < 2) return false; // shortest: "::"
      string_type port, subn;
      typename string_type::size_type p;
      if((p = adr.find('[')) != string_type::npos) { // [ v6 address </subn?> ] <:port?>
        auto p1 = adr.find(']');
        if(p != 0 || p1==string_type::npos || p1 < p) return false; // if [ is must be at the beginning
        if(p1 > 0 && (p1 < adr.size()-1)) {
          port = adr.substr(p1+1); // port may be empty or still contain ":"
        }
        adr.resize(p1);
        adr = adr.substr(p+1); // [ ] stripped, address may contain net mask
      } else if(adr.find(']') != string_type::npos) {
        return false; // ] without [
      }
      if((p = adr.find('/')) != string_type::npos) { // <v4/v6 address> </subn?> <:port?>
        if(p >= adr.size()-1) return false; // / is last character
        subn = adr.substr(p+1);
        auto p1 = subn.find(':');
        if(p1 != string_type::npos) {
          if(!port.empty()) return false; // [address/sub:port]:port not allowed
          if(p1==0 || p1 >= subn.size()-1) return false; // : is first or last char
          port = subn.substr(p1+1);
          subn.resize(p1);
        }
        adr.resize(p);  // <v4/v6 address>
      } else if((p=adr.find(':')) != string_type::npos) { // check if v4 with port
        if(adr.rfind(':') == p) { // only one ":" ->v4 is in the string (v6 has at least 2)
          if(!port.empty()) return false; // not possible
          if(p==0 || p >= subn.size()-1) return false; // : is first or last char
          port = adr.substr(p+1); // port may be empty or still contain :
          adr.resize(p);
        }
      }
      if(!port.empty() && port[0] == ':') { // Remove potential leading colon
        port = port.substr(1);
      }
      if(!port.empty() && (unsigned)std::count_if(port.begin(), port.end(), ::isdigit) < port.size()) {
        return false; // port contains non-digits
      } else if(subn.size() > 3) {
        return false; // even v6 subnet spec has only 3 digits
      } else  if(!subn.empty() && (unsigned)std::count_if(subn.begin(),subn.end(),::isdigit)<subn.size()) {
        return false; // subnet contains non-digits
      }
      // adr is now a plain v4, v6 or invalid address
      if(adr.find(':') != string_type::npos) {
        // assume v6
        auto addr = sockaddr_in6();
        memset(&addr, 0, sizeof(struct sockaddr_in6)); // @todo that should not be needed anymore.
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(0);
        if(::inet_pton(AF_INET6, adr.c_str(), &(addr.sin6_addr)) != 1) return false;
        address_.ss_family = AF_INET6;
        reinterpret_cast<sockaddr_in6&>(address_).sin6_addr = addr.sin6_addr;
      } else {
        // assume v4
        auto addr = ::sockaddr_in();
        addr.sin_family = AF_INET;
        addr.sin_port = htons(0);
        if(::inet_pton(AF_INET, adr.c_str(), &(addr.sin_addr)) != 1) return false;
        reinterpret_cast<sockaddr_in&>(address_).sin_addr = addr.sin_addr;
        address_.ss_family = AF_INET;
      }
      if(!port.empty()) ip_address::port((port_type)(::atol(port.c_str()))); // port check, only digits
      return true;
    }

  public:

    /**
     * Parse an address and additional data from a resource uri, e.g.
     *
     * - http://user@host:port/path,
     * - ftp://[::1]/path
     * - user@192.168.0.1
     *
     * First argument is your input string, second the address, the rest are the parsed output chunks.
     * Returns success.
     *
     * Note: This is a function intended to be used for ipv4/ipv6 clients.
     * Note: This function is not suitable to be used with subnet mask specifications (e.g. ab::c:d/64)
     *
     * @param string_type res_addr
     * @param ip_address& ip
     * @param string_type& service
     * @param string_type& user
     * @param string_type& host
     * @param string_type& path
     * @param bool lookup_host=false
     * @param family_type family=family_any
     * @param styp_type type=styp_tcp
     * @return bool
     */
    static bool parse_resource_address(string_type res_addr, ip_address& ip, string_type& service,
      string_type& user, string_type& host, string_type& path, bool lookup_host=false,
      family_type family=family_any, styp_type type=styp_tcp)
    {
      for(const auto c:res_addr) {
        if(!::isprint(c)) return false;
      }
      typename string_type::size_type pss, ps;
      if(res_addr.empty()) return false;
      pss = res_addr.find("://");
      ps = res_addr.find('/');
      if((pss != 0) && (pss != string_type::npos) && (ps > pss)) {
        service = res_addr.substr(0, pss);
        res_addr = res_addr.substr(pss+3);
      }
      if((ps = res_addr.find('/')) != string_type::npos) {
        if(ps == 0) return false; // only path or generally invalid, could cause an exception
        path = res_addr.substr(ps);
        res_addr.resize(res_addr.size() - path.size());
      }
      if((pss = res_addr.find('@')) != string_type::npos) {
        if(pss == 0) return false; // @ first char --> empty user
        user = res_addr.substr(0, pss);
        res_addr = res_addr.substr(pss+1);
      }
      string_type port;
      if(res_addr.find('[') != string_type::npos) {
        if((pss = res_addr.find("]:")) != string_type::npos) {
          if(pss >= res_addr.size()-2) return false;
          port = res_addr.substr(pss+2);
          res_addr.resize(pss+1); // including the ]
          if(std::count_if(port.begin(), port.end(), ::isdigit) < (int)port.size()) return false;
        }
      } else {
        if((pss = res_addr.find(":")) != string_type::npos) {
          if(pss >= res_addr.size()-1) return false;
          port = res_addr.substr(pss+1);
          res_addr.resize(pss);
          if(std::count_if(port.begin(), port.end(), ::isdigit) < (int)port.size()) return false;
        }
      }
      host = res_addr;
      ip.str(res_addr);
      if(!ip.valid() && lookup_host) {
        const hostlookup_list_type lst = host_lookup(res_addr, service, family, type);
        for(const auto& e:lst) {
          if(e.family == family_ipv4) { ip.address(e.adr); break; }
          if(e.family == family_ipv6) { ip.address(e.adr); break; }
        }
      }
      if(port.size() > 0) ip.port(port_type(::atol(port.c_str())));
      if((!ip.port()) && (!service.empty())) ip.port(port_by_service(service));
      return false;
    }

    /**
     * Performs a canonical name host lookup using ::getaddrinfo and returns a vector of the
     * resulting data.
     * @param string_type host
     * @param string_type service = ""
     * @param family_type family  = family_any
     * @param styp_type   type    = styp_any
     * @return hostlookup_list_type
     */
    static hostlookup_list_type host_lookup(string_type host, string_type service=string_type(), family_type family=family_any, styp_type type=styp_any)
    {
      hostlookup_list_type lst;
      if(host.empty()) return lst;
      ::addrinfo *ai=nullptr, *aii=nullptr, hints;
      char str_addr[INET6_ADDRSTRLEN+1];
      memset(str_addr, 0, sizeof(str_addr));
      memset(&hints, 0, sizeof(::addrinfo));
      hints.ai_family = family;
      hints.ai_socktype = type;
      hints.ai_flags = AI_CANONNAME;
      if(::getaddrinfo(host.c_str(), (service.empty() ? nullptr : service.c_str() ), &hints, &aii) || !aii) return lst;
      unsigned n_entries = 0;
      for(ai=aii; ai; ai=ai->ai_next) ++n_entries;
      if(n_entries > 0) {
        try {
          lst.reserve(n_entries);
          for(ai=aii; ai; ai=ai->ai_next) {
            str_addr[0] = 0;
            if(!ai->ai_addrlen || ai->ai_addrlen > sizeof(address_sas_type)) continue;
            if(ai->ai_family == AF_INET6) {
              if(ai->ai_addr->sa_family != AF_INET6 || ai->ai_addrlen != sizeof(::sockaddr_in6)
              || !::inet_ntop(AF_INET6, &((::sockaddr_in6*)ai->ai_addr)->sin6_addr, str_addr,
                ai->ai_addrlen)) continue;
            } else if(ai->ai_family == AF_INET) {
              if(ai->ai_addr->sa_family != AF_INET || ai->ai_addrlen != sizeof(::sockaddr_in)
              || !::inet_ntop(AF_INET, &((::sockaddr_in*)ai->ai_addr)->sin_addr, str_addr,
                ai->ai_addrlen)) continue;
            } else {
              continue;
            }
            hostlookup_data_type data;
            data.saddr = str_addr;
            data.family = (family_type)ai->ai_family;
            data.size = ai->ai_addrlen;
            data.shost = ai->ai_canonname ? ai->ai_canonname : "";
            memcpy(&data.adr, ai->ai_addr, ai->ai_addrlen);
            lst.push_back(data);
          }
        } catch(const std::exception& e) {
          if(aii) ::freeaddrinfo(aii);
          throw e;
        } catch(...) {
          lst.clear();
        }
      }
      if(aii) ::freeaddrinfo(aii);
      return lst;
    }

    /**
     * Returns a host name from a given address or an empty string if not found.
     * @param const ip_address& ip
     * @return string_type
     */
    static string_type hostname_by_address(const ip_address& ip)
    {
      ::hostent *sh = nullptr;
      if(ip.family() == AF_INET) {
        sh = ::gethostbyaddr(
        #ifdef _WIN32
        (const char*)
        #endif
          (&((::sockaddr_in const*)&ip.address())->sin_addr), sizeof(::sockaddr_in), AF_INET);
      } else if(ip.family() == AF_INET6) {
        sh = ::gethostbyaddr(
          #ifdef _WIN32
          (const char*)
          #endif
          (&((::sockaddr_in6 const*)&ip.address())->sin6_addr), sizeof(::sockaddr_in6), AF_INET6);
      }
      return ((!sh) || (!sh->h_name)) ? string_type() : string_type(sh->h_name);
    }

    /**
     * Returns a service name from a given port, such as "ftp", "https".
     * @return string_type
     */
    static string_type service_by_port(port_type port__) noexcept
    {
      servent* sa = ::getservbyport(htons(port__), nullptr);
      return (!sa || !sa->s_name) ? string_type() : string_type(sa->s_name);
    }

    /**
     * Returns the port number for a service specification, such as "ftp", "https".
     * @param const string_type& service
     * @return port_type
     */
    static port_type port_by_service(const string_type& service__) noexcept
    {
      servent* sa = ::getservbyname(service__.c_str(), nullptr);
      return (!sa || !sa->s_name) ? 0 : ntohs(sa->s_port);
    }

    /**
     * Returns the own host name
     */
    static string_type my_hostname()
    {
      char s[256]; s[sizeof(s)-1] = '\0';
      return (::gethostname(s, sizeof(s)-1) < 0) ? string_type() : string_type(s);
    }

  private:

    address_sas_type address_;
  };

}}}}

namespace duktape { namespace detail { namespace system { namespace sw {

  template <typename Address_type=ip_address<>>
  class basic_socket
  {
  public:

    using address_type = Address_type;
    using string_type = std::string;
    using size_type = size_t;
    using socketid_type = SCK_IDTYPE;
    using styp_type = typename address_type::styp_type;
    using timeout_type = unsigned;
    using errno_type = int;

    static constexpr socketid_type invalid_id = SCK_INVALIDID;
    static constexpr timeout_type default_timeout = 100;

  public:

    explicit basic_socket() noexcept : id_(invalid_id), adr_(), errno_(0), timeout_(default_timeout), checksent_(), listening_() {}
    explicit basic_socket(socketid_type id) noexcept : id_(id), adr_(), errno_(0), timeout_(default_timeout), checksent_(), listening_() {}
    explicit basic_socket(const address_type& adr) noexcept : id_(invalid_id), adr_(adr), errno_(0), timeout_(default_timeout), checksent_(), listening_() {}
    explicit basic_socket(socketid_type id, const address_type& adr) noexcept : id_(id), adr_(adr), errno_(0), timeout_(default_timeout), checksent_(), listening_() {}
    explicit basic_socket(const char* adr) noexcept : id_(invalid_id), adr_(adr), errno_(0), timeout_(default_timeout), checksent_(false), listening_(false) {}
    basic_socket(const basic_socket&) = delete;
    basic_socket(basic_socket&&) = default;
    basic_socket& operator=(const basic_socket&) = delete;
    basic_socket& operator=(basic_socket&&) = default;

    virtual ~basic_socket() noexcept
    {
      if(id_ == invalid_id) return;
      #ifndef OS_WINDOWS
        ::close(id_);
      #else
        ::closesocket(id_);
      #endif
    }

  public:

    /**
     * Returns true if the socket is a listening server socket.
     * @return bool
     */
    bool listening() const noexcept
    { return listening_; }

    /**
     * Returns if the socket is closed.
     * @return bool
     */
    bool closed() const noexcept
    { return id_ == invalid_id; }

    /**
     * Returns the socket id ("handle"/"descriptor").
     * @return socketid_type
     */
    socketid_type socket_id() const noexcept
    { return id_; }

    /**
     * Returns the ip_address.
     * @return const address_type &
     */
    const address_type& address() const noexcept
    { return adr_; }

    /**
     * Sets the new ip_address
     * @param const address_type& adr
     * @return basic_socket&
     */
    basic_socket& address(const address_type& adr)
    { adr_ = adr; return *this; }

    /**
     * Sets the new ip_address
     * @param const string_type& adr
     * @return basic_socket&
     */
    basic_socket& address(const string_type& adr)
    { adr_.str(adr); return *this; }

    /**
     * Returns the current socket error.
     * @return long
     */
    errno_type error() const noexcept
    { return errno_; }

    /**
     * Returns a string representation of the error
     * @return const string_type&
     */
    string_type error_text() const
    { return syserr_text(errno_); }

    /**
     * Get actual timeout value in ms for wait method
     * @return timeout_type
     */
    timeout_type timeout_ms() const noexcept
    { return timeout_; }

    /**
     * Set timeout value in ms for wait method
     * @param timeout_type t
     * @return basic_socket&
     */
    basic_socket& timeout_ms(timeout_type t) noexcept
    { timeout_ = t; return *this; }

    /**
     * Get a socket option.
     * @param int level
     * @param int optname
     * @return int
     */
    int option(int level, int optname) const noexcept
    {
      int i=0;
      socklen_t size = sizeof(i);
      #ifndef OS_WINDOWS
      if(::getsockopt(id_, level, optname, reinterpret_cast<void*>(&i), &size) == SCK_ERROR) error(SCK_ERRNO);
      #else
      if(::getsockopt(id_, level, optname, reinterpret_cast<char*>(&i), &size) == SCK_ERROR) error(SCK_ERRNO);
      #endif
      SCK_LTRACE("basic_socket::option(lvl=%d, opt=%d) = %d", level, optname, i);
      return i;
    }

    /**
     * Set a socket option.
     * @param int level
     * @param int optname
     * @param int newval
     * @return basic_socket&
     */
    basic_socket& option(int level, int optname, int newval) noexcept
    {
      SCK_LTRACE("basic_socket::option(lvl=%d, opt=%d, val=%d)", level, optname, newval);
      int i = newval;
      #ifndef OS_WINDOWS
      check_error(::setsockopt(id_, level, optname, reinterpret_cast<void*>(&i), socklen_t(sizeof(i))));
      #else
      check_error(::setsockopt(id_, level, optname, reinterpret_cast<char*>(&i), socklen_t(sizeof(i))));
      #endif
      return *this;
    }

    /**
     * Get socket send buffer size
     * @return int
     */
    int option_sendbuffer_size() const noexcept
    { return option(SOL_SOCKET, SO_SNDBUF); }

    /**
     * Get socket receive buffer size
     * @return int
     */
    int option_recvbuffer_size() const noexcept
    { return option(SOL_SOCKET, SO_RCVBUF); }

    /**
     * Returns if the socket is in error state. Automatically resets error.
     * @return bool
     */
    bool option_error() const noexcept
    { if (option(SOL_SOCKET, SO_ERROR) != 0) { close(); return true; } return false; }

    /**
     * Set if the socket can send with delay
     * @param bool enabled
     * @return basic_socket&
     */
    basic_socket& nodelay(bool enabled) noexcept
    { option(IPPROTO_TCP, TCP_NODELAY, enabled ? 1 : 0); return *this; }

    /**
     * Get if the socket can send with delay
     * @return bool
     */
    bool nodelay() const noexcept
    { return option(IPPROTO_TCP, TCP_NODELAY) != 0; }

    /**
     * Set if socket automatically sends keep alive packets
     * @param bool enabled
     * @return basic_socket&
     */
    basic_socket& keepalive(bool enabled) noexcept
    { option(SOL_SOCKET, SO_KEEPALIVE, enabled ? 1 : 0); return *this; }

    /**
     * Get if socket automatically sends keep alive packets
     * @return bool
     */
    bool keepalive() const noexcept
    { return option(SOL_SOCKET, SO_KEEPALIVE) != 0; }

    /**
     * Set if socket can resuse the local address (important for servers)
     * @param bool enabled
     * @return basic_socket&
     */
    basic_socket& reuseaddress(bool enabled) noexcept
    { option(SOL_SOCKET, SO_REUSEADDR, enabled ? 1 : 0); return *this; }

    /**
     * Get if socket can resuse the local address (important for servers)
     * @return bool
     */
    bool reuseaddress() const noexcept
    { return option(SOL_SOCKET, SO_REUSEADDR) != 0; }

    /**
     * Set nonblocking mode
     * @param bool enabled
     * @return basic_socket&
     */
    basic_socket& nonblocking(bool enabled) noexcept
    {
      #ifdef OS_WINDOWS
      u_long d = enabled ? 1:0;
      ::ioctlsocket(id_, FIONBIO, &d);
      #elif defined(O_NONBLOCK)
      int o = ::fcntl(id_, F_GETFL, 0);
      if(o >= 0) {
        if(enabled) o |= O_NONBLOCK; else o &= (~O_NONBLOCK);
        ::fcntl(id_, F_SETFL, o);
      }
      #elif defined(SO_NONBLOCK)
      long d = enabled ? 1:0;
      ::setsockopt(id_, SOL_SOCKET, SO_NONBLOCK, &d, sizeof(d));
      #endif
      return *this;
    }

    /**
     * Get nonblocking mode
     * @return bool
     */
    bool nonblocking() const noexcept
    {
      #if defined(O_NONBLOCK)
      int o = ::fcntl(id_, F_GETFL, 0);
      return (o > 0) && (o & O_NONBLOCK) != 0;
      #elif defined(SO_NONBLOCK)
      long d;
      ::getsockopt(id_, SOL_SOCKET, SO_NONBLOCK, &d, sizeof(d));
      return d != 0;
      #else
      return false;
      #endif
    }

    /**
     * Creates the socket, default TCP socket
     * @param styp_type type
     * @param int family=0
     * @param int protocol=0
     * @return bool
     */
    bool create(styp_type type, int family=0, int protocol=0) noexcept
    {
      SCK_LTRACE("basic_socket::create()");
      SCK_INIT_SYSTEM();
      close();
      error(0);
      if((!family) && adr_.valid()) family = adr_.family();
      if(!family) { error(SCK_EINVAL); return false; }
      id_ = ::socket(family, type, protocol);
      if(id_ == invalid_id) { check_error(SCK_ERROR); return false; }
      return true;
    }

    /**
     * TCP connect. If the method fails, the error can be fetched using the
     * error() method.
     * @return bool
     */
    bool connect() noexcept
    {
      SCK_LTRACE("basic_socket::connect()");
      if(!adr_.valid()) { error(SCK_EINVAL); close(); return false; }
      const auto adr = adr_.address();
      check_error(::connect(id_, reinterpret_cast<const sockaddr*>(&adr), static_cast<socklen_t>(adr_.size())));
      if(error()) { close(); return false; }
      nonblocking(false);
      return true;
    }

    /**
     * Binds a server socket to port/address. If the method fails, the error
     * can be fetched using the error() method.
     * @return bool
     */
    bool bind() noexcept
    {
      SCK_LTRACE("basic_socket::bind()");
      if(!adr_.valid()) { error(SCK_EINVAL); close(); return false; }
      const auto adr = adr_.address();
      check_error(::bind(id_, reinterpret_cast<const sockaddr*>(&adr), static_cast<socklen_t>(adr_.size())));
      if(error()) { close(); return false; }
      return true;
    }

    /**
     * Starts listening, returns success. If the method fails, the
     * error can be fetched using the error() method.
     * @return bool
     */
    bool listen(int max_listen_pending=1) noexcept
    {
      SCK_LTRACE("basic_socket::listen()");
      check_error(::listen(id_, max_listen_pending));
      if(error()) { close(); return false; }
      nonblocking(true);
      listening_ = true;
      return true;
    }

    /**
     * Stream socket accept to a provided acceptor socket, returns
     * boolean success.
     * @param basic_socket& acc
     * @return bool
     */
    bool accept(basic_socket& acc) noexcept
    {
      SCK_LTRACE("basic_socket::accept()");
      acc.close();
      acc.adr_.clear();
      typename address_type::address_sas_type sa;
      typename address_type::address_len_type lsa = sizeof(sa);
      socketid_type id = ::accept(id_, reinterpret_cast<sockaddr*>(&sa), &lsa);
      if(id == invalid_id) return false;
      if((!sa.ss_family) || (!lsa)) {
        #ifndef OS_WINDOWS
          ::close(id);
        #else
          ::closesocket(id);
        #endif
        return false;
      }
      acc.id_ = id;
      acc.adr_.address(sa);
      nonblocking(false);
      return true;
    }

    /**
     * Closes a socket. Before the method checks if the socket is already closed.
     * @return basic_socket&
     */
    basic_socket& close() noexcept
    {
      if(id_ == invalid_id) return *this;
      SCK_LTRACE("basic_socket::close()");
      const socketid_type iid = id_;
      id_ = invalid_id;
      listening_ = false;
      checksent_ = false;
      #ifndef OS_WINDOWS
        ::close(iid);
      #else
        ::shutdown(iid, SD_BOTH);
        ::closesocket(iid);
      #endif
      return *this;
    }

    /**
     * Waits until something has changed in the socket, either received new data,
     * send ready or error. Returns false if timeout, true if somthing changed.
     * @param bool *is_recv = nullptr
     * @param bool *is_sent = nullptr
     * @param int timeout_ms=100
     * @return bool
     */
    bool wait(bool *is_recv = nullptr, bool *is_sent = nullptr, int timeout_ms=100)
    {
      if(closed() || ((!is_recv) && (!is_sent))) return false;
      if(is_recv) *is_recv = false;
      if(is_sent) *is_sent = false;
      if(!checksent_) is_sent=nullptr;
      checksent_ = false;
      #ifndef OS_WINDOWS
        ::pollfd fd;
        fd.fd = id_;
        fd.events = (is_recv ? POLLIN:0) | ((checksent_ && is_sent) ? POLLOUT:0);
        fd.revents = 0;
        checksent_ = false;
        if(!fd.events) return false;
        int r = ::poll(&fd, 1, timeout_ms);
        if(r<0) { check_error(r); return false; }
        if(!r || !fd.revents) return false;
        if(is_recv && (fd.revents & POLLIN)) *is_recv = true;
        if(is_sent && (fd.revents & POLLOUT)) *is_sent = true;
      #else
        ::timeval to;
        ::fd_set ifd, ofd;
        to.tv_sec = timeout_ms / 1000;
        to.tv_usec= (timeout_ms % 1000) * 1000;
        FD_ZERO(&ifd); FD_ZERO(&ofd);
        if(is_recv) FD_SET(id_, &ifd);
        if(is_sent) FD_SET(id_, &ofd);
        int r = ::select(id_+1, is_recv ? (&ifd):0, is_sent ? (&ofd):0, 0, &to);
        if(r<0) { check_error(r); return false; }
        if((!r) || (!FD_ISSET(id_, &ifd) && (!FD_ISSET(id_, &ofd)))) return false;
        if(is_recv && FD_ISSET(id_, &ifd)) *is_recv = true;
        if(is_sent && FD_ISSET(id_, &ofd)) *is_sent = true;
      #endif
      return true;
    }

    /**
     * TCP send string. Returns the number of sent bytes, 0 on error or empty
     * input (check `error()` and `closed()` if 0 is returned).
     * @param const string_type& text
     * @return size_type
     */
    size_type send(const string_type& text)
    { return send(text.data(), text.size()); }

    /**
     * C compatible stream data transmission. Returns the number of bytes
     * sent, 0 on error or `size==0`.
     * @param const void* data
     * @param size_type size
     * @return size_type
     */
    size_type send(const void* data, size_type size)
    {
      if(!data || !size) return 0;
      const char *p = reinterpret_cast<const char*>(data);
      int l = static_cast<int>(size);
      while(l > 0) {
        auto r = ::send(id_, p, l, 0);
        if(r > 0)  {
          l -= r;
          p += r;
          checksent_ = true;
          continue;
        } else if(check_error(r) != SCK_EAGAIN) {
          return 0;
        } else {
          break;
        }
      }
      SCK_LTRACE("basic_socket::send(const void* data, datasize=%d)", int(size));
      return size - static_cast<size_type>(l);
    }

    /**
     * C compliant stream data reception. Returns the number of received
     * bytes, 0 on error or no data (check `error()` on 0-return).
     * @param void* data
     * @param size_type size
     * @return size_type
     */
    size_type recv(void* data, size_type size)
    {
      if(!data || !size) return 0;
      const auto l = ::recv(id_, reinterpret_cast<char*>(data), size, MSG_DONTWAIT);
      if(l > 0) return l; // data available
      if(l == 0) { SCK_LTRACE("basic_socket::recv() 0 bytes --> close"); close(); return 0; } // no, socket closed on other side.
      check_error(l); // Closes the socket if needed
      if(error()) SCK_LTRACE("basic_socket::recv() ERROR");
      return 0; // no data
    }

    /**
     * Receive text data to string.
     * @param size_type max_size
     * @return string_type
     */
    string_type recv(size_type max_size=4096)
    {
      if(max_size <= 0) return string_type();
      auto s = string_type(std::min(max_size, size_t(4096)), typename string_type::value_type('\0'));
      const auto n = recv(s.data(), s.size());
      if(n <= 0) return string_type();
      s.resize(size_t(n));
      return s;
    }

    /**
     * Datagram data reception, returns the number of bytes received, sets
     * the address content where. Returns 0 on no data (e.g. timeout) or error.
     * @param void* data
     * @param size_type size
     * @param address_type& adr
     * @return size_type
     */
    size_type recvfrom(void* data, size_type size, address_type& adr)
    {
      auto sa = typename address_type::address_sas_type();
      auto lsa = typename address_type::address_len_type(sizeof(sa));
      const auto r = ::recvfrom(id_, reinterpret_cast<char*>(data), size, 0, reinterpret_cast<sockaddr*>(&sa), &lsa);
      if(r > 0) { adr.address(sa); return size_type(r); }
      check_error(r);
      adr.clear();
      return 0;
    }

    /**
     * Datagram transmission to an address. Returns the number of bytes sent, 0 on 0-size or error.
     * @param const void* data
     * @param size_type size
     * @param const address_type& adr
     * @return size_type
     */
    size_type sendto(const void* data, size_type size, const address_type& adr)
    {
      if(!adr.valid()) { error(SCK_EINVAL); return -1; }
      const auto sa = adr.address();
      const auto r = ::sendto(id_, reinterpret_cast<const char*>(data), size, 0, reinterpret_cast<const sockaddr*>(&sa), typename address_type::address_len_type(adr.size()));
      if(r > 0) { checksent_ = true; return size_type(r); }
      check_error(r);
      SCK_LTRACE("basic_socket::sendto(): Failed to send");
      return 0;
    }

  protected:

    /**
     * Sets the new socket error, "const-mutable".
     * @param errno_type e
     */
    void error(errno_type e) const noexcept
    { errno_ = e; }

    /**
     * Analyzes the socket error. Closes the socket if it is fatal.
     * @param long fn_return
     * @return errno_type
     */
    errno_type check_error(long fn_return) const noexcept
    {
      if(fn_return == SCK_ERROR) {
        const errno_type e = SCK_ERRNO;
        switch(e) {
          // list of not fatal errors
          case SCK_EAGAIN:
          case SCK_ETIMEOUT:
          case SCK_EINTR:
            break;
          default:
            error(e);
            if(!closed()) {
              #ifdef OS_WINDOWS
                ::shutdown(id_, SD_BOTH);
                ::closesocket(id_);
              #else
                ::close(id_);
              #endif
              SCK_LTRACE("basic_socket::check_error() '%s' (socket id=%d)", error_text().c_str(), int(id_));
            }
            const_cast<socketid_type&>(id_) = invalid_id;
        }
        return e;
      }
      return -1;
    }

    /**
     * Get an error text form system
     * @param errno_type err
     * @return string_type
     */
    static string_type syserr_text(errno_type err)
    {
      if(!err) return string_type();
      #ifndef OS_WINDOWS
        return ::strerror(err);
      #else
        char buffer[256];
        if(::FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, nullptr, DWORD(err), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer)-1, nullptr)) {
          buffer[sizeof(buffer)-1] = '\0';
          return string_type(buffer);
        } else {
          return string_type("unknown error (") + std::to_string(err) + ")";
        }
      #endif
    }

  private:
    socketid_type id_;
    address_type adr_;
    mutable errno_type errno_;
    timeout_type timeout_;
    bool checksent_;
    bool listening_;
  };

}}}}

namespace duktape { namespace mod { namespace system { namespace socket {

  using native_socket = ::duktape::detail::system::sw::basic_socket<>;
  using namespace std;

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    const auto create_socket = [](native_socket::styp_type type, duktape::api& stack, native_socket& instance) {
      using namespace std;
      instance.close();
      if(!stack.is<native_socket::string_type>(0)) throw duktape::script_error("sys.socket: String needed indicating the address (e.g. '::1', '[fe80::0001]:443', '127.0.0.1:80', 'http://localhost/', '/path/to/sock', etc).");
      const auto adr_str = stack.get<native_socket::string_type>(0);
      if(stack.top() > 2) throw duktape::script_error("sys.socket: Too many arguments.");
      if((stack.top() == 2) && (!stack.is_object(1))) throw duktape::script_error("sys.socket: Second argument must be a settings/options object if specified.");
      auto adr = typename native_socket::address_type(adr_str);
      if(!adr.valid()) {
        bool lookup_host = false;
        auto family = native_socket::address_type::family_any;
        auto type = native_socket::address_type::styp_tcp;
        string service, user, host, path;
        adr.parse_resource_address(adr_str, adr, service, user, host, path, lookup_host, family, type);
        if(!adr.valid()) throw duktape::script_error(string("sys.socket: Invalid address '") + adr_str + "'");
      }
      instance.address(adr);
      if(!instance.create(type, adr.family())) {
        const auto err = instance.error_text();
        instance.close();
        throw duktape::script_error(string("sys.socket: Failed to create socket: ") + err);
      }
      if(stack.top() < 2) return;
      if(stack.has_prop_string(-1, "timeout")) {
        const auto t = native_socket::timeout_type(stack.property<int>("timeout", 0));
        if(t<1 || t>2000)  throw duktape::script_error("sys.socket: Timeout value is out of range (value is in milliseconds).");
        instance.timeout_ms(static_cast<native_socket::timeout_type>(t));
        if(instance.error()) throw duktape::script_error(string("sys.socket: Failed to set poll timeout: ") + instance.error_text());
      }
      if(stack.has_prop_string(-1, "nodelay")) {
        instance.nodelay(stack.property<bool>("nodelay", false));
        if(instance.error()) throw duktape::script_error(string("sys.socket: Failed to set nodelay option: ") + instance.error_text());
      }
      if(stack.has_prop_string(-1, "keepalive")) {
        instance.keepalive(stack.property<bool>("keepalive", false));
        if(instance.error()) throw duktape::script_error(string("sys.socket: Failed to set keepalive option: ") + instance.error_text());
      }
      if(stack.has_prop_string(-1, "reuseaddress")) {
        instance.reuseaddress(stack.property<bool>("reuseaddress", true));
        if(instance.error()) throw duktape::script_error(string("sys.socket: Failed to set 'reuseaddress' option: ") + instance.error_text());
      }
      if(stack.has_prop_string(-1, "nonblocking")) {
        instance.nonblocking(stack.property<bool>("nonblocking", false));
        if(instance.error()) throw duktape::script_error(string("sys.socket: Failed to set 'nonblocking' option: ") + instance.error_text());
      }
    };

    js.define(
      // Wrapped class specification and type.
      duktape::native_object<native_socket>("sys.socket")
      // Native constructor from script arguments.
      #if(0 && JSDOC)
      /**
       * Socket handling object constructor. No actual socket
       * is created until `connect()` or `listen()` is invoked.
       * After closing, the system socket will be closed and
       * cleaned up.
       *
       * @constructor
       * @throws {Error}
       *
       * @property {boolean} closed - Holds true if the socket is uninitialized.
       * @property {boolean} connected - Holds true if the socket connected to a server.
       * @property {number}  socket_id - OS specific socket handle/descriptor.
       * @property {string}  address - Address and port (format "<address>:<port>").
       * @property {number}  errno - OS specific error code of the last occurred error.
       * @property {string}  error - Error text/message of the last occurred error.
       * @property {number} timeout - Default poll timeout for reception in milliseconds.
       * @property {number} sendbuffer_size - OS specific underlying transmission buffer size in bytes.
       * @property {number} recvbuffer_size - OS specific underlying reception buffer size in bytes.
       * @property {boolean} nodelay - Holds true if the "nodelay" socket option is set (transmit immediately, no stream data collection).
       * @property {boolean} keepalive - Holds true if the "keepalive" socket option is set (TCP connection keep-alive).
       * @property {boolean} reuseaddress - Holds true if the "reuseaddress" socket option is set (for listening/bound sockets).
       * @property {boolean} nonblocking - Holds true if the "nonblocking" socket option is set (return immediately from recv/send, even if not everything is sent/nothing to receive).
       * @property {boolean} listening - Holds true if the socket is currently listening for incoming connections (server socket).
       */
      sys.socket = function() {};
      #endif
      .constructor([](duktape::api& stack) {
        if(stack.top()==0) {
          return new native_socket();
        } else {
          throw duktape::script_error("sys.socket constructor does not take any arguments.");
        }
      })
      .getter("closed", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.closed());
      })
      .getter("connected", [](duktape::api& stack, native_socket& instance) {
        stack.push(!instance.closed());
      })
      .getter("socket_id", [](duktape::api& stack, native_socket& instance) {
        stack.push(static_cast<long>(instance.socket_id()));
      })
      .getter("address", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.address().str());
      })
      .getter("errno", [](duktape::api& stack, native_socket& instance) {
        stack.push(static_cast<long>(instance.error()));
      })
      .getter("error", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.error_text());
      })
      .getter("timeout", [](duktape::api& stack, native_socket& instance) {
        stack.push(static_cast<double>(instance.timeout_ms()));
      })
      .setter("timeout", [](duktape::api& stack, native_socket& instance) {
        if(!stack.is<double>(0)) throw duktape::script_error("sys.socket: Timeout must be a number in milliseconds.");
        const auto t = stack.get<int>(0);
        if(t<2 || t>2000)  throw duktape::script_error("sys.socket: Timeout value is out of range (value is in milliseconds).");
        instance.timeout_ms(static_cast<native_socket::timeout_type>(t));
      })
      .getter("sendbuffer_size", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.option_sendbuffer_size());
      })
      .getter("recvbuffer_size", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.option_recvbuffer_size());
      })
      .getter("nodelay", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.nodelay());
      })
      .setter("nodelay", [](duktape::api& stack, native_socket& instance) {
        instance.nodelay(stack.get<bool>(0));
      })
      .getter("keepalive", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.keepalive());
      })
      .setter("keepalive", [](duktape::api& stack, native_socket& instance) {
        instance.keepalive(stack.get<bool>(0));
      })
      .getter("reuseaddress", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.reuseaddress());
      })
      .setter("reuseaddress", [](duktape::api& stack, native_socket& instance) {
        instance.reuseaddress(stack.get<bool>(0));
      })
      .getter("nonblocking", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.nonblocking());
      })
      .setter("nonblocking", [](duktape::api& stack, native_socket& instance) {
        instance.nonblocking(stack.get<bool>(0));
      })
      .getter("listening", [](duktape::api& stack, native_socket& instance) {
        stack.push(instance.listening());
      })
      #if(0 && JSDOC)
      /**
       * Low level socket option getting/setting. Refer to the
       * socket API manuals of the applicable operating system.
       * IF the `setvalue` is undefined (only 2 args used), the
       * method acts as a getter.
       * @param {number} level
       * @param {number} optname
       * @param {number} [setvalue]
       * @return {number}
       */
      sys.socket.prototype.option = function(level, optname, setvalue) {};
      #endif
      .method("option", [](duktape::api& stack, native_socket& instance) {
        if((!stack.is<int>(0)) || (!stack.is<int>(1))) throw duktape::script_error("sys.socket: 1st and 2nd argument of 'option' must be integers, see 'getsockopt'/'setsockopt'.");
        const auto level = stack.to<int>(0);
        const auto optname = stack.to<int>(1);
        if(stack.is_undefined(2) || stack.is_null(2)) {
          stack.top(0);
          stack.push(instance.option(level, optname));
        } else if(!stack.is<int>(2)) {
          throw duktape::script_error("sys.socket: 3rd argument of 'option' (if given for setting) must be an integer, see 'getsockopt'/'setsockopt'.");
        } else {
          instance.option(level, optname, stack.get<int>(2));
          stack.top(0);
          stack.push_this();
        }
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Closes the connection/server, cleans up the underlying
       * system socket.
       */
      sys.socket.prototype.close = function() {};
      #endif
      .method("close", [](duktape::api& stack, native_socket& instance) {
        instance.close();
        stack.push_this();
        return true;
      })
      .method("open", [](duktape::api& stack, native_socket& instance) {
        (void)stack;
        instance.close();
        throw duktape::script_error("Use `connect()` (e.g. for tcp/unix), `listen()` (e.g. tcp/unix), or `bind()` (e.g. for udp), there is no general socket 'open'.");
        {
          using namespace std;
          // (void)instance.create(SOCK_STREAM, 0, 0);
          // (void)instance.connect();
          // (void)instance.bind();
          // (void)instance.listen(1);
          // native_socket sck;
          // (void)instance.accept(sck);
          // (void)instance.wait(nullptr,nullptr,10);
          // (void)instance.send(string("TEST"));
          // string test = "TEST";
          // (void)instance.send(test.data(), test.size());
          // (void)instance.recv(test.data(), test.size());
          // (void)instance.recv();
          // (void)instance.recv(1024);
          // auto adr = native_socket::address_type();
          // (void)instance.recvfrom(test.data(), test.size(), adr);
          // (void)instance.sendto(test.data(), test.size(), adr);

        }
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Initiate a connection to a server, returns after the connection
       * is established, throws on error. The address string defines the
       * type of socket. For TCP connection, the port has to be attached
       * using with the pattern "<address>:<port>". IPv6 addresses are to
       * be escaped as known from browsers, e.g."[fe80::0001]:<port>".
       * Additional options may be specified as plain object, these are
       * applied after creating the socket and before connecting. They
       * correspond to the readable option related properties of the
       * socket object:
       *    options = {
       *      timeout: <number>,
       *      nodelay: <boolean>,
       *      keepalive: <boolean>,
       *      reuseaddress: <boolean>,
       *      nonblocking: <boolean>
       *    }
       *
       * @param {string} address
       * @param {object} options
       */
      sys.socket.prototype.connect = function() {};
      #endif
      .method("connect", [&](duktape::api& stack, native_socket& instance) {
        create_socket(SOCK_STREAM, stack, instance);
        if(!instance.connect()) throw duktape::script_error(string("sys.socket: Failed to connect to '") + instance.address().str() + "': " + instance.error_text());
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Bind the socket to the specified port and start listening. The address
       * may be specified as "0.0.0.0:<port>" or "[::]:<port>" accordingly.
       * The value `max_pending` specifies how many incoming connections are
       * kept in the pipeline until they are rejected.
       *
       * @param {string} address_port
       * @param {number} max_pending
       * @param {object} options
       */
      sys.socket.prototype.listen = function(address_port, max_pending, options) {};
      #endif
      .method("listen", [&](duktape::api& stack, native_socket& instance) {
        const auto max_pending = stack.is_undefined(1) ? int(1) : stack.get<int>(1);
        if((max_pending <= 0) || (max_pending > 4096)) throw duktape::script_error(string("sys.socket: 2nd argument (maximum number of pending connections) is out of range: ") + std::to_string(max_pending));
        create_socket(SOCK_STREAM, stack, instance);
        instance.reuseaddress(true);
        instance.nodelay(true);
        if(!instance.bind()) throw duktape::script_error(string("sys.socket: Failed to bind port for local address '") + instance.address().str() + "': " + instance.error_text());
        if(!instance.listen(max_pending)) throw duktape::script_error(string("sys.socket: Failed to listen at local address '") + instance.address().str() + "': " + instance.error_text());
        // @todo: Implement ACCEPTORS.
        instance.close();
        throw duktape::script_error("Listen (actually the acceptor socket handling) not implemented yet.");
        // ---
        stack.push_this();
        return true;
      })
      #if(0 && JSDOC)
      /**
       * Send data via the open socket.
       *
       * @param {string} data
       */
      sys.socket.prototype.send = function(data) {};
      #endif
      .method("send", [](duktape::api& stack, native_socket& instance) {
        using namespace std;
        if(instance.closed()) throw duktape::script_error(string("sys.socket: Failed to send, socket closed."));
        if(stack.is<native_socket::string_type>(0)) {
          const auto s = stack.get<native_socket::string_type>(0);
          auto n_sent = native_socket::string_type::size_type(0);
          stack.top(0);
          while(n_sent < s.size()) {
            const auto n = instance.send(&s.data()[n_sent], s.size()-n_sent);
            if(!n && instance.error()) throw duktape::script_error(string("sys.socket: Failed to send: ") + instance.error_text());
            if(instance.closed()) throw duktape::script_error(string("sys.socket: Failed to send, socket was closed."));
            n_sent += n;
            if(!n) break; // non-blocking sends 0 bytes if the socket is clogged.
          }
          stack.push(n_sent);
          return true;
        } else if(stack.is_buffer(0)) {
          throw duktape::script_error("sys.socket: Sending binary buffer data not yet implemented.");
        } else {
          throw duktape::script_error(string("sys.socket: Data to send must be string or buffer, but type is ") + stack.get_typename(0));
        }
        return false;
      })
      #if(0 && JSDOC)
      /**
       * Receive text via the open socket connection.
       * Returns an empty string if no data are received
       * withing the specified timeout (milliseconds), or
       * the defined default timeout accordingly.
       *
       * @param {number} [timeout]
       * @return {string}
       */
      sys.socket.prototype.recv = function(timeout) {};
      #endif
      .method("recv", [](duktape::api& stack, native_socket& instance) {
        using namespace std;
        auto timeout_ms = std::max(0, stack.get<int>(0));
        stack.top(0);
        if(instance.closed()) throw duktape::script_error(string("sys.socket: Receiving failed, socket closed."));
        auto rx = false;
        if((timeout_ms > 0) && (!instance.wait(&rx, nullptr, timeout_ms)) && instance.error()) throw duktape::script_error(string("sys.socket: Receiving data poll failed: ") + instance.error_text());
        if(!rx) { stack.push(string()); return true; }
        auto s = instance.recv();
        if(s.empty() && instance.error()) throw duktape::script_error(string("sys.socket: Receiving failed: ") + instance.error_text());
        stack.push(std::move(s));
        return true;
      })
    );
  }

}}}}

#endif
