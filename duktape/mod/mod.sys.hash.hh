/**
 * @file duktape/mod/mod.hash.hh
 * @package de.atwillys.cc.duktape
 * @license MIT
 * @authors Stefan Wilhelm (stfwi, <cerbero s@atwillys.de>)
 * @platform linux, bsd, windows
 * @standard >= c++11
 * @requires duk_config.h duktape.h duktape.c >= v2.1
 * @requires Duktape CFLAGS -DDUK_USE_CPP_EXCEPTIONS
 * @cxxflags -std=c++11 -W -Wall -Wextra -pedantic -fstrict-aliasing
 * @requires WIN32 CXXFLAGS -D_WIN32_WINNT>=0x0601 -DWINVER>=0x0601 -D_WIN32_IE>=0x0900
 *
 * -----------------------------------------------------------------------------
 *
 * Duktape ECMA engine C++ wrapper, hashing functions.
 *
 * Contains classes/functions of swlib-cc template library encompassing:
 *
 *  - CRC8  (PEC polynomial, init value and final XOR) for string and buffer.
 *  - CRC16 (USB polynomial, init value and final XOR) for string and buffer.
 *  - CRC32 (CCITT polynomial, init value and final XOR) for string and buffer.
 *  - MD5 for string, buffer and file
 *  - SHA1 for string, buffer and file
 *  - SHA512 for string, buffer and file
 *
 * Note: The CRC algorithms are already quite old (not in contemporary
 *       c++ style), but they are approved to work. Versions tracking
 *       of the original library files is via the GIT commits of the
 *       lib. Licenses are compliant to the license of this file.
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
#ifndef DUKTAPE_MOD_HASHES_EXT_HH
#define DUKTAPE_MOD_HASHES_EXT_HH

// @version: #f84cdfe 2009-11-01T20:38:02+01:00
/**
 * @package de.atwillys.cc.swl
 * @license MIT
 * @author Stefan Wilhelm (stfwi)
 * @file crc.hh
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++98
 * -----------------------------------------------------------------------------
 *
 * CRC16(USB) / CRC32(CCITT) calculation class template, usage:
 *
 *  uint16_t checksum = sw::crc16::calculate(pointer_to_data, size_of_data);
 *
 *  uint32_t checksum = sw::crc32::calculate(pointer_to_data, size_of_data);
 *
 * -------------------------------------------------------------------------------------
 * +++ MIT license +++
 * Copyright (c) 2008-2022, Stefan Wilhelm <cerbero s@atwilly s.de>
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
 * -------------------------------------------------------------------------------------
 */
#ifndef SW_CRC_HH
#define SW_CRC_HH

#include <string>
#include <cstring>
#if defined(OS_WINDOWS) || defined (_WINDOWS_) || defined(_WIN32) || defined(__MSC_VER)
#include <stdint.h>
#else
#include <inttypes.h>
#endif

namespace sw { namespace detail {

  /**
   * Type selective lookup tables
   */
  template <typename AccType>
  struct crc_lookups { static const AccType tab[256]; };

  template <>
  const uint16_t crc_lookups<uint16_t>::tab[256] = {
    0x0000,0xc0c1,0xc181,0x0140,0xc301,0x03c0,0x0280,0xc241,0xc601,0x06c0,0x0780,0xc741,
    0x0500,0xc5c1,0xc481,0x0440,0xcc01,0x0cc0,0x0d80,0xcd41,0x0f00,0xcfc1,0xce81,0x0e40,
    0x0a00,0xcac1,0xcb81,0x0b40,0xc901,0x09c0,0x0880,0xc841,0xd801,0x18c0,0x1980,0xd941,
    0x1b00,0xdbc1,0xda81,0x1a40,0x1e00,0xdec1,0xdf81,0x1f40,0xdd01,0x1dc0,0x1c80,0xdc41,
    0x1400,0xd4c1,0xd581,0x1540,0xd701,0x17c0,0x1680,0xd641,0xd201,0x12c0,0x1380,0xd341,
    0x1100,0xd1c1,0xd081,0x1040,0xf001,0x30c0,0x3180,0xf141,0x3300,0xf3c1,0xf281,0x3240,
    0x3600,0xf6c1,0xf781,0x3740,0xf501,0x35c0,0x3480,0xf441,0x3c00,0xfcc1,0xfd81,0x3d40,
    0xff01,0x3fc0,0x3e80,0xfe41,0xfa01,0x3ac0,0x3b80,0xfb41,0x3900,0xf9c1,0xf881,0x3840,
    0x2800,0xe8c1,0xe981,0x2940,0xeb01,0x2bc0,0x2a80,0xea41,0xee01,0x2ec0,0x2f80,0xef41,
    0x2d00,0xedc1,0xec81,0x2c40,0xe401,0x24c0,0x2580,0xe541,0x2700,0xe7c1,0xe681,0x2640,
    0x2200,0xe2c1,0xe381,0x2340,0xe101,0x21c0,0x2080,0xe041,0xa001,0x60c0,0x6180,0xa141,
    0x6300,0xa3c1,0xa281,0x6240,0x6600,0xa6c1,0xa781,0x6740,0xa501,0x65c0,0x6480,0xa441,
    0x6c00,0xacc1,0xad81,0x6d40,0xaf01,0x6fc0,0x6e80,0xae41,0xaa01,0x6ac0,0x6b80,0xab41,
    0x6900,0xa9c1,0xa881,0x6840,0x7800,0xb8c1,0xb981,0x7940,0xbb01,0x7bc0,0x7a80,0xba41,
    0xbe01,0x7ec0,0x7f80,0xbf41,0x7d00,0xbdc1,0xbc81,0x7c40,0xb401,0x74c0,0x7580,0xb541,
    0x7700,0xb7c1,0xb681,0x7640,0x7200,0xb2c1,0xb381,0x7340,0xb101,0x71c0,0x7080,0xb041,
    0x5000,0x90c1,0x9181,0x5140,0x9301,0x53c0,0x5280,0x9241,0x9601,0x56c0,0x5780,0x9741,
    0x5500,0x95c1,0x9481,0x5440,0x9c01,0x5cc0,0x5d80,0x9d41,0x5f00,0x9fc1,0x9e81,0x5e40,
    0x5a00,0x9ac1,0x9b81,0x5b40,0x9901,0x59c0,0x5880,0x9841,0x8801,0x48c0,0x4980,0x8941,
    0x4b00,0x8bc1,0x8a81,0x4a40,0x4e00,0x8ec1,0x8f81,0x4f40,0x8d01,0x4dc0,0x4c80,0x8c41,
    0x4400,0x84c1,0x8581,0x4540,0x8701,0x47c0,0x4680,0x8641,0x8201,0x42c0,0x4380,0x8341,
    0x4100,0x81c1,0x8081,0x4040
  };

  template <>
  const uint32_t crc_lookups<uint32_t>::tab[256] = {
    0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
    0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
    0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
    0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
    0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
    0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
    0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
    0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
    0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
    0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
    0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
    0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
    0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
    0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
    0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
    0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
    0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
    0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
    0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
    0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
    0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
    0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
    0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
    0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
    0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
    0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
    0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
    0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
    0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
    0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
    0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
    0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
  };

  /**
   * Template class basic_crc
   */
  template <typename acc_type, typename size_type, acc_type initial_crc_value, acc_type final_xor_value>
  class basic_crc
  {
  public:

    /**
     * Calculate CRC16, std::string
     * @param const std::string & s
     * @return acc_type
     */
    static inline acc_type calculate(const std::string & s)
    { return calculate(s.c_str(), s.length()); }

    /**
     * Calculate CRC16, C string
     * @param const char* c_str
     * @return acc_type
     */
    static inline acc_type calculate(const char* c_str)
    { return (!c_str) ? 0 : (calculate(c_str, strlen(c_str))); }

    /**
     * Calculate CRC16, raw data and length
     * @param const void *data
     * @param size_type size
     * @return acc_type
     */
    static acc_type calculate(const void *data, size_type size)
    {
      // Compiler is gently asked to as much as possible in registers, but depending
      // on the processor it will not to that.
      acc_type crc = initial_crc_value;
      const unsigned char *p = static_cast<const unsigned char*>(data);
      if(data) {
        while(size--) crc = (crc_lookups<acc_type>::tab[((crc) ^ (*p++)) & 0xff] ^ ((crc) >> 8));
      }
      return crc ^ final_xor_value;
    }
  };
}}

namespace sw {
  typedef detail::basic_crc<uint16_t, size_t, 0xffff     , 0xffff> crc16;
  typedef detail::basic_crc<uint32_t, size_t, 0xffffffff , 0xffffffff> crc32;
}

/**
 * @package de.atwillys.cc.swl
 * @license MIT
 * @author Stefan Wilhelm (stfwi)
 * @file crc.hh
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++98
 *
 * CRC8 (PEC)
 * @param const void *data
 * @param uint8_t size
 * @return typename T
 */
namespace sw {
  template <typename=void>
  uint8_t crc8(const void *data, uint8_t size)
  {
    const uint8_t *p = static_cast<const uint8_t*>(data);
    uint16_t crc = 0;
    for (uint8_t j = size; j; --j, ++p) {
      crc ^= ((*p) << 8);
      for(uint8_t i = 8; i > 0; --i) {
        crc = ((crc & 0x8000u) ? (crc ^ (0x1070u << 3)) : (crc)) << 1;
      }
    }
    return (uint8_t)(crc >> 8);
  }
}

#endif

// @version: #e6719f1
/**
 * @package de.atwillys.cc.swl
 * @file md5.hh
 * @author Stefan Wilhelm
 * @license MIT
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++98
 *
 * MD5 calculation class template.
 */
#ifndef MD5_HH
#define MD5_HH

#if defined(OS_WINDOWS) || defined (_WINDOWS_) || defined(_WIN32) || defined(__MSC_VER)
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace sw { namespace detail {

  /**
   * @class basic_md5
   * @template
   */
  template <typename Char_Type=char>
  class basic_md5
  {
  public:

    using string_type = std::basic_string<Char_Type>;

  public:

    explicit basic_md5() : buf_(), cnt_(), sum_() { clear(); }
    basic_md5(const basic_md5&) = delete;
    basic_md5(basic_md5&&) = default;
    basic_md5& operator=(const basic_md5&) = delete;
    basic_md5& operator=(basic_md5&&) = default;
    ~basic_md5() = default;

  public:

    /**
     * Clear/reset all internal buffers and states.
     */
    void clear() noexcept
    {
      cnt_[0] = cnt_[1] = 0;
      sum_[0] = 0x67452301; sum_[1] = 0xefcdab89; sum_[2] = 0x98badcfe; sum_[3] = 0x10325476;
      memset(buf_, 0, sizeof buf_);
    }

    /**
     * Push new binary data into the internal buf_ and recalculate the checksum.
     * @param const void* data
     * @param size_t size
     */
    void update(const void *data, uint32_t size)
    {
      uint32_t index = cnt_[0] / 8 % 64;
      if((cnt_[0] += (size << 3)) < (size << 3)) cnt_[1]++; // Update number of bits
      cnt_[1] += (size >> 29);
      uint32_t i = 0, thresh = 64-index; // number of bytes to fill in buffer
      if(size >= thresh) { // transform as many times as possible.
        memcpy(&buf_[index], data, thresh); // fill buffer first, transform
        transform(buf_);
        for(i=thresh; i+64 <= size; i+=64) transform(&static_cast<const uint8_t*>(data)[i]);
        index = 0;
      }
      memcpy(&buf_[index], &(static_cast<const uint8_t*>(data)[i]), size-i); // remainder
    }

    /**
     * Finanlise checksum, return hex string.
     * @return string_type
     */
    std::string final_result()
    {
      #define U32_B(O_, I_, len) { \
        for (uint32_t i = 0, j = 0; j < len; i++, j += 4) { \
          (O_)[j] = (I_)[i] & 0xff; \
          (O_)[j+1] = ((I_)[i] >> 8) & 0xff; \
          (O_)[j+2] = ((I_)[i] >> 16) & 0xff; \
          (O_)[j+3] = ((I_)[i] >> 24) & 0xff; \
        } \
      }
      uint8_t padding[64];
      memset(padding, 0, sizeof(padding));
      padding[0] = 0x80;
      uint8_t bits[8]; // Save number of bits
      U32_B(bits, cnt_, 8);
      uint32_t index = cnt_[0] / 8 % 64; // pad out to 56 mod 64.
      uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
      update(padding, padLen);
      update(bits, 8); // Append length (before padding)
      uint8_t res[16];
      U32_B(res, sum_, 16); // Store state in digest
      std::basic_stringstream<Char_Type> ss; // hex string
      for (unsigned i = 0; i < 16; ++i) { // stream hex includes endian conversion
        ss << std::hex << std::setfill('0') << std::setw(2) << (res[i] & 0xff);
      }
      clear();
      return ss.str();
      #undef U32_B
    }

  public:

    /**
     * Calculates the MD5 for a given string.
     * @param const string_type & s
     * @return string_type
     */
    static string_type calculate(const string_type & s)
    { basic_md5 r; r.update(s.data(), s.length()); return r.final_result(); }

    /**
     * Calculates the MD5 for a given C-string.
     * @param const char* s
     * @return string_type
     */
    static string_type calculate(const void* data, size_t size)
    { basic_md5 r; r.update(data, size); return r.final_result(); }

    /**
     * Calculates the MD5 for a stream. Returns an empty string on error.
     * @param std::istream & is
     * @return string_type
     */
    static string_type calculate(std::istream & is)
    {
      basic_md5 r;
      char data[64];
      while(is.good() && is.read(data, sizeof(data)).good()) {
        r.update(data, sizeof(data));
      }
      if(!is.eof()) return string_type();
      if(is.gcount()) r.update(data, is.gcount());
      return r.final_result();
    }

    /**
     * Calculates the MD5 checksum for a given file, either read binary or as text.
     * @param const string_type & path
     * @param bool binary = true
     * @return string_type
     */
    static string_type file(const string_type & path, bool binary=true)
    {
      std::ifstream fs;
      fs.open(path.c_str(), binary ? (std::ios::in|std::ios::binary) : (std::ios::in));
      string_type s = calculate(fs);
      fs.close();
      return s;
    }

  private:

    /**
     * Performs the MD5 transformation on a given block
     * @param uint32_t *block
     */
    void transform(const uint8_t* block)
    {
      #define SHA256F1(x,y,z) (((x)&(y)) | (~(x)&(z)))
      #define SHA256F2(x,y,z) (((x)&(z)) | ((y)&(~(z))))
      #define SHA256F3(x,y,z) ((x)^(y)^(z))
      #define SHA256F4(x,y,z) ((y)^((x)|(~(z))))
      #define SHA256RL(x,n) (((x)<<(n))|((x)>>(32-(n))))
      #define FF(a,b,c,d,x,s,ac) { a = SHA256RL(a+ SHA256F1(b,c,d) + (x) + (ac), (s)) + (b); }
      #define GG(a,b,c,d,x,s,ac) { a = SHA256RL(a + SHA256F2(b,c,d) + (x) + (ac), (s)) + (b); }
      #define HH(a,b,c,d,x,s,ac) { a = SHA256RL(a + SHA256F3(b,c,d) + x + ac, s) + b; }
      #define II(a,b,c,d,x,s,ac) { a = SHA256RL(a + SHA256F4(b,c,d) + x + ac, s) + b; }

      #define B_U32(output, input, len) { \
        for(unsigned i = 0, j = 0; j < len; i++, j += 4) { \
          (output)[i] = ((uint32_t)(input)[j]) | (((uint32_t)(input)[j+1]) << 8) | \
          (((uint32_t)(input)[j+2]) << 16) | (((uint32_t)(input)[j+3]) << 24); \
        } \
      }

      uint32_t a = sum_[0], b = sum_[1], c = sum_[2], d = sum_[3], x[16];
      B_U32 (x, block, 64);
      FF(a,b,c,d,x[0],7,0xd76aa478);   FF(d,a,b,c,x[1],12,0xe8c7b756);
      FF(c,d,a,b,x[2],17,0x242070db);  FF(b,c,d,a,x[3],22,0xc1bdceee);
      FF(a,b,c,d,x[4],7,0xf57c0faf);   FF(d,a,b,c,x[5],12,0x4787c62a);
      FF(c,d,a,b,x[6],17,0xa8304613);  FF(b,c,d,a,x[7],22,0xfd469501);
      FF(a,b,c,d,x[8],7,0x698098d8);   FF(d,a,b,c,x[9],12,0x8b44f7af);
      FF(c,d,a,b,x[10],17,0xffff5bb1); FF(b,c,d,a,x[11],22,0x895cd7be);
      FF(a,b,c,d,x[12],7,0x6b901122);  FF(d,a,b,c,x[13],12,0xfd987193);
      FF(c,d,a,b,x[14],17,0xa679438e); FF(b,c,d,a,x[15],22,0x49b40821);
      GG(a,b,c,d,x[1],5,0xf61e2562);   GG(d,a,b,c,x[6],9,0xc040b340);
      GG(c,d,a,b,x[11],14,0x265e5a51); GG(b,c,d,a,x[0],20,0xe9b6c7aa);
      GG(a,b,c,d,x[5],5,0xd62f105d);   GG(d,a,b,c,x[10],9,0x2441453);
      GG(c,d,a,b,x[15],14,0xd8a1e681); GG(b,c,d,a,x[4],20,0xe7d3fbc8);
      GG(a,b,c,d,x[9],5,0x21e1cde6);   GG(d,a,b,c,x[14],9,0xc33707d6);
      GG(c,d,a,b,x[3],14,0xf4d50d87);  GG(b,c,d,a,x[8],20,0x455a14ed);
      GG(a,b,c,d,x[13],5,0xa9e3e905);  GG(d,a,b,c,x[2],9,0xfcefa3f8);
      GG(c,d,a,b,x[7],14,0x676f02d9);  GG(b,c,d,a,x[12],20,0x8d2a4c8a);
      HH(a,b,c,d,x[5],4,0xfffa3942);   HH(d,a,b,c,x[8],11,0x8771f681);
      HH(c,d,a,b,x[11],16,0x6d9d6122); HH(b,c,d,a,x[14],23,0xfde5380c);
      HH(a,b,c,d,x[1],4,0xa4beea44);   HH(d,a,b,c,x[4],11,0x4bdecfa9);
      HH(c,d,a,b,x[7],16,0xf6bb4b60);  HH(b,c,d,a,x[10],23,0xbebfbc70);
      HH(a,b,c,d,x[13],4,0x289b7ec6);  HH(d,a,b,c,x[0],11,0xeaa127fa);
      HH(c,d,a,b,x[3],16,0xd4ef3085);  HH(b,c,d,a,x[6],23,0x4881d05);
      HH(a,b,c,d,x[9],4,0xd9d4d039);   HH(d,a,b,c,x[12],11,0xe6db99e5);
      HH(c,d,a,b,x[15],16,0x1fa27cf8); HH(b,c,d,a,x[2],23,0xc4ac5665);
      II(a,b,c,d,x[0],6,0xf4292244);   II(d,a,b,c,x[7],10,0x432aff97);
      II(c,d,a,b,x[14],15,0xab9423a7); II(b,c,d,a,x[5],21,0xfc93a039);
      II(a,b,c,d,x[12],6,0x655b59c3);  II(d,a,b,c,x[3],10,0x8f0ccc92);
      II(c,d,a,b,x[10],15,0xffeff47d); II(b,c,d,a,x[1],21,0x85845dd1);
      II(a,b,c,d,x[8],6,0x6fa87e4f);   II(d,a,b,c,x[15],10,0xfe2ce6e0);
      II(c,d,a,b,x[6],15,0xa3014314);  II(b,c,d,a,x[13],21,0x4e0811a1);
      II(a,b,c,d,x[4],6,0xf7537e82);   II(d,a,b,c,x[11],10,0xbd3af235);
      II(c,d,a,b,x[2],15,0x2ad7d2bb);  II(b,c,d,a,x[9],21,0xeb86d391);
      sum_[0] += a; sum_[1] += b; sum_[2] += c; sum_[3] += d;
      memset(x, 0, sizeof x);
      #undef SHA256F1
      #undef SHA256F2
      #undef SHA256F3
      #undef SHA256F4
      #undef SHA256RL
      #undef FF
      #undef GG
      #undef HH
      #undef II
    }

  private:

    uint8_t buf_[64];
    uint32_t cnt_[2];
    uint32_t sum_[4];
  };

}}

namespace sw {
  using md5 = detail::basic_md5<>;
}
#endif

// @version: #e6719f1
/**
 * @package de.atwillys.cc.swl
 * @license %, public domain
 * @author Steve Reid <steve@edmweb.com> (original C source)
 * @author Volker Grabsch <vog@notjusthosting.com> (Small changes to fit into bglibs)
 * @author Bruce Guenter <bruce@untroubled.org> (Translation to simpler C++ Code)
 * @author Stefan Wilhelm <cerbero s@atwillys.de> (class template rewrite, types, endianess)
 * @file sha1.hh
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++98
 *
 * SHA1 calculation class template.
 */
#ifndef SHA1_HH
#define SHA1_HH

#if defined(OS_WINDOWS) || defined (_WINDOWS_) || defined(_WIN32) || defined(__MSC_VER)
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

namespace sw { namespace detail {

  /**
   * @class basic_sha1
   * @template
   */
  template <typename Char_Type=char>
  class basic_sha1
  {
  public:

    using string_type = std::basic_string<Char_Type>;

  public:

    explicit basic_sha1() : iterations_(), sum_(), buf_() { buf_.reserve(64); clear(); }
    basic_sha1(const basic_sha1&) = delete;
    basic_sha1(basic_sha1&&) = default;
    basic_sha1& operator=(const basic_sha1&) = delete;
    basic_sha1& operator=(basic_sha1&&) = default;
    ~basic_sha1() = default;

  public:

    /**
     * Clear/reset all internal buffers and states.
     */
    void clear()
    {
      sum_[0] = 0x67452301; sum_[1] = 0xefcdab89; sum_[2] = 0x98badcfe; sum_[3] = 0x10325476;
      sum_[4] = 0xc3d2e1f0; iterations_ = 0; buf_.clear();
    }

    /**
     * Push new binary data into the internal buf_ and recalculate the checksum.
     * @param const void* data
     * @param size_t size
     */
    void update(const void* data, size_t size)
    {
      if(!data || !size) return;
      const char* p = static_cast<const char*>(data);
      uint32_t block[16];
      if(!buf_.empty()) { // Deal with the remaining buf_ data
        while(size && buf_.length() < 64) { buf_ += *p++; --size; } // Copy bytes
        if(buf_.length() < 64) return; // Not enough data
        const char* pp = (const char*) buf_.data();
        for(unsigned i = 0; i < 16; ++i) {
          #if (defined (BYTE_ORDER)) && (defined (BIG_ENDIAN)) && ((BYTE_ORDER == BIG_ENDIAN))
          block[i] = (pp[0] << 0) | (pp[1] << 8) | (pp[2] << 16) | (pp[3] << 24);
          #else
          block[i] = (pp[3] << 0) | (pp[2] << 8) | (pp[1] << 16) | (pp[0] << 24);
          #endif
          pp += 4;
        }
        buf_.clear();
        transform(block);
      }
      while(size >= 64) { // Transform full blocks
        for(unsigned i = 0; i < 16; ++i) {
          #if (defined (BYTE_ORDER)) && (defined (BIG_ENDIAN)) && ((BYTE_ORDER == BIG_ENDIAN))
          block[i] = (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
          #else
          block[i] = (p[3] << 0) | (p[2] << 8) | (p[1] << 16) | (p[0] << 24);
          #endif
          p += 4;
        }
        transform(block);
        size -= 64;
      }
      while(size--) {
        buf_ += *p++; // Transfer remaining bytes into the buf_
      }
    }

    /**
     * Finanlise checksum, return hex string.
     * @return string_type
     */
    string_type final()
    {
      uint64_t total_bits = (iterations_ * 64 + buf_.size()) * 8;
      buf_ += (char) 0x80;
      typename std::string::size_type sz = buf_.size();
      while (buf_.size() < 64) buf_ += (char) 0;
      uint32_t block[16];
      for(unsigned i = 0; i < 16; i++) {
        #if (defined (BYTE_ORDER)) && (defined (BIG_ENDIAN)) && ((BYTE_ORDER == BIG_ENDIAN))
        block[i] = ((buf_[4*i+0] & 0xff) << 0) | ((buf_[4*i+1] & 0xff) << 8) |
          ((buf_[4*i+2] & 0xff) << 16) | ((buf_[4*i+3] & 0xff) << 24);
        #else
        block[i] = ((buf_[4*i+3] & 0xff) << 0) | ((buf_[4*i+2] & 0xff) << 8) |
          ((buf_[4*i+1] & 0xff) << 16) | ((buf_[4*i+0] & 0xff) << 24);
        #endif
      }
      if(sz > 56) {
        transform(block);
        for(unsigned i=0; i<14; ++i) block[i] = 0;
      }
      block[15] = (total_bits >>  0);
      block[14] = (total_bits >> 32);
      transform(block);
      std::basic_stringstream<Char_Type> ss; // hex string
      for (unsigned i = 0; i < 5; ++i) { // stream hex includes endian conversion
        ss << std::hex << std::setfill('0') << std::setw(8) << (sum_[i] & 0xffffffff);
      }
      clear();
      return ss.str();
    }

  public:

    /**
     * Calculates the SHA1 for a given string.
     * @param const string_type & s
     * @return string_type
     */
    static string_type calculate(const string_type & s)
    { basic_sha1 r; r.update(s.data(), s.length()); return r.final(); }

    /**
     * Calculates the SHA1 for a given C-string.
     * @param const char* s
     * @return string_type
     */
    static string_type calculate(const void* data, size_t size)
    { basic_sha1 r; r.update(data, size); return r.final(); }

    /**
     * Calculates the SHA1 for a stream. Returns an empty string on error.
     * @param std::istream & is
     * @return string_type
     */
    static string_type calculate(std::istream & is)
    {
      basic_sha1 r;
      char data[64];
      while(is.good() && is.read(data, sizeof(data)).good()) {
        r.update(data, sizeof(data));
      }
      if(!is.eof()) return string_type();
      if(is.gcount()) r.update(data, is.gcount());
      return r.final();
    }

    /**
     * Calculates the SHA1 checksum for a given file, either read binary or as text.
     * @param const string_type & path
     * @param bool binary = true
     * @return string_type
     */
    static string_type file(const string_type & path, bool binary=true)
    {
      std::ifstream fs;
      fs.open(path.c_str(), binary ? (std::ios::in|std::ios::binary) : (std::ios::in));
      string_type s = calculate(fs);
      fs.close();
      return s;
    }

  private:

    /**
     * Performs the SHA1 transformation on a given block
     * @param uint32_t *block
     */
    void transform(uint32_t *block)
    {
      #define SHA256ROL(value, bits) (((value) << (bits)) | (((value) & 0xffffffff) >> (32-(bits))))
      #define SHA256BLK(i) (block[i&15]=SHA256ROL(block[(i+13)&15]^block[(i+8)&15]^block[(i+2)&15]^block[i&15],1))
      #define SHA1R0(v,w,x,y,z,i) z += ((w&(x^y))^y) + block[i] + 0x5a827999 + SHA256ROL(v,5); w=SHA256ROL(w,30);
      #define SHA1R1(v,w,x,y,z,i) z += ((w&(x^y))^y) + SHA256BLK(i) + 0x5a827999 + SHA256ROL(v,5); w=SHA256ROL(w,30);
      #define SHA1R2(v,w,x,y,z,i) z += (w^x^y) + SHA256BLK(i) + 0x6ed9eba1 + SHA256ROL(v,5); w=SHA256ROL(w,30);
      #define SHA1R3(v,w,x,y,z,i) z += (((w|x)&y)|(w&x)) + SHA256BLK(i) + 0x8f1bbcdc + SHA256ROL(v,5); w=SHA256ROL(w,30);
      #define SHA1R4(v,w,x,y,z,i) z += (w^x^y) + SHA256BLK(i) + 0xca62c1d6 + SHA256ROL(v,5); w=SHA256ROL(w,30);
      uint32_t a = sum_[0], b = sum_[1], c = sum_[2], d = sum_[3], e = sum_[4];
      SHA1R0(a,b,c,d,e, 0); SHA1R0(e,a,b,c,d, 1); SHA1R0(d,e,a,b,c, 2); SHA1R0(c,d,e,a,b, 3); SHA1R0(b,c,d,e,a, 4);
      SHA1R0(a,b,c,d,e, 5); SHA1R0(e,a,b,c,d, 6); SHA1R0(d,e,a,b,c, 7); SHA1R0(c,d,e,a,b, 8); SHA1R0(b,c,d,e,a, 9);
      SHA1R0(a,b,c,d,e,10); SHA1R0(e,a,b,c,d,11); SHA1R0(d,e,a,b,c,12); SHA1R0(c,d,e,a,b,13); SHA1R0(b,c,d,e,a,14);
      SHA1R0(a,b,c,d,e,15); SHA1R1(e,a,b,c,d,16); SHA1R1(d,e,a,b,c,17); SHA1R1(c,d,e,a,b,18); SHA1R1(b,c,d,e,a,19);
      SHA1R2(a,b,c,d,e,20); SHA1R2(e,a,b,c,d,21); SHA1R2(d,e,a,b,c,22); SHA1R2(c,d,e,a,b,23); SHA1R2(b,c,d,e,a,24);
      SHA1R2(a,b,c,d,e,25); SHA1R2(e,a,b,c,d,26); SHA1R2(d,e,a,b,c,27); SHA1R2(c,d,e,a,b,28); SHA1R2(b,c,d,e,a,29);
      SHA1R2(a,b,c,d,e,30); SHA1R2(e,a,b,c,d,31); SHA1R2(d,e,a,b,c,32); SHA1R2(c,d,e,a,b,33); SHA1R2(b,c,d,e,a,34);
      SHA1R2(a,b,c,d,e,35); SHA1R2(e,a,b,c,d,36); SHA1R2(d,e,a,b,c,37); SHA1R2(c,d,e,a,b,38); SHA1R2(b,c,d,e,a,39);
      SHA1R3(a,b,c,d,e,40); SHA1R3(e,a,b,c,d,41); SHA1R3(d,e,a,b,c,42); SHA1R3(c,d,e,a,b,43); SHA1R3(b,c,d,e,a,44);
      SHA1R3(a,b,c,d,e,45); SHA1R3(e,a,b,c,d,46); SHA1R3(d,e,a,b,c,47); SHA1R3(c,d,e,a,b,48); SHA1R3(b,c,d,e,a,49);
      SHA1R3(a,b,c,d,e,50); SHA1R3(e,a,b,c,d,51); SHA1R3(d,e,a,b,c,52); SHA1R3(c,d,e,a,b,53); SHA1R3(b,c,d,e,a,54);
      SHA1R3(a,b,c,d,e,55); SHA1R3(e,a,b,c,d,56); SHA1R3(d,e,a,b,c,57); SHA1R3(c,d,e,a,b,58); SHA1R3(b,c,d,e,a,59);
      SHA1R4(a,b,c,d,e,60); SHA1R4(e,a,b,c,d,61); SHA1R4(d,e,a,b,c,62); SHA1R4(c,d,e,a,b,63); SHA1R4(b,c,d,e,a,64);
      SHA1R4(a,b,c,d,e,65); SHA1R4(e,a,b,c,d,66); SHA1R4(d,e,a,b,c,67); SHA1R4(c,d,e,a,b,68); SHA1R4(b,c,d,e,a,69);
      SHA1R4(a,b,c,d,e,70); SHA1R4(e,a,b,c,d,71); SHA1R4(d,e,a,b,c,72); SHA1R4(c,d,e,a,b,73); SHA1R4(b,c,d,e,a,74);
      SHA1R4(a,b,c,d,e,75); SHA1R4(e,a,b,c,d,76); SHA1R4(d,e,a,b,c,77); SHA1R4(c,d,e,a,b,78); SHA1R4(b,c,d,e,a,79);
      sum_[0] += a; sum_[1] += b; sum_[2] += c; sum_[3] += d; sum_[4] += e; iterations_++;
      #undef SHA256ROL
      #undef SHA256BLK
      #undef SHA1R0
      #undef SHA1R1
      #undef SHA1R2
      #undef SHA1R3
      #undef SHA1R4
    }

  private:

    uint64_t iterations_; // Number of iterations
    uint32_t sum_[5];     // Intermediate checksum digest buffer
    std::string buf_;     // Intermediate buffer for remaining pushed data
  };
}}

namespace sw {
  using sha1 = detail::basic_sha1<>;
}

#endif

// @version: #e6719f1
/**
 * @package de.atwillys.cc.swl
 * @file sha512.hh
 * @author Stefan Wilhelm (stfwi)
 * @license MIT
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++98
 *
 * SHA512 calculation class template.
 * -------------------------------------------------------------------------------------
 * +++ MIT license +++
 * Copyright (c) 2008-2022, Stefan Wilhelm <cerberos@atwillys.de>
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
 * -------------------------------------------------------------------------------------
 */
#ifndef SHA512_HH
#define SHA512_HH

#if defined(OS_WINDOWS) || defined (_WINDOWS_) || defined(_WIN32) || defined(__MSC_VER)
#include <inttypes.h>
#else
#include <stdint.h>
#endif
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

namespace sw { namespace detail {

  /**
   * @class basic_sha512
   * @template
   */
  template <typename Char_Type=char>
  class basic_sha512
  {
  public:

    using string_type = std::basic_string<Char_Type>;

  public:

    explicit basic_sha512() noexcept : iterations_(), sum_(), sz_(), block_() { clear(); }
    basic_sha512(const basic_sha512&) = delete;
    basic_sha512(basic_sha512&&) = default;
    basic_sha512& operator=(const basic_sha512&) = delete;
    basic_sha512& operator=(basic_sha512&&) = default;
    ~basic_sha512() noexcept = default;

  public:

    /**
     * Clear/reset all internal buffers and states.
     */
    void clear() noexcept
    {
      sum_[0] = 0x6a09e667f3bcc908; sum_[1] = 0xbb67ae8584caa73b;
      sum_[2] = 0x3c6ef372fe94f82b; sum_[3] = 0xa54ff53a5f1d36f1;
      sum_[4] = 0x510e527fade682d1; sum_[5] = 0x9b05688c2b3e6c1f;
      sum_[6] = 0x1f83d9abfb41bd6b; sum_[7] = 0x5be0cd19137e2179;
      sz_ = 0; iterations_ = 0; memset(&block_, 0, sizeof(block_));
    }

    /**
     * Push new binary data into the internal buf_ and recalculate the checksum.
     * @param const void* data
     * @param size_t size
     */
    void update(const void* data, size_t size)
    {
      unsigned nb=0, n=0, n_tail=0;
      const uint8_t *p = nullptr;
      n = 128 - sz_;
      n_tail = size < n ? size : n;
      memcpy(&block_[sz_], data, n_tail);
      if (sz_ + size < 128) { sz_ += size; return; }
      n = size - n_tail;
      nb = n >> 7;
      p = &(static_cast<const uint8_t*>(data)[n_tail]);
      transform(block_, 1);
      transform(p, nb);
      n_tail = n & 0x7f;
      memcpy(block_, &p[nb << 7], n_tail);
      sz_ = n_tail;
      iterations_ += (nb+1) << 7;
    }

    /**
     * Finanlise checksum, return hex string.
     * @return string_type
     */
    string_type final_data()
    {
      #if (defined (BYTE_ORDER)) && (defined (BIG_ENDIAN)) && ((BYTE_ORDER == BIG_ENDIAN))
      #define U32_B(x,b) *((b)+0)=(uint8_t)((x)); *((b)+1)=(uint8_t)((x)>>8); \
              *((b)+2)=(uint8_t)((x)>>16); *((b)+3)=(uint8_t)((x)>>24);
      #else
      #define U32_B(x,b) *((b)+3)=(uint8_t)((x)); *((b)+2)=(uint8_t)((x)>>8); \
              *((b)+1)=(uint8_t)((x)>>16); *((b)+0)=(uint8_t)((x)>>24);
      #endif
      unsigned nb = 0, n = 0;
      uint64_t n_total = 0;
      nb = 1 + ((0x80-17) < (sz_ & 0x7f));
      n_total = (iterations_ + sz_) << 3;
      n = nb << 7;
      memset(block_ + sz_, 0, n - sz_);
      block_[sz_] = 0x80;
      U32_B(n_total, block_ + n-4);
      transform(block_, nb);
      std::basic_stringstream<Char_Type> ss; // hex string
      for (unsigned i = 0; i < 8; ++i) ss << std::hex << std::setfill('0') << std::setw(16) << (sum_[i]);
      clear();
      return ss.str();
      #undef U32_B
    }

  public:

    /**
     * Calculates the SHA256 for a given string.
     * @param const string_type & s
     * @return string_type
     */
    static string_type calculate(const string_type & s)
    {
      basic_sha512 r;
      r.update(s.data(), s.length());
      return r.final_data();
    }

    /**
     * Calculates the SHA256 for a given C-string.
     * @param const char* s
     * @return string_type
     */
    static string_type calculate(const void* data, size_t size)
    { basic_sha512 r; r.update(data, size); return r.final_data(); }

    /**
     * Calculates the SHA256 for a stream. Returns an empty string on error.
     * @param std::istream & is
     * @return string_type
     */
    static string_type calculate(std::istream & is)
    {
      basic_sha512 r;
      char data[64];
      while(is.good() && is.read(data, sizeof(data)).good()) r.update(data, sizeof(data));
      if(!is.eof()) return string_type();
      if(is.gcount()) r.update(data, is.gcount());
      return r.final_data();
    }

    /**
     * Calculates the SHA256 checksum for a given file, either read binary or as text.
     * @param const string_type & path
     * @param bool binary = true
     * @return string_type
     */
    static string_type file(const string_type & path, bool binary=true)
    {
      std::ifstream fs;
      fs.open(path.c_str(), binary ? (std::ios::in|std::ios::binary) : (std::ios::in));
      string_type s = calculate(fs);
      fs.close();
      return s;
    }

  private:

    /**
     * Performs the SHA256 transformation on a given block
     * @param uint32_t *block
     */
    void transform(const uint8_t *data, size_t size)
    {
      #define SHA256SR(x, n) (x >> n)
      #define SHA256RR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))
      #define SHA256RL(x, n) ((x << n) | (x >> ((sizeof(x) << 3) - n)))
      #define SHA256CH(x, y, z)  ((x & y) ^ (~x & z))
      #define SHA256MJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
      #define SHA256F1(x) (SHA256RR(x, 28) ^ SHA256RR(x, 34) ^ SHA256RR(x, 39))
      #define SHA256F2(x) (SHA256RR(x, 14) ^ SHA256RR(x, 18) ^ SHA256RR(x, 41))
      #define SHA256F3(x) (SHA256RR(x,  1) ^ SHA256RR(x,  8) ^ SHA256SR(x,  7))
      #define SHA256F4(x) (SHA256RR(x, 19) ^ SHA256RR(x, 61) ^ SHA256SR(x,  6))
      #if (defined (BYTE_ORDER)) && (defined (BIG_ENDIAN)) && ((BYTE_ORDER == BIG_ENDIAN))
      #define SHA256BU64(b,x) *(x)=((uint64_t)*((b)+0))|((uint64_t)*((b)+1)<<8)|\
        ((uint64_t)*((b)+2)<<16)|((uint64_t)*((b)+3)<<24)|((uint64_t)*((b)+4)<<32)|\
        ((uint64_t)*((b)+5)<<40)|((uint64_t)*((b)+6)<<48)|((uint64_t)*((b)+7)<<56);
      #else
      #define SHA256BU64(b,x) *(x)=((uint64_t)*((b)+7))|((uint64_t)*((b)+6)<<8)|\
        ((uint64_t)*((b)+5)<<16)|((uint64_t)*((b)+4)<<24)|((uint64_t)*((b)+3)<<32)|\
        ((uint64_t)*((b)+2)<<40)|((uint64_t)*((b)+1)<<48)|((uint64_t)*((b)+0)<<56);
      #endif
      uint64_t t=0, u=0, v[8], w[80];
      const uint8_t *tblock = nullptr;
      size_t j = 0;
      for(size_t i = 0; i < size; ++i) {
        tblock = data + (i << 7);
        for(j = 0; j < 16; ++j) SHA256BU64(&tblock[j<<3], &w[j]);
        for(j = 16; j < 80; ++j) w[j] = SHA256F4(w[j-2]) + w[j-7] + SHA256F3(w[j-15]) + w[j-16];
        for(j = 0; j < 8; ++j) v[j] = sum_[j];
        for(j = 0; j < 80; ++j) {
          t = v[7] + SHA256F2(v[4]) + SHA256CH(v[4], v[5], v[6]) + lut_[j] + w[j];
          u = SHA256F1(v[0]) + SHA256MJ(v[0], v[1], v[2]); v[7] = v[6]; v[6] = v[5]; v[5] = v[4];
          v[4] = v[3] + t; v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t + u;
        }
        for(j = 0; j < 8; ++j) sum_[j] += v[j];
      }
      #undef SHA256SR
      #undef SHA256RR
      #undef SHA256RL
      #undef SHA256CH
      #undef SHA256MJ
      #undef SHA256F1
      #undef SHA256F2
      #undef SHA256F3
      #undef SHA256F4
      #undef SHA256BU64
    }

  private:
    uint64_t iterations_; // Number of iterations
    uint64_t sum_[8];     // Intermediate checksum buffer
    unsigned sz_;         // Number of currently stored bytes in the block
    uint8_t  block_[256];
    static const uint64_t lut_[80]; // Lookup table
  };

  template <typename CT>
  const uint64_t basic_sha512<CT>::lut_[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
    0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
    0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
    0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
    0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
    0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
    0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
    0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
    0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
  };

}}

namespace sw {
  using sha512 = detail::basic_sha512<>;
}

#endif

#include "../duktape.hh"
#include <string>

namespace duktape { namespace detail { namespace system { namespace hash {

  template <typename=void>
  int crc8_wrapper(duktape::api& stack)
  {
    if(stack.is<std::string>(0)) {
      auto data = stack.get<std::string>(0);
      stack.push(::sw::crc8(data.c_str(), data.size()));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::crc8(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("crc8 input data have to be a string of buffer");
    }
  }

  template <typename=void>
  int crc16_wrapper(duktape::api& stack)
  {
    if(stack.is<std::string>(0)) {
      stack.push(::sw::crc16::calculate(stack.to<std::string>(0)));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::crc16::calculate(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("crc16 input data have to be a string of buffer");
    }
  }

  template <typename=void>
  int crc32_wrapper(duktape::api& stack)
  {
    if(stack.is<std::string>(0)) {
      stack.push(::sw::crc32::calculate(stack.to<std::string>(0)));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::crc32::calculate(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("crc32 input data have to be a string of buffer");
    }
  }

  template <typename=void>
  int md5_wrapper(duktape::api& stack)
  {
    if(stack.is<bool>(1) && stack.get<bool>(1)) {
      if(!stack.is<std::string>(0)) {
        return stack.throw_exception("md5: First argument must be a string for file checksum calculation.");
      } else {
        std::string cksum = ::sw::md5::file(stack.get<std::string>(0));
        if(cksum.empty()) {
          return stack.throw_exception("Failed to read file for MD5 checksum calculation.");
        } else {
          stack.push(cksum);
          return 1;
        }
      }
    } else if(stack.is<std::string>(0)) {
      stack.push(::sw::md5::calculate(stack.to<std::string>(0)));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::md5::calculate(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("md5 input data have to be a string of buffer");
    }
  }

  template <typename=void>
  int sha1_wrapper(duktape::api& stack)
  {
    if(stack.is<bool>(1) && stack.get<bool>(1)) {
      if(!stack.is<std::string>(0)) {
        return stack.throw_exception("SHA1: First argument must be a string for file checksum calculation.");
      } else {
        std::string cksum = ::sw::sha1::file(stack.get<std::string>(0));
        if(cksum.empty()) {
          return stack.throw_exception("Failed to read file for SHA1 checksum calculation.");
        } else {
          stack.push(cksum);
          return 1;
        }
      }
    } else if(stack.is<std::string>(0)) {
      stack.push(::sw::sha1::calculate(stack.to<std::string>(0)));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::sha1::calculate(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("SHA1 input data have to be a string of buffer");
    }
  }

  template <typename=void>
  int sha512_wrapper(duktape::api& stack)
  {
    if(stack.is<bool>(1) && stack.get<bool>(1)) {
      if(!stack.is<std::string>(0)) {
        return stack.throw_exception("SHA512: First argument must be a string for file checksum calculation.");
      } else {
        std::string cksum = ::sw::sha512::file(stack.get<std::string>(0));
        if(cksum.empty()) {
          return stack.throw_exception("Failed to read file for SHA512 checksum calculation.");
        } else {
          stack.push(cksum);
          return 1;
        }
      }
    } else if(stack.is<std::string>(0)) {
      stack.push(::sw::sha512::calculate(stack.to<std::string>(0)));
      return 1;
    } else if(stack.is_buffer(0) || stack.is_buffer_data(0)) {
      auto buffer = stack.buffer<std::vector<char>>(0);
      stack.push(::sw::sha512::calculate(buffer.data(), buffer.size()));
      return 1;
    } else {
      return stack.throw_exception("SHA512 input data have to be a string of buffer");
    }
  }

}}}}

namespace duktape { namespace mod { namespace system { namespace hash {

  using namespace ::duktape::detail::system::hash;

  /**
   * Export main relay. Adds all module functions to the specified engine.
   * @param duktape::engine& js
   */
  template <typename=void>
  static void define_in(duktape::engine& js)
  {
    #if(0 && JSDOC)
    /**
     * Hashing functionality of the system object.
     * @var {object}
     */
    sys.hash = {};
    #endif

    #if(0 && JSDOC)
    /**
     * CRC8 (PEC) of a string or buffer.
     * (PEC CRC is: polynomial: 0x07, initial value: 0x00, final XOR: 0x00)
     *
     * @param {string|buffer} data
     * @return {number}
     */
    sys.hash.crc8 = function(data) {};
    #endif
    js.define("sys.hash.crc8", crc8_wrapper, 1);

    #if(0 && JSDOC)
    /**
     * CRC16 (USB) of a string or buffer.
     * (USB CRC is: polynomial: 0x8005, initial value: 0xffff, final XOR: 0xffff)
     *
     * @param {string|buffer} data
     * @return {number}
     */
    sys.hash.crc16 = function(data) {};
    #endif
    js.define("sys.hash.crc16", crc16_wrapper, 1);

    #if(0 && JSDOC)
    /**
     * CRC32 (CCITT) of a string or buffer.
     *
     * @param {string|buffer} data
     * @return {number}
     */
    sys.hash.crc32 = function(data) {};
    #endif
    js.define("sys.hash.crc32", crc32_wrapper, 1);

    #if(0 && JSDOC)
    /**
     * MD5 of a string, buffer or file (if `isfile==true`).
     *
     * @param {string|buffer} data
     * @param {boolean} [isfile=false]
     * @return {string}
     */
    sys.hash.md5 = function(data, isfile) {};
    #endif
    js.define("sys.hash.md5", md5_wrapper, 2);

    #if(0 && JSDOC)
    /**
     * SHA1 of a string, buffer or file (if `isfile==true`).
     *
     * @param {string|buffer} data
     * @param {boolean} [isfile=false]
     * @return {string}
     */
    sys.hash.sha1 = function(data, isfile) {};
    #endif
    js.define("sys.hash.sha1", sha1_wrapper, 2);

    #if(0 && JSDOC)
    /**
     * SHA512 of a string, buffer or file (if `isfile==true`).
     *
     * @param {string|buffer} data
     * @param {boolean} [isfile=false]
     * @return {string}
     */
    sys.hash.sha512 = function(data, isfile) {};
    #endif
    js.define("sys.hash.sha512", sha512_wrapper, 2);
  }

}}}}

#endif
