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

#include <string>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <shark/schema/schema_defines.h>

namespace shark::schema {

    /// Where a declaration sits in its source file.
    /// sequence is the 0-based order among declarations of the same scope,
    /// following source order rather than the order of a typed vector.
    struct SourceSite {
        SourceLocation location;
        uint32_t sequence{0};
    };

    struct CommentInfo {
        SourceSite site;
        /// Token text of the comment, one entry per lexer token.
        std::vector<std::string> text;
        bool is_tailing{false};
    };

    struct EditionInfo {
        SourceSite site;
        std::string edition;
    };

    struct PackageInfo {
        SourceSite site;
        std::string classify;
        std::string package;
    };

    struct ImportInfo {
        SourceSite site;
        std::string classify;
        std::string path;
    };

    struct TypeNameInfo;

    /// One predicate call. is_integer has an empty argument list.
    /// is_subclass_of<Myclass> stores name "is_subclass_of" and one TypeNameInfo argument.
    struct ConstraintFunction {
        ConstraintFunction();
        ~ConstraintFunction();
        ConstraintFunction(const ConstraintFunction&);
        ConstraintFunction& operator=(const ConstraintFunction&);
        ConstraintFunction(ConstraintFunction&&) noexcept;
        ConstraintFunction& operator=(ConstraintFunction&&) noexcept;

        std::string name;
        std::vector<TypeNameInfo> arguments;
    };

    /// Boolean expression inside []. CALL is one ConstraintFunction.
    /// NOT has one operand. AND and OR have two.
    struct TypeConstraint {
        enum class Op : uint8_t {
            CALL,
            NOT,
            AND,
            OR,
        };

        TypeConstraint();
        ~TypeConstraint();
        TypeConstraint(const TypeConstraint&);
        TypeConstraint& operator=(const TypeConstraint&);
        TypeConstraint(TypeConstraint&&) noexcept;
        TypeConstraint& operator=(TypeConstraint&&) noexcept;

        Op op{Op::CALL};
        ConstraintFunction function;
        std::vector<TypeConstraint> operands;
    };

    /// A type written in source.
    /// name is the scoped type or template name, dots included, without arguments.
    /// list<int32> stores name "list" and one argument "int32".
    /// The full name, such as "list<int32>", is produced from name and arguments. It is not stored.
    struct TypeNameInfo {
        TypeNameInfo();
        ~TypeNameInfo();
        TypeNameInfo(const TypeNameInfo&);
        TypeNameInfo& operator=(const TypeNameInfo&);
        TypeNameInfo(TypeNameInfo&&) noexcept;
        TypeNameInfo& operator=(TypeNameInfo&&) noexcept;

        /// Scoped name. "list" for list<int32>, "pkg.Foo" for a dotted name.
        std::string name;
        /// True when this name is a placeholder declared for a generic, such as K in struct<K>.
        /// False when the type is instantiated. list<int32> is a fixed type, so this flag is false.
        /// Default is false.
        bool is_parameter{false};
        /// Specialization arguments. Empty when the type is not generic.
        /// Nested parameters are further TypeNameInfo values here.
        std::vector<TypeNameInfo> arguments;
        /// The [] expression on this name. Empty when the name has no constraint.
        std::optional<TypeConstraint> constraint;
    };

    inline ConstraintFunction::ConstraintFunction() = default;
    inline ConstraintFunction::~ConstraintFunction() = default;
    inline ConstraintFunction::ConstraintFunction(const ConstraintFunction&) = default;
    inline ConstraintFunction& ConstraintFunction::operator=(const ConstraintFunction&) = default;
    inline ConstraintFunction::ConstraintFunction(ConstraintFunction&&) noexcept = default;
    inline ConstraintFunction& ConstraintFunction::operator=(ConstraintFunction&&) noexcept = default;

    inline TypeConstraint::TypeConstraint() = default;
    inline TypeConstraint::~TypeConstraint() = default;
    inline TypeConstraint::TypeConstraint(const TypeConstraint&) = default;
    inline TypeConstraint& TypeConstraint::operator=(const TypeConstraint&) = default;
    inline TypeConstraint::TypeConstraint(TypeConstraint&&) noexcept = default;
    inline TypeConstraint& TypeConstraint::operator=(TypeConstraint&&) noexcept = default;

    inline TypeNameInfo::TypeNameInfo() = default;
    inline TypeNameInfo::~TypeNameInfo() = default;
    inline TypeNameInfo::TypeNameInfo(const TypeNameInfo&) = default;
    inline TypeNameInfo& TypeNameInfo::operator=(const TypeNameInfo&) = default;
    inline TypeNameInfo::TypeNameInfo(TypeNameInfo&&) noexcept = default;
    inline TypeNameInfo& TypeNameInfo::operator=(TypeNameInfo&&) noexcept = default;

    /// One serializable value. kind selects which fields are meaningful.
    /// Nested values (list items, map keys and values, struct fields) each
    /// carry their own SourceSite. sequence is the order within that layer.
    /// Map keys/values and struct field names/values are parallel arrays of equal length.
    enum class ValueKind : uint8_t {
        EMPTY,
        BOOL,
        INT,
        FLOAT,
        STRING,
        IDENT,
        LIST,
        MAP,
        STRUCT,
        ENUM,
    };

    struct Value {
        SourceSite site;
        ValueKind kind{ValueKind::EMPTY};

        bool boolean{false};
        int64_t integer{0};
        double floating{0};

        /// STRING text, IDENT name, or ENUM enumerator name.
        std::string text;
        /// Set for IDENT and ENUM when binding resolved an enum type. Empty otherwise.
        std::string enum_type;

        std::vector<Value> elements;
        std::vector<Value> keys;
        std::vector<Value> values;
        std::vector<std::string> field_names;
        std::vector<Value> field_values;
    };

    struct AnnotationInfo {
        SourceSite site;
        std::string classify;
        std::string key;
        bool file_level{false};
        /// Empty name when the annotation has no `<Type>`.
        TypeNameInfo type_name;
        /// Bound value. An untyped payload with several scalars is a LIST.
        Value value;
    };

    enum class EnumUnderlying : uint8_t {
        NONE,
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
    };

    struct EnumValueInfo {
        SourceSite site;
        std::string name;
        int64_t value{0};
        /// Empty when value is an integer literal.
        /// Otherwise the constexpr reference as written, dots included.
        std::string constexpr_ref;
        std::vector<AnnotationInfo> annotations;
    };

    struct EnumInfo {
        SourceSite site;
        std::string name;
        bool is_class{false};
        EnumUnderlying underlying{EnumUnderlying::NONE};
        std::vector<EnumValueInfo> values;
        std::vector<AnnotationInfo> annotations;
    };

    struct AliasInfo {
        SourceSite site;
        std::string name;
        /// Target type as written, including generic arguments. The resolved symbol is not stored.
        TypeNameInfo target;
        /// True for `using Name = Target`, false for `typedef Target Name`.
        bool is_using{false};
        std::vector<AnnotationInfo> annotations;
    };

    enum class PhysicalType : uint8_t {
        VOID,
        BOOL,
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
        INT64,
        UINT64,
        INT128,
        UINT128,
        FLOAT32,
        FLOAT64,
        STRING,
        STRUCT,
        LIST,
        SET,
        MAP,
        OPTIONAL,
    };

    enum class FieldLabel : uint8_t {
        REQUIRED,
        OPTIONAL,
        CONSTEXPR,
        ATOMIC,
    };

    struct FieldInfo {
        SourceSite site;
        std::string name;
        FieldLabel label{FieldLabel::REQUIRED};
        /// Written type, including generic arguments. Resolved type pointers are not stored.
        TypeNameInfo type_name;
        /// True when the written type uses a generic parameter.
        bool generic{false};
        uint16_t number{0};
        /// EMPTY when the field has no default.
        Value default_value;
        /// True when a constexpr struct, union, or container is kept but is not a compile-time constant.
        bool static_const{false};
        std::vector<AnnotationInfo> annotations;
    };

    struct StructInfo {
        SourceSite site;
        /// Declaration name. Generic parameters are arguments on this name.
        TypeNameInfo name;
        bool is_union{false};
        bool builtin{false};
        bool generic{false};
        PhysicalType phy_type{PhysicalType::VOID};
        std::vector<FieldInfo> fields;
        std::vector<StructInfo> nested_structs;
        std::vector<EnumInfo> nested_enums;
        std::vector<AliasInfo> aliases;
        std::vector<AnnotationInfo> annotations;
    };

    struct FunctionInfo {
        SourceSite site;
        std::string name;
        /// Type parameters written after function. Empty when the function is not generic.
        std::vector<TypeNameInfo> parameters;
        /// Return type as written. Resolved struct and enum pointers are not stored.
        TypeNameInfo return_type;
        std::vector<FieldInfo> params;
        std::vector<AnnotationInfo> annotations;
    };

    struct ServletInfo {
        SourceSite site;
        /// Declaration name. Generic parameters are arguments on this name.
        TypeNameInfo name;
        /// Base types as written, including generic arguments. Resolved servlet pointers are not stored.
        std::vector<TypeNameInfo> bases;
        std::vector<FunctionInfo> methods;
        std::vector<AnnotationInfo> annotations;
    };

    struct FileData {
        SourceSite site;
        std::string path;
        std::optional<EditionInfo> edition;
        std::vector<CommentInfo> comments;
        std::vector<PackageInfo> packages;
        std::vector<ImportInfo> imports;
        std::vector<AnnotationInfo> annotations;
        std::vector<EnumInfo> enums;
        std::vector<StructInfo> structs;
        std::vector<FunctionInfo> functions;
        std::vector<ServletInfo> servlets;
        std::vector<FieldInfo> constexprs;
        /// Debug only. Not used by codegen or by loading symbols into the pool.
        std::vector<std::string> indirect_dependencies;
    };
}  // namespace shark::schema
