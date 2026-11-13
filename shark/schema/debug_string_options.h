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

#include <string>
#include <vector>

namespace shark {

    // Options when generating machine-parsable output from a descriptor with
    // DebugString().
    struct DebugStringOptions {
        // include original user comments as recorded in SourceLocation entries. N.B.
        // that this must be |false| by default: several other pieces of code (for
        // example, the C++ code generation for fields in the proto compiler) rely on
        // DebugString() output being unobstructed by user comments.
        bool include_comments;
        // If true, elide the braced body in the debug string.
        bool elide_group_body;
        bool elide_oneof_body;

        DebugStringOptions()
            : include_comments(false)
            , elide_group_body(false)
            , elide_oneof_body(false) {
        }
    };
}  // namespace shark
