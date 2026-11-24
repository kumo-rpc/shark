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
    /// Servlet declarations.
    ///
    /// A servlet is a business unit. It groups functions that process data.
    /// It does not describe a transport protocol.
    /// Leading comments and annotations are set by the caller.
    /// Bases after `:` are dotted names and are not bound here.
    /// A trailing comma in the base list is allowed.
    ///
    /// servlet Read {
    ///     function int64 get(required string key @ 1);
    /// }
    ///
    /// /// Query service.
    /// annotation key = "query";
    /// servlet Query : shark.Read, shark.Write, {
    ///     function void ping();
    /// }
    ///
    ////////////////////////////////////////////////
    class ServletNode : public SymbolNode {
    public:
        friend class RegistryPool;
        ServletNode(const FileNode* f, RegistryPool * pool) :SymbolNode(NodeType::SERVLET, f, pool) {}
        ~ServletNode() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        std::vector<CommentNode*>    leading_comment;
        CommentNode                  *tailing_comment{nullptr};
        std::vector<AnnotationNode*>  annotations;
        std::vector<std::string_view> base_names;
        std::vector<const ServletNode*> bases;
        std::vector<FunctionNode*>    method;
        const StructNode*            container{nullptr};

    private:
        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_base_name(Lexer<ExpressionLexer>& lexer);
        bool parse_function(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentNode*>& comments,
            std::vector<AnnotationNode*>& annotations);
    };
}  // namespace shark::parser
