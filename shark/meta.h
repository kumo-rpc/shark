// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <cstdint>

namespace shark {


    // Describes a single field of a message.  To get the descriptor for a given
    // field, first get the Descriptor for the message in which it is defined,
    // then call Descriptor::FindFieldByName().  To get a FieldDescriptor for
    // an extension, do one of the following:
    // - Get the Descriptor or FileDescriptor for its containing scope, then
    //   call Descriptor::FindExtensionByName() or
    //   FileDescriptor::FindExtensionByName().
    // - Given a DescriptorPool, call DescriptorPool::FindExtensionByNumber() or
    //   DescriptorPool::FindExtensionByPrintableName().
    // Use DescriptorPool to construct your own descriptors.
    // Field type. 0 is reserved for errors. Numbers are the descriptor values.


    // Wire-format field type. Numbers match SharkType. Kept separate so the lite
    // runtime does not depend on the descriptor.
    enum class SharkFieldType : uint8_t {
        BOOL = 1,
        INT8 = 2,
        UINT8 = 3,
        INT16 = 4,
        UINT16 = 5,
        INT32 = 6,
        UINT32 = 7,
        INT64 = 8,
        UINT64 = 9,
        FLOAT = 10,
        FLOAT16 = 11,
        FLOAT8 = 12,
        DOUBLE = 13,
        ENUM = 14,
        MESSAGE = 15,
        STRING = 16,
        BYTES = 17,
        FIXED32 = 18,
        FIXED64 = 19,
        SFIXED32 = 20,
        SFIXED64 = 21,
        SINT32 = 22,
        SINT64 = 23,
        MAX = 23
    };

    // Lite alternative to SharkFieldCppType.  Must be kept in sync.
    enum class SharkFieldCppType : uint8_t {
        CPPTYPE_BOOL = 1,
        CPPTYPE_INT8 = 2,
        CPPTYPE_UINT8 = 3,
        CPPTYPE_INT16 = 4,
        CPPTYPE_UINT16 = 5,
        CPPTYPE_INT32 = 6,
        CPPTYPE_UINT32 = 7,
        CPPTYPE_INT64 = 8,
        CPPTYPE_UINT64 = 9,
        CPPTYPE_FLOAT = 10,
        CPPTYPE_FLOAT16 = 11,
        CPPTYPE_FLOAT8 = 12,
        CPPTYPE_DOUBLE = 13,
        CPPTYPE_ENUM = 14,
        CPPTYPE_MESSAGE = 15,
        CPPTYPE_STRING = 16,
        MAX = 16,
    };

    // Whether a field is optional, required, or repeated. 0 is reserved for errors.
    enum class FieldLabel : uint8_t {
        UNKNOWN = 0,
        OPTIONAL = 1,
        REQUIRED = 2,
        REPEATED = 3,
        MAX = 3,
    };

    // Syntax of this file.
    enum class SyntaxEdition : uint8_t {
        SYNTAX_UNKNOWN = 0,
        SYNTAX_PROTO2 = 2,
        // Reserved. Do not reuse 3. Building a file with this syntax fails.
        SYNTAX_PROTO3 = 3,
        MAX = 3
    };

}  // namespace shark
