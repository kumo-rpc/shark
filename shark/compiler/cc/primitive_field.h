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

    class PrimitiveFieldGenerator : public FieldGenerator {
    public:
        PrimitiveFieldGenerator(const FieldDescriptor* descriptor,
            const Options& options);
        ~PrimitiveFieldGenerator() override;

        // implements FieldGenerator ---------------------------------------
        void GeneratePrivateMembers(io::Printer* printer) const override;
        void GenerateAccessorDeclarations(io::Printer* printer) const override;
        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateMergingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override { }
        void GenerateCopyConstructorCode(io::Printer* printer) const override;
        void GenerateSerializeWithCachedSizesToArray(
            io::Printer* printer) const override;
        void GenerateByteSize(io::Printer* printer) const override;
        void GenerateConstexprAggregateInitializer(
            io::Printer* printer) const override;
        void GenerateAggregateInitializer(io::Printer* printer) const override;
        void GenerateCopyAggregateInitializer(io::Printer* printer) const override;

    private:
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(PrimitiveFieldGenerator);
    };

    class PrimitiveOneofFieldGenerator : public PrimitiveFieldGenerator {
    public:
        PrimitiveOneofFieldGenerator(const FieldDescriptor* descriptor,
            const Options& options);
        ~PrimitiveOneofFieldGenerator() override;

        // implements FieldGenerator ---------------------------------------
        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override;

    private:
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(PrimitiveOneofFieldGenerator);
    };

    class RepeatedPrimitiveFieldGenerator : public FieldGenerator {
    public:
        RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor,
            const Options& options);
        ~RepeatedPrimitiveFieldGenerator() override;

        // implements FieldGenerator ---------------------------------------
        void GeneratePrivateMembers(io::Printer* printer) const override;
        void GenerateAccessorDeclarations(io::Printer* printer) const override;
        void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
        void GenerateClearingCode(io::Printer* printer) const override;
        void GenerateMergingCode(io::Printer* printer) const override;
        void GenerateSwappingCode(io::Printer* printer) const override;
        void GenerateConstructorCode(io::Printer* printer) const override { }
        void GenerateCopyConstructorCode(io::Printer* /*printer*/) const override {
            GOOGLE_CHECK(!ShouldSplit(descriptor_, options_));
        }
        void GenerateDestructorCode(io::Printer* printer) const override;
        void GenerateSerializeWithCachedSizesToArray(
            io::Printer* printer) const override;
        void GenerateByteSize(io::Printer* printer) const override;
        void GenerateConstexprAggregateInitializer(
            io::Printer* printer) const override;
        void GenerateAggregateInitializer(io::Printer* printer) const override;
        void GenerateCopyAggregateInitializer(io::Printer* printer) const override;

    private:
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(RepeatedPrimitiveFieldGenerator);
    };

} // namespace shark::compiler::cc
