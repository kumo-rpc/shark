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

#include <shark/compiler/cpp/servlet.h>

#include <map>
#include <string>

namespace shark::compiler::cpp {

    void ServletGenerator::generate_fwd_declare(io::Printer& printer) {
        if (_servlet->container() != nullptr) {
            return;
        }
        const SchemaTypeName* type_name = _servlet->type_name();
        if (type_name == nullptr || type_name->name().empty() || type_name->argument_size() != 0) {
            return;
        }
        std::map<std::string, std::string> vars;
        vars["name"] = type_name->name();
        printer.print(vars, "class $name$;\n");
    }

    void ServletGenerator::generate_generic_fwd_declare(io::Printer& printer) {
        if (_servlet->container() != nullptr) {
            return;
        }
        const SchemaTypeName* type_name = _servlet->type_name();
        if (type_name == nullptr || type_name->name().empty() || type_name->argument_size() == 0) {
            return;
        }
        std::string params;
        for (uint32_t i = 0; i < type_name->argument_size(); ++i) {
            const SchemaTypeName* argument = type_name->argument(i);
            if (argument == nullptr || argument->name().empty()) {
                return;
            }
            if (!params.empty()) {
                params.append(", ");
            }
            params.append("typename ");
            params.append(argument->name());
        }
        std::map<std::string, std::string> vars;
        vars["params"] = params;
        vars["name"] = type_name->name();
        printer.print(vars, "template <$params$>\n");
        printer.print(vars, "class $name$;\n");
    }

}  // namespace shark::compiler::cpp
