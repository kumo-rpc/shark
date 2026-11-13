#include <shark/compiler/cpp/any_field.h>

#include <shark/compiler/cpp/helpers.h>
#include <shark/io/printer.h>

namespace shark::compiler::cpp {

    namespace {

        void SetAnyVariables(const FieldDescriptor* descriptor,
            std::map<std::string, std::string>* variables,
            const Options& options) {
            SetCommonFieldVariables(descriptor, variables, options);
            (*variables)["any"] = "::shark::Any";
        }

    } // namespace

    AnyFieldGenerator::AnyFieldGenerator(const FieldDescriptor* descriptor,
        const Options& options)
        : FieldGenerator(descriptor, options) {
        SetAnyVariables(descriptor, &variables_, options);
    }

    AnyFieldGenerator::~AnyFieldGenerator() { }

    void AnyFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$any$ $name$_;\n");
    }

    void AnyFieldGenerator::GenerateAccessorDeclarations(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "$deprecated_attr$const $any$& $name$() const;\n"
            "$deprecated_attr$$any$* mutable_$name$();\n"
            "private:\n"
            "const $any$& _internal_$name$() const;\n"
            "public:\n");
    }

    void AnyFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline const $any$& $classname$::_internal_$name$() const {\n"
            "  return $field$;\n"
            "}\n"
            "inline const $any$& $classname$::$name$() const {\n"
            "  return _internal_$name$();\n"
            "}\n"
            "inline $any$* $classname$::mutable_$name$() {\n"
            "  $set_hasbit$\n"
            "  $field$.set_arena(GetArenaForAllocation());\n"
            "  return &$field$;\n"
            "}\n");
    }

    void AnyFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.Clear();\n");
    }

    void AnyFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "if (from._internal_has_$name$()) {\n"
            "  _this->mutable_$name$()->MergeFrom(from._internal_$name$());\n"
            "}\n");
    }

    void AnyFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.InternalSwap(&other->$field$);\n");
    }

    void AnyFieldGenerator::GenerateConstructorCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.InitDefault();\n");
    }

    void AnyFieldGenerator::GenerateCopyConstructorCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "_this->$field$.InitDefault();\n"
            "if (!from.$field$.empty()) {\n"
            "  _this->$field$.MergeFrom(from.$field$);\n"
            "}\n");
    }

    void AnyFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.Destroy();\n");
    }

    void AnyFieldGenerator::GenerateSerializeWithCachedSizesToArray(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "target = this->_internal_$name$()._InternalSerialize(\n"
            "    $number$, target, stream);\n");
    }

    void AnyFieldGenerator::GenerateByteSize(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "total_size += $tag_size$ +\n"
            "    ::shark::internal::WireFormatLite::LengthDelimitedSize(\n"
            "        this->_internal_$name$().ByteSizeLong());\n");
    }

    void AnyFieldGenerator::GenerateConstexprAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("/*decltype($field$)*/{::_pbi::ConstantInitialized{}}");
    }

    void AnyFieldGenerator::GenerateAggregateInitializer(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("decltype($field$){arena}");
    }

    void AnyFieldGenerator::GenerateCopyAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("decltype($field$){from.$field$}");
    }

    AnyOneofFieldGenerator::AnyOneofFieldGenerator(const FieldDescriptor* descriptor,
        const Options& options)
        : AnyFieldGenerator(descriptor, options) {
        SetCommonOneofFieldVariables(descriptor, &variables_);
    }

    AnyOneofFieldGenerator::~AnyOneofFieldGenerator() { }

    void AnyOneofFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline const $any$& $classname$::_internal_$name$() const {\n"
            "  return $field$;\n"
            "}\n"
            "inline const $any$& $classname$::$name$() const {\n"
            "  return _internal_has_$name$() ? $field$\n"
            "                               : ::shark::Any::default_instance();\n"
            "}\n"
            "inline $any$* $classname$::mutable_$name$() {\n"
            "  if (!_internal_has_$name$()) {\n"
            "    clear_$oneof_name$();\n"
            "    set_has_$name$();\n"
            "    $field$.InitDefault();\n"
            "  }\n"
            "  $field$.set_arena(GetArenaForAllocation());\n"
            "  return &$field$;\n"
            "}\n");
    }

    void AnyOneofFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.Destroy();\n");
    }

    void AnyOneofFieldGenerator::GenerateSwappingCode(io::Printer* printer) const { }

    void AnyOneofFieldGenerator::GenerateConstructorCode(io::Printer* printer) const { }

    void AnyOneofFieldGenerator::GenerateDestructorCode(io::Printer* printer) const { }

    RepeatedAnyFieldGenerator::RepeatedAnyFieldGenerator(
        const FieldDescriptor* descriptor, const Options& options)
        : FieldGenerator(descriptor, options) {
        SetAnyVariables(descriptor, &variables_, options);
    }

    RepeatedAnyFieldGenerator::~RepeatedAnyFieldGenerator() { }

    void RepeatedAnyFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("::$proto_ns$::RepeatedPtrField< $any$ > $name$_;\n");
    }

    void RepeatedAnyFieldGenerator::GenerateAccessorDeclarations(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "$deprecated_attr$const $any$& $name$(int index) const;\n"
            "$deprecated_attr$$any$* mutable_$name$(int index);\n"
            "$deprecated_attr$$any$* add_$name$();\n"
            "$deprecated_attr$const ::$proto_ns$::RepeatedPtrField< $any$ >&\n"
            "    $name$() const;\n"
            "$deprecated_attr$::$proto_ns$::RepeatedPtrField< $any$ >*\n"
            "    mutable_$name$();\n"
            "private:\n"
            "const $any$& _internal_$name$(int index) const;\n"
            "$any$* _internal_add_$name$();\n"
            "public:\n");
    }

    void RepeatedAnyFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline const $any$& $classname$::_internal_$name$(int index) const {\n"
            "  return $field$.Get(index);\n"
            "}\n"
            "inline const $any$& $classname$::$name$(int index) const {\n"
            "  return _internal_$name$(index);\n"
            "}\n"
            "inline $any$* $classname$::mutable_$name$(int index) {\n"
            "  return $field$.Mutable(index);\n"
            "}\n"
            "inline $any$* $classname$::_internal_add_$name$() {\n"
            "  auto* any = $field$.Add();\n"
            "  any->set_arena(GetArenaForAllocation());\n"
            "  return any;\n"
            "}\n"
            "inline $any$* $classname$::add_$name$() {\n"
            "  return _internal_add_$name$();\n"
            "}\n"
            "inline const ::$proto_ns$::RepeatedPtrField< $any$ >&\n"
            "$classname$::$name$() const {\n"
            "  return $field$;\n"
            "}\n"
            "inline ::$proto_ns$::RepeatedPtrField< $any$ >*\n"
            "$classname$::mutable_$name$() {\n"
            "  return &$field$;\n"
            "}\n");
    }

    void RepeatedAnyFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.Clear();\n");
    }

    void RepeatedAnyFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("_this->$field$.MergeFrom(from.$field$);\n");
    }

    void RepeatedAnyFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.InternalSwap(&other->$field$);\n");
    }

    void RepeatedAnyFieldGenerator::GenerateConstructorCode(io::Printer* printer) const { }

    void RepeatedAnyFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.~RepeatedPtrField();\n");
    }

    void RepeatedAnyFieldGenerator::GenerateSerializeWithCachedSizesToArray(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "for (int i = 0, n = this->$field$.size(); i < n; i++) {\n"
            "  target = this->$field$.Get(i)._InternalSerialize(\n"
            "      $number$, target, stream);\n"
            "}\n");
    }

    void RepeatedAnyFieldGenerator::GenerateByteSize(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "total_size += $tag_size$ * this->$field$.size();\n"
            "for (int i = 0, n = this->$field$.size(); i < n; i++) {\n"
            "  total_size += ::shark::internal::WireFormatLite::LengthDelimitedSize(\n"
            "      this->$field$.Get(i).ByteSizeLong());\n"
            "}\n");
    }

} // namespace shark::compiler::cpp
