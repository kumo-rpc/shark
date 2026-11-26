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
#include <shark/parser/lexer.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    ////////////////////////////////////////////////
    /// Import declarations.
    ///
    /// import "shark/compiler/idl.shark";
    /// import  'shark/compiler/idl.shark';
    /// import.java "shark/compiler/idl.shark";
    /// import.go  'github.com/example/package';
    /// import.cpp 'shark/compiler/idl.h';
    ///
    /// Optional extension identifier for language generators.
    /// The identifier is reserved for downstream code generation
    /// and is not interpreted by the parser.
    ///
    /// import[.identify] 'shark/compiler/idl.h';
    ///
    class ImportParser : public Node {
    public:
        ImportParser(const FileParser *f, RegistryPool *p) : Node(SymbolType::IMPORT, f, p){}
        ~ImportParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        CommentParser      *leading_comment{nullptr};
        CommentParser      *tailing_comment{nullptr};
        std::vector<AnnotationParser*> annotations;
        std::string_view  classify;
        std::string_view  path;


    };
}  // namespace shark::parser
