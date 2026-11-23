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

#include <shark/parser/node.h>
#include <shark/parser/file.h>

namespace shark::parser {

     std::string_view Node::get_absolute_path() const {
         if (!file) {
             return "";
         }
         return file->absolute_path;
     }
     std::string_view Node::get_path() const {
         if (!file) {
             return "";
         }
         return file->path;
     }

    std::string Node::make_node_info(ErrorType err) const {
        return turbo::str_sprintf("%s:%d:%d [%s]",
            get_absolute_path(),
            start_location.lineno,
            start_location.column,
            to_string(err)
            );
    }

}  // namespace shark::parser
