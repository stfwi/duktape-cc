/**
 * @file mod.conv.hh
 * @package de.atwillys.cc.duktape.ext
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++17
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++17 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 * -----------------------------------------------------------------------------
 *
 * Binary/integral conversion functionality.
 */
#ifndef DUKTAPE_MOD_CONV_HH
#define DUKTAPE_MOD_CONV_HH

/**
 * @snipplet sw/endian.hh
 * @package de.atwillys.cc.swl
 * @license MIT
 * @author Stefan Wilhelm (stfwi)
 * @platform linux, bsd, windows
 * @standard >= c++11
 * @version #41f5b9c0d1b91c0e94c9273ae68c2c7b22f6a8d7
*/
#ifndef SW_ENDIAN_41f5b9c0
#define SW_ENDIAN_41f5b9c0

  #if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
    #include <endian.h>
  #elif defined(__APPLE__) && defined(__MACH__)
    #include <machine/endian.h>
  #elif defined(__OpenBSD__)
    #include <machine/endian.h>
  #elif defined(BSD) || defined(__FreeBSD__) || defined(__NETBSD__) || defined(__NetBSD__) \
    || defined(__bsdi__) || defined(__FreeBSD__)
    #include <sys/endian.h>
  #endif

  namespace duktape { namespace ext { namespace detail { namespace conv { namespace {

    enum class endian { unknown=0, big=1, little=2, middle=3 };

    constexpr endian machine_endianness()
    {
      #ifdef MACHINE_BYTEORDER__undef
        #error "MACHINE_BYTEORDER__undef already defined"
      #endif
      #if  (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)) \
        || (defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN))
        #define MACHINE_BYTEORDER__undef 1
      #elif (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)) \
        || (defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN))
        #define MACHINE_BYTEORDER__undef 2
      #elif (defined(__BYTE_ORDER) && defined(__PDP_ENDIAN) && (__BYTE_ORDER == __PDP_ENDIAN)) \
        || (defined(_BYTE_ORDER) && defined(_PDP_ENDIAN) && (_BYTE_ORDER == _PDP_ENDIAN))
        #define MACHINE_BYTEORDER__undef 3
      #elif (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN))
        #define MACHINE_BYTEORDER__undef 1
      #elif (!defined(__BIG_ENDIAN__) && defined(__LITTLE_ENDIAN__)) || (!defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN))
        #define MACHINE_BYTEORDER__undef 2
      #elif defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIPSEB) \
        || defined(__MIPSEB) || defined(__MIPSEB__)
        #define MACHINE_BYTEORDER__undef 1
      #elif defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) \
        || defined(__MIPSEL) || defined(__MIPSEL__)
        #define MACHINE_BYTEORDER__undef 2
      #elif defined(__amd64__) || defined(__amd64) || defined(__x86_64) || defined(__x86_64__) \
        || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__i386) \
        || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) \
        || defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__ia64) || defined(__itanium__)
        #define MACHINE_BYTEORDER__undef 2
      #elif defined(__arm__) || defined(__thumb__) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARM)
        #if (defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))
          #define MACHINE_BYTEORDER__undef 2 /* Boost: Windows on ARM is little endian */
        #else
          #define MACHINE_BYTEORDER__undef 1
        #endif
      #elif (defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))
        #define MACHINE_BYTEORDER__undef 2
      #else
        #define MACHINE_BYTEORDER__undef 0 /* unknown */
      #endif
      {
        static_assert((MACHINE_BYTEORDER__undef >= 1) && (MACHINE_BYTEORDER__undef <= 3), "Unknown target machine endianess.");
        return (MACHINE_BYTEORDER__undef == 1) ? (endian::big) : ((MACHINE_BYTEORDER__undef == 2) ? (endian::little) : (endian::middle));
      }
      #undef MACHINE_BYTEORDER__undef
    }

    template <typename=void>
    inline const char* endianness_name(endian value) noexcept
    {
      switch(value) {
        case endian::big: return "big";
        case endian::little: return "little";
        case endian::middle: return "middle";
        default: return "unknown";
      }
    }

    template <endian FromEndianess, endian ToEndianess, typename Integer>
    constexpr Integer convert_endianess(const Integer value) noexcept
    {
      static_assert((FromEndianess!=endian::middle) && (ToEndianess!=endian::middle), "Tell me when that is still needed.");
      if/*constexpr*/(FromEndianess==ToEndianess) return value;
      if/*constexpr*/(sizeof(Integer)==1) return value;
      // No byte swaps etc, the compiler inline and unroll that:
      auto i = sizeof(Integer);
      auto swapped = Integer(0);
      auto val = value;
      while(i--) {
        swapped = (swapped<<8) | (val & Integer(0xff));
        val >>= 8; // NOLINT: Will overflow at i==0, no problem.
      }
      return swapped;
    }

  }}}}}

#endif

#include <duktape/duktape.hh>
#include <string>

namespace duktape { namespace ext { namespace detail { namespace conv { namespace {

  template <typename C, typename N>
  constexpr C nibble_d2h(N val) noexcept
  { return C(((val) > 10 ? ('A'-10+char(val)) : ('0'+char(val)))); }

  template <typename N, typename C>
  constexpr N nibble_h2d(C ch) noexcept
  {
    if((ch>='0') && (ch<='9')) return N(ch-'0');
    if((ch>='A') && (ch<='F')) return N(ch-('A'-10));
    if((ch>='a') && (ch<='f')) return N(ch-('a'-10));
    return N(-1);
  }

  /**
   * Returns the machine endianess name ("little", "big", "middle").
   * @return string
   */
  template <typename=void>
  inline std::string get_machine_endianess() noexcept
  { return endianness_name(machine_endianness()); }

  /**
   * Converts a hex string to a number, where the endianess
   * of the hex string is specified, and the string size must
   * strictly correspond to the word size (must be zero-padded).
   */
  template <typename T, endian E>
  inline int from_hex(duktape::api& stack)
  {
    using namespace std;
    if(stack.top()!=1) throw duktape::script_error("fromHex: Needs one string argument.");
    if(!stack.is<string>(0)) throw duktape::script_error("fromHex: Argument is no string.");
    string hex = stack.get<string>(0);
    stack.top(0);
    if((hex.size()>=2) && (hex[0]=='0') && ((hex[1]=='x') || (hex[1]=='X'))) hex = hex.substr(2);
    if(hex.size() != (sizeof(T)*2)) throw duktape::script_error(string("fromHex: hex string does not match the conversion word size: '") + hex + "'");
    T val = T(0);
    for(auto c:hex) {
      auto n = nibble_h2d<int>(c);
      if(n<0) throw duktape::script_error(string("fromHex: invalid hex character in '") + hex + "'");
      val = (val<<4)|T(n);
    }
    stack.push(convert_endianess<E, machine_endianness()>(val));
    return 1;
  }

  /**
   * Converts a native number to a hex string, where the endianess of
   * the hex string is specified. The string size is zero padded to fit
   * the word size of the number.
   */
  template <typename T, endian E>
  inline int to_hex(duktape::api& stack)
  {
    using namespace std;
    if(stack.top()!=1) throw duktape::script_error("toHex: Needs one Number argument.");
    if(!stack.is<double>(0)) throw duktape::script_error("toHex: Argument is no number.");
    const auto dec = stack.get<double>(0);
    if((dec < numeric_limits<T>::min()) || (dec > numeric_limits<T>::max())) throw duktape::script_error(string("toHex: Number exceeds the numeric value range of the conversion: ") + to_string(dec));
    stack.top(0);
    auto val = convert_endianess<machine_endianness(), E>(typename make_unsigned<T>::type(static_cast<T>(dec)));
    auto s = string(sizeof(T)*2, '0');
    auto i = s.size();
    while(val) {
      const auto digit = (val & 0xfu);
      s[--i] = (digit<0xa) ? ('0'+char(digit)) : (('a'-10)+char(digit));
      if(!i) break;
      val >>= 4u;
    }
    stack.push(s);
    return 1;
  }

}}}}}

namespace duktape { namespace mod { namespace ext { namespace conv {

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    using namespace duktape::ext::detail::conv;
    using namespace std;
    using namespace sw;
    // Note: U8 big/little endian are nonsense but registered for completeness.
    js.define("Number.machineEndianess", get_machine_endianess<>);
    js.define("Number.fromHexS32", from_hex<int32_t, machine_endianness()>);
    js.define("Number.fromHexS16", from_hex<int16_t, machine_endianness()>);
    js.define("Number.fromHexS8",  from_hex<int8_t, machine_endianness()>);
    js.define("Number.fromHexU32", from_hex<uint32_t, machine_endianness()>);
    js.define("Number.fromHexU16", from_hex<uint16_t, machine_endianness()>);
    js.define("Number.fromHexU8",  from_hex<uint8_t, machine_endianness()>);
    js.define("Number.fromHexS32BE", from_hex<int32_t, endian::big>);
    js.define("Number.fromHexS16BE", from_hex<int16_t, endian::big>);
    js.define("Number.fromHexS8BE",  from_hex<int8_t, endian::big>);
    js.define("Number.fromHexU32BE", from_hex<uint32_t, endian::big>);
    js.define("Number.fromHexU16BE", from_hex<uint16_t, endian::big>);
    js.define("Number.fromHexU8BE",  from_hex<uint8_t, endian::big>);
    js.define("Number.fromHexS32LE", from_hex<int32_t, endian::little>);
    js.define("Number.fromHexS16LE", from_hex<int16_t, endian::little>);
    js.define("Number.fromHexS8LE",  from_hex<int8_t, endian::little>);
    js.define("Number.fromHexU32LE", from_hex<uint32_t, endian::little>);
    js.define("Number.fromHexU16LE", from_hex<uint16_t, endian::little>);
    js.define("Number.fromHexU8LE",  from_hex<uint8_t, endian::little>);
    js.define("Number.toHexS32", to_hex<int32_t, machine_endianness()>);
    js.define("Number.toHexS16", to_hex<int16_t, machine_endianness()>);
    js.define("Number.toHexS8",  to_hex<int8_t, machine_endianness()>);
    js.define("Number.toHexU32", to_hex<uint32_t, machine_endianness()>);
    js.define("Number.toHexU16", to_hex<uint16_t, machine_endianness()>);
    js.define("Number.toHexU8",  to_hex<uint8_t, machine_endianness()>);
    js.define("Number.toHexS32BE", to_hex<int32_t, endian::big>);
    js.define("Number.toHexS16BE", to_hex<int16_t, endian::big>);
    js.define("Number.toHexS8BE",  to_hex<int8_t, endian::big>);
    js.define("Number.toHexU32BE", to_hex<uint32_t, endian::big>);
    js.define("Number.toHexU16BE", to_hex<uint16_t, endian::big>);
    js.define("Number.toHexU8BE",  to_hex<uint8_t, endian::big>);
    js.define("Number.toHexS32LE", to_hex<int32_t, endian::little>);
    js.define("Number.toHexS16LE", to_hex<int16_t, endian::little>);
    js.define("Number.toHexS8LE",  to_hex<int8_t, endian::little>);
    js.define("Number.toHexU32LE", to_hex<uint32_t, endian::little>);
    js.define("Number.toHexU16LE", to_hex<uint16_t, endian::little>);
    js.define("Number.toHexU8LE",  to_hex<uint8_t, endian::little>);
  }

}}}}

#endif
