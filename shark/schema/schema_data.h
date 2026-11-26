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

namespace shark::schema {

    /// 1-based line and column of the declaration in the source file.
    /// Same meaning as shark::parser::SourceLocation.
    struct SourceLocation {
        int32_t line{0};
        int32_t column{0};
    };

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

    /// A type name written in source, including generic arguments.
    /// A plain name has an empty argument list. A generic argument that is
    /// itself a type is nested here; a bare type argument is a name with no arguments.
    struct TypeNameInfo {
        std::string name;
        std::vector<TypeNameInfo> arguments;
    };

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
        bool typed{false};
        /// Set when typed is true. Empty name when the annotation has no `<Type>`.
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
        /// Empty when value is an integer literal. Otherwise the constexpr name.
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
        /// Name written after typedef or using. The resolved target symbol is not stored.
        std::string target_name;
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

    enum class PlaceholderConstraint : uint8_t {
        KEY,
        ALL,
    };

    struct PlaceholderInfo {
        SourceSite site;
        std::string name;
        PlaceholderConstraint constraint{PlaceholderConstraint::ALL};
        /// Types this placeholder accepts. Empty when every type is allowed.
        /// The count is the size of this list.
        std::vector<PhysicalType> allowed;
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
        std::string name;
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
        /// Return type as written. Resolved struct and enum pointers are not stored.
        std::string return_type_name;
        std::vector<FieldInfo> params;
        std::vector<AnnotationInfo> annotations;
    };

    struct ServletInfo {
        SourceSite site;
        std::string name;
        /// Base servlet names as written. Resolved base servlet pointers are not stored.
        std::vector<std::string> base_names;
        std::vector<FunctionInfo> methods;
        std::vector<AnnotationInfo> annotations;
    };

    struct FileData {
        SourceSite site;
        std::string path;
        std::string absolute_path;
        /// Effective package name. The package declarations themselves are in packages.
        std::string package;
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
        std::vector<std::string> indirect_dependencies;
    };
}  // namespace shark::schema
