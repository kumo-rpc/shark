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

#include <shark/schema/uri.h>

#include <filesystem>

namespace shark {

    std::string package_to_cpp_namespace(std::string_view package) {
        std::string out;
        out.reserve(package.size());
        for (char c : package) {
            if (c == '.') {
                out.append("::");
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    turbo::Result<std::string> path_to_cpp_file(std::string_view path, bool cc) {
        if (path.empty()) {
            return turbo::invalid_argument_error("path is empty");
        }
        if (std::filesystem::path(path).is_absolute()) {
            return turbo::invalid_argument_error("path is absolute");
        }
        std::string file(path);
        constexpr std::string_view kSuffix = ".shark";
        if (file.size() >= kSuffix.size()
            && file.compare(file.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
            file.resize(file.size() - kSuffix.size());
            file.append(cc ? ".sk.cc" : ".sk.h");
        }
        return file;
    }

}  // namespace shark
