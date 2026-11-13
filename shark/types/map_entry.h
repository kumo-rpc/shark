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

#include <shark/format/wired/wire_format_lite.h>
#include <shark/generated/generated_message_reflection.h>
#include <shark/port.h>
#include <shark/reflection_ops.h>
#include <shark/types/map_entry_lite.h>
#include <shark/types/map_type_handler.h>
#include <shark/unknown_field_set.h>

// Must be included last.
#include <shark/port_def.inc>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace shark {
    class Arena;
    namespace internal {
        template <typename Derived, typename Key, typename Value,
            SharkFieldType kKeyFieldType,
            SharkFieldType kValueFieldType>
        class MapField;
    }
} // namespace shark

namespace shark {
    namespace internal {

        // MapEntry is the returned shark::Message when calling AddMessage of
        // shark::Reflection. In order to let it work with generated message
        // reflection, its in-memory type is the same as generated message with the same
        // fields. However, in order to decide the in-memory type of key/value, we need
        // to know both their cpp type in generated api and proto type. In
        // implementation, all in-memory types have related wire format functions to
        // support except ArenaStringPtr. Therefore, we need to define another type with
        // supporting wire format functions. Since this type is only used as return type
        // of MapEntry accessors, it's named MapEntry accessor type.
        //
        // cpp type:               the type visible to users in public API.
        // proto type:             SharkFieldType of the field.
        // in-memory type:         type of the data member used to stored this field.
        // MapEntry accessor type: type used in MapEntry getters/mutators to access the
        //                         field.
        //
        // cpp type | proto type  | in-memory type | MapEntry accessor type
        // int32_t    TYPE_INT32    int32_t          int32_t
        // int32_t    TYPE_FIXED32  int32_t          int32_t
        // string     TYPE_STRING   ArenaStringPtr   string
        // FooEnum    TYPE_ENUM     int              int
        // FooMessage TYPE_MESSAGE  FooMessage*      FooMessage
        //
        // The in-memory types of primitive types can be inferred from its proto type,
        // while we need to explicitly specify the cpp type if proto type is
        // TYPE_MESSAGE to infer the in-memory type.
        template <typename Derived, typename Key, typename Value,
            SharkFieldType kKeyFieldType,
            SharkFieldType kValueFieldType>
        class MapEntry : public MapEntryImpl<Derived, Message, Key, Value,
                             kKeyFieldType, kValueFieldType> {
        public:
            constexpr MapEntry() { }
            explicit MapEntry(Arena* arena)
                : MapEntryImpl<Derived, Message, Key, Value, kKeyFieldType,
                      kValueFieldType>(arena) { }
            ~MapEntry() override {
                Message::_internal_metadata_.template Delete<UnknownFieldSet>();
            }
            typedef void InternalArenaConstructable_;
            typedef void DestructorSkippable_;

            typedef typename MapEntryImpl<Derived, Message, Key, Value, kKeyFieldType,
                kValueFieldType>::KeyTypeHandler KeyTypeHandler;
            typedef
                typename MapEntryImpl<Derived, Message, Key, Value, kKeyFieldType,
                    kValueFieldType>::ValueTypeHandler ValueTypeHandler;
            size_t SpaceUsedLong() const override {
                size_t size = sizeof(Derived);
                size += KeyTypeHandler::SpaceUsedInMapEntryLong(this->key_);
                size += ValueTypeHandler::SpaceUsedInMapEntryLong(this->value_);
                return size;
            }

        private:
            friend class ::SHARK_NAMESPACE_ID::Arena;
            template <typename C, typename K, typename V,
                SharkFieldType k_wire_type, SharkFieldType>
            friend class internal::MapField;

            GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MapEntry);
        };

    } // namespace internal
} // namespace shark

#include <shark/port_undef.inc>
