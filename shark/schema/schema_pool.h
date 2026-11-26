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

#include <shark/schema/schema_defines.h>

#include <fermat/maps/btree_map.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace shark {

    class SchemaPoolBase;
    class AnnotationBase;
    class EnumBase;
    class EnumValueBase;
    class FieldBase;
    class StructBase;
    class AliasBase;
    class FunctionBase;
    class ServletBase;
    class FileBase;

    class SymbolBase {
        friend class SchemaPoolBase;
        friend class CommentBase;
        friend class EditionBase;
        friend class PackageBase;
        friend class ImportBase;
        friend class ConstValue;
        friend class FieldBase;
        friend class EnumValueBase;
        friend class EnumBase;
        friend class StructBase;
        friend class AliasBase;
        friend class FunctionBase;
        friend class ServletBase;
        friend class FileBase;
        friend class AnnotationBase;
    public:
        virtual ~SymbolBase() = default;

        SymbolType             symbol_type_{SymbolType::NONE};
        SourceLocation         source_location_;
        SchemaPoolBase        *pool_{nullptr};
        uint32_t               sequence_{0};

    private:
        SymbolBase() = default;
    };

    class CommentBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::vector<std::string>  comments_;
        bool                      is_tail_{false};

    private:
        CommentBase() = default;
    };


    class EditionBase: public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string    edition_;
        CommentBase   *leading_comment_{nullptr};
        CommentBase   *tailing_comment_{nullptr};

    private:
        EditionBase() = default;
    };

    class PackageBase: public SymbolBase {
        friend class SchemaPoolBase;
    public:
        CommentBase    *leading_comment_{nullptr};
        CommentBase    *tailing_comment_{nullptr};
        std::string     classify_;
        std::string     package_;

    private:
        PackageBase() = default;
    };

    class ImportBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        CommentBase                      *leading_comment_{nullptr};
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::string                       classify_;
        std::string                       path_;

    private:
        ImportBase() = default;
    };

    class ConstValue : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        struct List {
            std::vector<ConstValue*> elements_;
        };

        struct Map {
            std::vector<std::pair<ConstValue*, ConstValue*>> entries_;
        };

        struct Identifier {
            std::string              name_;
            const EnumBase          *enum_type_{nullptr};
            const EnumValueBase     *enumerator_{nullptr};
        };

        struct StructField {
            const FieldBase *field_{nullptr};
            ConstValue      *value_{nullptr};
        };

        struct Struct {
            std::vector<StructField> fields_;
        };

        using Value = std::variant<
            std::monostate,
            bool,
            int64_t,
            double,
            std::string,
            Identifier,
            List,
            Map,
            Struct>;

        Value value_;

    private:
        ConstValue() = default;
    };

    class EnumValueBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        int64_t                           value_{0};
        std::vector<AnnotationBase*>     annotations_;
        const EnumBase                   *parent_enum_{nullptr};

    private:
        EnumValueBase() = default;
    };

    class EnumBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        enum class UnderlyingType {
            NONE,
            INT8,
            UINT8,
            INT16,
            UINT16,
            INT32,
            UINT32,
        };

        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::vector<EnumValueBase*>       values_;
        bool                              is_class_{false};
        UnderlyingType                    underlying_{UnderlyingType::NONE};
        const StructBase                 *container_{nullptr};

    private:
        EnumBase() = default;
    };

    struct TypeName;

    struct ConstraintFunction {
        ConstraintFunction();
        ~ConstraintFunction();
        ConstraintFunction(const ConstraintFunction&);
        ConstraintFunction& operator=(const ConstraintFunction&);
        ConstraintFunction(ConstraintFunction&&) noexcept;
        ConstraintFunction& operator=(ConstraintFunction&&) noexcept;

        std::string           name_;
        std::vector<TypeName> arguments_;
    };

    struct TypeConstraint {
        enum class Op {
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

        Op                              op_{Op::CALL};
        ConstraintFunction              function_;
        std::vector<TypeConstraint>     operands_;
    };

    struct TypeName {
        TypeName();
        ~TypeName();
        TypeName(const TypeName&);
        TypeName& operator=(const TypeName&);
        TypeName(TypeName&&) noexcept;
        TypeName& operator=(TypeName&&) noexcept;

        std::string                      name_;
        bool                             is_parameter_{false};
        std::vector<TypeName>            arguments_;
        std::optional<TypeConstraint>    constraint_;
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

    inline TypeName::TypeName() = default;
    inline TypeName::~TypeName() = default;
    inline TypeName::TypeName(const TypeName&) = default;
    inline TypeName& TypeName::operator=(const TypeName&) = default;
    inline TypeName::TypeName(TypeName&&) noexcept = default;
    inline TypeName& TypeName::operator=(TypeName&&) noexcept = default;

    class StructBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        TypeName                          name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::vector<FieldBase*>           fields_;
        std::vector<StructBase*>          nested_types_;
        std::vector<EnumBase*>            nested_enums_;
        std::vector<AliasBase*>           aliases_;
        const StructBase                 *container_{nullptr};
        bool                              is_union_{false};
        bool                              builtin_{false};
        bool                              generic_{false};
        PhysicalType                      phy_type_{PhysicalType::VOID};

    private:
        StructBase();
        ~StructBase() override;
    };

    class AliasBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::string                       target_name_;
        const SymbolBase                 *target_{nullptr};
        const StructBase                 *container_{nullptr};
        bool                              is_using_{false};

    private:
        AliasBase() = default;
    };

    class FunctionBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        TypeName                          return_type_;
        std::vector<FieldBase*>           params_;
        std::vector<const StructBase*>    args_;
        const StructBase                 *ret_type_{nullptr};
        const EnumBase                   *ret_enum_{nullptr};
        const StructBase                 *container_{nullptr};

    private:
        FunctionBase();
        ~FunctionBase() override;
    };

    class ServletBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        TypeName                          name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::vector<TypeName>             base_names_;
        std::vector<const ServletBase*>   bases_;
        std::vector<FunctionBase*>        method_;
        const StructBase                 *container_{nullptr};

    private:
        ServletBase() = default;
    };

    class FileBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::string                       path_;
        std::string                       package_;
        EditionBase                      *edition_{nullptr};
        std::vector<PackageBase*>         packages_;
        std::vector<ImportBase*>          imports_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        std::vector<EnumBase*>            enums_;
        std::vector<StructBase*>          structs_;
        std::vector<ServletBase*>         servlets_;
        std::vector<FunctionBase*>        functions_;
        std::vector<FieldBase*>           constexprs_;
        std::vector<FileBase*>            dependencies_;

    private:
        FileBase() = default;
    };

    class SchemaPoolBase {
    public:
        template <typename T>
        T* create() {
            static_assert(std::is_base_of_v<SymbolBase, T>);
            auto owned = std::unique_ptr<T>(new T());
            T* node = owned.get();
            node->pool_ = this;
            node->sequence_ = static_cast<uint32_t>(nodes_.size());
            nodes_.push_back(std::move(owned));
            return node;
        }

        bool add_symbol(SymbolBase* symbol);
        SymbolBase* get_symbol(std::string_view full_name);
        const SymbolBase* get_symbol(std::string_view full_name) const;

        bool add_file(FileBase* file);
        FileBase* get_file(std::string_view path);
        const FileBase* get_file(std::string_view path) const;

    private:
        std::vector<std::unique_ptr<SymbolBase>>          nodes_;
        fermat::btree_map<std::string, SymbolBase*>       symbols_;
        fermat::btree_map<std::string, FileBase*>         files_;
    };

    class AnnotationBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::vector<CommentBase*>     leading_comment_;
        CommentBase                  *tailing_comment_{nullptr};
        bool                          file_level_{false};
        std::string                   classify_;
        std::string                   key_;
        TypeName                      type_name_;
        ConstValue                   *value_{nullptr};

    private:
        AnnotationBase();
        ~AnnotationBase() override;
    };

    inline StructBase::StructBase() = default;
    inline StructBase::~StructBase() = default;
    inline FunctionBase::FunctionBase() = default;
    inline FunctionBase::~FunctionBase() = default;
    inline AnnotationBase::AnnotationBase() = default;
    inline AnnotationBase::~AnnotationBase() = default;

    class FieldBase : public SymbolBase {
        friend class SchemaPoolBase;
    public:
        std::string                       name_;
        std::string                       full_name_;
        std::string                       full_name_without_package_;
        std::vector<CommentBase*>         leading_comment_;
        CommentBase                      *tailing_comment_{nullptr};
        std::vector<AnnotationBase*>     annotations_;
        FieldLabel                        label_{FieldLabel::REQUIRED};
        TypeName                          type_ref_;
        bool                              generic_{false};
        const StructBase                 *type_{nullptr};
        const EnumBase                   *enum_type_{nullptr};
        const StructBase                 *container_{nullptr};
        uint16_t                          id_{0};
        ConstValue                       *default_value_{nullptr};
        bool                              static_const_{false};

    private:
        FieldBase() = default;
    };

}  // namespace shark
