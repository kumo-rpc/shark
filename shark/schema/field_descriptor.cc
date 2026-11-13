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

#include <string>
#include <vector>
#include <shark/stubs/logging.h>
#include <shark/schema/file_descriptor.h>
#include <shark/meta.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    const SharkFieldCppType
        FieldDescriptor::kTypeToCppTypeMap[kMaxFieldType + 1] = {
            static_cast<SharkFieldCppType>(0), // 0 is reserved for errors

            SharkFieldCppType::CPPTYPE_BOOL, // SharkFieldType::BOOL
            SharkFieldCppType::CPPTYPE_INT8, // SharkFieldType::INT8
            SharkFieldCppType::CPPTYPE_UINT8, // SharkFieldType::UINT8
            SharkFieldCppType::CPPTYPE_INT16, // SharkFieldType::INT16
            SharkFieldCppType::CPPTYPE_UINT16, // SharkFieldType::UINT16
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::INT32
            SharkFieldCppType::CPPTYPE_UINT32, // SharkFieldType::UINT32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::INT64
            SharkFieldCppType::CPPTYPE_UINT64, // SharkFieldType::UINT64
            SharkFieldCppType::CPPTYPE_FLOAT, // SharkFieldType::FLOAT
            SharkFieldCppType::CPPTYPE_FLOAT16, // SharkFieldType::FLOAT16
            SharkFieldCppType::CPPTYPE_FLOAT8, // SharkFieldType::FLOAT8
            SharkFieldCppType::CPPTYPE_DOUBLE, // SharkFieldType::DOUBLE
            SharkFieldCppType::CPPTYPE_ENUM, // SharkFieldType::ENUM
            SharkFieldCppType::CPPTYPE_MESSAGE, // SharkFieldType::MESSAGE
            SharkFieldCppType::CPPTYPE_STRING, // SharkFieldType::STRING
            SharkFieldCppType::CPPTYPE_STRING, // SharkFieldType::BYTES
            SharkFieldCppType::CPPTYPE_UINT32, // SharkFieldType::FIXED32
            SharkFieldCppType::CPPTYPE_UINT64, // SharkFieldType::FIXED64
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::SFIXED32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::SFIXED64
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::SINT32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::SINT64
        };

    const char* const FieldDescriptor::kTypeToName[kMaxFieldType + 1] = {
        "ERROR", // 0 is reserved for errors

        "bool", // SharkFieldType::BOOL
        "int8", // SharkFieldType::INT8
        "uint8", // SharkFieldType::UINT8
        "int16", // SharkFieldType::INT16
        "uint16", // SharkFieldType::UINT16
        "int32", // SharkFieldType::INT32
        "uint32", // SharkFieldType::UINT32
        "int64", // SharkFieldType::INT64
        "uint64", // SharkFieldType::UINT64
        "float", // SharkFieldType::FLOAT
        "float16", // SharkFieldType::FLOAT16
        "float8", // SharkFieldType::FLOAT8
        "double", // SharkFieldType::DOUBLE
        "enum", // SharkFieldType::ENUM
        "message", // SharkFieldType::MESSAGE
        "string", // SharkFieldType::STRING
        "bytes", // SharkFieldType::BYTES
        "fixed32", // SharkFieldType::FIXED32
        "fixed64", // SharkFieldType::FIXED64
        "sfixed32", // SharkFieldType::SFIXED32
        "sfixed64", // SharkFieldType::SFIXED64
        "sint32", // SharkFieldType::SINT32
        "sint64", // SharkFieldType::SINT64
    };

    const char* const FieldDescriptor::kCppTypeToName[kMaxFieldCppType + 1] = {
        "ERROR", // 0 is reserved for errors

        "bool", // SharkFieldCppType::CPPTYPE_BOOL
        "int8", // SharkFieldCppType::CPPTYPE_INT8
        "uint8", // SharkFieldCppType::CPPTYPE_UINT8
        "int16", // SharkFieldCppType::CPPTYPE_INT16
        "uint16", // SharkFieldCppType::CPPTYPE_UINT16
        "int32", // SharkFieldCppType::CPPTYPE_INT32
        "uint32", // SharkFieldCppType::CPPTYPE_UINT32
        "int64", // SharkFieldCppType::CPPTYPE_INT64
        "uint64", // SharkFieldCppType::CPPTYPE_UINT64
        "float", // SharkFieldCppType::CPPTYPE_FLOAT
        "float16", // SharkFieldCppType::CPPTYPE_FLOAT16
        "float8", // SharkFieldCppType::CPPTYPE_FLOAT8
        "double", // SharkFieldCppType::CPPTYPE_DOUBLE
        "enum", // SharkFieldCppType::CPPTYPE_ENUM
        "message", // SharkFieldCppType::CPPTYPE_MESSAGE
        "string", // SharkFieldCppType::CPPTYPE_STRING
    };

    const char* const FieldDescriptor::kLabelToName[kMaxFieldLabel + 1] = {
        "ERROR", // 0 is reserved for errors

        "optional", // FieldLabel::OPTIONAL
        "required", // FieldLabel::REQUIRED
        "repeated", // FieldLabel::REPEATED
    };

}  // namespace shark
#include <shark/port_undef.inc>
