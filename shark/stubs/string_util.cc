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


#include <shark/stubs/string_util.h>

namespace shark {

    std::string ToCamelCase(const std::string& input, bool lower_first) {
        bool capitalize_next = !lower_first;
        std::string result;
        result.reserve(input.size());

        for (char character : input) {
            if (character == '_') {
                capitalize_next = true;
            } else if (capitalize_next) {
                result.push_back(ToUpper(character));
                capitalize_next = false;
            } else {
                result.push_back(character);
            }
        }

        // Lower-case the first letter.
        if (lower_first && !result.empty()) {
            result[0] = ToLower(result[0]);
        }

        return result;
    }

    std::string ToJsonName(const std::string& input) {
        bool capitalize_next = false;
        std::string result;
        result.reserve(input.size());

        for (char character : input) {
            if (character == '_') {
                capitalize_next = true;
            } else if (capitalize_next) {
                result.push_back(ToUpper(character));
                capitalize_next = false;
            } else {
                result.push_back(character);
            }
        }

        return result;
    }

}  // namespace shark
