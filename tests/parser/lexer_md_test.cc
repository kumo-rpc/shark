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
#include <tests/config.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <qtest/md/md_test.h>

namespace {

std::string quote_token_text(std::string_view text) {
    std::string out;
    out.push_back('"');
    for (unsigned char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20 || c >= 0x80) {
                char hex[5];
                std::snprintf(hex, sizeof(hex), "\\x%02X", c);
                out += hex;
            } else {
                out.push_back(static_cast<char>(c));
            }
            break;
        }
    }
    out.push_back('"');
    return out;
}

std::string join_tag_text(const md_test::MdTagBlock& block) {
    std::string raw;
    for (size_t i = 0; i < block.lines.size(); ++i) {
        if (i != 0) {
            raw.push_back('\n');
        }
        std::string line = md_test::unescape_wire_input(block.lines[i].text);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        raw += line;
    }
    return raw;
}

md_test::TagProcessResult lex_input(const md_test::MdCase& md_case, const md_test::MdTagBlock& source, int) {
    md_test::TagProcessResult result;
    const std::string query = join_tag_text(source);
    size_t max_query_size = 0;
    if (const auto limit = md_case.option("max-query-size")) {
        max_query_size = static_cast<size_t>(std::stoul(*limit));
    }
    const bool positions = md_case.option_bool("positions");

    shark::parser::Lexer lexer(query, max_query_size);
    for (;;) {
        const shark::parser::Token token = lexer.next_token();
        std::string line;
        if (positions) {
            line += std::to_string(token.line);
            line.push_back(':');
            line += std::to_string(token.column);
            line.push_back(' ');
        }
        line += shark::parser::get_token_name(token.type);
        line.push_back(' ');
        line += quote_token_text(token.view());
        result.lines.push_back(std::move(line));
        if (token.is_end()) {
            break;
        }
    }
    return result;
}

md_test::MdSuite make_lexer_suite() {
    md_test::MdComparison comparison;
    comparison.name = "tokens";
    comparison.source_tag = "input";
    comparison.target_tag = "expect";
    comparison.processor = lex_input;
    comparison.default_mode = md_test::CompareMode::kOrdered;
    comparison.target_required = true;

    md_test::MdSuite suite;
    suite.name = "lexer";
    suite.type_prefixes = {"shark-lexer"};
    suite.default_type = "shark-lexer";
    suite.full_scan = false;
    suite.comparisons.push_back(std::move(comparison));
    return suite;
}

const md_test::MdSuite kSuite = make_lexer_suite();
MD_REGISTER_SUITE(kSuite);

const std::vector<md_test::MdTestParam> kParams = [] {
    md_test::MdLoadFilter filter;
    filter.type_prefixes = kSuite.type_prefixes;
    filter.default_type = kSuite.default_type;
    const std::vector<md_test::MdCase> cases = md_test::load_md_cases(
        testing::lexer_markdown_dir(),
        {"tokens.md", "strings.md", "comments.md", "positions.md", "limits.md"},
        &filter);
    return md_test::MdSuiteRegistry::instance().expand_params(kSuite.name, cases);
}();

std::string lexer_param_name(const testing::TestParamInfo<md_test::MdTestParam>& info) {
    std::string name = info.param.gtest_name();
    if (name.empty()) {
        name = "case_" + std::to_string(info.index);
    }
    for (char& c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0) {
            c = '_';
        }
    }
    return name;
}

class LexerMdTest : public testing::TestWithParam<md_test::MdTestParam> { };

TEST_P(LexerMdTest, Run) {
    const md_test::MdTestParam& param = GetParam();
    const md_test::MdRunResult result = md_test::run_comparison(param.md_case, param.comparison, param.scan);
    md_test::report_run_result_to_gtest(result, {&param.md_case, &param.comparison, param.scan});
    if (!result.skipped && result.processor_ok && result.report.ok()) {
        SUCCEED();
    }
}

INSTANTIATE_TEST_SUITE_P(Lexer, LexerMdTest, testing::ValuesIn(kParams), lexer_param_name);

} // namespace
