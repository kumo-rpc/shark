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

#include <shark/compiler/code_generator.h>
#include <string>

// Must be included last.
#include <shark/port_def.inc>

namespace shark::compiler::cc {

    // CodeGenerator implementation which generates a C++ source file and
    // header.  If you create your own protocol compiler binary and you want
    // it to support C++ output, you can do so by registering an instance of this
    // CodeGenerator with the CommandLineInterface in your main() function.
    class PROTOC_EXPORT CppGenerator : public CodeGenerator {
    public:
        CppGenerator();
        ~CppGenerator() override;

        enum class Runtime {
            kGoogle3, // Use the internal google3 runtime.
            kOpensource, // Use the open-source runtime.

            // Use the open-source runtime with google3 #include paths.  We make these
            // absolute to avoid ambiguity, so the runtime will be #included like:
            //   #include "third_party/protobuf/.../shark/message.h"
            kOpensourceGoogle3
        };

        void set_opensource_runtime(bool opensource) {
            opensource_runtime_ = opensource;
        }

        // If set to a non-empty string, generated code will do:
        //   #include "<BASE>/shark/message.h"
        // instead of:
        //   #include <shark/message.h>
        // This has no effect if opensource_runtime = false.
        void set_runtime_include_base(const std::string& base) {
            runtime_include_base_ = base;
        }

        // implements CodeGenerator ----------------------------------------
        bool Generate(const FileDescriptor* file, const std::string& parameter,
            GeneratorContext* generator_context,
            std::string* error) const override;

        uint64_t GetSupportedFeatures() const override {
            // We don't fully support this yet, but this is needed to unblock the tests,
            // and we will have full support before the experimental flag is removed.
            return FEATURE_PROTO3_OPTIONAL;
        }

    private:
        bool opensource_runtime_ = true;
        std::string runtime_include_base_;
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(CppGenerator);
    };

} // namespace shark::compiler::cc

#include <shark/port_undef.inc>
