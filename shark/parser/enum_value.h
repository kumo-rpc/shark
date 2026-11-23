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
#include <shark/parser/symbol.h>
#include <shark/parser/lexer.h>
#include <shark/parser/annotation.h>
#include <shark/parser/fwd.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    ////////////////////////////////////////////////
    /// enum value declarations.
    /// enum abc {
    ///     annotation key1 = "value1";
    ///     annotation key2 = "value2";
    ///     annotation.go key2 = "value2";
    ///     /// leading comment
    ///     ENUM_VALUE0 = 0;
    ///     /// leading comment
    ///     annotation key1 = "value1";
    ///     annotation key2 = "value2";
    ///     annotation.go key2 = "value2";
    ///     ENUM_VALUE1 = 5;  ///! tailing comment
    /// }[;]
    ////////////////////////////////////////////////
    class EnumValueNode : public SymbolNode {
    public:
        ~EnumValueNode() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        std::vector<CommentNode*>      leading_comment;
        CommentNode                   *tailing_comment;
        int64_t                       value{0};
        std::vector<AnnotationNode*>  annotations;
        const EnumNode *parent_enum{nullptr};

    private:
        friend class RegistryPool;
        EnumValueNode(const FileNode* f, RegistryPool *pool) :SymbolNode(NodeType::ENUM_VALUE, f, pool) {}
        bool parse_leading(Lexer<ExpressionLexer> &lexer);

        bool parse_value(Lexer<ExpressionLexer> &lexer);
    };
}  // namespace shark::parser
