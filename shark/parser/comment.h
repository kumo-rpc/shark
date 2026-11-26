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

#include <shark/parser/node.h>
#include <shark/parser/lexer.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    class CommentParser : public Node {
    public:
        CommentParser(const FileParser*f,  RegistryPool *p) :Node(NodeType::COMMENT, f, p){}
        ~CommentParser() override = default;

        static bool is_tailing_prefix(std::string_view comment);

        bool parse_tailing(Lexer<ExpressionLexer> &lexer, const std::function<bool(std::string_view)> & func =CommentParser::is_tailing_prefix);

        bool parse_leading(Lexer<ExpressionLexer> &lexer,
            const std::function<bool(std::string_view)> & func =CommentParser::is_tailing_prefix);

        [[nodiscard]] bool empty() const {
            return content.empty();
        }

        std::vector<Token>   content;
        bool                 is_tailing{false};
    protected:
    };
}  // namespace shark::parser
