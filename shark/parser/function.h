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

    class ServletParser;

    ////////////////////////////////////////////////
    /// Function declarations.
    ///
    /// Leading comments and annotations are set by the caller.
    /// The return type and parameter types are dotted names.
    /// They are not bound here.
    /// Parameters use the same form as struct fields.
    /// A trailing comma inside the parameter list is allowed.
    ///
    /// /// Add two numbers.
    /// annotation key = "add";
    /// function<K[is_integer]> K add(K a, K b);
    /// function int64 add(required int64 a @ 1);
    /// function void ping();
    ///
    ////////////////////////////////////////////////
    class FunctionParser : public SymbolNode {
    public:
        friend class RegistryPool;
        FunctionParser(const FileParser* f, RegistryPool * pool) :SymbolNode(SymbolType::FUNCTION, f, pool) {}
        ~FunctionParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        std::vector<CommentParser*>        leading_comment;
        CommentParser                     *tailing_comment{nullptr};
        std::vector<AnnotationParser*>     annotations;
        /// Type parameters or specialization arguments written after `function`.
        std::vector<TypeName>              parameters;
        TypeName                           return_type;
        std::vector<FieldParser*>          params;
        std::vector<const StructParser*>   args;
        SymbolNode*                        ret_type{nullptr};
        const StructParser*                container{nullptr};
        const ServletParser*               servlet{nullptr};

        bool bind_function();

    private:
        bool bind_named_type(TypeName& type_name, bool allow_void, SymbolNode*& resolved);
        bool skip_gap(Lexer<ExpressionLexer>& lexer);
        bool parse_parameters(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
