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
#include <shark/parser/type_name.h>
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
    /// servlet MyStraits<T> {
    ///     function int64 get(required string key @ 1);
    /// }
    ///
    /// servlet Query : shark.Read, shark.Write<int32>, {
    ///     function void ping();
    /// }
    ///
    ////////////////////////////////////////////////
    class ServletParser : public SymbolNode {
    public:
        friend class RegistryPool;
        ServletParser(const FileParser* f, RegistryPool * pool) :SymbolNode(SymbolType::SERVLET, f, pool) {}
        ~ServletParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        enum class BindStep {
            DONE,
            WAIT,
            FAIL,
        };

        BindStep bind_servlet();

        std::vector<CommentParser*>    leading_comment;
        CommentParser                  *tailing_comment{nullptr};
        std::vector<AnnotationParser*>  annotations;
        /// Declaration name, including generic arguments. `name` stays the bare symbol name.
        TypeName                        declared_name;
        std::vector<TypeName>           base_names;
        std::vector<const ServletParser*> bases;
        std::vector<FunctionParser*>    method;
        const StructParser*            container{nullptr};

    private:
        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_base_name(Lexer<ExpressionLexer>& lexer);
        bool parse_function(Lexer<ExpressionLexer>& lexer,
            std::vector<CommentParser*>& comments,
            std::vector<AnnotationParser*>& annotations);
    };
}  // namespace shark::parser
