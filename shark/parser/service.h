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
    class ServiceNode : public SymbolNode {
    public:
        friend class RegistryPool;
        ServiceNode(const FileNode* f, RegistryPool * pool) :SymbolNode(NodeType::STRUCT, f, pool) {}
        ~ServiceNode() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        std::vector<CommentNode*>    leading_comment;
        CommentNode                  *tailing_comment;
        std::vector<AnnotationNode*>  annotations;
        std::vector<FunctionNode*>   method;
        const StructNode*            container{nullptr};


    };
}  // namespace shark::parser
