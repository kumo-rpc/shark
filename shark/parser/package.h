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

namespace shark::parser {

    ////////////////////////////////////////////////
    /// Package declarations.
    ///
    /// package "shark.compiler.idl";
    /// package 'shark.compiler.idl';
    /// package.java "idl.compiler.shark";
    /// package.go  'github.com/example/package';
    /// package.cpp 'shark/compiler/idl.h';
    ///
    /// The default package is equivalent to `package.cpp`.
    ///
    /// An optional extension identifier can be specified for
    /// language-specific package handling. The identifier is
    /// reserved for downstream code generation and is not
    /// interpreted by the parser.
    ///
    /// package[.identify] 'shark.compiler.idl';
    ////////////////////////////////////////////////
    class PackageParser : public Node {
    public:
        PackageParser(const FileParser* f, RegistryPool *p) :Node(SymbolType::PACKAGE, f,p){}
        ~PackageParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);

        CommentParser      *leading_comment{nullptr};
        CommentParser      *tailing_comment{nullptr};
        std::string_view  classify;
        std::string_view  package;
    };
}  // namespace shark::parser
