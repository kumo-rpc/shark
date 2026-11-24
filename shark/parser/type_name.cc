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

#include <shark/parser/type_name.h>
#include <string>

namespace shark::parser {

    TypeName::TypeName() = default;
    TypeName::~TypeName() = default;
    TypeName::TypeName(const TypeName& other) = default;
    TypeName& TypeName::operator=(const TypeName& other) = default;
    TypeName::TypeName(TypeName&& other) noexcept = default;
    TypeName& TypeName::operator=(TypeName&& other) noexcept = default;

    namespace {

        void append_type_ref(std::string& out, const TypeRef& ref) {
            if (const auto* plain = std::get_if<std::string_view>(&ref)) {
                out.append(plain->data(), plain->size());
                return;
            }
            const TypeName& type = std::get<TypeName>(ref);
            out.append(type.name.data(), type.name.size());
            out.push_back('<');
            for (std::size_t i = 0; i < type.args.size(); ++i) {
                if (i != 0) {
                    out.append(", ");
                }
                append_type_ref(out, type.args[i].value);
            }
            out.push_back('>');
        }

    }  // namespace

    std::string format_type_ref(const TypeRef& ref) {
        std::string out;
        append_type_ref(out, ref);
        return out;
    }

}  // namespace shark::parser
