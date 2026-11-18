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

#include <string_view>
#include <string>
#include <cstdint>
#include <turbo/strings/ascii.h>

namespace shark {



    std::string ToCamelCase(const std::string& input, bool lower_first);

    std::string ToJsonName(const std::string& input);


   inline  bool is_upper_camel_case(const std::string& name) {
        if (name.empty()) {
            return true;
        }
        // Name must start with an upper case character.
        if (!turbo::ascii_isupper(name[0])) {
            return false;
        }
        // Must not contains underscore.
        for (const char c : name) {
            if (c == '_') {
                return false;
            }
        }
        return true;
    }

    inline bool is_upper_underscore(const std::string& name) {
        for (const char c : name) {
            if (!turbo::ascii_isupper(c) && c != '_' && !turbo::ascii_isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool is_lower_underscore(const std::string& name) {
        for (const char c : name) {
            if (!turbo::ascii_islower(c) && c != '_' && !turbo::ascii_isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool is_number_follow_underscore(const std::string& name) {
        for (int i = 1; i < name.length(); i++) {
            const char c = name[i];
            if (turbo::ascii_isdigit(c) && name[i - 1] == '_') {
                return true;
            }
        }
        return false;
    }

    inline  std::string ToLowercaseWithoutUnderscores(const std::string& name) {
       std::string result;
       for (char character : name) {
           if (character != '_') {
               if (character >= 'A' && character <= 'Z') {
                   result.push_back(character - 'A' + 'a');
               } else {
                   result.push_back(character);
               }
           }
       }
       return result;
   }
}  // namespace shark
