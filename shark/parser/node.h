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

#include <cstdint>
#include <shark/common/clog.h>
#include <shark/parser/fwd.h>
#include <string>
#include <turbo/format/str_format.h>
#include <turbo/strings/str_cat.h>

namespace shark::parser {

    /// Categorizes errors encountered during lexing, preprocessing, and parsing.
    enum class ErrorType : std::uint8_t {
        /// No error occurred.
        NONE = 0,

        /// Unterminated block comment or malformed comment syntax.
        COMMENT,

        /// Lexing error (e.g., invalid character, unterminated string/char literal).
        LEXICAL,

        /// Grammar or structural syntax error.
        SYNTAX,

        /// Missing expected token, delimiter, file, or required field.
        MISSING,

        /// Semantic or type mismatch error (e.g., duplicate definition, invalid value).
        SEMANTIC,

        /// Preprocessor directive or macro expansion error (e.g., cyclic include).
        PREPROCESSOR,

        /// File system or I/O error (e.g., include file not found, permission denied).
        IO,

        /// Unexpected internal parser state or assertion failure.
        INTERNAL,
    };

    /// Returns a human-readable string representation of `type`.
    [[nodiscard]] constexpr std::string_view to_string(ErrorType type) noexcept {
        switch (type) {
        case ErrorType::NONE:
            return "NONE";
        case ErrorType::COMMENT:
            return "COMMENT";
        case ErrorType::LEXICAL:
            return "LEXICAL";
        case ErrorType::SYNTAX:
            return "SYNTAX";
        case ErrorType::MISSING:
            return "MISSING";
        case ErrorType::SEMANTIC:
            return "SEMANTIC";
        case ErrorType::PREPROCESSOR:
            return "PREPROCESSOR";
        case ErrorType::IO:
            return "IO";
        case ErrorType::INTERNAL:
            return "INTERNAL";
        }
        return "UNKNOWN";
    }

    class Node {
    public:
        virtual ~Node() = default;

        [[nodiscard]] bool is_symbol() const {
            static constexpr auto kMin = static_cast<int32_t>(SymbolType::SYMBOL_MIN);
            static constexpr auto kMax = static_cast<int32_t>(SymbolType::SYMBOL_MIN);
            auto n = static_cast<int32_t>(node_type);
            return n >= kMin && n <= kMax;
        }

        template <typename... Args>
        void report_debug(ErrorType err, const turbo::FormatSpec<Args...>& format,
            const Args&... args) {
            shark::common::report_debug(turbo::str_cat(make_node_info(err), " debug: ", turbo::str_sprintf(format, args...)));
        }
        template <typename... Args>
        void report_info(ErrorType err, const turbo::FormatSpec<Args...>& format,
            const Args&... args) {
            shark::common::report_info(turbo::str_cat(make_node_info(err), " info: ", turbo::str_sprintf(format, args...)));
        }

        template <typename... Args>
        void report_warn(ErrorType err, const turbo::FormatSpec<Args...>& format,
            const Args&... args) {
            shark::common::report_warn(turbo::str_cat(make_node_info(err), " warn: ", turbo::str_sprintf(format, args...)));
        }

        template <typename... Args>
        void report_error(ErrorType err, const turbo::FormatSpec<Args...>& format,
            const Args&... args) {
            shark::common::report_error(turbo::str_cat(make_node_info(err), " error: ", turbo::str_sprintf(format, args...)));
        }

       [[nodiscard]] virtual  std::string_view get_absolute_path() const;
       [[nodiscard]] virtual  std::string_view get_path() const;

        /// only hold store, not enum but,
        /// leave it to user.
        /// 0 mean no type, reserved.
        SymbolType node_type { SymbolType::NONE };
        SourceLocation start_location;
        const FileParser* file { nullptr };
        RegistryPool* pool { nullptr };

    protected:
        std::string make_node_info(ErrorType err) const;

    protected:
        Node(SymbolType type, const FileParser* f, RegistryPool* p)
            : node_type(type)
            , file(f)
            , pool(p) {
        }
    };

} // namespace shark::parser
