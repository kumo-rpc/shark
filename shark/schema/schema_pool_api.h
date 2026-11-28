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

#include <shark/schema/schema_pool.h>

namespace shark {

    class SchemaComment;
    class SchemaEdition;
    class SchemaPackage;
    class SchemaImport;
    class SchemaAnnotation;
    class SchemaConstValue;
    class SchemaEnumValue;
    class SchemaEnum;
    class SchemaTypeName;
    class SchemaTypeConstraint;
    class SchemaField;
    class SchemaTypeAlias;
    class SchemaFunctionAlias;
    class SchemaServletAlias;
    class SchemaStruct;
    class SchemaFunction;
    class SchemaServlet;
    class SchemaFile;

    class SchemaComment : protected CommentBase {
    public:
        SchemaComment() = delete;

        static const SchemaComment* from(const CommentBase* node) {
            return static_cast<const SchemaComment*>(node);
        }

        [[nodiscard]] uint32_t comment_size() const {
            return static_cast<uint32_t>(comments_.size());
        }
        [[nodiscard]] const std::string& comment(uint32_t index) const {
            return comments_[index];
        }
        [[nodiscard]] bool is_tailing() const {
            return is_tail_;
        }
    };

    class SchemaTypeName : protected TypeName {
    public:
        SchemaTypeName() = delete;
        static const SchemaTypeName* from(const TypeName* node) {
            return static_cast<const SchemaTypeName*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] bool is_parameter() const { return is_parameter_; }
        [[nodiscard]] uint32_t argument_size() const { return static_cast<uint32_t>(arguments_.size()); }
        [[nodiscard]] const SchemaTypeName* argument(uint32_t index) const;
        [[nodiscard]] const SchemaTypeConstraint* constraint() const;
    };

    class SchemaTypeConstraint : protected TypeConstraint {
    public:
        SchemaTypeConstraint() = delete;
        static const SchemaTypeConstraint* from(const TypeConstraint* node) {
            return static_cast<const SchemaTypeConstraint*>(node);
        }
        [[nodiscard]] Op op() const { return op_; }
        [[nodiscard]] const std::string& function_name() const { return function_.name_; }
        [[nodiscard]] uint32_t function_argument_size() const {
            return static_cast<uint32_t>(function_.arguments_.size());
        }
        [[nodiscard]] const SchemaTypeName* function_argument(uint32_t index) const {
            return SchemaTypeName::from(&function_.arguments_[index]);
        }
        [[nodiscard]] uint32_t operand_size() const { return static_cast<uint32_t>(operands_.size()); }
        [[nodiscard]] const SchemaTypeConstraint* operand(uint32_t index) const {
            return from(&operands_[index]);
        }
    };

    inline const SchemaTypeName* SchemaTypeName::argument(uint32_t index) const {
        return from(&arguments_[index]);
    }
    inline const SchemaTypeConstraint* SchemaTypeName::constraint() const {
        if (!constraint_.has_value()) {
            return nullptr;
        }
        return SchemaTypeConstraint::from(&*constraint_);
    }

    class SchemaConstValue : protected ConstValue {
    public:
        enum class Kind {
            NONE,
            BOOL,
            INT,
            FLOAT,
            STRING,
            IDENTIFIER,
            LIST,
            MAP,
            STRUCT,
        };

        SchemaConstValue() = delete;
        static const SchemaConstValue* from(const ConstValue* node) {
            return static_cast<const SchemaConstValue*>(node);
        }
        [[nodiscard]] Kind kind() const;
        [[nodiscard]] bool bool_value() const { return std::get<bool>(value_); }
        [[nodiscard]] int64_t int_value() const { return std::get<int64_t>(value_); }
        [[nodiscard]] double float_value() const { return std::get<double>(value_); }
        [[nodiscard]] const std::string& string_value() const { return std::get<std::string>(value_); }
        [[nodiscard]] const std::string& identifier_name() const { return std::get<Identifier>(value_).name_; }
        [[nodiscard]] const SchemaEnum* identifier_enum() const;
        [[nodiscard]] const SchemaEnumValue* identifier_enumerator() const;
        [[nodiscard]] uint32_t element_size() const {
            return static_cast<uint32_t>(std::get<List>(value_).elements_.size());
        }
        [[nodiscard]] const SchemaConstValue* element(uint32_t index) const {
            return from(std::get<List>(value_).elements_[index]);
        }
        [[nodiscard]] uint32_t entry_size() const {
            return static_cast<uint32_t>(std::get<Map>(value_).entries_.size());
        }
        [[nodiscard]] const SchemaConstValue* entry_key(uint32_t index) const {
            return from(std::get<Map>(value_).entries_[index].first);
        }
        [[nodiscard]] const SchemaConstValue* entry_value(uint32_t index) const {
            return from(std::get<Map>(value_).entries_[index].second);
        }
        [[nodiscard]] uint32_t field_size() const {
            return static_cast<uint32_t>(std::get<Struct>(value_).fields_.size());
        }
        [[nodiscard]] const SchemaField* field(uint32_t index) const;
        [[nodiscard]] const SchemaConstValue* field_value(uint32_t index) const {
            return from(std::get<Struct>(value_).fields_[index].value_);
        }
    };

    class SchemaEdition : protected EditionBase {
    public:
        SchemaEdition() = delete;
        static const SchemaEdition* from(const EditionBase* node) {
            return static_cast<const SchemaEdition*>(node);
        }
        [[nodiscard]] const std::string& edition() const { return edition_; }
        [[nodiscard]] const SchemaComment* leading_comment() const { return SchemaComment::from(leading_comment_); }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
    };

    class SchemaPackage : protected PackageBase {
    public:
        SchemaPackage() = delete;
        static const SchemaPackage* from(const PackageBase* node) {
            return static_cast<const SchemaPackage*>(node);
        }
        [[nodiscard]] const std::string& classify() const { return classify_; }
        [[nodiscard]] const std::string& package() const { return package_; }
        [[nodiscard]] const SchemaComment* leading_comment() const { return SchemaComment::from(leading_comment_); }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
    };

    class SchemaAnnotation : protected AnnotationBase {
    public:
        SchemaAnnotation() = delete;
        static const SchemaAnnotation* from(const AnnotationBase* node) {
            return static_cast<const SchemaAnnotation*>(node);
        }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] bool file_level() const { return file_level_; }
        [[nodiscard]] const std::string& classify() const { return classify_; }
        [[nodiscard]] const std::string& key() const { return key_; }
        [[nodiscard]] const SchemaTypeName* type_name() const { return SchemaTypeName::from(&type_name_); }
        [[nodiscard]] const SchemaConstValue* value() const { return SchemaConstValue::from(value_); }
    };

    class SchemaImport : protected ImportBase {
    public:
        SchemaImport() = delete;
        static const SchemaImport* from(const ImportBase* node) {
            return static_cast<const SchemaImport*>(node);
        }
        [[nodiscard]] const SchemaComment* leading_comment() const { return SchemaComment::from(leading_comment_); }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] const std::string& classify() const { return classify_; }
        [[nodiscard]] const std::string& path() const { return path_; }
    };

    class SchemaType : protected TypeInfo {
        friend bool type_is_integer(const SchemaType *type);
        friend bool type_is_void(const SchemaType *type);
        friend bool type_is_primitive(const SchemaType *type);
        friend bool type_is_pod(const SchemaType *type);
        friend bool type_is_signed(const SchemaType *type);
        friend bool type_is_numeric(const SchemaType *type);
        friend bool type_is_container(const SchemaType *type);
        friend bool type_is_string(const SchemaType *type);
        friend bool type_is_constexpr(const SchemaType *type);
        friend bool type_is_list(const SchemaType *type);
        friend bool type_is_map(const SchemaType *type);
        friend bool type_is_set(const SchemaType *type);
    public:
        SchemaType() = delete;
        static const SchemaType* from(const TypeInfo* node) {
            return static_cast<const SchemaType*>(node);
        }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] const SchemaStruct* container() const;
        [[nodiscard]] PhysicalType phy_type() const { return phy_type_; }
    };

    class SchemaEnum : protected EnumBase {
    public:
        SchemaEnum() = delete;
        static const SchemaEnum* from(const EnumBase* node) {
            return static_cast<const SchemaEnum*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] uint32_t value_size() const { return static_cast<uint32_t>(values_.size()); }
        [[nodiscard]] const SchemaEnumValue* value(uint32_t index) const;
        [[nodiscard]] bool is_class() const { return is_class_; }
        [[nodiscard]] UnderlyingType underlying() const { return underlying_; }
        [[nodiscard]] const SchemaStruct* container() const;
    };

    class SchemaEnumValue : protected EnumValueBase {
    public:
        SchemaEnumValue() = delete;
        static const SchemaEnumValue* from(const EnumValueBase* node) {
            return static_cast<const SchemaEnumValue*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] int64_t value() const { return value_; }
        [[nodiscard]] int32_t line() const { return source_location_.line; }
        [[nodiscard]] const SchemaField* constexpr_field() const;
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] const SchemaEnum* parent_enum() const { return SchemaEnum::from(parent_enum_); }
    };

    class SchemaField : protected FieldBase {
    public:
        SchemaField() = delete;
        static const SchemaField* from(const FieldBase* node) {
            return static_cast<const SchemaField*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] FieldLabel label() const { return label_; }
        [[nodiscard]] const SchemaTypeName* type_name() const { return SchemaTypeName::from(&type_ref_); }
        [[nodiscard]] bool generic() const { return generic_; }
        [[nodiscard]] const SchemaType* type() const;
        [[nodiscard]] const SchemaStruct* container() const;
        [[nodiscard]] uint16_t id() const { return id_; }
        [[nodiscard]] const SchemaConstValue* default_value() const { return SchemaConstValue::from(default_value_); }
        [[nodiscard]] bool static_const() const { return static_const_; }
    };

    class SchemaTypeAlias : protected TypeAlias {
    public:
        SchemaTypeAlias() = delete;
        static const SchemaTypeAlias* from(const TypeAlias* node) {
            return static_cast<const SchemaTypeAlias*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] const SchemaType* target() const { return SchemaType::from(target_); }
        [[nodiscard]] const SchemaStruct* container() const;
        [[nodiscard]] bool is_using() const { return is_using_; }
    };

    class SchemaStruct : protected StructBase {
    public:
        SchemaStruct() = delete;
        static const SchemaStruct* from(const StructBase* node) {
            return static_cast<const SchemaStruct*>(node);
        }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] const SchemaTypeName* type_name() const { return SchemaTypeName::from(&name_); }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] uint32_t field_size() const { return static_cast<uint32_t>(fields_.size()); }
        [[nodiscard]] const SchemaField* field(uint32_t index) const { return SchemaField::from(fields_[index]); }
        [[nodiscard]] uint32_t nested_type_size() const { return static_cast<uint32_t>(nested_types_.size()); }
        [[nodiscard]] const SchemaStruct* nested_type(uint32_t index) const { return from(nested_types_[index]); }
        [[nodiscard]] uint32_t nested_enum_size() const { return static_cast<uint32_t>(nested_enums_.size()); }
        [[nodiscard]] const SchemaEnum* nested_enum(uint32_t index) const { return SchemaEnum::from(nested_enums_[index]); }
        [[nodiscard]] uint32_t alias_size() const { return static_cast<uint32_t>(aliases_.size()); }
        [[nodiscard]] const SchemaTypeAlias* alias(uint32_t index) const { return SchemaTypeAlias::from(aliases_[index]); }
        [[nodiscard]] uint32_t function_alias_size() const { return static_cast<uint32_t>(function_aliases_.size()); }
        [[nodiscard]] const SchemaFunctionAlias* function_alias(uint32_t index) const;
        [[nodiscard]] uint32_t servlet_alias_size() const { return static_cast<uint32_t>(servlet_aliases_.size()); }
        [[nodiscard]] const SchemaServletAlias* servlet_alias(uint32_t index) const;
        [[nodiscard]] const SchemaStruct* container() const { return from(container_); }
        [[nodiscard]] bool is_union() const { return is_union_; }
        [[nodiscard]] bool generic() const { return generic_; }
        [[nodiscard]] PhysicalType phy_type() const { return phy_type_; }
    };

    class SchemaPrimitive : protected PrimitiveBase {
    public:
        SchemaPrimitive() = delete;
        static const SchemaPrimitive* from(const PrimitiveBase* node) {
            return static_cast<const SchemaPrimitive*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] const SchemaStruct* container() const { return SchemaStruct::from(container_); }
        [[nodiscard]] PhysicalType phy_type() const { return phy_type_; }
    };

    class SchemaFunction : protected FunctionBase {
    public:
        SchemaFunction() = delete;
        static const SchemaFunction* from(const FunctionBase* node) {
            return static_cast<const SchemaFunction*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] uint32_t parameter_size() const { return static_cast<uint32_t>(parameters_.size()); }
        [[nodiscard]] const SchemaTypeName* parameter(uint32_t index) const {
            return SchemaTypeName::from(&parameters_[index]);
        }
        [[nodiscard]] const SchemaTypeName* return_type() const { return SchemaTypeName::from(&return_type_); }
        [[nodiscard]] uint32_t param_size() const { return static_cast<uint32_t>(params_.size()); }
        [[nodiscard]] const SchemaField* param(uint32_t index) const { return SchemaField::from(params_[index]); }
        [[nodiscard]] uint32_t arg_size() const { return static_cast<uint32_t>(args_.size()); }
        [[nodiscard]] const SchemaStruct* arg(uint32_t index) const;
        [[nodiscard]] const SchemaType* ret_type() const;
        [[nodiscard]] const SchemaStruct* container() const;
        [[nodiscard]] const SchemaServlet* servlet() const;
    };

    class SchemaServlet : protected ServletBase {
    public:
        SchemaServlet() = delete;
        static const SchemaServlet* from(const ServletBase* node) {
            return static_cast<const SchemaServlet*>(node);
        }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] const SchemaTypeName* type_name() const { return SchemaTypeName::from(&name_); }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] uint32_t base_name_size() const { return static_cast<uint32_t>(base_names_.size()); }
        [[nodiscard]] const SchemaTypeName* base_name(uint32_t index) const {
            return SchemaTypeName::from(&base_names_[index]);
        }
        [[nodiscard]] uint32_t base_size() const { return static_cast<uint32_t>(bases_.size()); }
        [[nodiscard]] const SchemaServlet* base(uint32_t index) const { return from(bases_[index]); }
        [[nodiscard]] uint32_t method_size() const { return static_cast<uint32_t>(method_.size()); }
        [[nodiscard]] const SchemaFunction* method(uint32_t index) const { return SchemaFunction::from(method_[index]); }
        [[nodiscard]] const SchemaStruct* container() const { return SchemaStruct::from(container_); }
    };

    class SchemaFunctionAlias : protected FunctionAlias {
    public:
        SchemaFunctionAlias() = delete;
        static const SchemaFunctionAlias* from(const FunctionAlias* node) {
            return static_cast<const SchemaFunctionAlias*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const SchemaFunction* target() const { return SchemaFunction::from(target_); }
        [[nodiscard]] bool is_using() const { return is_using_; }
    };

    class SchemaServletAlias : protected ServletAlias {
    public:
        SchemaServletAlias() = delete;
        static const SchemaServletAlias* from(const ServletAlias* node) {
            return static_cast<const SchemaServletAlias*>(node);
        }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const SchemaServlet* target() const { return SchemaServlet::from(target_); }
        [[nodiscard]] bool is_using() const { return is_using_; }
    };

    class SchemaFile : protected FileBase {
    public:
        SchemaFile() = delete;
        static const SchemaFile* from(const FileBase* node) {
            return static_cast<const SchemaFile*>(node);
        }
        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] const std::string& full_name() const { return full_name_; }
        [[nodiscard]] const std::string& full_name_without_package() const { return full_name_without_package_; }
        [[nodiscard]] const std::string& path() const { return path_; }
        [[nodiscard]] const std::string& package() const { return package_; }
        [[nodiscard]] const SchemaEdition* edition() const { return SchemaEdition::from(edition_); }
        [[nodiscard]] uint32_t package_size() const { return static_cast<uint32_t>(packages_.size()); }
        [[nodiscard]] const SchemaPackage* package_at(uint32_t index) const { return SchemaPackage::from(packages_[index]); }
        [[nodiscard]] uint32_t import_size() const { return static_cast<uint32_t>(imports_.size()); }
        [[nodiscard]] const SchemaImport* import(uint32_t index) const { return SchemaImport::from(imports_[index]); }
        [[nodiscard]] uint32_t leading_comment_size() const { return static_cast<uint32_t>(leading_comment_.size()); }
        [[nodiscard]] const SchemaComment* leading_comment(uint32_t index) const {
            return SchemaComment::from(leading_comment_[index]);
        }
        [[nodiscard]] const SchemaComment* tailing_comment() const { return SchemaComment::from(tailing_comment_); }
        [[nodiscard]] uint32_t annotation_size() const { return static_cast<uint32_t>(annotations_.size()); }
        [[nodiscard]] const SchemaAnnotation* annotation(uint32_t index) const {
            return SchemaAnnotation::from(annotations_[index]);
        }
        [[nodiscard]] uint32_t enum_size() const { return static_cast<uint32_t>(enums_.size()); }
        [[nodiscard]] const SchemaEnum* enum_at(uint32_t index) const { return SchemaEnum::from(enums_[index]); }
        [[nodiscard]] uint32_t struct_size() const { return static_cast<uint32_t>(structs_.size()); }
        [[nodiscard]] const SchemaStruct* struct_at(uint32_t index) const { return SchemaStruct::from(structs_[index]); }
        [[nodiscard]] uint32_t primitive_size() const { return static_cast<uint32_t>(primitives_.size()); }
        [[nodiscard]] const SchemaPrimitive* primitive_at(uint32_t index) const { return SchemaPrimitive::from(primitives_[index]); }
        [[nodiscard]] uint32_t servlet_size() const { return static_cast<uint32_t>(servlets_.size()); }
        [[nodiscard]] const SchemaServlet* servlet(uint32_t index) const { return SchemaServlet::from(servlets_[index]); }
        [[nodiscard]] uint32_t function_size() const { return static_cast<uint32_t>(functions_.size()); }
        [[nodiscard]] const SchemaFunction* function(uint32_t index) const { return SchemaFunction::from(functions_[index]); }
        [[nodiscard]] uint32_t constexpr_size() const { return static_cast<uint32_t>(constexprs_.size()); }
        [[nodiscard]] const SchemaField* constexpr_at(uint32_t index) const { return SchemaField::from(constexprs_[index]); }
        [[nodiscard]] uint32_t dependency_size() const { return static_cast<uint32_t>(dependencies_.size()); }
        [[nodiscard]] const SchemaFile* dependency(uint32_t index) const { return from(dependencies_[index]); }
    };

    inline SchemaConstValue::Kind SchemaConstValue::kind() const {
        switch (value_.index()) {
            case 1: return Kind::BOOL;
            case 2: return Kind::INT;
            case 3: return Kind::FLOAT;
            case 4: return Kind::STRING;
            case 5: return Kind::IDENTIFIER;
            case 6: return Kind::LIST;
            case 7: return Kind::MAP;
            case 8: return Kind::STRUCT;
            default: return Kind::NONE;
        }
    }
    inline const SchemaEnum* SchemaConstValue::identifier_enum() const {
        return SchemaEnum::from(std::get<Identifier>(value_).enum_type_);
    }
    inline const SchemaEnumValue* SchemaConstValue::identifier_enumerator() const {
        return SchemaEnumValue::from(std::get<Identifier>(value_).enumerator_);
    }
    inline const SchemaField* SchemaConstValue::field(uint32_t index) const {
        return SchemaField::from(std::get<Struct>(value_).fields_[index].field_);
    }
    inline const SchemaField* SchemaEnumValue::constexpr_field() const {
        return SchemaField::from(constexpr_field_);
    }
    inline const SchemaEnumValue* SchemaEnum::value(uint32_t index) const {
        return SchemaEnumValue::from(values_[index]);
    }
    inline const SchemaStruct* SchemaType::container() const {
        return SchemaStruct::from(container_);
    }
    inline const SchemaStruct* SchemaEnum::container() const {
        return SchemaStruct::from(container_);
    }
    inline const SchemaType* SchemaField::type() const {
        return SchemaType::from(type_);
    }
    inline const SchemaStruct* SchemaField::container() const {
        return SchemaStruct::from(container_);
    }
    inline const SchemaFunctionAlias* SchemaStruct::function_alias(uint32_t index) const {
        return SchemaFunctionAlias::from(function_aliases_[index]);
    }
    inline const SchemaServletAlias* SchemaStruct::servlet_alias(uint32_t index) const {
        return SchemaServletAlias::from(servlet_aliases_[index]);
    }
    inline const SchemaStruct* SchemaTypeAlias::container() const {
        return SchemaStruct::from(container_);
    }
    inline const SchemaStruct* SchemaFunction::arg(uint32_t index) const {
        return SchemaStruct::from(args_[index]);
    }
    inline const SchemaType* SchemaFunction::ret_type() const {
        return SchemaType::from(ret_type_);
    }
    inline const SchemaStruct* SchemaFunction::container() const {
        return SchemaStruct::from(container_);
    }
    inline const SchemaServlet* SchemaFunction::servlet() const {
        return SchemaServlet::from(servlet_);
    }

}  // namespace shark
