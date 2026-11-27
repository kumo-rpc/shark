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

#include <shark/parser/fwd.h>
#include <shark/schema/schema_defines.h>

#include <optional>
#include <string_view>

namespace shark::parser {

    /// Registers builtin scalar types on `pool`. Nodes are allocated by the pool.
    bool load_builtin_types(RegistryPool& pool);

    /// Maps a builtin phy_type annotation value, such as "INT32", to PhysicalType.
    std::optional<PhysicalType> physical_type(std::string_view name);

}  // namespace shark::parser
