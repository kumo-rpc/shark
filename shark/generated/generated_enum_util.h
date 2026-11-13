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


#include <type_traits>

#include <shark/stubs/strutil.h>
#include <shark/message_lite.h>

// Must be included last.
#include <shark/port_def.inc>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace shark {


namespace internal {

// The table entry format for storing enum name-to-value mapping used with lite
// protos. This struct and the following related functions should only be used
// by protobuf generated code.
struct EnumEntry {
  StringPiece name;
  int value;
};

// Looks up a numeric enum value given the string name.
SHARK_EXPORT bool LookUpEnumValue(const EnumEntry* enums, size_t size,
                                     StringPiece name, int* value);

// Looks up an enum name given the numeric value.
SHARK_EXPORT int LookUpEnumName(const EnumEntry* enums,
                                   const int* sorted_indices, size_t size,
                                   int value);

// Initializes the list of enum names in std::string form.
SHARK_EXPORT bool InitializeEnumStrings(
    const EnumEntry* enums, const int* sorted_indices, size_t size,
    internal::ExplicitlyConstructed<std::string>* enum_strings);

}  // namespace internal
}  // namespace shark

#include <shark/port_undef.inc>

