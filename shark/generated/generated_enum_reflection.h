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

// Author: jasonh@google.com (Jason Hsueh)
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.
// It provides reflection support for generated enums, and is included in
// generated .skp.h files and should have minimal dependencies. The methods are
// implemented in generated_message_reflection.cc.

#pragma once

#include <string>

#include <shark/generated/generated_enum_util.h>
#include <shark/port.h>
#include <shark/stubs/strutil.h>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

// Must be included last.
#include <shark/port_def.inc>

namespace shark {
    class EnumDescriptor;
} // namespace shark

namespace shark {

    namespace internal {

        // Helper for EnumType_Parse functions: try to parse the string 'name' as
        // an enum name of the given type, returning true and filling in value on
        // success, or returning false and leaving value unchanged on failure.
        SHARK_EXPORT bool ParseNamedEnum(const EnumDescriptor* descriptor,
            ConstStringParam name, int* value);

        template <typename EnumType>
        bool ParseNamedEnum(const EnumDescriptor* descriptor, ConstStringParam name,
            EnumType* value) {
            int tmp;
            if (!ParseNamedEnum(descriptor, name, &tmp))
                return false;
            *value = static_cast<EnumType>(tmp);
            return true;
        }

        // Just a wrapper around printing the name of a value. The main point of this
        // function is not to be inlined, so that you can do this without including
        // descriptor.h.
        SHARK_EXPORT const std::string& NameOfEnum(const EnumDescriptor* descriptor,
            int value);

    } // namespace internal
} // namespace shark

#include <shark/port_undef.inc>
