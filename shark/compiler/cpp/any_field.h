#pragma once

#include <map>
#include <string>

#include <shark/compiler/cpp/field.h>

namespace shark::compiler::cpp {

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

} // namespace shark::compiler::cpp
