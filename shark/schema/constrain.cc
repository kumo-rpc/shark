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

#include <shark/schema/constrain.h>
#include <shark/common/clog.h>

#include <string>
#include <string_view>
#include <variant>

namespace shark {

    namespace {

        bool has_trait(const StructBase* type, std::string_view trait) {
            if (type == nullptr) {
                return false;
            }
            for (AnnotationBase* annotation : type->annotations_) {
                if (annotation == nullptr || annotation->classify_ != "builtin" || annotation->key_ != "traits") {
                    continue;
                }
                if (annotation->value_ == nullptr) {
                    continue;
                }
                if (!std::holds_alternative<ConstValue::List>(annotation->value_->value_)) {
                    continue;
                }
                for (ConstValue* element : std::get<ConstValue::List>(annotation->value_->value_).elements_) {
                    if (element == nullptr) {
                        continue;
                    }
                    if (!std::holds_alternative<std::string>(element->value_)) {
                        continue;
                    }
                    if (std::get<std::string>(element->value_) == trait) {
                        return true;
                    }
                }
            }
            return false;
        }

    }  // namespace

    bool is_integer(const StructBase *type) { return has_trait(type, "is_integer"); }
    bool is_void(const StructBase *type) { return has_trait(type, "is_void"); }
    bool is_primitive(const StructBase *type) { return has_trait(type, "is_primitive"); }
    bool is_pod(const StructBase *type) { return has_trait(type, "is_pod"); }
    bool is_signed(const StructBase *type) { return has_trait(type, "is_signed"); }
    bool is_numeric(const StructBase *type) { return has_trait(type, "is_numeric"); }
    bool is_container(const StructBase *type) { return has_trait(type, "is_container"); }
    bool is_string(const StructBase *type) { return type != nullptr && type->phy_type_ == PhysicalType::STRING; }
    bool is_list(const StructBase *type) { return type != nullptr && type->phy_type_ == PhysicalType::LIST; }
    bool is_map(const StructBase *type) { return type != nullptr && type->phy_type_ == PhysicalType::MAP; }
    bool is_set(const StructBase *type) { return type != nullptr && type->phy_type_ == PhysicalType::SET; }

    void register_constrain(SchemaPoolBase *pool) {
        pool->add_constrain("is_integer", is_integer);
        pool->add_constrain("is_void", is_void);
        pool->add_constrain("is_primitive", is_primitive);
        pool->add_constrain("is_pod", is_pod);
        pool->add_constrain("is_signed", is_signed);
        pool->add_constrain("is_numeric", is_numeric);
        pool->add_constrain("is_container", is_container);
        pool->add_constrain("is_string", is_string);
        pool->add_constrain("is_list", is_list);
        pool->add_constrain("is_map", is_map);
        pool->add_constrain("is_set", is_set);
    }

    namespace {

        const StructBase* lookup_struct(SchemaPoolBase* pool, const FileBase* file, const TypeName& type) {
            if (pool == nullptr || type.is_parameter_) {
                return nullptr;
            }
            SymbolBase* symbol = pool->get_symbol(type.name_);
            if (symbol == nullptr && file != nullptr && !file->package_.empty()) {
                symbol = pool->get_symbol(file->package_ + "." + type.name_);
            }
            if (symbol == nullptr || symbol->symbol_type_ != SymbolType::STRUCT) {
                return nullptr;
            }
            return static_cast<const StructBase*>(symbol);
        }

        std::string located(const FileBase* file, const SourceLocation& location, std::string_view level, const std::string& message) {
            const std::string path = file == nullptr ? std::string() : file->absolute_path_;
            return path + ":" + std::to_string(location.line) + ":" + std::to_string(location.column) + " [SEMANTIC] " + std::string(level) + ": " + message;
        }

        bool eval_constrain(SchemaPoolBase* pool, const FileBase* file, const SourceLocation& location, const TypeConstraint& constraint, StructBase* type) {
            switch (constraint.op_) {
                case TypeConstraint::Op::CALL: {
                    constrain_func* func = pool->get_constrain(constraint.function_.name_);
                    if (func == nullptr || !(*func)) {
                        shark::common::report_error(located(file, location, "error", "constrain `" + constraint.function_.name_ + "` was not found"));
                        return false;
                    }
                    return (*func)(type);
                }
                case TypeConstraint::Op::NOT:
                    if (constraint.operands_.empty()) {
                        return true;
                    }
                    return !eval_constrain(pool, file, location, constraint.operands_[0], type);
                case TypeConstraint::Op::AND:
                    if (constraint.operands_.size() < 2) {
                        return true;
                    }
                    return eval_constrain(pool, file, location, constraint.operands_[0], type)
                        && eval_constrain(pool, file, location, constraint.operands_[1], type);
                case TypeConstraint::Op::OR:
                    if (constraint.operands_.size() < 2) {
                        return true;
                    }
                    return eval_constrain(pool, file, location, constraint.operands_[0], type)
                        || eval_constrain(pool, file, location, constraint.operands_[1], type);
            }
            return true;
        }

        bool check_type_arguments(SchemaPoolBase* pool, const FileBase* file, const SourceLocation& location, const TypeName& use) {
            if (use.arguments_.empty() || use.is_parameter_) {
                return true;
            }
            const StructBase* generic = use.type_;
            if (generic == nullptr) {
                shark::common::report_error(located(file, location, "error", "constrain `" + use.name_ + "` type was not bound"));
                return false;
            }
            if (generic->name_.arguments_.size() != use.arguments_.size()) {
                shark::common::report_error(located(file, location, "error", "constrain `" + use.name_ + "` argument count mismatch"));
                return false;
            }
            for (std::size_t i = 0; i < generic->name_.arguments_.size(); ++i) {
                const TypeName& parameter = generic->name_.arguments_[i];
                const TypeName& argument = use.arguments_[i];
                if (argument.is_parameter_ || !parameter.constraint_.has_value()) {
                    continue;
                }
                if (argument.type_ == nullptr) {
                    shark::common::report_error(located(file, location, "error", "constrain `" + use.name_ + "` argument `" + argument.name_ + "` was not bound"));
                    return false;
                }
                if (!eval_constrain(pool, file, location, *parameter.constraint_, const_cast<StructBase*>(argument.type_))) {
                    return false;
                }
            }
            return true;
        }

    }  // namespace

    bool check_type_name(SchemaPoolBase* pool, const FileBase* file, const SourceLocation& location, const TypeName& type) {
        if (pool == nullptr || type.arguments_.empty() || type.is_parameter_) {
            return true;
        }
        shark::common::report_debug(located(file, location, "debug", "constrain `" + type.name_ + "` arguments `" + std::to_string(type.arguments_.size()) + "`"));
        if (!check_type_arguments(pool, file, location, type)) {
            return false;
        }
        for (const TypeName& argument : type.arguments_) {
            if (!check_type_name(pool, file, location, argument)) {
                return false;
            }
        }
        return true;
    }

    namespace {

        bool check_struct(SchemaPoolBase* pool, const FileBase* file, const StructBase* node) {
            if (node == nullptr) {
                return true;
            }
            for (const FieldBase* field : node->fields_) {
                if (field != nullptr && !check_type_name(pool, file, field->source_location_, field->type_ref_)) {
                    return false;
                }
            }
            for (const StructBase* nested : node->nested_types_) {
                if (!check_struct(pool, file, nested)) {
                    return false;
                }
            }
            return true;
        }

        void bind_type_name(SchemaPoolBase* pool, const FileBase* file, TypeName& type) {
            if (!type.is_parameter_) {
                type.type_ = lookup_struct(pool, file, type);
            }
            for (TypeName& argument : type.arguments_) {
                bind_type_name(pool, file, argument);
            }
        }

        void bind_struct(SchemaPoolBase* pool, const FileBase* file, StructBase* node) {
            if (node == nullptr) {
                return;
            }
            for (FieldBase* field : node->fields_) {
                if (field != nullptr) {
                    bind_type_name(pool, file, field->type_ref_);
                }
            }
            for (StructBase* nested : node->nested_types_) {
                bind_struct(pool, file, nested);
            }
        }

        void bind_function(SchemaPoolBase* pool, const FileBase* file, FunctionBase* node) {
            if (node == nullptr) {
                return;
            }
            for (FieldBase* field : node->params_) {
                if (field != nullptr) {
                    bind_type_name(pool, file, field->type_ref_);
                }
            }
            bind_type_name(pool, file, node->return_type_);
        }

        bool check_function(SchemaPoolBase* pool, const FileBase* file, const FunctionBase* node) {
            if (node == nullptr) {
                return true;
            }
            for (const FieldBase* field : node->params_) {
                if (field != nullptr && !check_type_name(pool, file, field->source_location_, field->type_ref_)) {
                    return false;
                }
            }
            return check_type_name(pool, file, node->source_location_, node->return_type_);
        }

    }  // namespace

    bool bind_schema_types(FileBase* file) {
        if (file == nullptr || file->pool_ == nullptr) {
            return false;
        }
        SchemaPoolBase* pool = file->pool_;
        for (StructBase* node : file->structs_) {
            bind_struct(pool, file, node);
        }
        for (FunctionBase* node : file->functions_) {
            bind_function(pool, file, node);
        }
        for (ServletBase* servlet : file->servlets_) {
            if (servlet == nullptr) {
                continue;
            }
            for (TypeName& base_name : servlet->base_names_) {
                bind_type_name(pool, file, base_name);
            }
            for (FunctionBase* node : servlet->method_) {
                bind_function(pool, file, node);
            }
        }
        return true;
    }

    bool run_constrain_check(const FileBase *file) {
        if (file == nullptr || file->pool_ == nullptr) {
            shark::common::report_debug("constrain check missing file");
            return false;
        }
        shark::common::report_debug(file->absolute_path_ + " constrain check");
        SchemaPoolBase* pool = file->pool_;
        for (const StructBase* node : file->structs_) {
            if (!check_struct(pool, file, node)) {
                return false;
            }
        }
        for (const FunctionBase* node : file->functions_) {
            if (!check_function(pool, file, node)) {
                return false;
            }
        }
        for (const ServletBase* servlet : file->servlets_) {
            if (servlet == nullptr) {
                continue;
            }
            for (const TypeName& base_name : servlet->base_names_) {
                if (!check_type_name(pool, file, servlet->source_location_, base_name)) {
                    return false;
                }
            }
            for (const FunctionBase* node : servlet->method_) {
                if (!check_function(pool, file, node)) {
                    return false;
                }
            }
        }
        shark::common::report_debug(file->absolute_path_ + " constrain check success");
        return true;
    }

}  // namespace shark

