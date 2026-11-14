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

#include <map>
#include <string>

#include <shark/compiler/cc/options.h>
#include <shark/stubs/common.h>

namespace shark {
    class FieldDescriptor; // descriptor.h
    namespace io {
        class Printer; // printer.h
    }
} // namespace shark

namespace shark::compiler::cc {


            class MessageSCCAnalyzer;

            // Generates code for an extension, which may be within the scope of some
            // message or may be at file scope.  This is much simpler than FieldGenerator
            // since extensions are just simple identifiers with interesting types.
            class ExtensionGenerator {
            public:
                // See generator.cc for the meaning of dllexport_decl.
                explicit ExtensionGenerator(const FieldDescriptor* descriptor,
                    const Options& options,
                    MessageSCCAnalyzer* scc_analyzer);
                ~ExtensionGenerator();

                // Header stuff.
                void GenerateDeclaration(io::Printer* printer) const;

                // Source file stuff.
                void GenerateDefinition(io::Printer* printer);

                bool IsScoped() const;

            private:
                const FieldDescriptor* descriptor_;
                std::string type_traits_;
                Options options_;
                MessageSCCAnalyzer* scc_analyzer_;

                std::map<std::string, std::string> variables_;

                SHARK_DISALLOW_EVIL_CONSTRUCTORS(ExtensionGenerator);
            };

}  // namespace shark::compiler::cc

