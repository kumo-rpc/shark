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


#include <shark/parser/annotation.h>
#include <shark/parser/comment.h>
#include <shark/parser/const_value.h>
#include <shark/parser/constants.h>
#include <shark/parser/lexer.h>
#include <shark/parser/symbol.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    // required: the field is present and serialized.
    // optional: the field may be absent.
    // atomic: same presence as required, and the generated field is atomic.
    // constexpr: a public compile-time constant. It is not serialized.
    // These four qualifiers do not combine. `static` is not a field qualifier.
    class FieldNode : public SymbolNode {
    public:
        friend class RegistryPool;
        FieldNode(const FileNode* f, RegistryPool * pool) :SymbolNode(NodeType::FIELD, f, pool) {}
        ~FieldNode() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        std::vector<CommentNode*>      leading_comment;
        CommentNode                   *tailing_comment{nullptr};
        std::vector<AnnotationNode*>   annotations;
        FieldLabel                     label{FieldLabel::REQUIRED};
        std::string_view               type_name;
        const StructNode*              type{nullptr};
        /// Set by the caller before parse. It is already fixed while parsing.
        /// nullptr means the field is declared at file scope, not inside a struct.
        const StructNode*              container{nullptr};
        uint16_t                       id{0};
        ConstValue*                    default_value{nullptr};

    private:
        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_type_name(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
