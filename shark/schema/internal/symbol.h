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
#include <shark/schema/internal/utils.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

     class Symbol {
    public:
        enum Type {
            NULL_SYMBOL,
            MESSAGE,
            FIELD,
            ONEOF,
            ENUM,
            ENUM_VALUE,
            ENUM_VALUE_OTHER_PARENT,
            SERVICE,
            METHOD,
            FULL_PACKAGE,
            SUB_PACKAGE,
            QUERY_KEY
        };

        Symbol() {
            static constexpr internal::SymbolBase null_symbol { };
            static_assert(null_symbol.symbol_type_ == NULL_SYMBOL, "");
            // Initialize with a sentinel to make sure `ptr_` is never null.
            ptr_ = &null_symbol;
        }

        // Every object we store derives from internal::SymbolBase, where we store the
        // symbol type enum.
        // Storing in the object can be done without using more space in most cases,
        // while storing it in the Symbol type would require 8 bytes.
#define DEFINE_MEMBERS(TYPE, TYPE_CONSTANT, FIELD)                                 \
    explicit Symbol(TYPE* value)                                                   \
        : ptr_(value) {                                                            \
        value->symbol_type_ = TYPE_CONSTANT;                                       \
    }                                                                              \
    const TYPE* FIELD() const {                                                    \
        return type() == TYPE_CONSTANT ? static_cast<const TYPE*>(ptr_) : nullptr; \
    }

        DEFINE_MEMBERS(Descriptor, MESSAGE, descriptor)
        DEFINE_MEMBERS(FieldDescriptor, FIELD, field_descriptor)
        DEFINE_MEMBERS(OneofDescriptor, ONEOF, oneof_descriptor)
        DEFINE_MEMBERS(EnumDescriptor, ENUM, enum_descriptor)
        DEFINE_MEMBERS(ServiceDescriptor, SERVICE, service_descriptor)
        DEFINE_MEMBERS(MethodDescriptor, METHOD, method_descriptor)
        DEFINE_MEMBERS(FileDescriptor, FULL_PACKAGE, file_descriptor)

        // We use a special node for subpackage FileDescriptor.
        // It is potentially added to the table with multiple different names, so we
        // need a separate place to put the name.
        struct Subpackage : internal::SymbolBase {
            int name_size;
            const FileDescriptor* file;
        };
        DEFINE_MEMBERS(Subpackage, SUB_PACKAGE, sub_package_file_descriptor)

        static Symbol EnumValue(EnumValueDescriptor* value, int n) {
            Symbol s;
            internal::SymbolBase* ptr;
            if (n == 0) {
                ptr = static_cast<internal::SymbolBaseN<0>*>(value);
                ptr->symbol_type_ = ENUM_VALUE;
            } else {
                ptr = static_cast<internal::SymbolBaseN<1>*>(value);
                ptr->symbol_type_ = ENUM_VALUE_OTHER_PARENT;
            }
            s.ptr_ = ptr;
            return s;
        }

        const EnumValueDescriptor* enum_value_descriptor() const {
            return type() == ENUM_VALUE
                ? static_cast<const EnumValueDescriptor*>(
                      static_cast<const internal::SymbolBaseN<0>*>(ptr_))
                : type() == ENUM_VALUE_OTHER_PARENT
                ? static_cast<const EnumValueDescriptor*>(
                      static_cast<const internal::SymbolBaseN<1>*>(ptr_))
                : nullptr;
        }

        // Not a real symbol.
        // Only used for heterogeneous lookups and never actually inserted in the
        // tables.
        // TODO(b/215557658): If we templetize QueryKey on the expected object type we
        // can skip the switches for the eq function altogether.
        struct QueryKey : internal::SymbolBase {
            StringPiece name;
            const void* parent;
            int field_number;

            // Adaptor functions to look like a Symbol to the comparators.
            StringPiece full_name() const { return name; }
            std::pair<const void*, int> parent_number_key() const {
                return { parent, field_number };
            }
            std::pair<const void*, StringPiece> parent_name_key() const {
                return { parent, name };
            }
        };
        // This constructor is implicit to allow for non-transparent lookups when
        // necessary.
        // For transparent lookup cases we query directly with the object without the
        // type erasure layer.
        Symbol(QueryKey& value)
            : ptr_(&value) { // NOLINT
            value.symbol_type_ = QUERY_KEY;
        }
        const QueryKey* query_key() const {
            return type() == QUERY_KEY ? static_cast<const QueryKey*>(ptr_) : nullptr;
        }
#undef DEFINE_MEMBERS

        Type type() const {
            return static_cast<Type>(ptr_->symbol_type_);
        }
        bool IsNull() const {
            return type() == NULL_SYMBOL;
        }
        bool IsType() const {
            return type() == MESSAGE || type() == ENUM;
        }
        bool IsAggregate() const {
            return IsType() || IsPackage() || type() == SERVICE;
        }
        bool IsPackage() const {
            return type() == FULL_PACKAGE || type() == SUB_PACKAGE;
        }

        const FileDescriptor* GetFile() const {
            switch (type()) {
            case MESSAGE:
                return descriptor()->file();
            case FIELD:
                return field_descriptor()->file();
            case ONEOF:
                return oneof_descriptor()->containing_type()->file();
            case ENUM:
                return enum_descriptor()->file();
            case ENUM_VALUE:
            case ENUM_VALUE_OTHER_PARENT:
                return enum_value_descriptor()->type()->file();
            case SERVICE:
                return service_descriptor()->file();
            case METHOD:
                return method_descriptor()->service()->file();
            case FULL_PACKAGE:
                return file_descriptor();
            case SUB_PACKAGE:
                return sub_package_file_descriptor()->file;
            default:
                return nullptr;
            }
        }

        StringPiece full_name() const {
            switch (type()) {
            case MESSAGE:
                return descriptor()->full_name();
            case FIELD:
                return field_descriptor()->full_name();
            case ONEOF:
                return oneof_descriptor()->full_name();
            case ENUM:
                return enum_descriptor()->full_name();
            case ENUM_VALUE:
            case ENUM_VALUE_OTHER_PARENT:
                return enum_value_descriptor()->full_name();
            case SERVICE:
                return service_descriptor()->full_name();
            case METHOD:
                return method_descriptor()->full_name();
            case FULL_PACKAGE:
                return file_descriptor()->package();
            case SUB_PACKAGE:
                return StringPiece(sub_package_file_descriptor()->file->package())
                    .substr(0, sub_package_file_descriptor()->name_size);
            case QUERY_KEY:
                return query_key()->full_name();
            default:
                GOOGLE_CHECK(false);
            }
            return "";
        }

        std::pair<const void*, StringPiece> parent_name_key() const {
            const auto or_file = [&](const void* p) { return p ? p : GetFile(); };
            switch (type()) {
            case MESSAGE:
                return { or_file(descriptor()->containing_type()), descriptor()->name() };
            case FIELD: {
                auto* field = field_descriptor();
                return { or_file(field->is_extension() ? field->extension_scope()
                                                       : field->containing_type()),
                    field->name() };
            }
            case ONEOF:
                return { oneof_descriptor()->containing_type(),
                    oneof_descriptor()->name() };
            case ENUM:
                return { or_file(enum_descriptor()->containing_type()),
                    enum_descriptor()->name() };
            case ENUM_VALUE:
                return { enum_value_descriptor()->type(),
                    enum_value_descriptor()->name() };
            case ENUM_VALUE_OTHER_PARENT: {
                // The outer-scope alias key is "EnumName.ValueName" — i.e. the
                // value's full_name with the package (and any containing-message)
                // prefix stripped. Both full_name and the enum/value names are
                // already persistent, so the StringPiece is valid for the table's
                // hash/compare lifetime.
                const EnumValueDescriptor* v = enum_value_descriptor();
                const EnumDescriptor* e = v->type();
                const std::string& full = v->full_name();
                int len = static_cast<int>(e->name().size() + 1 + v->name().size());
                int offset = static_cast<int>(full.size()) - len;
                return { or_file(e->containing_type()),
                    StringPiece(full.data() + offset, static_cast<size_t>(len)) };
            }
            case SERVICE:
                return { GetFile(), service_descriptor()->name() };
            case METHOD:
                return { method_descriptor()->service(), method_descriptor()->name() };
            case QUERY_KEY:
                return query_key()->parent_name_key();
            default:
                GOOGLE_CHECK(false);
            }
            return { };
        }

        std::pair<const void*, int> parent_number_key() const {
            switch (type()) {
            case FIELD:
                return { field_descriptor()->containing_type(),
                    field_descriptor()->number() };
            case ENUM_VALUE:
                return { enum_value_descriptor()->type(),
                    enum_value_descriptor()->number() };
            case QUERY_KEY:
                return query_key()->parent_number_key();
            default:
                GOOGLE_CHECK(false);
            }
            return { };
        }

    private:
        const internal::SymbolBase* ptr_;
    };

} // namespace shark


#include <shark/port_undef.inc>
