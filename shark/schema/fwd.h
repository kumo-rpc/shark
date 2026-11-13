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

#include <shark/port.h>
#include <shark/meta.h>
#include <shark/stubs/common.h>
#include <shark/stubs/logging.h>
#include <shark/stubs/mutex.h>
#include <shark/stubs/once.h>
#include <shark/enum_traits.h>
#include <cstdint>

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

        /// Non-owning view referencing an external byte sequence.
        STRING_VIEW = 16,

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

        /// SQL NULL type, representing an unknown or missing value.
        /// Physical type: VOID.
        NULLPTR = 1,

        /// Boolean value.
        /// Physical type: BOOL.
        BOOL = 2,

        /// Unsigned 8-bit integer.
        /// Physical type: UINT8.
        UINT8 = 3,

        /// Signed 8-bit integer.
        /// Physical type: INT8.
        INT8 = 4,

        /// Unsigned 16-bit integer.
        /// Physical type: UINT16.
        UINT16 = 5,

        /// Signed 16-bit integer.
        /// Physical type: INT16.
        INT16 = 6,

        /// Unsigned 32-bit integer.
        /// Physical type: UINT32.
        UINT32 = 7,

        /// Signed 32-bit integer.
        /// Physical type: INT32.
        INT32 = 8,

        /// Unsigned 64-bit integer.
        /// Physical type: UINT64.
        UINT64 = 9,

        /// Signed 64-bit integer.
        /// Physical type: INT64.
        INT64 = 10,

        /// Unsigned 128-bit integer.
        /// Physical type: UINT128.
        UINT128 = 11,

        /// Signed 128-bit integer.
        /// Physical type: INT128.
        INT128 = 12,

        /// 16-bit floating-point number.
        /// Physical type: FLOAT16.
        FLOAT16 = 13,

        /// 32-bit floating-point number.
        /// Physical type: FLOAT32.
        FLOAT32 = 14,

        /// 64-bit floating-point number.
        /// Physical type: FLOAT64.
        FLOAT64 = 15,

        /// Enumeration type, representing a named set of integer constants.
        /// Physical type: UINT8, UINT16, or UINT32.
        ENUM = 16,

        /// 64-bit fixed-point decimal number.
        /// Physical type: INT64.
        DECIMAL64 = 17,

        /// 128-bit fixed-point decimal number.
        /// Physical type: INT128.
        DECIMAL128 = 18,

        /// Timestamp type with microsecond (us) precision.
        /// Represents the number of microseconds since the Unix epoch.
        /// Physical type: INT64.
        TIMESTAMP = 19,

        /// UTF-8 encoded string.
        /// Physical type: STRING or STRING_VIEW.
        STRING = 20,

        /// Binary data, representing a sequence of bytes without character encoding.
        /// Physical type: STRING or STRING_VIEW.
        BINARY = 21,

        /// Bit string, representing a sequence of bits.
        /// Physical type: STRING or STRING_VIEW.
        BIT = 22,

        /// Variable-length sequence of values of a specified logical type.
        /// Physical type: LIST.
        LIST = 23,

        /// Fixed-length sequence of values of a specified logical type.
        /// Physical type: ARRAY.
        ARRAY = 24,

        /// Universally unique identifier (UUID).
        /// Physical type: UINT128.
        UUID = 25,

        /// Composite value containing named fields.
        /// Physical type: STRUCT.
        STRUCT = 26,

        /// Value representing one of several alternative logical types.
        /// Physical type: UNION.
        UNION = 27,

        /// Generic type representing any logical type.
        /// A type-system abstraction used for generic function parameters,
        /// return types, and type constraints.
        /// Does not represent a concrete runtime value or prescribe a physical layout.
        ANY = 28,

        /// Unnamed composite value containing positional fields.
        /// Physical type: STRUCT.
        TUPLE = 29,

        /// Collection of key-value pairs with specified key and value types.
        /// Physical type: MAP.
        MAP = 30,

        /// Collection of unique values of a specified logical type.
        /// Physical type: SET.
        SET = 31,

        /// Function or operation identifier.
        /// Represented by a symbol name, such as kumo.shark.add.
        /// Physical type: STRING or STRING_VIEW.
        SYMBOL = 32,

        /// Dynamically typed value that can hold a value of any logical type.
        /// A concrete runtime data type implemented using a built-in UNION
        /// with runtime type information.
        VARIANT = 33,

        /// Maximum logical type value.
        MAX = VARIANT,

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
