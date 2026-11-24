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


#include <shark/parser/comment.h>
#include <shark/parser/const_value.h>
#include <shark/parser/lexer.h>
#include <shark/parser/type_name.h>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <variant>

namespace shark::parser {

    class EnumValueNode;
    class FieldNode;
    class SymbolNode;

    enum class AnnotationKind {
        SCALAR,
        LIST,
        MAP,
        STRUCT,
        ENUM,
    };

    // Filled for `key<Type>` after binding. Accessors return null when `kind` does not match.
    struct AnnotationValue {
        struct Entry;
        struct Field;

        AnnotationValue() = default;
        AnnotationValue(const AnnotationValue&) = default;
        AnnotationValue& operator=(const AnnotationValue&) = default;
        AnnotationValue(AnnotationValue&&) noexcept;
        AnnotationValue& operator=(AnnotationValue&&) noexcept;
        ~AnnotationValue();

        AnnotationKind kind() const {
            return kind_;
        }
        const SymbolNode* type() const {
            return type_;
        }
        const bool* boolean() const {
            if (kind_ != AnnotationKind::SCALAR || literal_ == nullptr) {
                return nullptr;
            }
            return std::get_if<bool>(&literal_->value);
        }
        const int64_t* integer() const {
            if ((kind_ != AnnotationKind::SCALAR && kind_ != AnnotationKind::ENUM) || literal_ == nullptr) {
                return nullptr;
            }
            return std::get_if<int64_t>(&literal_->value);
        }
        const double* floating() const {
            if (kind_ != AnnotationKind::SCALAR || literal_ == nullptr) {
                return nullptr;
            }
            return std::get_if<double>(&literal_->value);
        }
        const std::string_view* string() const {
            if (kind_ != AnnotationKind::SCALAR || literal_ == nullptr) {
                return nullptr;
            }
            return std::get_if<std::string_view>(&literal_->value);
        }
        const EnumValueNode* enumerator() const {
            if (kind_ != AnnotationKind::ENUM) {
                return nullptr;
            }
            return enumerator_;
        }
        const std::vector<AnnotationValue>* elements() const {
            if (kind_ != AnnotationKind::LIST) {
                return nullptr;
            }
            return &elements_;
        }
        const std::vector<Entry>* entries() const {
            if (kind_ != AnnotationKind::MAP) {
                return nullptr;
            }
            return &entries_;
        }
        const std::vector<Field>* fields() const {
            if (kind_ != AnnotationKind::STRUCT) {
                return nullptr;
            }
            return &fields_;
        }
        const AnnotationValue* field(std::string_view name) const;

        AnnotationKind                   kind_{AnnotationKind::SCALAR};
        const SymbolNode*                type_{nullptr};
        const ConstValue*                literal_{nullptr};
        const EnumValueNode*             enumerator_{nullptr};
        std::vector<AnnotationValue>     elements_;
        std::vector<Entry>               entries_;
        std::vector<Field>               fields_;
    };

    struct AnnotationValue::Entry {
        AnnotationValue key;
        AnnotationValue value;
    };

    struct AnnotationValue::Field {
        const FieldNode* field{nullptr};
        AnnotationValue  value;
    };

    ////////////////////////////////////////////////
    /// Annotation declarations.
    ///
    /// annotation key = "value1";
    /// annotation key1 = 123;
    /// annotation key2 = 123.123;
    /// annotation key = true;
    /// annotation key2 = [123.123];
    /// annotation.cpp cppattr<CppAttr> = {level = 0, export = false};
    /// annotation.cpp ids<list<int32>> = [1, 2];
    /// annotation key2 = [123.123, "abc",456];
    /// annotation.cpp key = "value cpp";
    /// annotation.go key = "value go";
    /// annotation@ key = "file";
    /// annotation@.cpp key = "file cpp";
    /// `@` marks a file-level annotation. It is not attached to the next declaration.
    /// default annotation = annotation.cpp
    ////////////////////////////////////////////////
    class AnnotationNode : public Node {
    public:
        AnnotationNode(const FileNode* f, RegistryPool *p) :Node(NodeType::ANNOTATION, f, p){}
        ~AnnotationNode() override = default;

        using value_type = std::variant<
            std::string_view,
            int64_t,
            float,
            bool
            >;

        bool parse(Lexer<ExpressionLexer> &lexer);
        bool bind_typed();

        CommentNode      *leading_comment;
        CommentNode      *tailing_comment;
        bool              file_level{false};
        std::string_view classify;
        std::string_view key;
        std::vector<value_type>       payload;
        /// Set when the key is `key<Type>`. The value is `object`, a ConstValue,
        /// the same node a field default uses.
        bool              typed{false};
        TypeRef           type_ref;
        ConstValue*       object{nullptr};
        /// Valid when `typed` is true and `bind_typed` succeeded.
        AnnotationValue   value;

    private:
        bool parse_one(Lexer<ExpressionLexer> &lexer);
        bool parse_typed_value(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
