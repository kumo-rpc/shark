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

#include <shark/parser/export.h>

#include <shark/parser/alias.h>
#include <shark/parser/annotation.h>
#include <shark/parser/comment.h>
#include <shark/parser/const_value.h>
#include <shark/parser/edition.h>
#include <shark/parser/enum.h>
#include <shark/parser/enum_value.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/import.h>
#include <shark/parser/package.h>
#include <shark/parser/primitive.h>
#include <shark/parser/servlet.h>
#include <shark/parser/struct.h>
#include <shark/schema/schema_pool.h>
#include <turbo/strings/escaping.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace shark::parser {

    namespace {

        bool unescape_literal(std::string_view raw, std::string* out) {
            if (raw.size() >= 2 && (raw.front() == '"' || raw.front() == '\'' || raw.front() == '`') && raw.back() == raw.front()) {
                raw = raw.substr(1, raw.size() - 2);
            }
            return turbo::c_unescape(raw, out);
        }

        class Exporter {
        public:
            explicit Exporter(SchemaPoolBase& destination) : destination_(destination) {}

            bool run(FileParser& file);
            bool register_symbol(SymbolBase* out, SymbolNode* node);

        private:
            template <typename Parser, typename Schema>
            Schema* make(Parser* node, SymbolType type);

            void copy_type(shark::TypeName& out, const TypeName& in);
            void copy_constraint(shark::TypeConstraint& out, const TypeConstraint& in);
            CommentBase* copy_comment(CommentParser* node);
            AnnotationBase* copy_annotation(AnnotationParser* node);
            ConstValue* copy_value(ConstValueParser* node);
            void copy_symbol_name(std::string& name, std::string& full, std::string& bare, const SymbolNode& node);

            bool create_struct(StructParser* node);
            bool create_alias(AliasParser* alias);
            bool create_enum(EnumParser* node);
            bool create_primitive(PrimitiveParser* node);
            bool create_function(FunctionParser* node);
            bool fill_struct(StructParser* node);
            bool fill_enum(EnumParser* node);
            bool fill_primitive(PrimitiveParser* node);
            bool fill_function(FunctionParser* node);
            bool fill_field(FieldParser* node);

            SchemaPoolBase& destination_;
            std::unordered_map<const void*, SymbolBase*> made_;
        };

        template <typename Parser, typename Schema>
        Schema* Exporter::make(Parser* node, SymbolType type) {
            if (node == nullptr) {
                return nullptr;
            }
            auto found = made_.find(node);
            if (found != made_.end()) {
                return static_cast<Schema*>(found->second);
            }
            Schema* out = destination_.create<Schema>();
            out->symbol_type_ = type;
            out->source_location_ = {node->start_location.line, node->start_location.column};
            made_[node] = out;
            return out;
        }

        void Exporter::copy_constraint(shark::TypeConstraint& out, const TypeConstraint& in) {
            switch (in.op) {
                case TypeConstraint::Op::CALL:
                    out.op_ = shark::TypeConstraint::Op::CALL;
                    break;
                case TypeConstraint::Op::NOT:
                    out.op_ = shark::TypeConstraint::Op::NOT;
                    break;
                case TypeConstraint::Op::AND:
                    out.op_ = shark::TypeConstraint::Op::AND;
                    break;
                case TypeConstraint::Op::OR:
                    out.op_ = shark::TypeConstraint::Op::OR;
                    break;
            }
            out.function_.name_.assign(in.function.name.data(), in.function.name.size());
            out.function_.arguments_.resize(in.function.arguments.size());
            for (std::size_t i = 0; i < in.function.arguments.size(); ++i) {
                copy_type(out.function_.arguments_[i], in.function.arguments[i]);
            }
            out.operands_.resize(in.operands.size());
            for (std::size_t i = 0; i < in.operands.size(); ++i) {
                copy_constraint(out.operands_[i], in.operands[i]);
            }
        }

        void Exporter::copy_type(shark::TypeName& out, const TypeName& in) {
            out.name_.assign(in.name.data(), in.name.size());
            out.is_parameter_ = in.is_parameter;
            out.arguments_.resize(in.arguments.size());
            for (std::size_t i = 0; i < in.arguments.size(); ++i) {
                copy_type(out.arguments_[i], in.arguments[i]);
            }
            if (!in.constraint.has_value()) {
                out.constraint_.reset();
                return;
            }
            out.constraint_ = shark::TypeConstraint();
            copy_constraint(*out.constraint_, *in.constraint);
        }

        void Exporter::copy_symbol_name(std::string& name, std::string& full, std::string& bare, const SymbolNode& node) {
            name.assign(node.name.data(), node.name.size());
            full = node.full_name;
            bare = node.full_name_without_package;
        }

        CommentBase* Exporter::copy_comment(CommentParser* node) {
            CommentBase* out = make<CommentParser, CommentBase>(node, SymbolType::COMMENT);
            if (out == nullptr || !out->comments_.empty()) {
                return out;
            }
            out->is_tail_ = node->is_tailing;
            for (const Token& token : node->content) {
                std::string_view text = token.view();
                out->comments_.emplace_back(text.data(), text.size());
            }
            return out;
        }

        ConstValue* Exporter::copy_value(ConstValueParser* node) {
            ConstValue* out = make<ConstValueParser, ConstValue>(node, SymbolType::CONST);
            if (out == nullptr || !std::holds_alternative<std::monostate>(out->value_)) {
                if (out != nullptr && !std::holds_alternative<std::monostate>(node->value)) {
                    return out;
                }
            }
            if (out == nullptr) {
                return nullptr;
            }
            if (const auto* item = std::get_if<bool>(&node->value)) {
                out->value_ = *item;
            } else if (const auto* item = std::get_if<int64_t>(&node->value)) {
                out->value_ = *item;
            } else if (const auto* item = std::get_if<double>(&node->value)) {
                out->value_ = *item;
            } else if (const auto* item = std::get_if<std::string_view>(&node->value)) {
                std::string unescaped;
                if (!unescape_literal(*item, &unescaped)) {
                    return nullptr;
                }
                out->value_ = std::move(unescaped);
            } else if (const auto* item = std::get_if<ConstValueParser::Identifier>(&node->value)) {
                ConstValue::Identifier id;
                id.name_.assign(item->name.data(), item->name.size());
                id.enum_type_ = static_cast<const EnumBase*>(made_[item->enum_type]);
                id.enumerator_ = static_cast<const EnumValueBase*>(made_[item->enumerator]);
                out->value_ = std::move(id);
            } else if (const auto* item = std::get_if<ConstValueParser::List>(&node->value)) {
                ConstValue::List list;
                for (ConstValueParser* element : item->elements) {
                    list.elements_.push_back(copy_value(element));
                }
                out->value_ = std::move(list);
            } else if (const auto* item = std::get_if<ConstValueParser::Map>(&node->value)) {
                ConstValue::Map map;
                for (const auto& entry : item->entries) {
                    map.entries_.emplace_back(copy_value(entry.first), copy_value(entry.second));
                }
                out->value_ = std::move(map);
            } else if (const auto* item = std::get_if<ConstValueParser::Struct>(&node->value)) {
                ConstValue::Struct body;
                for (const auto& field : item->fields) {
                    ConstValue::StructField copied;
                    copied.field_ = static_cast<const FieldBase*>(made_[field.field]);
                    copied.value_ = copy_value(field.value);
                    body.fields_.push_back(copied);
                }
                out->value_ = std::move(body);
            }
            return out;
        }

        AnnotationBase* Exporter::copy_annotation(AnnotationParser* node) {
            AnnotationBase* out = make<AnnotationParser, AnnotationBase>(node, SymbolType::ANNOTATION);
            if (out == nullptr || !out->key_.empty()) {
                return out;
            }
            if (CommentBase* comment = copy_comment(node->leading_comment)) {
                out->leading_comment_.push_back(comment);
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            out->file_level_ = node->file_level;
            out->classify_.assign(node->classify.data(), node->classify.size());
            out->key_.assign(node->key.data(), node->key.size());
            copy_type(out->type_name_, node->type_ref);
            out->value_ = copy_value(node->object);
            return out;
        }

        namespace {

            const std::vector<AnnotationParser*>* own_annotations(SymbolNode* node) {
                switch (node->node_type) {
                    case SymbolType::STRUCT:
                        return &static_cast<StructParser*>(node)->annotations;
                    case SymbolType::PRIMITIVE:
                        return &static_cast<PrimitiveParser*>(node)->annotations;
                    case SymbolType::ENUM:
                        return &static_cast<EnumParser*>(node)->annotations;
                    case SymbolType::ENUM_VALUE:
                        return &static_cast<EnumValueParser*>(node)->annotations;
                    case SymbolType::FIELD:
                        return &static_cast<FieldParser*>(node)->annotations;
                    case SymbolType::FUNCTION:
                        return &static_cast<FunctionParser*>(node)->annotations;
                    case SymbolType::SERVLET:
                        return &static_cast<ServletParser*>(node)->annotations;
                    case SymbolType::ALIAS:
                        return &static_cast<AliasParser*>(node)->annotations;
                    default:
                        return nullptr;
                }
            }

            std::optional<bool> export_flag(const std::vector<AnnotationParser*>* annotations, std::string_view key) {
                if (annotations == nullptr) {
                    return std::nullopt;
                }
                for (AnnotationParser* annotation : *annotations) {
                    if (annotation->classify != "export" || annotation->key != key || annotation->object == nullptr) {
                        continue;
                    }
                    if (const bool* value = std::get_if<bool>(&annotation->object->value)) {
                        return *value;
                    }
                }
                return std::nullopt;
            }

            bool export_setting(SymbolNode* node, std::string_view key, bool fallback) {
                if (std::optional<bool> own = export_flag(own_annotations(node), key)) {
                    return *own;
                }
                if (node->file != nullptr) {
                    if (std::optional<bool> file_level = export_flag(&node->file->annotations, key)) {
                        return *file_level;
                    }
                }
                return fallback;
            }

        }  // namespace

        bool Exporter::register_symbol(SymbolBase* out, SymbolNode* node) {
            const bool type_name = node->node_type == SymbolType::STRUCT
                || node->node_type == SymbolType::ENUM
                || node->node_type == SymbolType::PRIMITIVE
                || node->node_type == SymbolType::ALIAS
                || node->node_type == SymbolType::SERVLET;
            if (type_name && node->node_type != SymbolType::PRIMITIVE) {
                SymbolBase* existing = destination_.get_symbol(node->name);
                if (existing != nullptr && existing->symbol_type_ == SymbolType::PRIMITIVE) {
                    node->report_error(ErrorType::SEMANTIC, "type `%s` duplicates builtin type", node->name);
                    return false;
                }
            }
            const bool include_package = export_setting(node, "include_package", true);
            const bool exclude_package = export_setting(node, "exclude_package", false);
            if (include_package && !destination_.add_symbol(out)) {
                return false;
            }
            if (exclude_package && node->full_name_without_package != node->full_name) {
                return destination_.add_symbol_name(node->full_name_without_package, out);
            }
            return true;
        }

        bool Exporter::create_enum(EnumParser* node) {
            EnumBase* out = make<EnumParser, EnumBase>(node, SymbolType::ENUM);
            if (out == nullptr) {
                return false;
            }
            copy_symbol_name(out->name_, out->full_name_, out->full_name_without_package_, *node);
            for (EnumValueParser* value : node->values) {
                EnumValueBase* copied = make<EnumValueParser, EnumValueBase>(value, SymbolType::ENUM_VALUE);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *value);
            }
            return register_symbol(out, node);
        }

        bool Exporter::create_primitive(PrimitiveParser* node) {
            PrimitiveBase* out = make<PrimitiveParser, PrimitiveBase>(node, SymbolType::PRIMITIVE);
            if (out == nullptr) {
                return false;
            }
            copy_symbol_name(out->name_, out->full_name_, out->full_name_without_package_, *node);
            return register_symbol(out, node);
        }

        bool Exporter::create_function(FunctionParser* node) {
            FunctionBase* out = make<FunctionParser, FunctionBase>(node, SymbolType::FUNCTION);
            if (out == nullptr) {
                return false;
            }
            copy_symbol_name(out->name_, out->full_name_, out->full_name_without_package_, *node);
            for (FieldParser* param : node->params) {
                FieldBase* copied = make<FieldParser, FieldBase>(param, SymbolType::FIELD);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *param);
            }
            if (!register_symbol(out, node)) {
                return false;
            }
            for (FieldParser* param : node->params) {
                if (!register_symbol(static_cast<FieldBase*>(made_[param]), param)) {
                    return false;
                }
            }
            return true;
        }

        bool Exporter::create_alias(AliasParser* alias) {
            SymbolNode* concrete = alias->file->concrete_type(const_cast<SymbolNode*>(alias->target));
            if (concrete != nullptr && (concrete->node_type == SymbolType::STRUCT || concrete->node_type == SymbolType::ENUM || concrete->node_type == SymbolType::PRIMITIVE)) {
                TypeAlias* copied = make<AliasParser, TypeAlias>(alias, SymbolType::TYPE_ALIAS);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *alias);
                return register_symbol(copied, alias);
            }
            if (concrete != nullptr && concrete->node_type == SymbolType::FUNCTION) {
                FunctionAlias* copied = make<AliasParser, FunctionAlias>(alias, SymbolType::FUNCTION_ALIAS);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *alias);
                return register_symbol(copied, alias);
            }
            if (concrete != nullptr && concrete->node_type == SymbolType::SERVLET) {
                ServletAlias* copied = make<AliasParser, ServletAlias>(alias, SymbolType::SERVLET_ALIAS);
                copied->full_name_ = alias->full_name;
                copied->full_name_without_package_ = alias->full_name_without_package;
                return register_symbol(copied, alias);
            }
            alias->report_error(ErrorType::SEMANTIC, "alias `%s` target `%s` was not bound", alias->name, alias->target_name);
            return false;
        }

        bool Exporter::create_struct(StructParser* node) {
            StructBase* out = make<StructParser, StructBase>(node, SymbolType::STRUCT);
            if (out == nullptr) {
                return false;
            }
            out->full_name_ = node->full_name;
            out->full_name_without_package_ = node->full_name_without_package;
            for (EnumParser* child : node->nested_enums) {
                if (!create_enum(child)) {
                    return false;
                }
            }
            for (StructParser* child : node->nested_types) {
                if (!create_struct(child)) {
                    return false;
                }
            }
            for (FieldParser* field : node->fields) {
                FieldBase* copied = make<FieldParser, FieldBase>(field, SymbolType::FIELD);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *field);
                if (!register_symbol(copied, field)) {
                    return false;
                }
            }
            for (AliasParser* alias : node->aliases) {
                if (!create_alias(alias)) {
                    return false;
                }
            }
            return register_symbol(out, node);
        }

        bool Exporter::fill_field(FieldParser* node) {
            FieldBase* out = static_cast<FieldBase*>(made_[node]);
            if (out == nullptr) {
                return true;
            }
            for (CommentParser* comment : node->leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            for (AnnotationParser* annotation : node->annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            out->label_ = node->label;
            copy_type(out->type_ref_, node->type_ref);
            out->generic_ = node->generic;
            out->type_ = static_cast<const TypeInfo*>(made_[node->type]);
            out->container_ = static_cast<const StructBase*>(made_[node->container]);
            out->id_ = node->id;
            out->default_value_ = copy_value(node->default_value);
            out->static_const_ = node->static_const;
            return true;
        }

        bool Exporter::fill_enum(EnumParser* node) {
            EnumBase* out = static_cast<EnumBase*>(made_[node]);
            for (CommentParser* comment : node->leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            for (AnnotationParser* annotation : node->annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            out->is_class_ = node->is_class;
            out->underlying_ = static_cast<EnumBase::UnderlyingType>(node->underlying);
            out->container_ = static_cast<const StructBase*>(made_[node->container]);
            for (EnumValueParser* value : node->values) {
                EnumValueBase* copied = static_cast<EnumValueBase*>(made_[value]);
                for (CommentParser* comment : value->leading_comment) {
                    copied->leading_comment_.push_back(copy_comment(comment));
                }
                copied->tailing_comment_ = copy_comment(value->tailing_comment);
                copied->value_ = value->value;
                for (AnnotationParser* annotation : value->annotations) {
                    copied->annotations_.push_back(copy_annotation(annotation));
                }
                copied->parent_enum_ = out;
                out->values_.push_back(copied);
                if (!register_symbol(copied, value)) {
                    return false;
                }
            }
            return true;
        }

        bool Exporter::fill_function(FunctionParser* node) {
            FunctionBase* out = static_cast<FunctionBase*>(made_[node]);
            for (CommentParser* comment : node->leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            for (AnnotationParser* annotation : node->annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            out->parameters_.resize(node->parameters.size());
            for (std::size_t i = 0; i < node->parameters.size(); ++i) {
                copy_type(out->parameters_[i], node->parameters[i]);
            }
            copy_type(out->return_type_, node->return_type);
            for (FieldParser* param : node->params) {
                if (!fill_field(param)) {
                    return false;
                }
                out->params_.push_back(static_cast<FieldBase*>(made_[param]));
            }
            out->ret_type_ = static_cast<const TypeInfo*>(made_[node->ret_type]);
            out->container_ = static_cast<const StructBase*>(made_[node->container]);
            out->servlet_ = static_cast<const ServletBase*>(made_[node->servlet]);
            return true;
        }

        bool Exporter::fill_struct(StructParser* node) {
            StructBase* out = static_cast<StructBase*>(made_[node]);
            for (CommentParser* comment : node->leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            for (AnnotationParser* annotation : node->annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            copy_type(out->name_, node->declared_name);
            out->is_union_ = node->is_union;
            out->generic_ = node->generic;
            out->phy_type_ = node->phy_type;
            out->container_ = static_cast<const StructBase*>(made_[node->container]);
            for (EnumParser* child : node->nested_enums) {
                if (!fill_enum(child)) {
                    return false;
                }
                out->nested_enums_.push_back(static_cast<EnumBase*>(made_[child]));
            }
            for (StructParser* child : node->nested_types) {
                if (!fill_struct(child)) {
                    return false;
                }
                out->nested_types_.push_back(static_cast<StructBase*>(made_[child]));
            }
            for (FieldParser* field : node->fields) {
                if (!fill_field(field)) {
                    return false;
                }
                out->fields_.push_back(static_cast<FieldBase*>(made_[field]));
            }
            for (AliasParser* alias : node->aliases) {
                SymbolBase* copied = made_[alias];
                SymbolBase* target = made_[alias->target];
                if (copied->symbol_type_ == SymbolType::TYPE_ALIAS) {
                    TypeAlias* type_alias = static_cast<TypeAlias*>(copied);
                    for (CommentParser* comment : alias->leading_comment) {
                        type_alias->leading_comment_.push_back(copy_comment(comment));
                    }
                    type_alias->tailing_comment_ = copy_comment(alias->tailing_comment);
                    for (AnnotationParser* annotation : alias->annotations) {
                        type_alias->annotations_.push_back(copy_annotation(annotation));
                    }
                    type_alias->target_ = static_cast<const TypeInfo*>(target);
                    type_alias->container_ = out;
                    type_alias->is_using_ = alias->is_using;
                    out->aliases_.push_back(type_alias);
                } else if (copied->symbol_type_ == SymbolType::FUNCTION_ALIAS) {
                    FunctionAlias* function_alias = static_cast<FunctionAlias*>(copied);
                    for (CommentParser* comment : alias->leading_comment) {
                        function_alias->leading_comment_.push_back(copy_comment(comment));
                    }
                    function_alias->tailing_comment_ = copy_comment(alias->tailing_comment);
                    for (AnnotationParser* annotation : alias->annotations) {
                        function_alias->annotations_.push_back(copy_annotation(annotation));
                    }
                    function_alias->target_ = static_cast<const FunctionBase*>(target);
                    function_alias->container_ = out;
                    function_alias->is_using_ = alias->is_using;
                    out->function_aliases_.push_back(function_alias);
                } else {
                    ServletAlias* servlet_alias = static_cast<ServletAlias*>(copied);
                    for (CommentParser* comment : alias->leading_comment) {
                        servlet_alias->leading_comment_.push_back(copy_comment(comment));
                    }
                    servlet_alias->tailing_comment_ = copy_comment(alias->tailing_comment);
                    for (AnnotationParser* annotation : alias->annotations) {
                        servlet_alias->annotations_.push_back(copy_annotation(annotation));
                    }
                    servlet_alias->target_ = static_cast<const ServletBase*>(target);
                    servlet_alias->container_ = out;
                    servlet_alias->is_using_ = alias->is_using;
                    out->servlet_aliases_.push_back(servlet_alias);
                }
            }
            return true;
        }

        bool Exporter::fill_primitive(PrimitiveParser* node) {
            PrimitiveBase* out = static_cast<PrimitiveBase*>(made_[node]);
            for (CommentParser* comment : node->leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(node->tailing_comment);
            for (AnnotationParser* annotation : node->annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            out->phy_type_ = node->phy_type;
            out->container_ = static_cast<const StructBase*>(made_[node->container]);
            return true;
        }

        bool Exporter::run(FileParser& file) {
            FileBase* out = make<FileParser, FileBase>(&file, SymbolType::FILE);
            out->name_.assign(file.name.data(), file.name.size());
            out->full_name_ = file.full_name;
            out->full_name_without_package_ = file.full_name_without_package;
            out->path_ = file.path;
            out->absolute_path_ = file.absolute_path;
            out->package_ = file.package;
            if (!destination_.add_file(out)) {
                return false;
            }

            for (EnumParser* node : file.enums) {
                if (!create_enum(node)) {
                    return false;
                }
            }
            for (StructParser* node : file.structs) {
                if (!create_struct(node)) {
                    return false;
                }
            }
            for (PrimitiveParser* node : file.primitives) {
                if (!create_primitive(node)) {
                    return false;
                }
            }
            for (FieldParser* node : file.constexprs) {
                FieldBase* copied = make<FieldParser, FieldBase>(node, SymbolType::FIELD);
                copy_symbol_name(copied->name_, copied->full_name_, copied->full_name_without_package_, *node);
                if (!register_symbol(copied, node)) {
                    return false;
                }
            }
            for (FunctionParser* node : file.functions) {
                if (!create_function(node)) {
                    return false;
                }
            }
            for (ServletParser* node : file.servlets) {
                ServletBase* copied = make<ServletParser, ServletBase>(node, SymbolType::SERVLET);
                copied->full_name_ = node->full_name;
                copied->full_name_without_package_ = node->full_name_without_package;
                if (!register_symbol(copied, node)) {
                    return false;
                }
                for (FunctionParser* method : node->method) {
                    if (!create_function(method)) {
                        return false;
                    }
                }
            }

            if (file.edition != nullptr) {
                EditionBase* edition = make<EditionParser, EditionBase>(file.edition, SymbolType::EDITION);
                edition->leading_comment_ = copy_comment(file.edition->leading_comment);
                edition->edition_.assign(file.edition->edition.data(), file.edition->edition.size());
                out->edition_ = edition;
            }
            for (PackageParser* node : file.packages) {
                PackageBase* copied = make<PackageParser, PackageBase>(node, SymbolType::PACKAGE);
                copied->leading_comment_ = copy_comment(node->leading_comment);
                copied->tailing_comment_ = copy_comment(node->tailing_comment);
                copied->classify_.assign(node->classify.data(), node->classify.size());
                copied->package_.assign(node->package.data(), node->package.size());
                out->packages_.push_back(copied);
            }
            for (ImportParser* node : file.imports) {
                ImportBase* copied = make<ImportParser, ImportBase>(node, SymbolType::IMPORT);
                copied->leading_comment_ = copy_comment(node->leading_comment);
                copied->tailing_comment_ = copy_comment(node->tailing_comment);
                for (AnnotationParser* annotation : node->annotations) {
                    copied->annotations_.push_back(copy_annotation(annotation));
                }
                copied->classify_.assign(node->classify.data(), node->classify.size());
                if (!unescape_literal(node->path, &copied->path_)) {
                    return false;
                }
                out->imports_.push_back(copied);
            }
            for (CommentParser* comment : file.leading_comment) {
                out->leading_comment_.push_back(copy_comment(comment));
            }
            out->tailing_comment_ = copy_comment(file.tailing_comment);
            for (AnnotationParser* annotation : file.annotations) {
                out->annotations_.push_back(copy_annotation(annotation));
            }
            for (EnumParser* node : file.enums) {
                if (!fill_enum(node)) {
                    return false;
                }
                out->enums_.push_back(static_cast<EnumBase*>(made_[node]));
            }
            for (StructParser* node : file.structs) {
                if (!fill_struct(node)) {
                    return false;
                }
                out->structs_.push_back(static_cast<StructBase*>(made_[node]));
            }
            for (PrimitiveParser* node : file.primitives) {
                if (!fill_primitive(node)) {
                    return false;
                }
                out->primitives_.push_back(static_cast<PrimitiveBase*>(made_[node]));
            }
            for (FieldParser* node : file.constexprs) {
                if (!fill_field(node)) {
                    return false;
                }
                out->constexprs_.push_back(static_cast<FieldBase*>(made_[node]));
            }
            for (FunctionParser* node : file.functions) {
                if (!fill_function(node)) {
                    return false;
                }
                out->functions_.push_back(static_cast<FunctionBase*>(made_[node]));
            }
            for (ServletParser* node : file.servlets) {
                ServletBase* copied = static_cast<ServletBase*>(made_[node]);
                for (CommentParser* comment : node->leading_comment) {
                    copied->leading_comment_.push_back(copy_comment(comment));
                }
                copied->tailing_comment_ = copy_comment(node->tailing_comment);
                for (AnnotationParser* annotation : node->annotations) {
                    copied->annotations_.push_back(copy_annotation(annotation));
                }
                copy_type(copied->name_, node->declared_name);
                copied->base_names_.resize(node->base_names.size());
                for (std::size_t i = 0; i < node->base_names.size(); ++i) {
                    copy_type(copied->base_names_[i], node->base_names[i]);
                }
                for (const ServletParser* base : node->bases) {
                    copied->bases_.push_back(static_cast<const ServletBase*>(made_[base]));
                }
                for (FunctionParser* method : node->method) {
                    if (!fill_function(method)) {
                        return false;
                    }
                    copied->method_.push_back(static_cast<FunctionBase*>(made_[method]));
                }
                out->servlets_.push_back(copied);
            }
            return true;
        }

    }  // namespace

    bool export_file(SchemaPoolBase& destination, FileParser& file) {
        Exporter exporter(destination);
        return exporter.run(file);
    }

}  // namespace shark::parser
