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

#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <cstdint>
#include <shark/enum_traits.h>
#include <shark/meta.h>
#include <shark/port.h>
#include <shark/stubs/common.h>
#include <shark/stubs/logging.h>
#include <shark/stubs/mutex.h>
#include <shark/stubs/once.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    enum class PhysicalType : uint8_t {
        /// A void type with no physical storage.
        /// Used as a placeholder for values without a physical representation.
        VOID = 0,

        /// Boolean value stored as an 8-bit value.
        BOOL = 1,

        /// Unsigned 8-bit integer.
        UINT8 = 2,

        /// Signed 8-bit integer.
        INT8 = 3,

        /// Unsigned 16-bit little-endian integer.
        UINT16 = 4,

        /// Signed 16-bit little-endian integer.
        INT16 = 5,

        /// Unsigned 32-bit little-endian integer.
        UINT32 = 6,

        /// Signed 32-bit little-endian integer.
        INT32 = 7,

        /// Unsigned 64-bit little-endian integer.
        UINT64 = 8,

        /// Signed 64-bit little-endian integer.
        INT64 = 9,

        /// Unsigned 128-bit integer.
        UINT128 = 10,

        /// Signed 128-bit integer.
        INT128 = 11,

        /// 16-bit floating-point value.
        FLOAT16 = 12,

        /// 32-bit floating-point value.
        FLOAT32 = 13,

        /// 64-bit floating-point value.
        FLOAT64 = 14,

        /// Variable-length byte sequence with owned storage.
        /// Does not impose any character encoding requirements.
        STRING = 15,

        /// Variable-length sequence of values of a specified logical type.
        LIST = 17,

        /// Fixed-length sequence of values of a specified logical type.
        ARRAY = 18,

        /// Collection of key-value pairs with specified key and value types.
        MAP = 19,

        /// Collection of unique values of a specified logical type.
        SET = 20,

        /// Composite value containing named or indexed fields.
        /// Also represents tuple values, where field names are positional indices.
        STRUCT = 21,

        /// Value representing one of several alternative logical types.
        UNION = 22,

        /// User-defined physical type.
        UDF = 254,

        /// Maximum physical type value.
        MAX = UDF,

        /// Invalid physical type.
        INVALID = 255
    };

    enum class LogicalTypeId : uint8_t {
        /// Void type, representing the absence of a value.
        VOID = 0,

        /// Boolean value.
        /// Physical type: BOOL.
        BOOL = 1,

        /// Unsigned 8-bit integer.
        /// Physical type: UINT8.
        UINT8 = 2,

        /// Signed 8-bit integer.
        /// Physical type: INT8.
        INT8 = 3,

        /// Unsigned 16-bit integer.
        /// Physical type: UINT16.
        UINT16 = 4,

        /// Signed 16-bit integer.
        /// Physical type: INT16.
        INT16 = 5,

        /// Unsigned 32-bit integer.
        /// Physical type: UINT32.
        UINT32 = 6,

        /// Signed 32-bit integer.
        /// Physical type: INT32.
        INT32 = 7,

        /// Unsigned 64-bit integer.
        /// Physical type: UINT64.
        UINT64 = 8,

        /// Signed 64-bit integer.
        /// Physical type: INT64.
        INT64 = 9,

        /// Unsigned 128-bit integer.
        /// Physical type: UINT128.
        UINT128 = 10,

        /// Signed 128-bit integer.
        /// Physical type: INT128.
        INT128 = 11,

        /// 16-bit floating-point number.
        /// Physical type: FLOAT16.
        FLOAT16 = 12,

        /// 32-bit floating-point number.
        /// Physical type: FLOAT32.
        FLOAT32 = 13,

        /// 64-bit floating-point number.
        /// Physical type: FLOAT64.
        FLOAT64 = 14,

        /// Enumeration type, representing a named set of integer constants.
        /// Physical type: UINT8, UINT16, or UINT32.
        ENUM = 15,

        /// UTF-8 encoded string.
        /// Physical type: STRING.
        STRING = 16,

        /// Binary data, representing a sequence of bytes without character encoding.
        /// Physical type: STRING.
        BINARY = 17,

        /// Variable-length sequence of values of a specified logical type.
        /// Physical type: LIST.
        LIST = 18,

        /// Composite value containing named fields.
        /// Physical type: STRUCT.
        STRUCT = 19,

        /// Value representing one of several alternative logical types.
        /// Physical type: UNION.
        UNION = 20,

        /// Generic type representing any logical type.
        /// A type-system abstraction used for generic function parameters,
        /// return types, and type constraints.
        /// Does not represent a concrete runtime value or prescribe a physical layout.
        ANY = 21,

        /// Collection of key-value pairs with specified key and value types.
        /// Physical type: MAP.
        MAP = 22,

        /// Collection of unique values of a specified logical type.
        /// Physical type: SET.
        SET = 23,

        /// Maximum logical type value.
        MAX = SET,

        /// Invalid logical type.
        INVALID = 255,
    };

    /////////////////////////////////////////

    /// Defined in this file.
    class Descriptor;
    class FieldDescriptor;
    class OneofDescriptor;
    class EnumDescriptor;
    class EnumValueDescriptor;
    class ServiceDescriptor;
    class MethodDescriptor;
    class FileDescriptor;
    class DescriptorDatabase;
    class DescriptorPool;

    // Defined in descriptor.shark
    class DescriptorProto;
    class DescriptorProto_ExtensionRange;
    class FieldDescriptorProto;
    class OneofDescriptorProto;
    class EnumDescriptorProto;
    class EnumValueDescriptorProto;
    class ServiceDescriptorProto;
    class MethodDescriptorProto;
    class FileDescriptorProto;
    class MessageOptions;
    class FieldOptions;
    class OneofOptions;
    class EnumOptions;
    class EnumValueOptions;
    class ExtensionRangeOptions;
    class ServiceOptions;
    class MethodOptions;
    class FileOptions;
    class UninterpretedOption;
    class SourceCodeInfo;

    // Defined in message.h
    class Message;
    class Reflection;

    // Defined in descriptor.cc
    class DescriptorBuilder;
    class FileDescriptorTables;
    class Symbol;

    // Defined in unknown_field_set.h.
    class UnknownField;

    // Defined in command_line_interface.cc
    namespace compiler {
        class CommandLineInterface;
        namespace cpp {
            // Defined in helpers.h
            class Formatter;
        } // namespace cpp
        namespace cc {
            // Defined in helpers.h
            class Formatter;
        } // namespace cpp
    } // namespace compiler

    namespace descriptor_unittest {
        class DescriptorTest;
    } // namespace descriptor_unittest

    // Defined in printer.h
    namespace io {
        class Printer;
    } // namespace io

    // NB, all indices are zero-based.
    struct SourceLocation {
        int start_line;
        int end_line;
        int start_column;
        int end_column;

        // Doc comments found at the source location.
        // See the comments in SourceCodeInfo.Location (descriptor.shark) for details.
        std::string leading_comments;
        std::string trailing_comments;
        std::vector<std::string> leading_detached_comments;
    };

    // Options when generating machine-parsable output from a descriptor with
    // DebugString().
    struct DebugStringOptions {
        // include original user comments as recorded in SourceLocation entries. N.B.
        // that this must be |false| by default: several other pieces of code (for
        // example, the C++ code generation for fields in the proto compiler) rely on
        // DebugString() output being unobstructed by user comments.
        bool include_comments;
        // If true, elide the braced body in the debug string.
        bool elide_group_body;
        bool elide_oneof_body;

        DebugStringOptions()
            : include_comments(false)
            , elide_group_body(false)
            , elide_oneof_body(false) {
        }
    };

    // A class to handle the simplest cases of a lazily linked descriptor
    // for a message type that isn't built at the time of cross linking,
    // which is needed when a pool has lazily_build_dependencies_ set.
    // Must be instantiated as mutable in a descriptor.
    namespace internal {

        // The classes in this file represent a significant memory footprint for the
        // library. We make sure we are not accidentally making them larger by
        // hardcoding the struct size for a specific platform. Use as:
        //
        //   SHARK_INTERNAL_CHECK_CLASS_SIZE(type, expected_size_in_x84-64);
        //

#if !defined(SHARK_INTERNAL_CHECK_CLASS_SIZE)
#define SHARK_INTERNAL_CHECK_CLASS_SIZE(t, expected)
#endif

        class FlatAllocator;

        class SHARK_EXPORT LazyDescriptor {
        public:
            // Init function to be called at init time of a descriptor containing
            // a LazyDescriptor.
            void Init() {
                descriptor_ = nullptr;
                once_ = nullptr;
            }

            // Sets the value of the descriptor if it is known during the descriptor
            // building process. Not thread safe, should only be called during the
            // descriptor build process. Should not be called after SetLazy has been
            // called.
            void Set(const Descriptor* descriptor);

            // Sets the information needed to lazily cross link the descriptor at a later
            // time, SetLazy is not thread safe, should be called only once at descriptor
            // build time if the symbol wasn't found and building of the file containing
            // that type is delayed because lazily_build_dependencies_ is set on the pool.
            // Should not be called after Set() has been called.
            void SetLazy(StringPiece name, const FileDescriptor* file);

            // Returns the current value of the descriptor, thread-safe. If SetLazy(...)
            // has been called, will do a one-time cross link of the type specified,
            // building the descriptor file that contains the type if necessary.
            inline const Descriptor* Get(const ServiceDescriptor* service) {
                Once(service);
                return descriptor_;
            }

        private:
            void Once(const ServiceDescriptor* service);

            const Descriptor* descriptor_;
            // The once_ flag is followed by a NUL terminated string for the type name.
            internal::once_flag* once_;
        };

        class SHARK_EXPORT SymbolBase {
        private:
            friend class shark::Symbol;
            uint8_t symbol_type_;
        };

        // Some types have more than one SymbolBase because they have multiple
        // identities in the table. We can't have duplicate direct bases, so we use this
        // intermediate base to do so.
        // See BuildEnumValue for details.
        template <int N>
        class SHARK_EXPORT SymbolBaseN : public SymbolBase { };

    } // namespace internal

} // namespace shark

#include <shark/port_undef.inc>
