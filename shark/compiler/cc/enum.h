// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
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
#include <set>
#include <string>

#include <shark/compiler/cc/options.h>
#include <shark/descriptor.h>

namespace shark {
    namespace io {
        class Printer; // printer.h
    }
} // namespace shark

namespace shark::compiler::cc {

            class EnumGenerator {
            public:
                // See generator.cc for the meaning of dllexport_decl.
                EnumGenerator(const EnumDescriptor* descriptor,
                    const std::map<std::string, std::string>& vars,
                    const Options& options);
                ~EnumGenerator();

                // Generate header code defining the enum.  This code should be placed
                // within the enum's package namespace, but NOT within any class, even for
                // nested enums.
                void GenerateDefinition(io::Printer* printer, bool inner = false);

                // Generate specialization of EnumTraitImpl<MyEnum>().
                // Precondition: in shark namespace.
                void GenerateGetEnumDescriptorSpecializations(io::Printer* printer);

                // For enums nested within a message, generate code to import all the enum's
                // symbols (e.g. the enum type name, all its values, etc.) into the class's
                // namespace.  This should be placed inside the class definition in the
                // header.
                void GenerateSymbolImports(io::Printer* printer) const;

                // Source file stuff.

                // Generate non-inline methods related to the enum, such as IsValidValue().
                // Goes in the .cc file. EnumDescriptors are stored in an array, idx is
                // the index in this array that corresponds with this enum.
                void GenerateMethods(int idx, io::Printer* printer);
                void generate_template_methods(int idx, io::Printer* printer);

            private:
                const EnumDescriptor* descriptor_;
                const std::string classname_;
                const Options& options_;
                // whether to generate the *_ARRAYSIZE constant.
                const bool generate_array_size_;

                std::map<std::string, std::string> variables_;

                const EnumValueDescriptor* min_value_;
                const EnumValueDescriptor* max_value_;
                bool inner_ = false;

                friend class FileGenerator;
                SHARK_DISALLOW_EVIL_CONSTRUCTORS(EnumGenerator);
            };

} // namespace namespace shark::compiler::cc
