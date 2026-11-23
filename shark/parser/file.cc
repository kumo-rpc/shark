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

#include <shark/parser/file.h>
#include <shark/parser/pool.h>

#include <turbo/strings/match.h>
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

        const bool prefixed = absolute_path_in.size() > path_in.size()
            && turbo::ends_with(absolute_path_in, path_in)
            && absolute_path_in[absolute_path_in.size() - path_in.size() - 1] == '/';
        if (!prefixed) {
            report_error(
                ErrorType::IO,
                "absolute_path `%s` is not of the form prefix/path for `%s`",
                absolute_path_in,
                path_in);
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

    /// 1. lexer not all
    /// 2. parse edition
    /// 3. parse package
    /// return true
    bool FileNode::parse() {
        KLOG(INFO)<<1;
        if (!lexer) {
            report_error(ErrorType::INTERNAL, "lexer is null");
            return false;
        }

        lexer->consume_white_space(true);
        edition = pool->create_node<Edition>(this);
        if (!edition->parse(*lexer)) {
            return false;
        }

        lexer->consume_white_space(true);
        auto* package = pool->create_node<PackageNode>(this);
        KLOG(INFO)<<1;
        if (!package->parse(*lexer)) {
            return false;
        }
        packages.push_back(package);
        return true;
    }
}  // namespace shark::parser

