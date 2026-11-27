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
#include <shark/parser/const_value.h>
#include <shark/parser/lexer.h>
#include <shark/parser/type_name.h>
#include <string>
#include <string_view>
#include <vector>

namespace shark::parser {

    class EnumValueParser;
    class FieldParser;
    class SymbolNode;

    ////////////////////////////////////////////////
    /// Annotation declarations.
    ///
    /// annotation key = "value1";
    /// annotation key1 = 123;
    /// annotation key2 = 123.123;
    /// annotation key = true;
    /// annotation key2 = [123.123];
    /// annotation.cpp cppattr<CppAttr> = {level = 0, export = false};
    /// annotation.cpp ids<list<int32>> = [1, 2];
    /// annotation key2 = [123.123, "abc",456];
    /// annotation.cpp key = "value cpp";
    /// annotation.go key = "value go";
    /// annotation@ key = "file";
    /// annotation@.cpp key = "file cpp";
    /// `@` marks a file-level annotation. It is not attached to the next declaration.
    /// default annotation = annotation.cpp
    ////////////////////////////////////////////////
    class AnnotationParser : public Node {
    public:
        AnnotationParser(const FileParser* f, RegistryPool *p) :Node(SymbolType::ANNOTATION, f, p){}
        ~AnnotationParser() override = default;

        bool parse(Lexer<ExpressionLexer> &lexer);
        bool bind_typed();

        CommentParser      *leading_comment;
        CommentParser      *tailing_comment;
        bool              file_level{false};
        std::string_view classify;
        std::string_view key;
        /// Set when the key is `key<Type>`.
        bool              typed{false};
        TypeName          type_ref;
        /// The value, typed or untyped. Struct values are rewritten to ConstValueParser::Struct during bind_typed.
        ConstValueParser* object{nullptr};

    private:
        bool parse_typed_value(Lexer<ExpressionLexer>& lexer);
    };
}  // namespace shark::parser
