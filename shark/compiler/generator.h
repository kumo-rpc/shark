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

#include <shark/schema/schema_pool.h>
#include <shark/schema/schema_pool_api.h>
#include <turbo/flags/declare.h>
#include <shark/io/printer.h>

#include <string>
#include <vector>

TURBO_DECLARE_FLAG(bool, debug);
TURBO_DECLARE_FLAG(std::string, file);
TURBO_DECLARE_FLAG(std::vector<std::string>, include);
TURBO_DECLARE_FLAG(std::string, lang);
TURBO_DECLARE_FLAG(std::string, out);

namespace shark::compiler {

    class Generator {
    public:
        virtual ~Generator() = default;

        virtual bool generate_code(const SchemaFile* file) = 0;
    };

}  // namespace shark::compiler

