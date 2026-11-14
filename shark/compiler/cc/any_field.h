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

#include <shark/compiler/cc/field.h>

namespace shark::compiler::cc {

    class AnyFieldGenerator : public FieldGenerator {
    public:
        AnyFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
        ~AnyFieldGenerator() override;

        void GeneratePrivateMembers(io::Printer* printer) const override;
        void GenerateAccessorDeclarations(io::Printer* printer) const override;
        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateMergingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override;
        void GenerateCopyConstructorCode(io::Printer* printer) const override;
        void GenerateDestructorCode(io::Printer* printer) const override;
        void GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const override;
        void GenerateByteSize(io::Printer* printer) const override;
        void GenerateConstexprAggregateInitializer(io::Printer* printer) const override;
        void GenerateAggregateInitializer(io::Printer* printer) const override;
        void GenerateCopyAggregateInitializer(io::Printer* printer) const override;
    };

    class AnyOneofFieldGenerator : public AnyFieldGenerator {
    public:
        AnyOneofFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
        ~AnyOneofFieldGenerator() override;

        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override;
        void GenerateDestructorCode(io::Printer* printer) const override;
    };

    class RepeatedAnyFieldGenerator : public FieldGenerator {
    public:
        RepeatedAnyFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
        ~RepeatedAnyFieldGenerator() override;

        void GeneratePrivateMembers(io::Printer* printer) const override;
        void GenerateAccessorDeclarations(io::Printer* printer) const override;
        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateMergingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override;
        void GenerateDestructorCode(io::Printer* printer) const override;
        void GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const override;
        void GenerateByteSize(io::Printer* printer) const override;
    };

} // namespace shark::compiler::cc
