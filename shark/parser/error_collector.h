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

#include <functional>
#include <optional>
#include <shark/common/clog.h>
#include <shark/parser/node.h>
#include <string>
#include <turbo/format/str_format.h>
#include <turbo/strings/str_cat.h>
#include <vector>

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
        case ErrorType::NONE:         return "NONE";
        case ErrorType::COMMENT:      return "COMMENT";
        case ErrorType::LEXICAL:      return "LEXICAL";
        case ErrorType::SYNTAX:       return "SYNTAX";
        case ErrorType::MISSING:      return "MISSING";
        case ErrorType::SEMANTIC:     return "SEMANTIC";
        case ErrorType::PREPROCESSOR: return "PREPROCESSOR";
        case ErrorType::IO:           return "IO";
        case ErrorType::INTERNAL:     return "INTERNAL";
        }
        return "UNKNOWN";
    }

    class ErrorCollector {
    public:
        ErrorCollector(std::string_view file) :_file(file) {}
        ~ErrorCollector() = default;

        [[nodiscard]] bool ok() const {
            return _type == ErrorType::NONE;
        }

       [[nodiscard]] ErrorType type() const {
            return _type;
        }

        [[nodiscard]] const std::string &message() const {
            return _message;
        }

        [[nodiscard]] const std::string &file() const {
            return _file;
        }

        [[nodiscard]] const SourceLocation &location() const {
            return _location;
        }

        void report_error(ErrorType type,SourceLocation location, std::string err) {
            if (_type == ErrorType::NONE) {
                return;
            }
            _type = type;
            _location = location;
            _message = err;
        }
    private:
        std::string _file;
        ErrorType   _type{ErrorType::NONE};
        SourceLocation _location;
        std::string    _message;
    };

}