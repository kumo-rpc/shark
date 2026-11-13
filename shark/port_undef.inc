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

// #undefs all macros defined in port_def.inc.  See comments in port_def.inc
// for more info.

#ifndef SHARK_NAMESPACE
#error "port_undef.inc must be included after port_def.inc"
#endif

#undef SHARK_BUILTIN_BSWAP16
#undef SHARK_BUILTIN_BSWAP32
#undef SHARK_BUILTIN_BSWAP64
#undef SHARK_GNUC_MIN
#undef SHARK_MSC_VER_MIN
#undef SHARK_CPLUSPLUS_MIN
#undef SHARK_NAMESPACE
#undef SHARK_NAMESPACE_ID
#undef SHARK_ALWAYS_INLINE
#undef SHARK_NDEBUG_INLINE
#undef SHARK_MUSTTAIL
#undef SHARK_TAILCALL
#undef SHARK_COLD
#undef SHARK_NOINLINE
#undef SHARK_SECTION_VARIABLE
#undef SHARK_FUNC_ALIGN
#undef SHARK_RETURNS_NONNULL
#undef SHARK_ATTRIBUTE_REINITIALIZES
#undef SHARK_RTTI
#undef SHARK_FIELD_OFFSET
#undef SHARK_PREDICT_TRUE
#undef SHARK_PREDICT_FALSE
#undef SHARK_EXPORT
#undef PROTOC_EXPORT
#undef SHARK_FORCE_COPY_IN_RELEASE
#undef SHARK_FORCE_COPY_IN_SWAP
#undef SHARK_FORCE_COPY_IN_MOVE
#undef SHARK_FORCE_RESET_IN_CLEAR
#undef SHARK_FORCE_COPY_DEFAULT_STRING
#undef SHARK_NAMESPACE_OPEN
#undef SHARK_NAMESPACE_CLOSE
#undef SHARK_ASSUME
#undef SHARK_EXPORT_TEMPLATE_DECLARE
#undef SHARK_EXPORT_TEMPLATE_DEFINE
#undef SHARK_FUTURE_FINAL
#undef SHARK_LITTLE_ENDIAN
#undef SHARK_BIG_ENDIAN
#undef SHARK_MESSAGE_OWNED_ARENA_EXPERIMENT
#undef SHARK_CONSTINIT
#undef SHARK_ATTRIBUTE_WEAK
#undef SHARK_HAVE_ATTRIBUTE_WEAK
#undef SHARK_ATTRIBUTE_NO_DESTROY
#undef SHARK_ATTRIBUTE_STANDALONE_DEBUG
#undef SHARK_ATTRIBUTE_INIT_PRIORITY1
#undef SHARK_ATTRIBUTE_INIT_PRIORITY2
#undef SHARK_PRAGMA_INIT_SEG
#undef SHARK_ASAN
#undef SHARK_MSAN
#undef SHARK_TSAN
#undef SHARK_TAIL_CALL_TABLE_PARSER_ENABLED
#undef SHARK_TC_PARAM_DECL
#undef SHARK_EXCLUSIVE_LOCKS_REQUIRED
#undef SHARK_LOCKS_EXCLUDED
#undef SHARK_NO_THREAD_SAFETY_ANALYSIS
#undef SHARK_GUARDED_BY

#ifdef SHARK_FUTURE_BREAKING_CHANGES
#undef SHARK_FUTURE_BREAKING_CHANGES
#endif

// Restore macro that may have been #undef'd in port_def.inc.
#ifdef _MSC_VER
#pragma pop_macro("CREATE_NEW")
#pragma pop_macro("DELETE")
#pragma pop_macro("DOUBLE_CLICK")
#pragma pop_macro("ERROR")
#pragma pop_macro("ERROR_BUSY")
#pragma pop_macro("ERROR_INSTALL_FAILED")
#pragma pop_macro("ERROR_NOT_FOUND")
#pragma pop_macro("GetClassName")
#pragma pop_macro("GetMessage")
#pragma pop_macro("GetObject")
#pragma pop_macro("IGNORE")
#pragma pop_macro("IN")
#pragma pop_macro("INPUT_KEYBOARD")
#pragma pop_macro("OUT")
#pragma pop_macro("OPTIONAL")
#pragma pop_macro("min")
#pragma pop_macro("max")
#pragma pop_macro("NEAR")
#pragma pop_macro("NO_DATA")
#pragma pop_macro("NO_ERROR")
#pragma pop_macro("REASON_UNKNOWN")
#pragma pop_macro("SERVICE_DISABLED")
#pragma pop_macro("SEVERITY_ERROR")
#pragma pop_macro("STRICT")
#pragma pop_macro("STATUS_PENDING")
#pragma pop_macro("timezone")
#endif

#ifdef __APPLE__
#pragma pop_macro("DOMAIN")
#pragma pop_macro("TRUE")
#pragma pop_macro("FALSE")
#pragma pop_macro("UID_MAX")
#endif  // __APPLE__

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
#pragma pop_macro("DEBUG")
#endif // defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Pop the warning(push) from port_def.inc
#ifdef _MSC_VER
#pragma warning(pop)
#endif
