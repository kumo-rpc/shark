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

#include <shark/compiler/cpp/generator.h>
#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <turbo/cli/cli.h>
#include <turbo/flags/flag.h>
#include <turbo/strings/ascii.h>

#include <shark/common/clog.h>
#include <turbo/log/logging.h>
#include <string>
#include <string_view>

namespace {

std::string reject_empty(const std::string& value) {
    if (value.empty()) {
        return "must not be empty";
    }
    return {};
}

bool is_cpp_lang(std::string_view lang) {
    if (lang.size() != 3) {
        return false;
    }
    return turbo::ascii_tolower(static_cast<unsigned char>(lang[0])) == 'c'
        && turbo::ascii_tolower(static_cast<unsigned char>(lang[1])) == 'p'
        && turbo::ascii_tolower(static_cast<unsigned char>(lang[2])) == 'p';
}

}  // namespace

int main(int argc, char** argv) {
    xcli::App app{"shark compiler"};
    app.add_flag("-D,--debug", FLAGS_debug);
    app.add_option("-F,--file", FLAGS_file)->required()->check(reject_empty);
    app.add_option("-I,--include", FLAGS_include)->required()->expected(-1)->check(reject_empty);
    app.add_option("-l,--lang", FLAGS_lang)->required()->check(reject_empty);
    app.add_option("-o,--out", FLAGS_out)->required()->check(reject_empty);
    XCLI_PARSE(app, argc, argv);

    const std::string& file = turbo::GetFlag(FLAGS_file);
    const std::vector<std::string>& includes = turbo::GetFlag(FLAGS_include);
    if (turbo::GetFlag(FLAGS_debug)) {
        shark::common::clog_enable_debug();
    }
    shark::parser::FileParser* parsed = shark::parser::RegistryPool::instance().build_file_recursive(file, includes);
    if (parsed == nullptr) {
        return 1;
    }
    shark::SchemaPoolBase& schema = shark::parser::RegistryPool::instance().schema_pool();
    const shark::SchemaFile* schema_file = shark::SchemaFile::from(schema.get_file(parsed->path));
    if (schema_file == nullptr) {
        return 1;
    }
    if (!is_cpp_lang(turbo::GetFlag(FLAGS_lang))) {
        shark::common::report_error("unsupported language: " + turbo::GetFlag(FLAGS_lang));
        return 1;
    }
    shark::compiler::cpp::CppGenerator generator;
    if (!generator.generate_code(schema_file)) {
        return 1;
    }
    return 0;
}
