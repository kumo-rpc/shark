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

#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace shark::parser {


std::string_view TokenText(const Token& t) {
    return std::string_view{t.begin, t.size()};
}

std::vector<std::pair<TokenType, std::string_view>> Tokenize(std::string_view query,
    size_t max_query_size = 0) {
    Lexer lexer(query, max_query_size);
    std::vector<std::pair<TokenType, std::string_view>> out;
    for (;;) {
        Token t = lexer.next_token();
        out.emplace_back(t.type, TokenText(t));
        if (t.is_end())
            break;
    }
    return out;
}

TEST(Lexer, LineColumn) {
    Lexer lexer("a b\r\nc");
    Token a = lexer.next_token();
    EXPECT_EQ(a.view(), "a");
    EXPECT_EQ(a.line, 1);
    EXPECT_EQ(a.column, 1);

    Token sp = lexer.next_token();
    EXPECT_EQ(sp.type, TokenType::Whitespace);
    EXPECT_EQ(sp.line, 1);
    EXPECT_EQ(sp.column, 2);

    Token b = lexer.next_token();
    EXPECT_EQ(b.view(), "b");
    EXPECT_EQ(b.line, 1);
    EXPECT_EQ(b.column, 3);

    Token nl = lexer.next_token();
    EXPECT_EQ(nl.type, TokenType::Whitespace);
    EXPECT_EQ(nl.line, 1);
    EXPECT_EQ(nl.column, 4);

    Token c = lexer.next_token();
    EXPECT_EQ(c.view(), "c");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);

    Token end = lexer.next_token();
    EXPECT_TRUE(end.is_end());
    EXPECT_EQ(end.line, 2);
    EXPECT_EQ(end.column, 2);

    Lexer empty("");
    Token eos = empty.next_token();
    EXPECT_TRUE(eos.is_end());
    EXPECT_EQ(eos.line, 1);
    EXPECT_EQ(eos.column, 1);
}

TEST(Lexer, MultilineLineColumn) {
    // ibex/parser/grammar.ibex
    constexpr std::string_view kGrammar = R"(// shot comment
/// long comment
/* multi lone comment
   line 0
   line 1
*

edition = "x2026"

import "ibex/schema.ibex"
import.cxx  "cstd"
import.java  "com.kumo.pub.algos"
import.go  "github.com/kumo-rpc/go-ibex"


package kumo.pub
package.java  "com.kumo.pub.algos"
package.go  "github.com/kumo-rpc/go-ibex"

struct MyStruct {
    optional int8 = 1;
}
)";

    Lexer lexer(kGrammar);

    Token slash = lexer.next_token();
    EXPECT_EQ(slash.type, TokenType::Comment);
    EXPECT_EQ(slash.view(), "// shot comment");
    EXPECT_EQ(slash.line, 1);
    EXPECT_EQ(slash.column, 1);

    Token nl1 = lexer.next_token();
    EXPECT_EQ(nl1.type, TokenType::Whitespace);
    EXPECT_EQ(nl1.view(), "\n");
    EXPECT_EQ(nl1.line, 1);
    EXPECT_EQ(nl1.column, 16);

    Token slash3 = lexer.next_token();
    EXPECT_EQ(slash3.type, TokenType::Comment);
    EXPECT_EQ(slash3.view(), "/// long comment");
    EXPECT_EQ(slash3.line, 2);
    EXPECT_EQ(slash3.column, 1);

    Token nl2 = lexer.next_token();
    EXPECT_EQ(nl2.type, TokenType::Whitespace);
    EXPECT_EQ(nl2.view(), "\n");
    EXPECT_EQ(nl2.line, 2);
    EXPECT_EQ(nl2.column, 17);

    Token block = lexer.next_token();
    EXPECT_EQ(block.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(block.line, 3);
    EXPECT_EQ(block.column, 1);
    EXPECT_EQ(block.view(), kGrammar.substr(std::string_view("// shot comment\n/// long comment\n").size()));

    Token end = lexer.next_token();
    EXPECT_TRUE(end.is_end());
    EXPECT_EQ(end.line, 23);
    EXPECT_EQ(end.column, 1);

    Lexer look(kGrammar);
    EXPECT_TRUE(look.consume("// shot comment"));
    EXPECT_TRUE(look.consume("\n"));
    EXPECT_TRUE(look.look_at("/// long comment"));
    EXPECT_EQ(look.line(), 2);
    EXPECT_EQ(look.column(), 1);
    look.consume();
    EXPECT_TRUE(look.consume("\n"));
    EXPECT_EQ(look.look_at().type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(look.line(), 3);
    EXPECT_EQ(look.column(), 1);
}

TEST(Lexer, LookAtAndConsume) {
    Lexer lexer("ab cd");
    EXPECT_EQ(lexer.line(), 1);
    EXPECT_EQ(lexer.column(), 1);
    EXPECT_TRUE(lexer.look_at("ab"));
    EXPECT_EQ(lexer.look_at().view(), "ab");
    EXPECT_TRUE(lexer.consume("ab"));
    EXPECT_TRUE(lexer.look_at(" "));
    lexer.consume();
    EXPECT_TRUE(lexer.look_at("cd"));
    EXPECT_EQ(lexer.line(), 1);
    EXPECT_EQ(lexer.column(), 4);
    EXPECT_FALSE(lexer.consume("nope"));
    EXPECT_EQ(lexer.look_at().view(), "cd");
    lexer.consume();
    EXPECT_TRUE(lexer.look_at().is_end());
    EXPECT_FALSE(lexer.look_at(""));
    EXPECT_FALSE(lexer.consume(""));
    lexer.consume();
    EXPECT_TRUE(lexer.look_at().is_end());
}

TEST(Lexer, ConsumeTypeAndWhitespace) {
    Lexer lexer("ab \t\ncd");
    EXPECT_FALSE(lexer.consume(TokenType::Whitespace));
    EXPECT_TRUE(lexer.consume(TokenType::BareWord));
    EXPECT_EQ(lexer.look_at().view(), " \t\n");
    lexer.consume_white_space(false);
    EXPECT_TRUE(lexer.look_at("cd"));
    EXPECT_EQ(lexer.line(), 2);
    EXPECT_EQ(lexer.column(), 1);
    EXPECT_TRUE(lexer.consume(TokenType::BareWord));
    EXPECT_TRUE(lexer.consume(TokenType::EndOfStream));
    EXPECT_TRUE(lexer.look_at().is_end());
    EXPECT_TRUE(lexer.consume(TokenType::EndOfStream));
    EXPECT_TRUE(lexer.look_at().is_end());
    EXPECT_FALSE(lexer.consume(TokenType::BareWord));

    Lexer spaced("a  /*c*/  b");
    EXPECT_TRUE(spaced.consume(TokenType::BareWord));
    spaced.consume_white_space(true);
    EXPECT_EQ(spaced.look_at().type, TokenType::Comment);
    EXPECT_FALSE(spaced.consume(TokenType::Whitespace));
    EXPECT_TRUE(spaced.consume(TokenType::Comment));
    spaced.consume_white_space(false);
    EXPECT_TRUE(spaced.look_at("b"));

    Lexer open("/*");
    EXPECT_TRUE(open.consume(TokenType::ErrorMultilineCommentIsNotClosed));
    EXPECT_TRUE(open.look_at().is_end());
    EXPECT_TRUE(open.consume(TokenType::EndOfStream));
    EXPECT_TRUE(open.look_at().is_end());
}

TEST(Lexer, TokenIndex) {
    Lexer lexer("a b");
    EXPECT_EQ(lexer.token_index(), 0);
    EXPECT_EQ(lexer.next_token().view(), "a");
    EXPECT_EQ(lexer.token_index(), 1);
    EXPECT_EQ(lexer.next_token().type, TokenType::Whitespace);
    EXPECT_EQ(lexer.token_index(), 2);
    EXPECT_EQ(lexer.next_token().view(), "b");
    EXPECT_EQ(lexer.token_index(), 3);
    EXPECT_TRUE(lexer.next_token().is_end());
    EXPECT_EQ(lexer.token_index(), 4);
    EXPECT_TRUE(lexer.next_token().is_end());
    EXPECT_EQ(lexer.token_index(), 4);

    Lexer empty("");
    EXPECT_EQ(empty.token_index(), 0);
    EXPECT_TRUE(empty.next_token().is_end());
    EXPECT_EQ(empty.token_index(), 1);
    EXPECT_TRUE(empty.next_token().is_end());
    EXPECT_EQ(empty.token_index(), 1);
}

TEST(Lexer, TokenIndexCountsError) {
    Lexer unclosed("a /*");
    EXPECT_EQ(unclosed.next_token().view(), "a");
    EXPECT_EQ(unclosed.token_index(), 1);
    EXPECT_EQ(unclosed.next_token().type, TokenType::Whitespace);
    EXPECT_EQ(unclosed.token_index(), 2);
    Token err = unclosed.next_token();
    EXPECT_EQ(err.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(unclosed.token_index(), 3);
    EXPECT_TRUE(unclosed.next_token().is_end());
    EXPECT_EQ(unclosed.token_index(), 4);
    EXPECT_TRUE(unclosed.next_token().is_end());
    EXPECT_EQ(unclosed.token_index(), 4);

    Lexer quote("'abc");
    EXPECT_EQ(quote.next_token().type, TokenType::ErrorSingleQuoteIsNotClosed);
    EXPECT_EQ(quote.token_index(), 1);
    EXPECT_TRUE(quote.next_token().is_end());
    EXPECT_EQ(quote.token_index(), 2);
}

TEST(Lexer, Empty) {
    auto tokens = Tokenize("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].first, TokenType::EndOfStream);
    EXPECT_TRUE(tokens[0].second.empty());
}

TEST(Lexer, SimpleSelect) {
    auto tokens = Tokenize("SELECT a FROM t");
    ASSERT_EQ(tokens.size(), 8u);
    EXPECT_EQ(tokens[0], std::make_pair(TokenType::BareWord, std::string_view("SELECT")));
    EXPECT_EQ(tokens[1], std::make_pair(TokenType::Whitespace, std::string_view(" ")));
    EXPECT_EQ(tokens[2], std::make_pair(TokenType::BareWord, std::string_view("a")));
    EXPECT_EQ(tokens[3], std::make_pair(TokenType::Whitespace, std::string_view(" ")));
    EXPECT_EQ(tokens[4], std::make_pair(TokenType::BareWord, std::string_view("FROM")));
    EXPECT_EQ(tokens[5], std::make_pair(TokenType::Whitespace, std::string_view(" ")));
    EXPECT_EQ(tokens[6], std::make_pair(TokenType::BareWord, std::string_view("t")));
    EXPECT_EQ(tokens[7].first, TokenType::EndOfStream);
}

TEST(Lexer, Numbers) {
    auto tokens = Tokenize("123 1.5 1e10 0x1f 1_000");
    EXPECT_EQ(tokens[0], std::make_pair(TokenType::Number, std::string_view("123")));
    EXPECT_EQ(tokens[2], std::make_pair(TokenType::Number, std::string_view("1.5")));
    EXPECT_EQ(tokens[4], std::make_pair(TokenType::Number, std::string_view("1e10")));
    EXPECT_EQ(tokens[6], std::make_pair(TokenType::Number, std::string_view("0x1f")));
    EXPECT_EQ(tokens[8], std::make_pair(TokenType::Number, std::string_view("1_000")));
}

TEST(Lexer, DotVsFloat) {
    auto chained = Tokenize("x.1.1");
    EXPECT_EQ(chained[0].first, TokenType::BareWord);
    EXPECT_EQ(chained[1].first, TokenType::Dot);
    EXPECT_EQ(chained[2], std::make_pair(TokenType::Number, std::string_view("1")));
    EXPECT_EQ(chained[3].first, TokenType::Dot);
    EXPECT_EQ(chained[4], std::make_pair(TokenType::Number, std::string_view("1")));

    auto leading = Tokenize(".1");
    EXPECT_EQ(leading[0], std::make_pair(TokenType::Number, std::string_view(".1")));
}

TEST(Lexer, StringsAndQuotedIdentifiers) {
    auto tokens = Tokenize("'hello''world' \"x\" `y`");
    EXPECT_EQ(tokens[0], std::make_pair(TokenType::StringLiteral, std::string_view("'hello''world'")));
    EXPECT_EQ(tokens[2], std::make_pair(TokenType::QuotedIdentifier, std::string_view("\"x\"")));
    EXPECT_EQ(tokens[4], std::make_pair(TokenType::QuotedIdentifier, std::string_view("`y`")));
}

TEST(Lexer, UnclosedQuotes) {
    EXPECT_EQ(Tokenize("'abc")[0].first, TokenType::ErrorSingleQuoteIsNotClosed);
    EXPECT_EQ(Tokenize("\"abc")[0].first, TokenType::ErrorDoubleQuoteIsNotClosed);
    EXPECT_EQ(Tokenize("`abc")[0].first, TokenType::ErrorBackQuoteIsNotClosed);
}

TEST(Lexer, Comments) {
    auto line = Tokenize("a-- c\nb");
    EXPECT_EQ(line[0].first, TokenType::BareWord);
    EXPECT_EQ(line[1], std::make_pair(TokenType::Comment, std::string_view("-- c")));
    EXPECT_EQ(line[2].first, TokenType::Whitespace);
    EXPECT_EQ(line[3], std::make_pair(TokenType::BareWord, std::string_view("b")));

    auto slash = Tokenize("a/* x */b");
    EXPECT_EQ(slash[1], std::make_pair(TokenType::Comment, std::string_view("/* x */")));
    EXPECT_EQ(slash[2], std::make_pair(TokenType::BareWord, std::string_view("b")));

    EXPECT_EQ(Tokenize("/*")[0].first, TokenType::ErrorMultilineCommentIsNotClosed);

    auto hash_comment = Tokenize("# hello");
    EXPECT_EQ(hash_comment[0].first, TokenType::Comment);
    EXPECT_EQ(Tokenize("#hello")[0].first, TokenType::Error);
}

TEST(Lexer, UnclosedComment) {
    Lexer only_open("/*");
    Token open = only_open.next_token();
    EXPECT_EQ(open.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(open.view(), "/*");
    EXPECT_EQ(open.line, 1);
    EXPECT_EQ(open.column, 1);
    Token open_end = only_open.next_token();
    EXPECT_TRUE(open_end.is_end());
    EXPECT_EQ(open_end.line, 1);
    EXPECT_EQ(open_end.column, 3);

    Lexer block(R"(a
/* not closed
   line 0
)");
    EXPECT_EQ(block.next_token().view(), "a");
    Token nl = block.next_token();
    EXPECT_EQ(nl.type, TokenType::Whitespace);
    EXPECT_EQ(nl.line, 1);
    EXPECT_EQ(nl.column, 2);
    Token unclosed = block.next_token();
    EXPECT_EQ(unclosed.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(unclosed.view(), R"(/* not closed
   line 0
)");
    EXPECT_EQ(unclosed.line, 2);
    EXPECT_EQ(unclosed.column, 1);
    EXPECT_TRUE(block.next_token().is_end());

    Lexer nested("/* outer /* inner");
    Token nest = nested.next_token();
    EXPECT_EQ(nest.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(nest.view(), "/* outer /* inner");
    EXPECT_EQ(nest.line, 1);
    EXPECT_EQ(nest.column, 1);

    Lexer tail("/* ok */ /* still open");
    Token closed = tail.next_token();
    EXPECT_EQ(closed.type, TokenType::Comment);
    EXPECT_EQ(closed.view(), "/* ok */");
    EXPECT_EQ(closed.line, 1);
    EXPECT_EQ(closed.column, 1);
    Token gap = tail.next_token();
    EXPECT_EQ(gap.type, TokenType::Whitespace);
    EXPECT_EQ(gap.column, 9);
    Token rest = tail.next_token();
    EXPECT_EQ(rest.type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(rest.view(), "/* still open");
    EXPECT_EQ(rest.line, 1);
    EXPECT_EQ(rest.column, 10);

    Lexer look("x /*");
    EXPECT_TRUE(look.consume("x"));
    EXPECT_TRUE(look.consume(" "));
    EXPECT_TRUE(look.look_at().is_error());
    EXPECT_EQ(look.look_at().type, TokenType::ErrorMultilineCommentIsNotClosed);
    EXPECT_EQ(look.line(), 1);
    EXPECT_EQ(look.column(), 3);
    EXPECT_FALSE(look.look_at("/*"));
    EXPECT_FALSE(look.consume("/*"));
    EXPECT_EQ(look.look_at().view(), "/*");
}

TEST(Lexer, Operators) {
    auto tokens = Tokenize("( ) + - * / % < > = <=> != <> -> :: || |> @ @@ ?");
    std::vector<TokenType> types;
    for (const auto& t : tokens) {
        if (t.first != TokenType::Whitespace && t.first != TokenType::EndOfStream)
            types.push_back(t.first);
    }

    EXPECT_EQ(types, (std::vector<TokenType> {
                          TokenType::OpeningRoundBracket,
                          TokenType::ClosingRoundBracket,
                          TokenType::Plus,
                          TokenType::Minus,
                          TokenType::Asterisk,
                          TokenType::Slash,
                          TokenType::Percent,
                          TokenType::Less,
                          TokenType::Greater,
                          TokenType::Equals,
                          TokenType::Spaceship,
                          TokenType::NotEquals,
                          TokenType::NotEquals,
                          TokenType::Arrow,
                          TokenType::DoubleColon,
                          TokenType::Concatenation,
                          TokenType::PipeOperator,
                          TokenType::At,
                          TokenType::DoubleAt,
                          TokenType::QuestionMark,
                      }));
}

TEST(Lexer, BracketsCommaSemicolon) {
    auto tokens = Tokenize("[]{},;");
    EXPECT_EQ(tokens[0].first, TokenType::OpeningSquareBracket);
    EXPECT_EQ(tokens[1].first, TokenType::ClosingSquareBracket);
    EXPECT_EQ(tokens[2].first, TokenType::OpeningCurlyBrace);
    EXPECT_EQ(tokens[3].first, TokenType::ClosingCurlyBrace);
    EXPECT_EQ(tokens[4].first, TokenType::Comma);
    EXPECT_EQ(tokens[5].first, TokenType::Semicolon);
}

TEST(Lexer, ArrowMinusColonPipe) {
    EXPECT_EQ(Tokenize("-")[0].first, TokenType::Minus);
    EXPECT_EQ(Tokenize("->")[0].first, TokenType::Arrow);
    EXPECT_EQ(Tokenize(":")[0].first, TokenType::Colon);
    EXPECT_EQ(Tokenize("|")[0].first, TokenType::PipeMark);
    EXPECT_EQ(Tokenize("!")[0].first, TokenType::ErrorSingleExclamationMark);
    EXPECT_EQ(Tokenize("\\G")[0].first, TokenType::VerticalDelimiter);
    EXPECT_EQ(Tokenize("\\X")[0].first, TokenType::Error);
}

TEST(Lexer, DollarAndHereDoc) {
    EXPECT_EQ(Tokenize("$")[0].first, TokenType::DollarSign);
    auto hd = Tokenize("$tag$hello$tag$");
    EXPECT_EQ(hd[0], std::make_pair(TokenType::HereDoc, std::string_view("$tag$hello$tag$")));
}

TEST(Lexer, HexBinString) {
    EXPECT_EQ(Tokenize("x'abc'")[0].first, TokenType::StringLiteral);
    EXPECT_EQ(Tokenize("b'01'")[0].first, TokenType::StringLiteral);
    EXPECT_EQ(Tokenize("x'ab")[0].first, TokenType::ErrorSingleQuoteIsNotClosed);
}

TEST(Lexer, UnicodeWhitespaceAndMinus) {
    auto nbsp = Tokenize("a\xC2\xA0"
                        "b");
    EXPECT_EQ(nbsp[0].first, TokenType::BareWord);
    EXPECT_EQ(nbsp[1].first, TokenType::Whitespace);
    EXPECT_EQ(nbsp[2].first, TokenType::BareWord);

    auto minus = Tokenize("\xE2\x88\x92");
    EXPECT_EQ(minus[0].first, TokenType::Minus);
    EXPECT_EQ(minus[0].second.size(), 3u);
}

TEST(Lexer, UnicodeQuotes) {
    auto s = Tokenize("\xE2\x80\x98"
                      "hi"
                      "\xE2\x80\x99");
    EXPECT_EQ(s[0].first, TokenType::StringLiteral);

    auto q = Tokenize("\xE2\x80\x9C"
                      "id"
                      "\xE2\x80\x9D");
    EXPECT_EQ(q[0].first, TokenType::QuotedIdentifier);
}

TEST(Lexer, MaxQuerySize) {
    auto tokens = Tokenize("abcdef", 3);
    EXPECT_EQ(tokens[0].first, TokenType::ErrorMaxQuerySizeExceeded);
    EXPECT_EQ(tokens[0].second, std::string_view("abcdef"));
}

TEST(Lexer, TokenFlagsAndNames) {
    Token ws(TokenType::Whitespace, nullptr, nullptr);
    EXPECT_FALSE(ws.is_significant());
    EXPECT_FALSE(ws.is_error());
    EXPECT_FALSE(ws.is_end());

    Token err(TokenType::Error, nullptr, nullptr);
    EXPECT_TRUE(err.is_error());

    EXPECT_STREQ(shark::parser::get_token_name(TokenType::BareWord), "BareWord");
    EXPECT_STREQ(shark::parser::get_error_token_description(TokenType::ErrorSingleQuoteIsNotClosed),
        "Single quoted string is not closed");
    EXPECT_STREQ(shark::parser::get_error_token_description(TokenType::BareWord), "Not an error");
}

TEST(Lexer, ExpressionLexerAndWrapper) {
    std::string_view q = "x";
    shark::parser::ExpressionLexer core(q);
    Lexer wrapped(q);
    EXPECT_EQ(core.next_token().type, TokenType::BareWord);
    EXPECT_EQ(wrapped.next_token().type, TokenType::BareWord);
}

} // namespace  shark::parser
