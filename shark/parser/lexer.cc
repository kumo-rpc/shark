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

#include <shark/parser/lexer.h>
#include <array>
#include <turbo/strings/ascii.h>
#include <turbo/strings/find_symbols.h>
#include <turbo/strings/strip.h>
#include <turbo/unicode/api/utf8.h>

namespace shark::parser {

    inline bool is_number_separator(bool is_start_of_block, bool is_hex, const char* pos, const char* end) {
        if (*pos != '_')
            return false;
        if (is_start_of_block && *pos == '_')
            return false; // e.g. _123, 12e_3
        if (pos + 1 < end && !(is_hex ? isxdigit(pos[1]) : isdigit(pos[1])))
            return false; // e.g. 1__2, 1_., 1_e, 1_p, 1_;
        if (pos + 1 == end)
            return false; // e.g. 12_
        return true;
    }


    const char* get_token_name(TokenType type) {

        static const std::array<const char*, static_cast<size_t>(TokenType::KMaxTokenType)> kNames = {
            "Whitespace",
            "Comment",
            "BareWord",
            "Number",
            "StringLiteral",
            "QuotedIdentifier",
            "OpeningRoundBracket",
            "ClosingRoundBracket",
            "OpeningSquareBracket",
            "ClosingSquareBracket",
            "OpeningCurlyBrace",
            "ClosingCurlyBrace",
            "Comma",
            "Semicolon",
            "VerticalDelimiter",
            "Dot",
            "Asterisk",
            "HereDoc",
            "DollarSign",
            "Plus",
            "Minus",
            "Slash",
            "Percent",
            "Arrow",
            "QuestionMark",
            "Colon",
            "Caret",
            "DoubleColon",
            "Equals",
            "NotEquals",
            "Less",
            "Greater",
            "LessOrEquals",
            "GreaterOrEquals",
            "Spaceship",
            "PipeMark",
            "PipeOperator",
            "Concatenation",
            "At",
            "DoubleAt",
            "EndOfStream",
            "Error",
            "ErrorMultilineCommentIsNotClosed",
            "ErrorSingleQuoteIsNotClosed",
            "ErrorDoubleQuoteIsNotClosed",
            "ErrorBackQuoteIsNotClosed",
            "ErrorSingleExclamationMark",
            "ErrorSinglePipeMark",
            "ErrorWrongNumber",
            "ErrorMaxQuerySizeExceeded",
        };
        auto idx = static_cast<size_t>(type);
        if (kNames.size() > idx) {
            return kNames[idx];
        }
        return "InvalidToken";
    }

    const char* get_error_token_description(TokenType type) {
        switch (type) {
        case TokenType::Error:
            return "Unrecognized token";
        case TokenType::ErrorMultilineCommentIsNotClosed:
            return "Multiline comment is not closed";
        case TokenType::ErrorSingleQuoteIsNotClosed:
            return "Single quoted string is not closed";
        case TokenType::ErrorDoubleQuoteIsNotClosed:
            return "Double quoted string is not closed";
        case TokenType::ErrorBackQuoteIsNotClosed:
            return "Back quoted string is not closed";
        case TokenType::ErrorSingleExclamationMark:
            return "Exclamation mark can only occur in != operator";
        case TokenType::ErrorSinglePipeMark:
            return "Pipe symbol could only occur in || operator";
        case TokenType::ErrorWrongNumber:
            return "Wrong number";
        case TokenType::ErrorMaxQuerySizeExceeded:
            return "Max query size exceeded (can be increased with the `max_query_size` setting)";
        default:
            return "Not an error";
        }
    }


        namespace {

        /// This must be consistent with functions in ReadHelpers.h
        template <char quote>
        Token quoted_string(const char*& pos, const char* const token_begin, const char* const end,
            TokenType success_token, TokenType error_token) {
            ++pos;
            while (true) {
                pos = turbo::find_first_symbols<quote, '\\'>(pos, end);
                if (pos >= end)
                    return Token(error_token, token_begin, end);

                if (*pos == quote) {
                    ++pos;
                    if (pos < end && *pos == quote) {
                        ++pos;
                        continue;
                    }
                    return Token(success_token, token_begin, pos);
                }

                if (*pos == '\\') {
                    ++pos;
                    if (pos >= end)
                        return Token(error_token, token_begin, end);
                    ++pos;
                    continue;
                }

                KUMO_UNREACHABLE();
            }
        }

        Token quoted_string_with_unicode_quotes(const char*& pos, const char* const token_begin, const char* const end,
            char expected_end_byte, TokenType success_token, TokenType error_token) {
            /// ‘: e2 80 98
            /// ’: e2 80 99
            /// “: e2 80 9c
            /// ”: e2 80 9d

            while (true) {
                pos = turbo::find_first_symbols<'\xE2'>(pos, end);
                if (pos + 2 >= end)
                    return Token(error_token, token_begin, end);

                if (pos[0] == '\xE2' && pos[1] == '\x80' && pos[2] == expected_end_byte) {
                    pos += 3;
                    return Token(success_token, token_begin, pos);
                }

                ++pos;
            }
        }

        Token quoted_hex_or_bin_string(const char*& pos, const char* const token_begin, const char* const end) {
            constexpr char quote = '\'';

            KUMO_ASSERT(pos[1] == quote);

            bool hex = (*pos == 'x' || *pos == 'X');

            pos += 2;

            if (hex) {
                while (pos < end && turbo::ascii_isxdigit(*pos))
                    ++pos;
            } else {
                pos = turbo::find_first_not_symbols<'0', '1'>(pos, end);
            }

            if (pos >= end || *pos != quote) {
                pos = end;
                return Token(TokenType::ErrorSingleQuoteIsNotClosed, token_begin, end);
            }

            ++pos;
            return Token(TokenType::StringLiteral, token_begin, pos);
        }

    }
    
    void ExpressionLexer::stamp(Token& token) {
        auto advance = [&](const char* until) {
            while (_loc < until) {
                if (*_loc == '\n') {
                    ++_line;
                    _column = 1;
                } else {
                    ++_column;
                }
                ++_loc;
            }
        };
        advance(token.begin);
        token.line = _line;
        token.column = _column;
        advance(token.end);
    }

    const Token& ExpressionLexer::look_at() {
        if (!_current)
            _current = next_token();
        return *_current;
    }

    bool ExpressionLexer::look_at(std::string_view tok) {
        const Token& cur = look_at();
        if (cur.is_end() || cur.is_error())
            return false;
        return cur.view() == tok;
    }

    void ExpressionLexer::consume() {
        if (_current && _current->is_end())
            return;
        _current = next_token();
    }

    bool ExpressionLexer::consume(std::string_view tok) {
        if (!look_at(tok))
            return false;
        consume();
        return true;
    }

    bool ExpressionLexer::consume(TokenType type) {
        if (look_at().type != type)
            return false;
        if (type == TokenType::EndOfStream)
            return true;
        consume();
        return true;
    }

    void ExpressionLexer::consume_white_space(bool all) {
        if (look_at().type != TokenType::Whitespace)
            return;
        consume();
        if (!all)
            return;
        while (look_at().type == TokenType::Whitespace)
            consume();
    }

    int ExpressionLexer::line() {
        return look_at().line;
    }

    int ExpressionLexer::column() {
        return look_at().column;
    }

    Token ExpressionLexer::next_token() {
        Token res = next_token_impl();
        if (_begin && _max_query_size && !res.is_end()
            && res.end > _begin + _max_query_size)
            res.type = TokenType::ErrorMaxQuerySizeExceeded;
        stamp(res);
        if (res.is_significant())
            prev_significant_token_type = res.type;
        if (!(res.is_end() && _counted_end)) {
            ++_token_index;
            if (res.is_end())
                _counted_end = true;
        }
        return res;
    }


    Token ExpressionLexer::next_token_impl() {
        if (_pos >= _end)
            return Token(TokenType::EndOfStream, _end, _end);

        const char* const token_begin = _pos;

        auto comment_until_end_of_line = [&]() mutable {
            /// This means that newline in single-line comment cannot be escaped.
            _pos = turbo::find_first_symbols<'\n'>(_pos, _end);
            return Token(TokenType::Comment, token_begin, _pos);
        };

        switch (*_pos) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\f':
        case '\v': {
            ++_pos;
            while (_pos < _end && turbo::ascii_isspace(*_pos))
                ++_pos;
            return Token(TokenType::Whitespace, token_begin, _pos);
        }

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            /// The task is not to parse a number or check correctness, but only to skip it.

            /// Disambiguation: if previous token was dot, then we could parse only simple integer,
            ///  for chained tuple access operators (x.1.1) to work.
            //  Otherwise it will be tokenized as x . 1.1, not as x . 1 . 1
            if (prev_significant_token_type == TokenType::Dot) {
                ++_pos;
                while (_pos < _end && (turbo::ascii_isdigit(*_pos) || is_number_separator(false, false, _pos, _end)))
                    ++_pos;
            } else {
                bool start_of_block = false;
                /// 0x, 0b
                bool hex = false;
                if (_pos + 2 < _end && *_pos == '0' && (_pos[1] == 'x' || _pos[1] == 'b' || _pos[1] == 'X' || _pos[1] == 'B')) {
                    bool is_valid = false;
                    if (_pos[1] == 'x' || _pos[1] == 'X') {
                        if (turbo::ascii_isxdigit(_pos[2])) {
                            hex = true;
                            is_valid = true; // hex
                        }
                    } else if (_pos[2] == '0' || _pos[2] == '1')
                        is_valid = true; // bin
                    if (is_valid) {
                        _pos += 2;
                        start_of_block = true;
                    } else
                        ++_pos; // consume the leading zero - could be an identifier
                } else
                    ++_pos;

                while (_pos < _end && ((hex ? turbo::ascii_isxdigit(*_pos) : turbo::ascii_isdigit(*_pos)) || is_number_separator(start_of_block, hex, _pos, _end))) {
                    ++_pos;
                    start_of_block = false;
                }

                /// decimal point
                if (_pos < _end && *_pos == '.') {
                    start_of_block = true;
                    ++_pos;
                    while (_pos < _end && ((hex ? turbo::ascii_isxdigit(*_pos) : turbo::ascii_isdigit(*_pos)) || is_number_separator(start_of_block, hex, _pos, _end))) {
                        ++_pos;
                        start_of_block = false;
                    }
                }

                /// exponentiation (base 10 or base 2)
                if (_pos + 1 < _end && (hex ? (*_pos == 'p' || *_pos == 'P') : (*_pos == 'e' || *_pos == 'E'))) {
                    start_of_block = true;
                    ++_pos;

                    /// sign of exponent. It is always decimal.
                    if (_pos + 1 < _end && (*_pos == '-' || *_pos == '+'))
                        ++_pos;

                    while (_pos < _end && (turbo::ascii_isdigit(*_pos) || is_number_separator(start_of_block, false, _pos, _end))) {
                        ++_pos;
                        start_of_block = false;
                    }
                }
            }

            /// Try to parse it to a identifier(1identifier_name), otherwise it return ErrorWrongNumber
            if (_pos < _end && turbo::ascii_isword(*_pos)) {
                ++_pos;
                while (_pos < _end && turbo::ascii_isword(*_pos))
                    ++_pos;

                for (const char* iterator = token_begin; iterator < _pos; ++iterator) {
                    if (!turbo::ascii_isword(*iterator) && *iterator != '$')
                        return Token(TokenType::ErrorWrongNumber, token_begin, _pos);
                }

                return Token(TokenType::BareWord, token_begin, _pos);
            }

            return Token(TokenType::Number, token_begin, _pos);
        }

        case '\'':
            return quoted_string<'\''>(_pos, token_begin, _end, TokenType::StringLiteral, TokenType::ErrorSingleQuoteIsNotClosed);
        case '"':
            return quoted_string<'"'>(_pos, token_begin, _end, TokenType::QuotedIdentifier, TokenType::ErrorDoubleQuoteIsNotClosed);
        case '`':
            return quoted_string<'`'>(_pos, token_begin, _end, TokenType::QuotedIdentifier, TokenType::ErrorBackQuoteIsNotClosed);

        case '(':
            return Token(TokenType::OpeningRoundBracket, token_begin, ++_pos);
        case ')':
            return Token(TokenType::ClosingRoundBracket, token_begin, ++_pos);
        case '[':
            return Token(TokenType::OpeningSquareBracket, token_begin, ++_pos);
        case ']':
            return Token(TokenType::ClosingSquareBracket, token_begin, ++_pos);
        case '{':
            return Token(TokenType::OpeningCurlyBrace, token_begin, ++_pos);
        case '}':
            return Token(TokenType::ClosingCurlyBrace, token_begin, ++_pos);
        case ',':
            return Token(TokenType::Comma, token_begin, ++_pos);
        case ';':
            return Token(TokenType::Semicolon, token_begin, ++_pos);

        case '.': /// qualifier, tuple access operator or start of floating point number
        {
            /// Just after identifier or complex expression or number (for chained tuple access like x.1.1 to work properly).
            if (_pos > _begin
                && (!(_pos + 1 < _end && turbo::ascii_isdigit(_pos[1]))
                    || prev_significant_token_type == TokenType::ClosingRoundBracket
                    || prev_significant_token_type == TokenType::ClosingSquareBracket
                    || prev_significant_token_type == TokenType::BareWord
                    || prev_significant_token_type == TokenType::QuotedIdentifier
                    || prev_significant_token_type == TokenType::Number))
                return Token(TokenType::Dot, token_begin, ++_pos);

            bool start_of_block = true;
            ++_pos;
            while (_pos < _end && (turbo::ascii_isdigit(*_pos) || is_number_separator(start_of_block, false, _pos, _end))) {
                ++_pos;
                start_of_block = false;
            }

            /// exponentiation
            if (_pos + 1 < _end && (*_pos == 'e' || *_pos == 'E')) {
                start_of_block = true;
                ++_pos;

                /// sign of exponent
                if (_pos + 1 < _end && (*_pos == '-' || *_pos == '+'))
                    ++_pos;

                while (_pos < _end && (turbo::ascii_isdigit(*_pos) || is_number_separator(start_of_block, false, _pos, _end))) {
                    ++_pos;
                    start_of_block = false;
                }
            }

            return Token(TokenType::Number, token_begin, _pos);
        }

        case '+':
            return Token(TokenType::Plus, token_begin, ++_pos);
        case '-': /// minus (-), arrow (->) or start of comment (--)
        {
            ++_pos;
            if (_pos < _end && *_pos == '>')
                return Token(TokenType::Arrow, token_begin, ++_pos);

            if (_pos < _end && *_pos == '-') {
                ++_pos;
                return comment_until_end_of_line();
            }

            return Token(TokenType::Minus, token_begin, _pos);
        }
        case '*':
            ++_pos;
            return Token(TokenType::Asterisk, token_begin, _pos);
        case '/': /// division (/) or start of comment (//, /*)
        {
            ++_pos;
            if (_pos < _end && (*_pos == '/' || *_pos == '*')) {
                if (*_pos == '/') {
                    ++_pos;
                    return comment_until_end_of_line();
                }

                ++_pos;

                /// Nested multiline comments are supported according to the SQL standard.
                size_t nesting_level = 1;

                while (_pos + 2 <= _end) {
                    if (_pos[0] == '/' && _pos[1] == '*') {
                        _pos += 2;
                        ++nesting_level;
                    } else if (_pos[0] == '*' && _pos[1] == '/') {
                        _pos += 2;
                        --nesting_level;

                        if (nesting_level == 0)
                            return Token(TokenType::Comment, token_begin, _pos);
                    } else
                        ++_pos;
                }
                _pos = _end;
                return Token(TokenType::ErrorMultilineCommentIsNotClosed, token_begin, _pos);
            }
            return Token(TokenType::Slash, token_begin, _pos);
        }
        case '#': /// start of single line comment, MySQL style
        { /// PostgreSQL has some operators using '#' character.
          /// For less ambiguity, we will recognize a comment only if # is followed by whitespace.
          /// or #! as a special case for "shebang".
          /// #hello - not a comment
          /// # hello - a comment
          /// #!/usr/bin/clickhouse-local --queries-file - a comment
            ++_pos;
            if (_pos < _end && (*_pos == ' ' || *_pos == '!'))
                return comment_until_end_of_line();
            return Token(TokenType::Error, token_begin, _pos);
        }
        case '%':
            return Token(TokenType::Percent, token_begin, ++_pos);
        case '=': /// =, ==
        {
            ++_pos;
            if (_pos < _end && *_pos == '=')
                ++_pos;
            return Token(TokenType::Equals, token_begin, _pos);
        }
        case '!': /// !=
        {
            ++_pos;
            if (_pos < _end && *_pos == '=')
                return Token(TokenType::NotEquals, token_begin, ++_pos);
            return Token(TokenType::ErrorSingleExclamationMark, token_begin, _pos);
        }
        case '<': /// <, <=, <>, <=>
        {
            ++_pos;
            if (_pos + 1 < _end && *_pos == '=' && *(_pos + 1) == '>') {
                _pos += 2;
                return Token(TokenType::Spaceship, token_begin, _pos);
            }
            if (_pos < _end && *_pos == '=')
                return Token(TokenType::LessOrEquals, token_begin, ++_pos);
            if (_pos < _end && *_pos == '>')
                return Token(TokenType::NotEquals, token_begin, ++_pos);
            return Token(TokenType::Less, token_begin, _pos);
        }
        case '>': /// >, >=
        {
            ++_pos;
            if (_pos < _end && *_pos == '=')
                return Token(TokenType::GreaterOrEquals, token_begin, ++_pos);
            return Token(TokenType::Greater, token_begin, _pos);
        }
        case '?':
            return Token(TokenType::QuestionMark, token_begin, ++_pos);
        case '^':
            return Token(TokenType::Caret, token_begin, ++_pos);
        case ':': {
            ++_pos;
            if (_pos < _end && *_pos == ':')
                return Token(TokenType::DoubleColon, token_begin, ++_pos);
            return Token(TokenType::Colon, token_begin, _pos);
        }
        case '|': {
            ++_pos;
            if (_pos < _end && *_pos == '|')
                return Token(TokenType::Concatenation, token_begin, ++_pos);
            if (_pos < _end && *_pos == '>')
                return Token(TokenType::PipeOperator, token_begin, ++_pos);
            return Token(TokenType::PipeMark, token_begin, _pos);
        }
        case '@': {
            ++_pos;
            if (_pos < _end && *_pos == '@')
                return Token(TokenType::DoubleAt, token_begin, ++_pos);
            return Token(TokenType::At, token_begin, _pos);
        }
        case '\\': {
            ++_pos;
            if (_pos < _end && *_pos == 'G')
                return Token(TokenType::VerticalDelimiter, token_begin, ++_pos);
            return Token(TokenType::Error, token_begin, _pos);
        }
        case '\xE2': {
            /// Mathematical minus symbol, UTF-8
            if (_pos + 3 <= _end && _pos[1] == '\x88' && _pos[2] == '\x92') {
                _pos += 3;
                return Token(TokenType::Minus, token_begin, _pos);
            }
            /// Unicode quoted string, ‘Hello’ or “World”.
            if (_pos + 5 < _end && _pos[0] == '\xE2' && _pos[1] == '\x80' && (_pos[2] == '\x98' || _pos[2] == '\x9C')) {
                const char expected_end_byte = _pos[2] + 1;
                TokenType success_token = _pos[2] == '\x98' ? TokenType::StringLiteral : TokenType::QuotedIdentifier;
                TokenType error_token = _pos[2] == '\x98' ? TokenType::ErrorSingleQuoteIsNotClosed : TokenType::ErrorDoubleQuoteIsNotClosed;
                _pos += 3;
                return quoted_string_with_unicode_quotes(_pos, token_begin, _end, expected_end_byte, success_token, error_token);
            }
            /// Other characters starting at E2 can be parsed, see trim_left_utf8
            [[fallthrough]];
        }
        default:
            if (*_pos == '$') {
                /// Try to capture a dollar sign as a start of heredoc

                const char* tag_end = turbo::find_first_symbols<'$'>(_pos + 1, _end);
                if (tag_end != _end) {
                    size_t heredoc_size = tag_end + 1 - _pos;

                    bool is_valid_name = true;
                    for (const char* name_pos = _pos + 1; name_pos < tag_end; ++name_pos) {
                        if (!turbo::ascii_isword(*name_pos)) {
                            is_valid_name = false;
                            break;
                        }
                    }

                    if (is_valid_name) {
                        const size_t len = _end - tag_end - 1;
                        size_t heredoc_end_position = std::string_view { tag_end + 1, len }.find(std::string_view { _pos, heredoc_size });
                        if (heredoc_end_position != std::string::npos) {
                            _pos = tag_end + 1 + heredoc_end_position + heredoc_size;
                            return Token(TokenType::HereDoc, token_begin, _pos);
                        }
                    }
                }

                if (((_pos + 1 < _end && !turbo::ascii_isword(_pos[1])) || _pos + 1 == _end)) {
                    /// Capture a standalone dollar sign
                    return Token(TokenType::DollarSign, token_begin, ++_pos);
                }
            }

            if (_pos + 2 < _end && _pos[1] == '\'' && (*_pos == 'x' || *_pos == 'b' || *_pos == 'X' || *_pos == 'B')) {
                return quoted_hex_or_bin_string(_pos, token_begin, _end);
            }

            if (turbo::ascii_isword(*_pos) || *_pos == '$') {
                ++_pos;
                while (_pos < _end && (turbo::ascii_isword(*_pos) || *_pos == '$'))
                    ++_pos;
                return Token(TokenType::BareWord, token_begin, _pos);
            }

            /// We will also skip unicode whitespaces in UTF-8 to support for queries copy-pasted from MS Word and similar.
            _pos = turbo::trim_left_utf8(_pos, _end);
            if (_pos > token_begin)
                return Token(TokenType::Whitespace, token_begin, _pos);

            ++_pos;
            while (_pos < _end && turbo::is_continuation_octet(*_pos))
                ++_pos;

            return Token(TokenType::Error, token_begin, _pos);
        }
    }

}  // namespace shark::parser


