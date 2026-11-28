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

#include <shark/schema/schema_pool_api.h>

#include <string>
#include <string_view>
#include <vector>

namespace shark {

    class Annotations {
    public:
        /// Drops one matching wrapper at a time: `<>`, `""`, or `''`.
        /// `<cstdint>` and `"cstdint"` both become `cstdint`.
        static std::string normalize_include(std::string_view text);

        ////////////////////////////////////////////////
        /// Object level. One type names its own include.
        ///
        /// annotation.cpp include = "cstdint";
        /// type int32;
        ///
        /// Returns `cstdint`.
        ///
        ////////////////////////////////////////////////
        static std::string cpp_include(const SchemaType* type);

        ////////////////////////////////////////////////
        /// File level. The same include on several types is written once here.
        /// `@` marks the annotation as file level.
        ///
        /// annotation.cpp include = "cstdint";
        /// type int8;
        /// annotation.cpp include = "cstdint";
        /// type int16;
        /// annotation.cpp include = "cstdint";
        /// type int32;
        ///
        /// is the same include three times. Write it once on the file:
        ///
        /// annotation@.cpp include = "cstdint";
        /// type int8;
        /// type int16;
        /// type int32;
        ///
        /// Returns `cstdint`.
        ///
        ////////////////////////////////////////////////
        static std::vector<std::string> cpp_includes(const SchemaFile* file);
    };

}  // namespace shark
