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

#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <shark/common/filepath.h>
#include <turbo/log/logging.h>

namespace shark::parser {

    void RegistryPool::add_file(FileNode *ptr) {
        std::string_view path = ptr->path;
        _files[path] = ptr;
    }

    bool RegistryPool::build_file(std::string_view file, bool recursive) {
        (void)recursive;

        std::string prefix;
        auto rel = shark::common::relative_path(file, includes(), &prefix);
        if (!rel.ok()) {
            KLOG(INFO)<<rel;
            return false;
        }

        const std::string absolute = prefix + "/" + *rel;
        if (get_file(*rel) != nullptr) {
            return true;
        }

        auto* node = create_symbol<FileNode>(nullptr);
        if (!node->load(absolute, *rel)) {
            return false;
        }
        add_file(node);
        return node->parse();
    }
}  // namespace shark::parser
