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

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <shark/endian.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark::internal {

    // Used for tags, could read up to 5 bytes which must be available.
    // Caller must ensure its safe to call.
    SHARK_EXPORT std::pair<const char*, uint32_t> ReadTagFallback(const char* p, uint32_t res);


    // Same as ParseVarint but only accept 5 bytes at most.
    inline const char* ReadTag(const char* p, uint32_t* out,
        uint32_t /*max_tag*/ = 0) {
        uint32_t res = static_cast<uint8_t>(p[0]);
        if (res < 128) {
            *out = res;
            return p + 1;
        }
        uint32_t second = static_cast<uint8_t>(p[1]);
        res += (second - 1) << 7;
        if (second < 128) {
            *out = res;
            return p + 2;
        }
        auto tmp = ReadTagFallback(p, res);
        *out = tmp.second;
        return tmp.first;
    }


    inline uint32_t field_number(uint32_t tag) {
        return tag >> 3;
    }

    inline uint32_t wire_type(uint32_t tag) {
        return tag & 7;
    }


    static constexpr uint32_t WIRE_TYPE_VARINT = 0u;
    static constexpr uint32_t WIRE_TYPE_FIXED64 = 1u;
    static constexpr uint32_t WIRE_TYPE_LENGTH_DELIMITED = 2u;
    static constexpr uint32_t WIRE_TYPE_START_GROUP = 3u;
    static constexpr uint32_t WIRE_TYPE_END_GROUP = 4u;
    static constexpr uint32_t WIRE_TYPE_FIXED32 = 5u;
}  // namespace shark


#include <shark/port_undef.inc>
