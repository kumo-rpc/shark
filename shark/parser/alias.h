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
#include <string_view>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    ////////////////////////////////////////////////
    /// Alias declarations. The node is a symbol. `name` is the alias.
    ///
    /// `using` names the alias first. `typedef` names the target symbol first.
    /// The target is a dotted name and is not bound here.
    /// Leading comments and annotations are set by the caller.
    ///
    /// using Id = int64;
    /// using PointId = shark.Point;
    ///
    /// /// File id.
    /// annotation key = "id";
    /// typedef int64 Id;
    ///
    ////////////////////////////////////////////////
    class AliasParser : public SymbolNode {
    public:
        ~AliasParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);
        bool bind_alias();

        std::vector<CommentParser*>     leading_comment;
        CommentParser                  *tailing_comment{nullptr};
        std::vector<AnnotationParser*>  annotations;
        std::string_view              target_name;
        const SymbolNode*             target{nullptr};
        const StructParser*             container{nullptr};
        bool                          is_using{false};

    private:
        friend class RegistryPool;
        AliasParser(const FileParser* f, RegistryPool * pool) :SymbolNode(SymbolType::ALIAS, f, pool) {}

        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_target_name(Lexer<ExpressionLexer>& lexer);
        bool finish(Lexer<ExpressionLexer>& lexer);
        bool parse_using(Lexer<ExpressionLexer>& lexer);
        bool parse_typedef(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
