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

namespace shark {

    // Note:  I distrust ctype.h due to locales.
    inline char ToUpper(char ch) {
        return (ch >= 'a' && ch <= 'z') ? (ch - 'a' + 'A') : ch;
    }

    inline char ToLower(char ch) {
        return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
    }

    std::string ToCamelCase(const std::string& input, bool lower_first);

    std::string ToJsonName(const std::string& input);

    inline bool IsUppercase(char c) {
        return c >= 'A' && c <= 'Z';
    }

    inline bool IsLowercase(char c) {
        return c >= 'a' && c <= 'z';
    }

    inline bool IsNumber(char c) {
        return c >= '0' && c <= '9';
    }

   inline  bool IsUpperCamelCase(const std::string& name) {
        if (name.empty()) {
            return true;
        }
        // Name must start with an upper case character.
        if (!IsUppercase(name[0])) {
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

    inline bool IsUpperUnderscore(const std::string& name) {
        for (const char c : name) {
            if (!IsUppercase(c) && c != '_' && !IsNumber(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool IsLowerUnderscore(const std::string& name) {
        for (const char c : name) {
            if (!IsLowercase(c) && c != '_' && !IsNumber(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool IsNumberFollowUnderscore(const std::string& name) {
        for (int i = 1; i < name.length(); i++) {
            const char c = name[i];
            if (IsNumber(c) && name[i - 1] == '_') {
                return true;
            }
        }
        return false;
    }
}  // namespace shark
