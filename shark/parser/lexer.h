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
#include <cstddef>
#include <optional>
#include <string_view>
#include <string>
#include <shark/parser/node.h>

namespace shark::parser {

    enum class TokenType : uint8_t {
        Whitespace,
        Comment,
        /// Either keyword (SELECT) or identifier (column)
        BareWord,
        /// Always non-negative. No leading plus. 123 or something like 123.456e12, 0x123p12
        Number,
        /// 'hello word', 'hello''word', 'hello\'word\\'
        StringLiteral,
        /// "x", `x`
        QuotedIdentifier,
        OpeningRoundBracket,
        ClosingRoundBracket,
        OpeningSquareBracket,
        ClosingSquareBracket,
        OpeningCurlyBrace,
        ClosingCurlyBrace,
        Comma,
        Semicolon,
        /// Vertical delimiter \G
        VerticalDelimiter,
        /// Compound identifiers, like a.b or tuple access operator a.1, (x, y).2.
        /// Need to be distinguished from floating point number with omitted integer part: .1
        Dot,
        /// Could be used as multiplication operator or on it's own: "SELECT *"
        Asterisk,
        HereDoc,
        DollarSign,
        Plus,
        Minus,
        Slash,
        Percent,
        /// ->. Should be distinguished from minus operator.
        Arrow,
        QuestionMark,
        Colon,
        Caret,
        DoubleColon,
        Equals,
        NotEquals,
        Less,
        Greater,
        LessOrEquals,
        GreaterOrEquals,
        /// <=>. Used in MySQL for NULL-safe equality comparison.
        Spaceship,
        PipeMark,
        /// |>. Pipe operator: FROM t |> WHERE x |> SELECT y
        PipeOperator,
        /// String concatenation operator: ||
        Concatenation,
        /// @. Used for specifying user names and also for MySQL-style variables.
        At,
        /// @@. Used for MySQL-style global variables.
        DoubleAt,
        /// Order is important. EndOfStream goes after all usual tokens,
        /// and special error tokens goes after EndOfStream.
        EndOfStream,
        /// Something unrecognized.
        Error,
        /// Something is wrong and we have more information.
        ErrorMultilineCommentIsNotClosed,
        ErrorSingleQuoteIsNotClosed,
        ErrorDoubleQuoteIsNotClosed,
        ErrorBackQuoteIsNotClosed,
        ErrorSingleExclamationMark,
        ErrorSinglePipeMark,
        ErrorWrongNumber,
        ErrorMaxQuerySizeExceeded,
        KMaxTokenType,
    };


    const char* get_token_name(TokenType type);
    const char* get_error_token_description(TokenType type);

    struct Token {
        TokenType type;
        const char* begin;
        const char* end;
        int line;
        /// point to begin.
        int column;

        [[nodiscard]] size_t size() const { return end - begin; }

        [[nodiscard]] std::string_view view() const {
            if (size()) {
                return std::string_view{begin, size()};
            }
            return "";
        }
       [[nodiscard]] SourceLocation location() const {
            return SourceLocation{line,column};
        }

        [[nodiscard]] std::string to_string() const {
            return std::string(view());
        }

        Token() = default;
        Token(TokenType type_, const char* begin_, const char* end_)
            : type(type_)
            , begin(begin_)
            , end(end_)
            , line(0)
            , column(0) { }
        Token(TokenType type_, const char* begin_, const char* end_, int line_, int col)
            : type(type_)
            , begin(begin_)
            , end(end_)
            , line(line_)
            , column(col) { }

       [[nodiscard]] bool is_significant() const {
            return type != TokenType::Whitespace && type != TokenType::Comment;
        }
        [[nodiscard]] bool is_error() const {
            return type > TokenType::EndOfStream;
        }
        [[nodiscard]] bool is_end() const {
            return type == TokenType::EndOfStream;
        }

        [[nodiscard]] bool  operator==(std::string_view str) const {
            return std::string_view(begin, end-begin) == str;
        }
        [[nodiscard]] bool  operator!=(std::string_view str) const {
            return std::string_view(begin, end-begin) != str;
        }
    };



    class ExpressionLexer {
    public:
        ExpressionLexer(const char* begin_, const char* end_, size_t max_query_size_ = 0)
            : _begin(begin_)
            , _pos(begin_)
            , _end(end_)
            , _loc(begin_)
            , _max_query_size(max_query_size_ <= kMaxQuerySizeLimit ? max_query_size_ : kMaxQuerySizeLimit) {
        }

        explicit ExpressionLexer(std::string_view query, size_t max_query_size_ = 0)
            : ExpressionLexer(query.data(), query.data() + query.size(), max_query_size_) {
        }

        /// 1-based index of the token most recently produced by next_token().
        /// 0 before any token is produced. An error token counts as one token.
        /// EndOfStream is counted once; further next_token() calls at the end do not
        /// increment. The value at the end is the total token count, not an array subscript.
        int token_index() const { return _token_index; }

        /////////////////////////////////////////////////////////////////////
        /// Pull interface. Each call lexes and returns the next token.
        /// Does not read or write the look-ahead cache.
        /// Not interchangeable with look_at / consume. Do not mix the two groups
        /// on the same lexer: they share the scan cursor and will fall out of step.
        Token next_token();

        /////////////////////////////////////////////////////////////////////
        /// Look-ahead interface. look_at / consume / line / column share one cached
        /// token. Not interchangeable with next_token(). Do not mix the two groups
        /// on the same lexer: they share the scan cursor and will fall out of step.
        const Token& look_at();

        /// Token.view() == tok. End and error tokens do not match.
        bool look_at(std::string_view tok);

        void consume();

        bool consume(std::string_view tok);

        bool consume(TokenType type);

        void consume_white_space(bool all);

        int line();

        int column();

    private:
        const char* const _begin;
        const char* _pos;
        const char* const _end;

        /// Byte cursor already accounted into _line / _column. Starts at _begin, line 1, column 1.
        const char* _loc;
        int _line = 1;
        int _column = 1;

        const size_t _max_query_size;

        /// Some reasonable size to at least avoid pointer overflows.
        static constexpr size_t kMaxQuerySizeLimit = 1'000'000'000;

        Token next_token_impl();
        void stamp(Token& token);

        /// This is needed to disambiguate tuple access operator from floating point number (.1).
        /// No previous token.
        TokenType prev_significant_token_type = TokenType::Whitespace;

        int _token_index = 0;
        bool _counted_end = false;

        /// Current token for look_at / consume. Empty means not pulled yet.
        /// next_token() does not read or write this.
        std::optional<Token> _current;
    };

    template<typename Core = ExpressionLexer>
    class Lexer : public Core{
    public:
        Lexer(const char* begin, const char* end, size_t max_query_size = 0)
            : Core(begin, end, max_query_size) {

        }

        explicit Lexer(std::string_view query, size_t max_query_size = 0)
           : Lexer(query.data(), query.data() + query.size(), max_query_size) {
        }

        using Core::next_token;
    };

}  // namespace namespace shark::parser

