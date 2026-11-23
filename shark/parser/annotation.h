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
#include <shark/parser/lexer.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <variant>

namespace shark::parser {

    ////////////////////////////////////////////////
    /// Annotation declarations.
    ///
    /// annotation key = "value1";
    /// annotation key1 = 123;
    /// annotation key2 = 123.123;
    /// annotation key2 = [123.123];
    /// annotation key2 = [123.123, "abc",456];
    /// annotation.cpp key = "value cpp";
    /// annotation.go key = "value go";
    /// default annotation = annotation.cpp
    ////////////////////////////////////////////////
    class AnnotationNode : public Node {
    public:
        AnnotationNode(const FileNode* f, RegistryPool *p) :Node(NodeType::ANNOTATION, f, p){}
        ~AnnotationNode() override = default;

        using value_type = std::variant<
            std::string_view,
            int64_t,
            float
            >;

        bool parse(Lexer<ExpressionLexer> &lexer);

        CommentNode      *leading_comment;
        CommentNode      *tailing_comment;
        std::string_view classify;
        std::string_view key;
        std::vector<value_type>       payload;

    private:
        bool parse_one(Lexer<ExpressionLexer> &lexer);
    };
}  // namespace shark::parser
