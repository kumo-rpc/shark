// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <assert.h>
#include <cstdint>
#include <string>

#include <turbo/macros/platform/platform.h>

#include <shark/port_def.inc>

// Legacy: some users reference these (internal-only) macros even though we
// don't need them any more.
#if defined(_MSC_VER) && defined(SHARK_USE_DLLS)
  #ifdef LIBSHARK_EXPORTS
    #define LIBSHARK_EXPORT __declspec(dllexport)
  #else
    #define LIBSHARK_EXPORT __declspec(dllimport)
  #endif
  #ifdef LIBPROTOC_EXPORTS
    #define LIBPROTOC_EXPORT   __declspec(dllexport)
  #else
    #define LIBPROTOC_EXPORT   __declspec(dllimport)
  #endif
#else
  #define LIBSHARK_EXPORT
  #define LIBPROTOC_EXPORT
#endif

#define SHARK_RUNTIME_DEPRECATED(message) [[deprecated(message)]]
#define SHARK_RUNTIME_DEPRECATED(message) \
  [[deprecated(message)]]

// ===================================================================
// from google3/base/port.h

#if (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L || \
     (defined(_MSC_VER) && _MSC_VER >= 1900))
// Define this to 1 if the code is compiled in C++11 mode; leave it
// undefined otherwise.  Do NOT define it to 0 -- that causes
// '#ifdef LANG_CXX11' to behave differently from '#if LANG_CXX11'.
#define LANG_CXX11 1
#else
#error "Protobuf requires at least C++11."
#endif

namespace shark {

using ConstStringParam = const std::string &;

typedef unsigned int uint;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

static const int32 kint32max = 0x7FFFFFFF;
static const int32 kint32min = -kint32max - 1;
static const int64 kint64max = int64_t{0x7FFFFFFFFFFFFFFF};
static const int64 kint64min = -kint64max - 1;
static const uint32 kuint32max = 0xFFFFFFFFu;
static const uint64 kuint64max = uint64_t{0xFFFFFFFFFFFFFFFFu};

#if KUMO_OS_NACL \
    || (defined(__ANDROID__) && defined(__clang__) \
        && (__clang_major__ == 3 && __clang_minor__ == 8) \
        && (__clang_patchlevel__ < 275480))
# define SHARK_USE_PORTABLE_LOG2
#endif

// ===================================================================
// from google3/util/bits/bits.h

class Bits {
 public:
  static uint32_t Log2FloorNonZero(uint32_t n) {
#if defined(__GNUC__)
  return 31 ^ static_cast<uint32_t>(__builtin_clz(n));
#elif defined(_MSC_VER)
  unsigned long where;
  _BitScanReverse(&where, n);
  return where;
#else
  return Log2FloorNonZero_Portable(n);
#endif
  }

  static uint32_t Log2FloorNonZero64(uint64_t n) {
    // Older versions of clang run into an instruction-selection failure when
    // it encounters __builtin_clzll:
    // https://bugs.chromium.org/p/nativeclient/issues/detail?id=4395
    // This includes arm-nacl-clang and clang in older Android NDK versions.
    // To work around this, when we build with those we use the portable
    // implementation instead.
#if defined(__GNUC__) && !defined(SHARK_USE_PORTABLE_LOG2)
  return 63 ^ static_cast<uint32_t>(__builtin_clzll(n));
#elif defined(_MSC_VER) && defined(_M_X64)
  unsigned long where;
  _BitScanReverse64(&where, n);
  return where;
#else
  return Log2FloorNonZero64_Portable(n);
#endif
  }
 private:
  static int Log2FloorNonZero_Portable(uint32_t n) {
    if (n == 0)
      return -1;
    int log = 0;
    uint32_t value = n;
    for (int i = 4; i >= 0; --i) {
      int shift = (1 << i);
      uint32_t x = value >> shift;
      if (x != 0) {
        value = x;
        log += shift;
      }
    }
    assert(value == 1);
    return log;
  }

  static int Log2FloorNonZero64_Portable(uint64_t n) {
    const uint32_t topbits = static_cast<uint32_t>(n >> 32);
    if (topbits == 0) {
      // Top bits are zero, so scan in bottom bits
      return static_cast<int>(Log2FloorNonZero(static_cast<uint32_t>(n)));
    } else {
      return 32 + static_cast<int>(Log2FloorNonZero(topbits));
    }
  }
};

}  // namespace shark

#include <shark/port_undef.inc>

