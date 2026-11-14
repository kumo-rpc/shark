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

    // ----------------------------------------------------------------------
    // ascii_isalnum()
    //    Check if an ASCII character is alphanumeric.  We can't use ctype's
    //    isalnum() because it is affected by locale.  This function is applied
    //    to identifiers in the protocol buffer language, not to natural-language
    //    strings, so locale should not be taken into account.
    // ascii_isdigit()
    //    Like above, but only accepts digits.
    // ascii_is_space()
    //    Check if the character is a space character.
    // ----------------------------------------------------------------------

    inline bool ascii_isalnum(char c) {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9');
    }

    inline bool ascii_isdigit(char c) {
        return ('0' <= c && c <= '9');
    }

    inline bool ascii_is_space(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    }

    inline bool ascii_is_upper(char c) {
        return c >= 'A' && c <= 'Z';
    }

    inline bool ascii_is_lower(char c) {
        return c >= 'a' && c <= 'z';
    }

    inline int hex_digit_to_int(char c) {
        /* Assume ASCII. */
        int x = static_cast<unsigned char>(c);
        if (x > '9') {
            x += 9;
        }
        return x & 0xf;
    }

    // Note:  I distrust ctype.h due to locales.
    inline char ascii_to_upper(char ch) {
        return (ch >= 'a' && ch <= 'z') ? (ch - 'a' + 'A') : ch;
    }

    inline char ascii_to_lower(char ch) {
        return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
    }

    std::string ToCamelCase(const std::string& input, bool lower_first);

    std::string ToJsonName(const std::string& input);


   inline  bool IsUpperCamelCase(const std::string& name) {
        if (name.empty()) {
            return true;
        }
        // Name must start with an upper case character.
        if (!ascii_is_upper(name[0])) {
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
            if (!ascii_is_upper(c) && c != '_' && !ascii_isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool IsLowerUnderscore(const std::string& name) {
        for (const char c : name) {
            if (!ascii_is_lower(c) && c != '_' && !ascii_isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    inline bool IsNumberFollowUnderscore(const std::string& name) {
        for (int i = 1; i < name.length(); i++) {
            const char c = name[i];
            if (ascii_isdigit(c) && name[i - 1] == '_') {
                return true;
            }
        }
        return false;
    }
}  // namespace shark
