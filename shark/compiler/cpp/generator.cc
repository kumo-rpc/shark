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

#include <shark/common/clog.h>
#include <shark/compiler/cpp/file.h>
#include <shark/compiler/cpp/generator.h>
#include <shark/io/file_zero_copy_stream.h>
#include <shark/schema/uri.h>
#include <turbo/flags/flag.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace shark::compiler::cpp {
    namespace {

        turbo::Result<std::filesystem::path> output_path(const SchemaFile* file, bool cc) {
            turbo::Result<std::string> relative = path_to_cpp_file(file->path(), cc);
            if (!relative.ok()) {
                return relative.status();
            }
            return std::filesystem::path(turbo::GetFlag(FLAGS_out)) / *relative;
        }

        bool write_output(const std::filesystem::path& path, const SchemaFile* file, bool header) {
            std::error_code error;
            const std::filesystem::path parent = path.parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent, error);
                if (error) {
                    shark::common::report_error("create directory failed: " + parent.string());
                    return false;
                }
            }
            if (std::filesystem::is_regular_file(path, error)) {
                shark::common::report_debug(path.string() + " exists, replace");
            }
            std::ofstream output(path, std::ios::out | std::ios::trunc);
            if (!output) {
                shark::common::report_error("create file failed: " + path.string());
                return false;
            }
            {
                io::OstreamOutputStream raw(&output);
                {
                    io::Printer printer(&raw, '$');
                    FileGenerator generator(file);
                    if (header) {
                        generator.generate_header(printer);
                    } else {
                        generator.generate_source(printer);
                    }
                    if (printer.failed()) {
                        shark::common::report_error("write file failed: " + path.string());
                        return false;
                    }
                }
            }
            output.flush();
            if (!output) {
                shark::common::report_error("save file failed: " + path.string());
                return false;
            }
            return true;
        }

    } // namespace

    CppGenerator::CppGenerator() = default;

    bool CppGenerator::generate_code(const SchemaFile* file) {
        if (file == nullptr) {
            shark::common::report_error("missing schema file");
            return false;
        }
        turbo::Result<std::filesystem::path> header = output_path(file, false);
        if (!header.ok()) {
            shark::common::report_error(std::string(header.status().message()));
            return false;
        }
        turbo::Result<std::filesystem::path> source = output_path(file, true);
        if (!source.ok()) {
            shark::common::report_error(std::string(source.status().message()));
            return false;
        }
        if (!write_output(*header, file, true)) {
            return false;
        }
        return write_output(*source, file, false);
    }

} // namespace shark::compiler::cpp
