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

#include <filesystem>
#include <turbo/status/result.h>
namespace shark::common {

    turbo::Result<std::string> absolute_path(std::string_view path);

    /// Resolves `path` to a normalized absolute path against an ordered list of prefixes.
    ///
    /// - If `path` is already absolute, validates that it resides under at least one
    ///   directory in `prefix`; returns a NotFound error otherwise.
    /// - If `path` is relative, probes each directory in `prefix` in order and returns
    ///   the normalized absolute path of the first existing match (preventing "../" escape).
    ///
    /// @param path   Target file or directory path (absolute or relative).
    /// @param prefix Ordered list of allowed base directories.
    /// @return The normalized generic absolute path on success, or an error Status.

    turbo::Result<std::string> absolute_path(std::string_view path,
        const std::vector<std::string> &prefix);


    /// not found
    turbo::Result<std::string> relative_path(std::string_view path,
        const std::vector<std::string> &prefix, std::string *prefix_out);

    /// Checks whether a regular file exists at `path`.
    ///
    /// @param path Target file path to verify.
    /// @return OkStatus if the path exists and is a regular file, or an error Status otherwise.
    turbo::Status check_file_exits(std::string_view path);

    /// Reads the entire contents of a small file at `path` into `out`.
    ///
    /// Validates that the target exists and is a regular file, pre-allocates
    /// the output buffer to avoid repeated reallocations, and reads in binary mode.
    ///
    /// @param path Path to the target file.
    /// @param out  Output buffer overwritten with the file contents on success.
    /// @return OkStatus on success, or an appropriate error Status on failure.
    turbo::Status read_file(std::string_view path, std::string &out);

}  // namespace shark::common
