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
#include <shark/parser/type_name.h>
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
    class FieldParser : public SymbolNode {
    public:
        friend class RegistryPool;
        FieldParser(const FileParser* f, RegistryPool * pool) :SymbolNode(NodeType::FIELD, f, pool) {}
        ~FieldParser() override = default;

        enum class End {
            STATEMENT,
            PARAMETER,
        };

        // STATEMENT consumes `;`. PARAMETER stops at `,` or `)` and leaves that token.
        bool parse(Lexer<ExpressionLexer> &lexer, End end = End::STATEMENT);

        std::vector<CommentParser*>      leading_comment;
        CommentParser                   *tailing_comment{nullptr};
        std::vector<AnnotationParser*>   annotations;
        FieldLabel                     label{FieldLabel::REQUIRED};
        TypeRef                        type_ref;
        bool                           generic{false};
        const StructParser*              type{nullptr};
        const EnumParser*                enum_type{nullptr};
        const PlaceholderParser*         placeholder{nullptr};
        /// Set by the caller before parse. It is already fixed while parsing.
        /// nullptr means the field is declared at file scope, not inside a struct.
        const StructParser*              container{nullptr};
        uint16_t                       id{0};
        ConstValueParser*                    default_value{nullptr};
        /// True when a constexpr struct, union, or container is kept but is not a
        /// compile-time constant. Codegen should emit static const.
        bool                           static_const{false};

    private:
        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_type_name(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
