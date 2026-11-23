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

#include <shark/common/filepath.h>
#include <turbo/strings/match.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace shark::common {

    namespace fs = std::filesystem;

    turbo::Result<std::string> absolute_path(std::string_view path) {
        if (path.empty()) {
            return turbo::invalid_argument_error("path is empty");
        }

        std::error_code ec;
        fs::path p = fs::weakly_canonical(fs::path(path), ec);
        if (ec) {
            ec.clear();
            p = fs::absolute(fs::path(path), ec).lexically_normal();
            if (ec) {
                return turbo::ErrnoToStatus(
                    ec.value(),
                    turbo::str_sprintf("failed to resolve absolute path for `%s`: %s",
                        path, ec.message()));
            }
        }
        return p.generic_string();
    }

    turbo::Result<std::string> relative_path(
        std::string_view path,
        const std::vector<std::string>& prefix,
        std::string* prefix_out) {
        if (path.empty()) {
            return turbo::invalid_argument_error("target path is empty");
        }
        if (prefix.empty()) {
            return turbo::not_found_error(
                turbo::str_sprintf("no include prefix provided for `%s`", path));
        }

        turbo::Result<std::string> target_res = absolute_path(path);
        if (!target_res.ok()) {
            return target_res.status();
        }
        const fs::path target_abs(*target_res);

        for (const auto& dir : prefix) {
            if (dir.empty()) {
                continue;
            }

            turbo::Result<std::string> base_res = absolute_path(dir);
            if (!base_res.ok()) {
                continue;
            }
            const fs::path base_abs(*base_res);

            std::error_code ec;
            fs::path rel_p = fs::relative(target_abs, base_abs, ec);
            if (ec || rel_p.empty()) {
                continue;
            }

            const auto first_comp = *rel_p.begin();
            if (rel_p == "." || first_comp == ".." || rel_p.is_absolute()) {
                continue;
            }

            if (prefix_out != nullptr) {
                *prefix_out = base_abs.generic_string();
            }
            return rel_p.generic_string();
        }

        return turbo::not_found_error(
            turbo::str_sprintf("path `%s` is not under any include prefix", path));
    }

    namespace {

        /// Checks whether `target` starts with `base` on a valid path boundary.
        /// Prevents false positives like "/a/bc" matching "/a/b".
        bool is_path_prefix(std::string_view target, std::string_view base) {
            if (!turbo::starts_with(target, base)) {
                return false;
            }
            return target.size() == base.size() ||
                   base.back() == '/' ||
                   target[base.size()] == '/';
        }

    }  // namespace


turbo::Result<std::string> absolute_path(
        std::string_view path,
        const std::vector<std::string>& prefix) {
    if (path.empty()) {
        return turbo::invalid_argument_error("target path is empty");
    }
    if (prefix.empty()) {
        return turbo::not_found_error(
            turbo::str_sprintf("no include prefix provided for `%s`", path));
    }

    const fs::path input_p(path);

    /// Case 1: Input is already an absolute path; verify it falls under a valid prefix.
    if (input_p.is_absolute()) {
        turbo::Result<std::string> target_res = absolute_path(path);
        if (!target_res.ok()) {
            return target_res.status();
        }
        const std::string target_str = fs::path(*target_res).generic_string();

        for (const auto& dir : prefix) {
            if (dir.empty()) {
                continue;
            }
            turbo::Result<std::string> base_res = absolute_path(dir);
            if (!base_res.ok()) {
                continue;
            }
            const std::string base_str = fs::path(*base_res).generic_string();

            if (is_path_prefix(target_str, base_str)) {
                return target_str;
            }
        }

        return turbo::not_found_error(
            turbo::str_sprintf("path `%s` is not under any include prefix", path));
    }

    /// Case 2: Input is a relative path; search prefixes in order for an existing entry.
    std::error_code ec;
    for (const auto& dir : prefix) {
        if (dir.empty()) {
            continue;
        }
        fs::path candidate = fs::path(dir) / input_p;
        if (fs::exists(candidate, ec) && !ec) {
            turbo::Result<std::string> res = absolute_path(candidate.generic_string());
            if (!res.ok()) {
                return res.status();
            }
            const std::string target_str = fs::path(*res).generic_string();

            /// Ensure normalized path does not escape `dir` via "../" traversal.
            turbo::Result<std::string> base_res = absolute_path(dir);
            if (base_res.ok() &&
                is_path_prefix(target_str, fs::path(*base_res).generic_string())) {
                return target_str;
            }
        }
    }

    return turbo::not_found_error(
        turbo::str_sprintf("path `%s` is not found under any include prefix", path));
}


    turbo::Status read_file(std::string_view path, std::string& out) {
        if (path.empty()) {
            return turbo::invalid_argument_error("file path is empty");
        }

        std::error_code ec;
        const fs::path file_path(path);

        if (!fs::exists(file_path, ec) || ec) {
            return turbo::not_found_error(
                turbo::str_sprintf("file `%s` does not exist", path));
        }

        if (!fs::is_regular_file(file_path, ec) || ec) {
            return turbo::invalid_argument_error(
                turbo::str_sprintf("path `%s` is not a regular file", path));
        }

        std::ifstream ifs(file_path, std::ios::in | std::ios::binary);
        if (!ifs.is_open()) {
            return turbo::permission_denied_error(
                turbo::str_sprintf("failed to open file `%s`", path));
        }

        const auto file_size = fs::file_size(file_path, ec);
        if (ec) {
            return turbo::internal_error(
                turbo::str_sprintf("failed to get file size for `%s`: %s",
                                   path, ec.message()));
        }

        out.clear();
        if (file_size == 0) {
            return turbo::ok_status();
        }

        out.resize(static_cast<std::size_t>(file_size));
        ifs.read(out.data(), static_cast<std::streamsize>(file_size));

        if (!ifs && !ifs.eof()) {
            out.clear();
            return turbo::internal_error(
                turbo::str_sprintf("failed to read file `%s`", path));
        }

        /// Adjust size in case fewer bytes were read than reported (e.g. pseudo-files).
        out.resize(static_cast<std::size_t>(ifs.gcount()));
        return turbo::ok_status();
    }


    turbo::Status check_file_exits(std::string_view path) {
        if (path.empty()) {
            return turbo::invalid_argument_error("file path is empty");
        }

        std::error_code ec;
        const fs::path file_path(path);

        if (!fs::exists(file_path, ec)) {
            if (ec) {
                return turbo::internal_error(
                    turbo::str_sprintf("failed to check file `%s`: %s",
                                       path, ec.message()));
            }
            return turbo::not_found_error(
                turbo::str_sprintf("file `%s` does not exist", path));
        }

        if (!fs::is_regular_file(file_path, ec) || ec) {
            return turbo::invalid_argument_error(
                turbo::str_sprintf("path `%s` is not a regular file", path));
        }

        return turbo::ok_status();
    }

} // namespace shark::common
