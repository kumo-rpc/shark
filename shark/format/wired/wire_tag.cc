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

#include <shark/format/wired/wire_tag.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark::internal {


    std::pair<const char*, uint32_t> ReadTagFallback(const char* p, uint32_t res) {
        for (std::uint32_t i = 2; i < 5; i++) {
            uint32_t byte = static_cast<uint8_t>(p[i]);
            res += (byte - 1) << (7 * i);
            if (SHARK_PREDICT_TRUE(byte < 128)) {
                return { p + i + 1, res };
            }
        }
        return { nullptr, 0 };
    }


}  // namespace shark


#include <shark/port_undef.inc>
