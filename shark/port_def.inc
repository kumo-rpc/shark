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

// This file defines common macros that are used in protobuf.
//
// To hide these definitions from the outside world (and to prevent collisions
// if more than one version of protobuf is #included in the same project) you
// must follow this pattern when #including port_def.inc in a header file:
//
// #include "other_header.h"
// #include "message.h"
// // etc.
//
// #include "port_def.inc"  // MUST be last header included
//
// // Definitions for this header.
//
// #include "port_undef.inc"
//
// This is a textual header with no include guard, because we want to
// detect/prohibit anytime it is #included twice without a corresponding
// #undef.

// The definitions in this file are intended to be portable across Clang,
// GCC, and MSVC. Function-like macros are usable without an #ifdef guard.
// Syntax macros (for example, attributes) are always defined, although
// they may be empty.
//
// Some definitions rely on the NDEBUG macro and/or (in MSVC) _DEBUG:
// - https://en.cppreference.com/w/c/error/assert
// - https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros#microsoft-specific-predefined-macros
//
// References for predefined macros:
// - Standard: https://en.cppreference.com/w/cpp/preprocessor/replace
// - Clang: https://clang.llvm.org/docs/LanguageExtensions.html
//          (see also GCC predefined macros)
// - GCC: https://gcc.gnu.org/onlinedocs/cpp/Predefined-Macros.html
// - MSVC: https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
// - Interactive (Clang/GCC only): https://www.compiler-explorer.com/z/hc6jKd3sj
//
// References for attributes (and extension attributes):
// - Standard: https://en.cppreference.com/w/cpp/language/attributes
// - Clang: https://clang.llvm.org/docs/AttributeReference.html
// - GCC: https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
//        (see Clang attribute docs as well)
//
// References for standard C++ language conformance (and minimum versions):
// - Clang: https://clang.llvm.org/cxx_status.html
// - GCC: https://gcc.gnu.org/projects/cxx-status.html
// - MSVC: https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance
//
// Historical release notes (which can help to determine minimum versions):
// - Clang: https://releases.llvm.org/
// - GCC: https://gcc.gnu.org/releases.html
// - MSVC: https://docs.microsoft.com/en-us/visualstudio/releases/2019/release-notes-history
//         https://docs.microsoft.com/en-us/visualstudio/releasenotes/vs2017-relnotes-history

// Portable fallbacks for C++20 feature test macros:
// https://en.cppreference.com/w/cpp/feature_test
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#define SHARK_has_cpp_attribute_DEFINED_
#endif

// Portable fallback for Clang's __has_feature macro:
// https://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
#ifndef __has_feature
#define __has_feature(x) 0
#define SHARK_has_feature_DEFINED_
#endif

// Portable fallback for Clang's __has_warning macro:
#ifndef __has_warning
#define __has_warning(x) 0
#define SHARK_has_warning_DEFINED_
#endif

// Portable fallbacks for the __has_attribute macro (GCC and Clang):
// https://clang.llvm.org/docs/LanguageExtensions.html#has-attribute
// https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html
#ifndef __has_attribute
#define __has_attribute(x) 0
#define SHARK_has_attribute_DEFINED_
#endif

// Portable fallback for __has_builtin (GCC and Clang):
// https://clang.llvm.org/docs/LanguageExtensions.html#has-builtin
// https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fbuiltin.html
#ifndef __has_builtin
#define __has_builtin(x) 0
#define SHARK_has_builtin_DEFINED_
#endif

// Portable SHARK_BUILTIN_BSWAPxx definitions
// Code must check for availability, e.g.: `defined(SHARK_BUILTIN_BSWAP32)`
#ifdef SHARK_BUILTIN_BSWAP16
#error SHARK_BUILTIN_BSWAP16 was previously defined
#endif
#ifdef SHARK_BUILTIN_BSWAP32
#error SHARK_BUILTIN_BSWAP32 was previously defined
#endif
#ifdef SHARK_BUILTIN_BSWAP64
#error SHARK_BUILTIN_BSWAP64 was previously defined
#endif
#if defined(__GNUC__) || __has_builtin(__builtin_bswap16)
#define SHARK_BUILTIN_BSWAP16(x) __builtin_bswap16(x)
#endif
#if defined(__GNUC__) || __has_builtin(__builtin_bswap32)
#define SHARK_BUILTIN_BSWAP32(x) __builtin_bswap32(x)
#endif
#if defined(__GNUC__) || __has_builtin(__builtin_bswap64)
#define SHARK_BUILTIN_BSWAP64(x) __builtin_bswap64(x)
#endif

// Portable check for GCC minimum version:
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__GNUC__) && defined(__GNUC_MINOR__) \
    && defined(__GNUC_PATCHLEVEL__)
#  define SHARK_GNUC_MIN(x, y) \
  (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#  define SHARK_GNUC_MIN(x, y) 0
#endif

// Portable check for MSVC minimum version:
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#if defined(_MSC_VER)
#define SHARK_MSC_VER_MIN(x) (_MSC_VER >= x)
#else
#define SHARK_MSC_VER_MIN(x) 0
#endif

// Portable check for minimum C++ language version:
// https://en.cppreference.com/w/cpp/preprocessor/replace
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#if !defined(_MSVC_LANG)
#define SHARK_CPLUSPLUS_MIN(x) (__cplusplus >= x)
#else
#define SHARK_CPLUSPLUS_MIN(x) (_MSVC_LANG >= x)
#endif

// Future versions of protobuf will include breaking changes to some APIs.
// This macro can be set to enable these API changes ahead of time, so that
// user code can be updated before upgrading versions of protobuf.
// SHARK_FUTURE_FINAL is used on classes that are historically not marked as
// final, but that may be marked final in future (breaking) releases.
// #define SHARK_FUTURE_BREAKING_CHANGES 1
// #define SHARK_FUTURE_FINAL final
#define SHARK_FUTURE_FINAL

#include <shark/version.h>

#if defined(SHARK_NAMESPACE) || defined(SHARK_NAMESPACE_ID)
#error SHARK_NAMESPACE or SHARK_NAMESPACE_ID was previously defined
#endif
#define SHARK_NAMESPACE "shark"
#define SHARK_NAMESPACE_ID shark
#define SHARK_NAMESPACE_OPEN namespace shark {
#define SHARK_NAMESPACE_CLOSE }  // namespace shark

#ifdef SHARK_ALWAYS_INLINE
#error SHARK_ALWAYS_INLINE was previously defined
#endif
// For functions we want to force inline.
#if defined(SHARK_NO_INLINE)
# define SHARK_ALWAYS_INLINE
#elif SHARK_GNUC_MIN(3, 1)
# define SHARK_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
# define SHARK_ALWAYS_INLINE __forceinline
#else
# define SHARK_ALWAYS_INLINE
#endif

#ifdef SHARK_NDEBUG_INLINE
#error SHARK_NDEBUG_INLINE was previously defined
#endif
// Avoid excessive inlining in non-optimized builds. Without other optimizations
// the inlining is not going to provide benefits anyway and the huge resulting
// functions, especially in the proto-generated serialization functions, produce
// stack frames so large that many tests run into stack overflows (b/32192897).
#if defined(NDEBUG) || (defined(_MSC_VER) && !defined(_DEBUG))
# define SHARK_NDEBUG_INLINE SHARK_ALWAYS_INLINE
#else
# define SHARK_NDEBUG_INLINE
#endif

// Note that SHARK_NOINLINE is an attribute applied to functions, to prevent
// them from being inlined by the compiler. This is different from
// SHARK_NO_INLINE, which is a user-supplied macro that disables forced
// inlining by SHARK_(ALWAYS|NDEBUG)_INLINE.
#ifdef SHARK_NOINLINE
#error SHARK_NOINLINE was previously defined
#endif
#if SHARK_GNUC_MIN(3, 1)
# define SHARK_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
// Seems to have been around since at least Visual Studio 2005
# define SHARK_NOINLINE __declspec(noinline)
#endif

#ifdef SHARK_MUSTTAIL
#error SHARK_MUSTTAIL was previously defined
#endif
#ifdef SHARK_TAILCALL
#error SHARK_TAILCALL was previously defined
#endif
#if __has_cpp_attribute(clang::musttail) && !defined(__arm__) && \
    !defined(_ARCH_PPC) && !defined(__wasm__) &&                 \
    !(defined(_MSC_VER) && defined(_M_IX86)) &&                  \
    !(defined(__NDK_MAJOR__) && __NDK_MAJOR <= 24)
#  ifndef PROTO2_OPENSOURCE
// Compilation fails on ARM32: b/195943306
// Compilation fails on powerpc64le: b/187985113
// Compilation fails on X86 Windows:
// https://github.com/llvm/llvm-project/issues/53271
#  endif
#define SHARK_MUSTTAIL [[clang::musttail]]
#define SHARK_TAILCALL true
#else
#define SHARK_MUSTTAIL
#define SHARK_TAILCALL false
#endif

#ifdef SHARK_EXCLUSIVE_LOCKS_REQUIRED
#error SHARK_EXCLUSIVE_LOCKS_REQUIRED was previously defined
#endif
#if __has_attribute(exclusive_locks_required)
#define SHARK_EXCLUSIVE_LOCKS_REQUIRED(...) \
  __attribute__((exclusive_locks_required(__VA_ARGS__)))
#else
#define SHARK_EXCLUSIVE_LOCKS_REQUIRED(...)
#endif

#ifdef SHARK_NO_THREAD_SAFETY_ANALYSIS
#error SHARK_NO_THREAD_SAFETY_ANALYSIS was previously defined
#endif
#if __has_attribute(no_thread_safety_analysis)
#define SHARK_NO_THREAD_SAFETY_ANALYSIS \
  __attribute__((no_thread_safety_analysis))
#else
#define SHARK_NO_THREAD_SAFETY_ANALYSIS
#endif

#ifdef SHARK_GUARDED_BY
#error SHARK_GUARDED_BY was previously defined
#endif
#if __has_attribute(guarded_by)
#define SHARK_GUARDED_BY(x) __attribute__((guarded_by(x)))
#else
#define SHARK_GUARDED_BY(x)
#endif

#ifdef SHARK_LOCKS_EXCLUDED
#error SHARK_LOCKS_EXCLUDED was previously defined
#endif
#if __has_attribute(locks_excluded)
#define SHARK_LOCKS_EXCLUDED(...) \
  __attribute__((locks_excluded(__VA_ARGS__)))
#else
#define SHARK_LOCKS_EXCLUDED(...)
#endif

#ifdef SHARK_COLD
#error SHARK_COLD was previously defined
#endif
#if __has_attribute(cold) || SHARK_GNUC_MIN(4, 3)
# define SHARK_COLD __attribute__((cold))
#else
# define SHARK_COLD
#endif

#ifdef SHARK_SECTION_VARIABLE
#error SHARK_SECTION_VARIABLE was previously defined
#endif
#if (__has_attribute(section) || defined(__GNUC__)) && defined(__ELF__)
// Place a variable in the given ELF section.
# define SHARK_SECTION_VARIABLE(x) __attribute__((section(#x)))
#else
# define SHARK_SECTION_VARIABLE(x)
#endif

#ifdef SHARK_FUNC_ALIGN
#error SHARK_FUNC_ALIGN was previously defined
#endif
#if __has_attribute(aligned) || SHARK_GNUC_MIN(4, 3)
#define SHARK_FUNC_ALIGN(bytes) __attribute__((aligned(bytes)))
#else
#define SHARK_FUNC_ALIGN(bytes)
#endif

#ifdef SHARK_RETURNS_NONNULL
#error SHARK_RETURNS_NONNULL was previously defined
#endif
#if __has_attribute(returns_nonnull) || SHARK_GNUC_MIN(4, 9)
#define SHARK_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define SHARK_RETURNS_NONNULL
#endif

#ifdef SHARK_ATTRIBUTE_REINITIALIZES
#error SHARK_ATTRIBUTE_REINITIALIZES was previously defined
#endif
#if __has_cpp_attribute(clang::reinitializes)
#define SHARK_ATTRIBUTE_REINITIALIZES [[clang::reinitializes]]
#else
#define SHARK_ATTRIBUTE_REINITIALIZES
#endif

// The minimum library version which works with the current version of the
// headers.

#ifdef SHARK_RTTI
#error SHARK_RTTI was previously defined
#endif
#if defined(SHARK_NO_RTTI) && SHARK_NO_RTTI
// A user-provided definition SHARK_NO_RTTI=1 disables RTTI.
#define SHARK_RTTI 0
#elif defined(__cpp_rtti)
// https://en.cppreference.com/w/cpp/feature_test
#define SHARK_RTTI 1
#elif __has_feature(cxx_rtti)
// https://clang.llvm.org/docs/LanguageExtensions.html#c-rtti
#define SHARK_RTTI 1
#elif defined(__GXX_RTTI)
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#define SHARK_RTTI 1
#elif defined(_CPPRTTI)
// https://docs.microsoft.com/en-us/cpp/build/reference/gr-enable-run-time-type-information
#define SHARK_RTTI 1
#else
#define SHARK_RTTI 0
#endif

// Returns the offset of the given field within the given aggregate type.
// This is equivalent to the ANSI C offsetof() macro.  However, according
// to the C++ standard, offsetof() only works on POD types, and GCC
// enforces this requirement with a warning.  In practice, this rule is
// unnecessarily strict; there is probably no compiler or platform on
// which the offsets of the direct fields of a class are non-constant.
// Fields inherited from superclasses *can* have non-constant offsets,
// but that's not what this macro will be used for.
#ifdef SHARK_FIELD_OFFSET
#error SHARK_FIELD_OFFSET was previously defined
#endif
#if defined(__clang__)
// For Clang we use __builtin_offsetof() and suppress the warning,
// to avoid Control Flow Integrity and UBSan vptr sanitizers from
// crashing while trying to validate the invalid reinterpret_casts.
#define SHARK_FIELD_OFFSET(TYPE, FIELD)                   \
  _Pragma("clang diagnostic push")                           \
  _Pragma("clang diagnostic ignored \"-Winvalid-offsetof\"") \
  __builtin_offsetof(TYPE, FIELD)                            \
  _Pragma("clang diagnostic pop")
#elif SHARK_GNUC_MIN(4, 8)
#define SHARK_FIELD_OFFSET(TYPE, FIELD) __builtin_offsetof(TYPE, FIELD)
#else  // defined(__clang__)
// Note that we calculate relative to the pointer value 16 here since if we
// just use zero, GCC complains about dereferencing a NULL pointer.  We
// choose 16 rather than some other number just in case the compiler would
// be confused by an unaligned pointer.
#define SHARK_FIELD_OFFSET(TYPE, FIELD)                                \
  static_cast< ::uint32_t>(reinterpret_cast<const char*>(                   \
                             &reinterpret_cast<const TYPE*>(16)->FIELD) - \
                         reinterpret_cast<const char*>(16))
#endif

#ifdef SHARK_EXPORT
#error SHARK_EXPORT was previously defined
#endif

#if defined(SHARK_USE_DLLS) && defined(_MSC_VER)
# if defined(LIBSHARK_EXPORTS)
#  define SHARK_EXPORT __declspec(dllexport)
#  define SHARK_EXPORT_TEMPLATE_DECLARE
#  define SHARK_EXPORT_TEMPLATE_DEFINE __declspec(dllexport)
# else
#  define SHARK_EXPORT __declspec(dllimport)
#  define SHARK_EXPORT_TEMPLATE_DECLARE
#  define SHARK_EXPORT_TEMPLATE_DEFINE __declspec(dllimport)
# endif  // defined(LIBSHARK_EXPORTS)
#elif defined(SHARK_USE_DLLS) && defined(LIBSHARK_EXPORTS)
# define SHARK_EXPORT __attribute__((visibility("default")))
# define SHARK_EXPORT_TEMPLATE_DECLARE __attribute__((visibility("default")))
# define SHARK_EXPORT_TEMPLATE_DEFINE
#else
# define SHARK_EXPORT
# define SHARK_EXPORT_TEMPLATE_DECLARE
# define SHARK_EXPORT_TEMPLATE_DEFINE
#endif

#ifdef PROTOC_EXPORT
#error PROTOC_EXPORT was previously defined
#endif

#if defined(SHARK_USE_DLLS) && defined(_MSC_VER)
# if defined(LIBPROTOC_EXPORTS)
#  define PROTOC_EXPORT __declspec(dllexport)
# else
#  define PROTOC_EXPORT __declspec(dllimport)
# endif  // defined(LIBPROTOC_EXPORTS)
#elif defined(SHARK_USE_DLLS) && defined(LIBPROTOC_EXPORTS)
# define PROTOC_EXPORT __attribute__((visibility("default")))
#else
# define PROTOC_EXPORT
#endif

#if defined(SHARK_PREDICT_TRUE) || defined(SHARK_PREDICT_FALSE)
#error SHARK_PREDICT_(TRUE|FALSE) was previously defined
#endif
#if SHARK_GNUC_MIN(3, 0)
# define SHARK_PREDICT_TRUE(x) (__builtin_expect(false || (x), true))
# define SHARK_PREDICT_FALSE(x) (__builtin_expect(false || (x), false))
#else
# define SHARK_PREDICT_TRUE(x) (x)
# define SHARK_PREDICT_FALSE(x) (x)
#endif

// Enable all stable experiments if this flag is set.  This allows us to group
// all of these experiments under a single build flag, which can be enabled in
// the protobuf.stable-experiments TAP project.
#ifdef SHARK_ENABLE_STABLE_EXPERIMENTS
#define SHARK_FORCE_MESSAGE_OWNED_ARENA
#endif  // !SHARK_ENABLE_STABLE_EXPERIMENTS

#ifdef SHARK_FORCE_COPY_IN_RELEASE
#error SHARK_FORCE_COPY_IN_RELEASE was previously defined
#endif

#ifdef SHARK_FORCE_COPY_IN_SWAP
#error SHARK_FORCE_COPY_IN_SWAP was previously defined
#endif

#ifdef SHARK_FORCE_COPY_IN_MOVE
#error SHARK_FORCE_COPY_IN_MOVE was previously defined
#endif

#ifdef SHARK_FORCE_RESET_IN_CLEAR
#error SHARK_FORCE_RESET_IN_CLEAR was previously defined
#endif

// Force copy the default string to a string field so that non-optimized builds
// have harder-to-rely-on address stability.
#ifdef SHARK_FORCE_COPY_DEFAULT_STRING
#error SHARK_FORCE_COPY_DEFAULT_STRING was previously defined
#endif

// SHARK_ASSUME(pred) tells the compiler that it can assume pred is true. To
// be safe, we also validate the assumption with a GOOGLE_DCHECK in unoptimized
// builds. The macro does not do anything useful if the compiler does not
// support __builtin_assume.
#ifdef SHARK_ASSUME
#error SHARK_ASSUME was previously defined
#endif
#if __has_builtin(__builtin_assume)
#define SHARK_ASSUME(pred) \
  GOOGLE_DCHECK(pred);               \
  __builtin_assume(pred)
#else
#define SHARK_ASSUME(pred) GOOGLE_DCHECK(pred)
#endif

// TODO(b/228173843): cleanup SHARK_LITTLE_ENDIAN in various 3p forks.
#if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define SHARK_LITTLE_ENDIAN 1
#ifdef SHARK_BIG_ENDIAN
#error Conflicting SHARK_BIG_ENDIAN was previously defined
#endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SHARK_BIG_ENDIAN 1
#elif defined(_WIN32) || defined(__x86_64__) || defined(__aarch64__)
#define SHARK_LITTLE_ENDIAN 1
#else
#error "endian detection failed for current compiler"
#endif

// For enabling message owned arena, one major blocker is semantic change from
// moving to copying when there is ownership transfer (e.g., move ctor, swap,
// set allocated, release). This change not only causes performance regression
// but also breaks users code (e.g., dangling reference). For top-level
// messages, since it owns the arena, we can mitigate the issue by transferring
// ownership of arena. However, we cannot do that for nested messages. In order
// to tell how many usages of nested messages affected by message owned arena,
// we need to simulate the arena ownership.
// This experiment is purely for the purpose of gathering data. All code guarded
// by this flag is supposed to be removed after this experiment.
#define SHARK_MESSAGE_OWNED_ARENA_EXPERIMENT
#ifdef SHARK_CONSTINIT
#error SHARK_CONSTINIT was previously defined
#endif
#if defined(__cpp_constinit) && !defined(_MSC_VER)
#define SHARK_CONSTINIT constinit
// Some older Clang versions incorrectly raise an error about
// constant-initializing weak default instance pointers. Versions 12.0 and
// higher seem to work, except that XCode 12.5.1 shows the error even though it
// uses Clang 12.0.5.
// Clang-cl on Windows raises error also.
#elif !defined(_MSC_VER) && __has_cpp_attribute(clang::require_constant_initialization) && \
    ((defined(__APPLE__) && __clang_major__ >= 13) ||                \
     (!defined(__APPLE__) && __clang_major__ >= 12))
#define SHARK_CONSTINIT [[clang::require_constant_initialization]]
#elif SHARK_GNUC_MIN(12, 2)
#define SHARK_CONSTINIT __constinit
// MSVC 17 currently seems to raise an error about constant-initialized pointers.
#elif defined(_MSC_VER) && _MSC_VER >= 1930
#define SHARK_CONSTINIT
#else
#define SHARK_CONSTINIT
#endif

// Some globals with an empty non-trivial destructor are annotated with
// no_destroy for performance reasons. It reduces the cost of these globals in
// non-opt mode and under sanitizers.
#ifdef SHARK_ATTRIBUTE_NO_DESTROY
#error SHARK_ATTRIBUTE_NO_DESTROY was previously defined
#endif
#if __has_cpp_attribute(clang::no_destroy)
#define SHARK_ATTRIBUTE_NO_DESTROY [[clang::no_destroy]]
#else
#define SHARK_ATTRIBUTE_NO_DESTROY
#endif

// Force clang to always emit complete debug info for a type.
// Clang uses constructor homing to determine when to emit debug info for a
// type. If the constructor of a type is never used, which can happen in some
// cases where member variables are constructed in place for optimization
// purposes (see b/208803175 for an example), the type will have incomplete
// debug info unless this attribute is used.
#ifdef SHARK_ATTRIBUTE_STANDALONE_DEBUG
#error SHARK_ATTRIBUTE_STANDALONE_DEBUG was previously defined
#endif
#if __has_cpp_attribute(clang::standalone_debug)
#define SHARK_ATTRIBUTE_STANDALONE_DEBUG [[clang::standalone_debug]]
#else
#define SHARK_ATTRIBUTE_STANDALONE_DEBUG
#endif

// Protobuf extensions and reflection require registration of the protos linked
// in the binary. Not until everything is registered does the runtime have a
// complete view on all protos. When code is using reflection or extensions
// in between registration calls this can lead to surprising behavior. By
// having the registration run first we mitigate this scenario.
// Highest priority is 101. We use 102 for registration, to allow code that
// really wants to higher priority to still beat us. Some initialization happens
// at higher priority, though, since it is needed before registration.
#ifdef SHARK_ATTRIBUTE_INIT_PRIORITY1
#error SHARK_ATTRIBUTE_INIT_PRIORITY1 was previously defined
#endif
#ifdef SHARK_ATTRIBUTE_INIT_PRIORITY2
#error SHARK_ATTRIBUTE_INIT_PRIORITY2 was previously defined
#endif
#if SHARK_GNUC_MIN(3, 0) && (!defined(__APPLE__) || defined(__clang__)) && \
    !((defined(sun) || defined(__sun)) &&                                     \
      (defined(__SVR4) || defined(__svr4__)))
#define SHARK_ATTRIBUTE_INIT_PRIORITY1 __attribute__((init_priority((101))))
#define SHARK_ATTRIBUTE_INIT_PRIORITY2 __attribute__((init_priority((102))))
#else
#define SHARK_ATTRIBUTE_INIT_PRIORITY1
#define SHARK_ATTRIBUTE_INIT_PRIORITY2
#endif

#ifdef SHARK_PRAGMA_INIT_SEG
#error SHARK_PRAGMA_INIT_SEG was previously defined
#endif
#ifdef _MSC_VER
#define SHARK_PRAGMA_INIT_SEG __pragma(init_seg(lib))
#else
#define SHARK_PRAGMA_INIT_SEG
#endif

#ifdef SHARK_ATTRIBUTE_WEAK
#error SHARK_ATTRIBUTE_WEAK was previously defined
#endif
#if __has_attribute(weak) && \
    !defined(__APPLE__) && \
    (!defined(_WIN32) || __clang_major__ < 9) && \
    !defined(__MINGW32__)
#define SHARK_ATTRIBUTE_WEAK __attribute__((weak))
#define SHARK_HAVE_ATTRIBUTE_WEAK 1
#else
#define SHARK_ATTRIBUTE_WEAK
#define SHARK_HAVE_ATTRIBUTE_WEAK 0
#endif

// Macros to detect sanitizers.
#ifdef SHARK_ASAN
#error SHARK_ASAN was previously defined
#endif
#ifdef SHARK_MSAN
#error SHARK_MSAN was previously defined
#endif
#ifdef SHARK_TSAN
#error SHARK_TSAN was previously defined
#endif
#if defined(__clang__)
#  if __has_feature(address_sanitizer)
#    define SHARK_ASAN 1
#  endif
#  if __has_feature(thread_sanitizer)
#    define SHARK_TSAN 1
#  endif
#  if __has_feature(memory_sanitizer)
#    define SHARK_MSAN 1
#  endif
#elif SHARK_GNUC_MIN(3, 0)
// Double-guard is needed for -Wundef:
#  ifdef __SANITIZE_ADDRESS__
#  if    __SANITIZE_ADDRESS__
#    define SHARK_ASAN 1
#  endif
#  endif
#  ifdef __SANITIZE_THREAD__
#  if    __SANITIZE_THREAD__
#    define SHARK_TSAN 1
#  endif
#  endif
#endif

// Tail call table-driven parsing can be enabled by defining
// SHARK_EXPERIMENTAL_USE_TAIL_CALL_TABLE_PARSER at compilation time. Note
// that this macro is for small-scale testing only, and is not supported.
#ifdef SHARK_TAIL_CALL_TABLE_PARSER_ENABLED
#error SHARK_TAIL_CALL_TABLE_PARSER_ENABLED was previously declared
#endif
#if defined(SHARK_EXPERIMENTAL_USE_TAIL_CALL_TABLE_PARSER)
#define SHARK_TAIL_CALL_TABLE_PARSER_ENABLED 1
#endif

#define SHARK_TC_PARAM_DECL                                 \
  ::shark::MessageLite *msg, const char *ptr,                 \
      ::shark::internal::ParseContext *ctx,                   \
      const ::shark::internal::TcParseTableBase *table,       \
      uint64_t hasbits, ::shark::internal::TcFieldData data

// ThreadSafeArenaz is turned off completely in opensource builds.

// Windows declares several inconvenient macro names.  We #undef them and then
// restore them in port_undef.inc.
#ifdef _MSC_VER
#pragma push_macro("CREATE_NEW")
#undef CREATE_NEW
#pragma push_macro("DELETE")
#undef DELETE
#pragma push_macro("DOUBLE_CLICK")
#undef DOUBLE_CLICK
#pragma push_macro("ERROR")
#undef ERROR
#pragma push_macro("ERROR_BUSY")
#undef ERROR_BUSY
#pragma push_macro("ERROR_INSTALL_FAILED")
#undef ERROR_INSTALL_FAILED
#pragma push_macro("ERROR_NOT_FOUND")
#undef ERROR_NOT_FOUND
#pragma push_macro("GetClassName")
#undef GetClassName
#pragma push_macro("GetMessage")
#undef GetMessage
#pragma push_macro("GetObject")
#undef GetObject
#pragma push_macro("IGNORE")
#undef IGNORE
#pragma push_macro("IN")
#undef IN
#pragma push_macro("INPUT_KEYBOARD")
#undef INPUT_KEYBOARD
#pragma push_macro("NO_ERROR")
#undef NO_ERROR
#pragma push_macro("OUT")
#undef OUT
#pragma push_macro("OPTIONAL")
#undef OPTIONAL
#pragma push_macro("min")
#undef min
#pragma push_macro("max")
#undef max
#pragma push_macro("NEAR")
#undef NEAR
#pragma push_macro("NO_DATA")
#undef NO_DATA
#pragma push_macro("REASON_UNKNOWN")
#undef REASON_UNKNOWN
#pragma push_macro("SERVICE_DISABLED")
#undef SERVICE_DISABLED
#pragma push_macro("SEVERITY_ERROR")
#undef SEVERITY_ERROR
#pragma push_macro("STATUS_PENDING")
#undef STATUS_PENDING
#pragma push_macro("STRICT")
#undef STRICT
#pragma push_macro("timezone")
#undef timezone
#endif  // _MSC_VER

#ifdef __APPLE__
// Inconvenient macro names from usr/include/math.h in some macOS SDKs.
#pragma push_macro("DOMAIN")
#undef DOMAIN
// Inconvenient macro names from /usr/include/mach/boolean.h in some macOS SDKs.
#pragma push_macro("TRUE")
#undef TRUE
#pragma push_macro("FALSE")
#undef FALSE
// Inconvenient macro names from usr/include/sys/syslimits.h in some macOS SDKs.
#pragma push_macro("UID_MAX")
#undef UID_MAX
#endif  // __APPLE__

#if defined(__clang__) || SHARK_GNUC_MIN(3, 0) || defined(_MSC_VER)
// Don't let Objective-C Macros interfere with proto identifiers with the same
// name.
#pragma push_macro("DEBUG")
#undef DEBUG
#endif // defined(__clang__) || SHARK_GNUC_MIN(3, 0) || defined(_MSC_VER)

#if SHARK_GNUC_MIN(3, 0)
// GCC does not allow disabling diagnostics within an expression:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60875, so we disable this one
// globally even though it's only used for SHARK_FIELD_OFFSET.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

// Silence some MSVC warnings in all our code.
#ifdef _MSC_VER
#pragma warning(push)
// For non-trivial unions
#pragma warning(disable : 4582)
#pragma warning(disable : 4583)
// For init_seg(lib)
#pragma warning(disable : 4073)
// To silence the fact that we will pop this push from another file
#pragma warning(disable : 5031)
// Conditional expression is constant
#pragma warning(disable: 4127)
// decimal digit terminates octal escape sequence
#pragma warning(disable: 4125)
#endif

// We don't want code outside port_def doing complex testing, so
// remove our portable condition test macros to nudge folks away from
// using it themselves.
#ifdef SHARK_has_cpp_attribute_DEFINED_
#  undef __has_cpp_attribute
#  undef SHARK_has_cpp_attribute_DEFINED_
#endif
#ifdef SHARK_has_feature_DEFINED_
#  undef __has_feature
#  undef SHARK_has_feature_DEFINED_
#endif
#ifdef SHARK_has_warning_DEFINED_
#  undef __has_warning
#  undef SHARK_has_warning_DEFINED_
#endif
#ifdef SHARK_has_attribute_DEFINED_
#  undef __has_attribute
#  undef SHARK_has_attribute_DEFINED_
#endif
#ifdef SHARK_has_builtin_DEFINED_
#  undef __has_builtin
#  undef SHARK_has_builtin_DEFINED_
#endif
