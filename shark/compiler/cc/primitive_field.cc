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

#include <shark/compiler/cc/primitive_field.h>

#include <shark/compiler/cc/helpers.h>
#include <shark/format/wired/wire_format.h>
#include <shark/io/printer.h>
#include <shark/stubs/strutil.h>

namespace shark::compiler::cc {

    using internal::WireFormatLite;

    namespace {

        // For encodings with fixed sizes, returns that size in bytes.  Otherwise
        // returns -1.
        int FixedSize(SharkFieldType type) {
            switch (type) {
            case SharkFieldType::INT32:
            case SharkFieldType::INT8:
            case SharkFieldType::UINT8:
            case SharkFieldType::INT16:
            case SharkFieldType::UINT16:
                return -1;
            case SharkFieldType::INT64:
                return -1;
            case SharkFieldType::UINT32:
                return -1;
            case SharkFieldType::UINT64:
                return -1;
            case SharkFieldType::SINT32:
                return -1;
            case SharkFieldType::SINT64:
                return -1;
            case SharkFieldType::FIXED32:
                return WireFormatLite::kFixed32Size;
            case SharkFieldType::FIXED64:
                return WireFormatLite::kFixed64Size;
            case SharkFieldType::SFIXED32:
                return WireFormatLite::kSFixed32Size;
            case SharkFieldType::SFIXED64:
                return WireFormatLite::kSFixed64Size;
            case SharkFieldType::FLOAT:
                return WireFormatLite::kFloatSize;
            case SharkFieldType::DOUBLE:
                return WireFormatLite::kDoubleSize;

            case SharkFieldType::BOOL:
                return WireFormatLite::kBoolSize;
            case SharkFieldType::ENUM:
                return -1;

            case SharkFieldType::STRING:
                return -1;
            case SharkFieldType::BYTES:
                return -1;
            case SharkFieldType::MESSAGE:
                return -1;

                // No default because we want the compiler to complain if any new
                // types are added.
            }
            GOOGLE_LOG(FATAL) << "Can't get here.";
            return -1;
        }

        void SetPrimitiveVariables(const FieldDescriptor* descriptor,
            std::map<std::string, std::string>* variables,
            const Options& options) {
            SetCommonFieldVariables(descriptor, variables, options);
            (*variables)["type"] = PrimitiveTypeName(options, descriptor->cpp_type());
            (*variables)["default"] = DefaultValue(options, descriptor);
            (*variables)["cached_byte_size_name"] = MakeVarintCachedSizeName(descriptor);
            bool cold = ShouldSplit(descriptor, options);
            (*variables)["cached_byte_size_field"] = MakeVarintCachedSizeFieldName(descriptor, cold);
            (*variables)["tag"] = StrCat(internal::WireFormat::MakeTag(descriptor));
            int fixed_size = FixedSize(descriptor->type());
            if (fixed_size != -1) {
                (*variables)["fixed_size"] = StrCat(fixed_size);
            }
            (*variables)["wire_format_field_type"] = SharkTypeTrait::enum_name(
                static_cast<SharkType>(descriptor->type()));
            (*variables)["full_name"] = descriptor->full_name();
        }

    } // namespace

    // ===================================================================

    PrimitiveFieldGenerator::PrimitiveFieldGenerator(
        const FieldDescriptor* descriptor, const Options& options)
        : FieldGenerator(descriptor, options) {
        SetPrimitiveVariables(descriptor, &variables_, options);
    }

    PrimitiveFieldGenerator::~PrimitiveFieldGenerator() { }

    void PrimitiveFieldGenerator::GeneratePrivateMembers(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$type$ $name$_;\n");
    }

    void PrimitiveFieldGenerator::GenerateAccessorDeclarations(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "$deprecated_attr$$type$ ${1$$name$$}$() const;\n"
            "$deprecated_attr$void ${1$set_$name$$}$($type$ value);\n"
            "private:\n"
            "$type$ ${1$_internal_$name$$}$() const;\n"
            "void ${1$_internal_set_$name$$}$($type$ value);\n"
            "public:\n",
            descriptor_);
    }

    void PrimitiveFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline $type$ $classname$::_internal_$name$() const {\n"
            "  return $field$;\n"
            "}\n"
            "inline $type$ $classname$::$name$() const {\n"
            "$annotate_get$"
            "  // @@protoc_insertion_point(field_get:$full_name$)\n"
            "  return _internal_$name$();\n"
            "}\n"
            "inline void $classname$::_internal_set_$name$($type$ value) {\n"
            "  $set_hasbit$\n"
            "  $field$ = value;\n"
            "}\n"
            "inline void $classname$::set_$name$($type$ value) {\n"
            "$maybe_prepare_split_message$"
            "  _internal_set_$name$(value);\n"
            "$annotate_set$"
            "  // @@protoc_insertion_point(field_set:$full_name$)\n"
            "}\n");
    }

    void PrimitiveFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$ = $default$;\n");
    }

    void PrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("_this->_internal_set_$name$(from._internal_$name$());\n");
    }

    void PrimitiveFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("swap($field$, other->$field$);\n");
    }

    void PrimitiveFieldGenerator::GenerateCopyConstructorCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("_this->$field$ = from.$field$;\n");
    }

    void PrimitiveFieldGenerator::GenerateSerializeWithCachedSizesToArray(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "target = stream->EnsureSpace(target);\n"
            "target = "
            "::_pbi::WireFormatLite::Write$declared_type$ToArray("
            "$number$, this->_internal_$name$(), target);\n");
    }

    void PrimitiveFieldGenerator::GenerateByteSize(io::Printer* printer) const {
        Formatter format(printer, variables_);
        int fixed_size = FixedSize(descriptor_->type());
        if (fixed_size == -1) {
            if (internal::WireFormat::TagSize(descriptor_->number(),
                    descriptor_->type())
                == 1) {
                // Adding one is very common and it turns out it can be done for
                // free inside of WireFormatLite, so we can save an instruction here.
                format(
                    "total_size += ::_pbi::WireFormatLite::"
                    "$declared_type$SizePlusOne(this->_internal_$name$());\n");
            } else {
                format(
                    "total_size += $tag_size$ +\n"
                    "  ::_pbi::WireFormatLite::$declared_type$Size(\n"
                    "    this->_internal_$name$());\n");
            }
        } else {
            format("total_size += $tag_size$ + $fixed_size$;\n");
        }
    }

    void PrimitiveFieldGenerator::GenerateConstexprAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("/*decltype($field$)*/$default$");
    }

    void PrimitiveFieldGenerator::GenerateAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        if (ShouldSplit(descriptor_, options_)) {
            format("decltype(Impl_::Split::$name$_){$default$}");
            return;
        }
        format("decltype($field$){$default$}");
    }

    void PrimitiveFieldGenerator::GenerateCopyAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("decltype($field$){}");
    }

    // ===================================================================

    PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
        const FieldDescriptor* descriptor, const Options& options)
        : PrimitiveFieldGenerator(descriptor, options) {
        SetCommonOneofFieldVariables(descriptor, &variables_);
    }

    PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() { }

    void PrimitiveOneofFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline $type$ $classname$::_internal_$name$() const {\n"
            "  if (_internal_has_$name$()) {\n"
            "    return $field$;\n"
            "  }\n"
            "  return $default$;\n"
            "}\n"
            "inline void $classname$::_internal_set_$name$($type$ value) {\n"
            "  if (!_internal_has_$name$()) {\n"
            "    clear_$oneof_name$();\n"
            "    set_has_$name$();\n"
            "  }\n"
            "  $field$ = value;\n"
            "}\n"
            "inline $type$ $classname$::$name$() const {\n"
            "$annotate_get$"
            "  // @@protoc_insertion_point(field_get:$full_name$)\n"
            "  return _internal_$name$();\n"
            "}\n"
            "inline void $classname$::set_$name$($type$ value) {\n"
            "  _internal_set_$name$(value);\n"
            "$annotate_set$"
            "  // @@protoc_insertion_point(field_set:$full_name$)\n"
            "}\n");
    }

    void PrimitiveOneofFieldGenerator::GenerateClearingCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$ = $default$;\n");
    }

    void PrimitiveOneofFieldGenerator::GenerateSwappingCode(
        io::Printer* printer) const {
        // Don't print any swapping code. Swapping the union will swap this field.
    }

    void PrimitiveOneofFieldGenerator::GenerateConstructorCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$ns$::_$classname$_default_instance_.$field$ = $default$;\n");
    }

    // ===================================================================

    RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
        const FieldDescriptor* descriptor, const Options& options)
        : FieldGenerator(descriptor, options) {
        SetPrimitiveVariables(descriptor, &variables_, options);

        if (descriptor->is_packed()) {
            variables_["packed_reader"] = "ReadPackedPrimitive";
            variables_["repeated_reader"] = "ReadRepeatedPrimitiveNoInline";
        } else {
            variables_["packed_reader"] = "ReadPackedPrimitiveNoInline";
            variables_["repeated_reader"] = "ReadRepeatedPrimitive";
        }
    }

    RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() { }

    void RepeatedPrimitiveFieldGenerator::GeneratePrivateMembers(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("::$proto_ns$::RepeatedField< $type$ > $name$_;\n");
        if (descriptor_->is_packed() && FixedSize(descriptor_->type()) == -1 && HasGeneratedMethods(descriptor_->file(), options_)) {
            format("mutable std::atomic<int> $cached_byte_size_name$;\n");
        }
    }

    void RepeatedPrimitiveFieldGenerator::GenerateAccessorDeclarations(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "private:\n"
            "$type$ ${1$_internal_$name$$}$(int index) const;\n"
            "const ::$proto_ns$::RepeatedField< $type$ >&\n"
            "    ${1$_internal_$name$$}$() const;\n"
            "void ${1$_internal_add_$name$$}$($type$ value);\n"
            "::$proto_ns$::RepeatedField< $type$ >*\n"
            "    ${1$_internal_mutable_$name$$}$();\n"
            "public:\n"
            "$deprecated_attr$$type$ ${1$$name$$}$(int index) const;\n"
            "$deprecated_attr$void ${1$set_$name$$}$(int index, $type$ value);\n"
            "$deprecated_attr$void ${1$add_$name$$}$($type$ value);\n"
            "$deprecated_attr$const ::$proto_ns$::RepeatedField< $type$ >&\n"
            "    ${1$$name$$}$() const;\n"
            "$deprecated_attr$::$proto_ns$::RepeatedField< $type$ >*\n"
            "    ${1$mutable_$name$$}$();\n",
            descriptor_);
    }

    void RepeatedPrimitiveFieldGenerator::GenerateInlineAccessorDefinitions(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format(
            "inline $type$ $classname$::_internal_$name$(int index) const {\n"
            "  return $field$.Get(index);\n"
            "}\n"
            "inline $type$ $classname$::$name$(int index) const {\n"
            "$annotate_get$"
            "  // @@protoc_insertion_point(field_get:$full_name$)\n"
            "  return _internal_$name$(index);\n"
            "}\n"
            "inline void $classname$::set_$name$(int index, $type$ value) {\n"
            "$annotate_set$"
            "  $field$.Set(index, value);\n"
            "  // @@protoc_insertion_point(field_set:$full_name$)\n"
            "}\n"
            "inline void $classname$::_internal_add_$name$($type$ value) {\n"
            "  $field$.Add(value);\n"
            "}\n"
            "inline void $classname$::add_$name$($type$ value) {\n"
            "  _internal_add_$name$(value);\n"
            "$annotate_add$"
            "  // @@protoc_insertion_point(field_add:$full_name$)\n"
            "}\n"
            "inline const ::$proto_ns$::RepeatedField< $type$ >&\n"
            "$classname$::_internal_$name$() const {\n"
            "  return $field$;\n"
            "}\n"
            "inline const ::$proto_ns$::RepeatedField< $type$ >&\n"
            "$classname$::$name$() const {\n"
            "$annotate_list$"
            "  // @@protoc_insertion_point(field_list:$full_name$)\n"
            "  return _internal_$name$();\n"
            "}\n"
            "inline ::$proto_ns$::RepeatedField< $type$ >*\n"
            "$classname$::_internal_mutable_$name$() {\n"
            "  return &$field$;\n"
            "}\n"
            "inline ::$proto_ns$::RepeatedField< $type$ >*\n"
            "$classname$::mutable_$name$() {\n"
            "$annotate_mutable_list$"
            "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
            "  return _internal_mutable_$name$();\n"
            "}\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateClearingCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.Clear();\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateMergingCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("_this->$field$.MergeFrom(from.$field$);\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateSwappingCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.InternalSwap(&other->$field$);\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateDestructorCode(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("$field$.~RepeatedField();\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateSerializeWithCachedSizesToArray(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        if (descriptor_->is_packed()) {
            if (FixedSize(descriptor_->type()) == -1) {
                format(
                    "{\n"
                    "  int byte_size = "
                    "$cached_byte_size_field$.load(std::memory_order_relaxed);\n"
                    "  if (byte_size > 0) {\n"
                    "    target = stream->Write$declared_type$Packed(\n"
                    "        $number$, _internal_$name$(), byte_size, target);\n"
                    "  }\n"
                    "}\n");
            } else {
                format(
                    "if (this->_internal_$name$_size() > 0) {\n"
                    "  target = stream->WriteFixedPacked($number$, _internal_$name$(), "
                    "target);\n"
                    "}\n");
            }
        } else {
            format(
                "for (int i = 0, n = this->_internal_$name$_size(); i < n; i++) {\n"
                "  target = stream->EnsureSpace(target);\n"
                "  target = ::_pbi::WireFormatLite::"
                "Write$declared_type$ToArray($number$, this->_internal_$name$(i), "
                "target);\n"
                "}\n");
        }
    }

    void RepeatedPrimitiveFieldGenerator::GenerateByteSize(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("{\n");
        format.Indent();
        int fixed_size = FixedSize(descriptor_->type());
        if (fixed_size == -1) {
            format(
                "size_t data_size = ::_pbi::WireFormatLite::\n"
                "  $declared_type$Size(this->$field$);\n");
        } else {
            format(
                "unsigned int count = static_cast<unsigned "
                "int>(this->_internal_$name$_size());\n"
                "size_t data_size = $fixed_size$UL * count;\n");
        }

        if (descriptor_->is_packed()) {
            format(
                "if (data_size > 0) {\n"
                "  total_size += $tag_size$ +\n"
                "    "
                "::_pbi::WireFormatLite::Int32Size(static_cast<$int32$>(data_size));\n"
                "}\n");
            if (FixedSize(descriptor_->type()) == -1) {
                format(
                    "int cached_size = ::_pbi::ToCachedSize(data_size);\n"
                    "$cached_byte_size_field$.store(cached_size,\n"
                    "                                std::memory_order_relaxed);\n");
            }
            format("total_size += data_size;\n");
        } else {
            format(
                "total_size += $tag_size$ *\n"
                "              "
                "::_pbi::FromIntSize(this->_internal_$name$_size());\n"
                "total_size += data_size;\n");
        }
        format.Outdent();
        format("}\n");
    }

    void RepeatedPrimitiveFieldGenerator::GenerateConstexprAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("/*decltype($field$)*/{}");
        if (descriptor_->is_packed() && FixedSize(descriptor_->type()) == -1 && HasGeneratedMethods(descriptor_->file(), options_)) {
            format("\n, /*decltype($cached_byte_size_field$)*/{0}");
        }
    }

    void RepeatedPrimitiveFieldGenerator::GenerateAggregateInitializer(
        io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("decltype($field$){arena}");
        if (descriptor_->is_packed() && FixedSize(descriptor_->type()) == -1 && HasGeneratedMethods(descriptor_->file(), options_)) {
            // std::atomic has no move constructor, which prevents explicit aggregate
            // initialization pre-C++17.
            format("\n, /*decltype($cached_byte_size_field$)*/{0}");
        }
    }

    void RepeatedPrimitiveFieldGenerator::GenerateCopyAggregateInitializer(
        io::Printer* printer) const {

        Formatter format(printer, variables_);
        format("decltype($field$){from.$field$}");
        if (descriptor_->is_packed() && FixedSize(descriptor_->type()) == -1 && HasGeneratedMethods(descriptor_->file(), options_)) {
            // std::atomic has no move constructor.
            format("\n, /*decltype($cached_byte_size_field$)*/{0}");
        }
    }

} // namespace shark::compiler::cc
