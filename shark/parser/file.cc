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

#include <shark/parser/constants.h>
#include <shark/parser/enum.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/servlet.h>
#include <shark/parser/struct.h>

#include <turbo/log/logging.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace shark::parser {

    bool FileNode::load(std::string_view absolute_path_in, std::string_view path_in) {
        KLOG(INFO)<<1<<222;
        if (absolute_path_in.empty() || path_in.empty()) {
            report_error(ErrorType::IO, "path and absolute_path must both be non-empty");
            return false;
        }

        absolute_path.assign(absolute_path_in.data(), absolute_path_in.size());
        path.assign(path_in.data(), path_in.size());

        std::error_code ec;
        if (!std::filesystem::is_regular_file(absolute_path, ec)) {
            report_error(ErrorType::IO, "file `%s` does not exist", absolute_path);
            return false;
        }

        std::ifstream in(absolute_path, std::ios::in | std::ios::binary);
        if (!in) {
            report_error(ErrorType::IO, "failed to open `%s`", absolute_path);
            return false;
        }

        const std::istreambuf_iterator<char> begin(in);
        const std::istreambuf_iterator<char> end;
        std::string data(begin, end);
        if (in.bad()) {
            report_error(ErrorType::IO, "failed to read `%s`", absolute_path);
            return false;
        }

        lexer.reset();
        file_data = std::move(data);
        lexer = std::make_unique<Lexer<ExpressionLexer>>(file_data);
        return true;
    }

    /// The file must start with edition. Package is a top-level element.
    bool FileNode::parse_header() {
        if (!lexer) {
            report_error(ErrorType::INTERNAL, "lexer is null");
            return false;
        }

        lexer->consume_white_space(true);
        edition = pool->create_node<Edition>(this);
        auto* edition_leading = pool->create_node<CommentNode>(this);
        if (!edition_leading->parse_leading(*lexer)) {
            return false;
        }
        if (!edition_leading->empty()) {
            edition->leading_comment = edition_leading;
        }
        return edition->parse(*lexer);
    }

    bool FileNode::parse_leading(std::vector<CommentNode*>& comments, std::vector<AnnotationNode*>& annotations) {
        while (true) {
            auto* comment = pool->create_node<CommentNode>(this);
            if (!comment->parse_leading(*lexer)) {
                return false;
            }
            if (!comment->empty()) {
                comments.push_back(comment);
            }

            auto t = lexer->look_at();
            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
            if (t.type == TokenType::EndOfStream) {
                return true;
            }
            if (t.type == TokenType::BareWord && t == "annotation") {
                auto* annotation = pool->create_node<AnnotationNode>(this);
                if (!annotation->parse(*lexer)) {
                    return false;
                }
                if (annotation->file_level) {
                    this->annotations.push_back(annotation);
                } else {
                    annotations.push_back(annotation);
                }
                continue;
            }
            return true;
        }
    }

    bool FileNode::parse() {
        if (!parse_header()) {
            return false;
        }

        while (true) {
            lexer->consume_white_space(true);
            std::vector<CommentNode*> comments;
            std::vector<AnnotationNode*> annotations;
            if (!parse_leading(comments, annotations)) {
                return false;
            }

            auto t = lexer->look_at();
            if (t.type == TokenType::EndOfStream) {
                return true;
            }
            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
            if (t.type == TokenType::BareWord && t == "package") {
                auto* package = pool->create_node<PackageNode>(this);
                if (!comments.empty()) {
                    package->leading_comment = comments.front();
                }
                if (!package->parse(*lexer)) {
                    return false;
                }
                packages.push_back(package);
                continue;
            }
            if (t.type == TokenType::BareWord && t == "enum") {
                auto* enum_node = pool->create_symbol<EnumNode>(this);
                enum_node->leading_comment = std::move(comments);
                enum_node->annotations = std::move(annotations);
                if (!enum_node->parse(*lexer)) {
                    return false;
                }
                enums.push_back(enum_node);
                continue;
            }
            if (t.type == TokenType::BareWord && (t == kStructKey || t == kUnionKey)) {
                auto* struct_node = pool->create_symbol<StructNode>(this);
                struct_node->leading_comment = std::move(comments);
                struct_node->annotations = std::move(annotations);
                if (!struct_node->parse(*lexer)) {
                    return false;
                }
                structs.push_back(struct_node);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kConstExprKey) {
                auto* constant = pool->create_symbol<FieldNode>(this);
                constant->leading_comment = std::move(comments);
                constant->annotations = std::move(annotations);
                if (!constant->parse(*lexer)) {
                    return false;
                }
                constexprs.push_back(constant);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kFunctionKey) {
                auto* function = pool->create_symbol<FunctionNode>(this);
                function->leading_comment = std::move(comments);
                function->annotations = std::move(annotations);
                if (!function->parse(*lexer)) {
                    return false;
                }
                functions.push_back(function);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kServletKey) {
                auto* servlet = pool->create_symbol<ServletNode>(this);
                servlet->leading_comment = std::move(comments);
                servlet->annotations = std::move(annotations);
                if (!servlet->parse(*lexer)) {
                    return false;
                }
                servlets.push_back(servlet);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kImportKey) {
                auto* import_node = pool->create_node<ImportNode>(this);
                if (!comments.empty()) {
                    import_node->leading_comment = comments.front();
                }
                import_node->annotations = std::move(annotations);
                if (!import_node->parse(*lexer)) {
                    return false;
                }
                imports.push_back(import_node);
                continue;
            }

            report_error(ErrorType::SYNTAX, "expected `package`, `enum`, `struct`, `union`, `servlet`, `function`, `constexpr` or `import`, but got `%s`", t.view());
            return false;
        }
    }
}  // namespace shark::parser

