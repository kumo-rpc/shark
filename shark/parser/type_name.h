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

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace shark::parser {

    class Node;
    class ExpressionLexer;
    template <typename Lex>
    class Lexer;

    struct TypeName;

    /// One predicate call. is_integer has an empty argument list.
    /// is_subclass_of<Myclass> stores name "is_subclass_of" and one TypeName argument.
    struct ConstraintFunction {
        ConstraintFunction();
        ~ConstraintFunction();
        ConstraintFunction(const ConstraintFunction&);
        ConstraintFunction& operator=(const ConstraintFunction&);
        ConstraintFunction(ConstraintFunction&&) noexcept;
        ConstraintFunction& operator=(ConstraintFunction&&) noexcept;

        std::string_view name;
        std::vector<TypeName> arguments;
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

    /// A type written in source. name has no arguments.
    /// list<int32> stores name "list" and one argument.
    /// K[is_integer] sets is_parameter only after binding. Parse leaves it false.
    struct TypeName {
        TypeName();
        ~TypeName();
        TypeName(const TypeName&);
        TypeName& operator=(const TypeName&);
        TypeName(TypeName&&) noexcept;
        TypeName& operator=(TypeName&&) noexcept;

        std::string_view name;
        bool is_parameter{false};
        std::vector<TypeName> arguments;
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

    inline TypeName::TypeName() = default;
    inline TypeName::~TypeName() = default;
    inline TypeName::TypeName(const TypeName&) = default;
    inline TypeName& TypeName::operator=(const TypeName&) = default;
    inline TypeName::TypeName(TypeName&&) noexcept = default;
    inline TypeName& TypeName::operator=(TypeName&&) noexcept = default;

    inline std::string_view type_ref_name(const TypeName& ref) {
        return ref.name;
    }

    std::string format_type_ref(const TypeName& ref);

    // Reads a dotted name, optional <...> arguments, and an optional [constraint].
    bool parse_type_ref(Node& node, Lexer<ExpressionLexer>& lexer, TypeName& out);

}  // namespace shark::parser
