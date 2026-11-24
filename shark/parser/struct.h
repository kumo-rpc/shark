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
#include <shark/parser/enum_value.h>
#include <shark/parser/annotation.h>
#include <shark/parser/lexer.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    ////////////////////////////////////////////////
    /// Enum declarations.
    ///
    /// enum Color {
    ///     RED = 0;
    ///     GREEN = 1;
    ///     BLUE = 2;
    /// }
    ///
    /// Enum declarations can use the `class` keyword.
    /// The default underlying type is `int32`.
    ///
    /// enum class Color {
    ///     RED = 0;
    ///     GREEN = 1;
    /// }
    ///
    /// An explicit underlying integer type can be specified
    /// for enum class declarations.
    ///
    /// enum class Color : uint16 {
    ///     RED = 0;
    ///     GREEN = 1;
    /// }
    ///
    /// Supported underlying types:
    ///     int8
    ///     uint8
    ///     int16
    ///     uint16
    ///     int32
    ///     uint32
    ///
    /// Enum declarations support leading comments and annotations.
    /// Comments and annotations can appear before the enum declaration.
    ///
    /// /// Enum comment.
    /// annotation key = "value";
    /// enum class Color {
    ///     RED = 0;
    /// }
    ///
    /// Enum values also support leading comments, annotations,
    /// and trailing comments.
    ///
    /// enum Color {
    ///     /// Value comment.
    ///     annotation key = "value";
    ///     RED = 0;
    ///
    ///     GREEN = 1;  ///! Value trailing comment.
    /// }
    ///
    /// The closing brace can optionally be followed by a semicolon.
    ///
    ////////////////////////////////////////////////
    class StructNode : public SymbolNode {
    public:
        friend class RegistryPool;
        StructNode(const FileNode* f, RegistryPool * pool) :SymbolNode(NodeType::STRUCT, f,pool) {}
        ~StructNode() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        enum class BindStep {
            DONE,
            WAIT,
            FAIL,
        };

        BindStep bind_struct();

        bool is_container_type() const {
            return phy_type >= PhysicalType::CONTAINER_MIN &&
                   phy_type <= PhysicalType::CONTAINER_MAX;
        }

        std::vector<CommentNode*>     leading_comment;
        CommentNode                  *tailing_comment{nullptr};
        std::vector<AnnotationNode*>  annotations;
        std::vector<FieldNode*>       fields;
        std::vector<StructNode*>      nested_types;
        std::vector<EnumNode*>        nested_enums;
        std::vector<AliasNode*>       aliases;
        const StructNode*             container{nullptr};
        bool                          is_union{false};
        bool                          builtin{false};
        bool                          generic{false};
        PhysicalType                  phy_type{PhysicalType::VOID};

    private:
        bool parse_field(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
        bool parse_nested_enum(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
        bool parse_nested_struct(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
        bool parse_nested_union(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
        bool parse_alias(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
    };
}  // namespace shark::parser
