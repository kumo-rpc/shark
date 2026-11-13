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

// Enum support utilities.
//
// Generated code never emits per-enum free functions like
// `{ClassName}_IsValid` / `{ClassName}_Name`. Instead, for every generated
// `enum class E` it emits a single explicit specialization of
// `EnumTraitImpl<E>`. All access goes through `EnumTrait<E>`, a thin
// dispatcher that also derives the common helpers. Because the key is the
// enum type itself, a nested enum (`Message::Enum`) is registered as
// `EnumTraitImpl<Message::Enum>` with no `Message_Enum` symbol leaking into
// the enclosing namespace.
//
// A generated specialization of `EnumTraitImpl<E>` must provide:
//
//   - `using underlying`  one of int8/uint8/int16/uint16/int32/uint32
//   - `enum_min`, `enum_max`        constexpr bounds
//   - `value_count`                 number of enumerators
//   - `is_valid(int64_t)`           raw wire value is a known enumerator
//   - `descriptor()`                const EnumDescriptor* (nullptr in lite)
//   - `enum_name(E)`                canonical name ("" in lite)
//   - `enum_parse(string_view, E*)  name -> value (false in lite)
//
// `EnumTrait<E>` adds `to_underlying`, `fits_underlying`, `fits_wire`,
// `underlying_min/max`, and forwards the rest.
//
// Call sites use `shark::enum_name(v)` / `EnumTrait<E>::to_underlying(v)`
// (E deduced from the argument), or name the type for int-taking helpers:
// `EnumTrait<MyEnum>::is_valid(n)`. There is no integral-argument overload
// of `enum_name`; cast first.
//
// The default `EnumTraitImpl<E>` has `underlying = void`, and `EnumTrait<E>`
// static_asserts against it: an enum that was never registered fails to
// compile rather than silently degrading. Use `is_shark_enum<E>()` to test
// registration without instantiating `EnumTrait<E>`.

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <string_view>
#include <limits>
#include <shark/port.h>

namespace shark {

    class EnumDescriptor;

    ////////////////////////////////////////////////////////////////////////
    /// generated
    ////////////////////////////////////////////////////////////////////////
    /// DO NOT instantiate directly. `EnumTraitImpl<E>` is a customization
    /// point: the code generator emits one explicit specialization per
    /// generated `enum class E`. The primary template below is the
    /// "unregistered" case (`underlying = void`) and exists only so that
    /// `is_shark_enum<E>()` can probe registration without triggering the
    /// static_assert in `EnumTrait<E>`.
    ///
    /// A specialization must provide all of:
    ///   using underlying;              int8/uint8/int16/uint16/int32/uint32
    ///   static constexpr E enum_min;
    ///   static constexpr E enum_max;
    ///   static constexpr uint32_t enum_array_size;
    ///   static bool is_valid(int64_t);        // range already checked by caller
    ///   static const EnumDescriptor* descriptor();
    ///   static const std::string& enum_name(E);
    ///   static bool enum_parse(std::string_view, E*);
    ///
    /// Example generated output:
    ///
    ///   enum class Color : uint8_t { RED = 1, GREEN = 2, BLUE = 3 };
    ///
    ///   template <>
    ///   struct EnumTraitImpl<Color> {
    ///       using underlying = uint8_t;
    ///       static constexpr Color enum_min = Color::RED;
    ///       static constexpr Color enum_max = Color::BLUE;
    ///       static constexpr uint32_t enum_array_size = 3;
    ///       static bool is_valid(int64_t v);
    ///       static const EnumDescriptor* descriptor();
    ///       static const std::string& enum_name(Color v);
    ///       static bool enum_parse(std::string_view name, Color* out);
    ///   };
    ///
    ///   // bodies of is_valid / descriptor / enum_name / enum_parse go to
    ///   // the generated .cc (lite mode: nullptr / "" / false).
    template <typename E>
    struct EnumTraitImpl {
        using underlying = void;
        static constexpr E enum_min = static_cast<E>(0);
        static constexpr E enum_max = static_cast<E>(0);

        static constexpr uint32_t value_count = 0;

        static constexpr bool has_descriptor = false;

        static inline const EnumDescriptor* descriptor() {
            return nullptr;
        }

        static inline const std::string& enum_name(E enum_t_value) {
            (void)enum_t_value;
            static const std::string kEmpty;
            return kEmpty;
        }

        static inline bool enum_parse(ConstStringParam name, E* value) {
            (void)name;
            (void)value;
            return false;
        }

        /// no need to check range for underlying
        static inline bool is_valid(int64_t value) {
            (void)value;
            return false;
        }
    };

    template <typename E, bool Has>
    struct EnumDescriptorProvider {};

    template <typename E>
    struct EnumDescriptorProvider<E, true> {
        static inline const EnumDescriptor* descriptor() {
            return EnumTraitImpl<E>::descriptor();
        }
    };

    template <typename E>
    struct EnumTrait : public EnumDescriptorProvider<E,EnumTraitImpl<E>::has_descriptor>{
        using underlying = typename EnumTraitImpl<E>::underlying;
        using enum_type = E;

        static_assert(!std::is_same_v<void,underlying>, "not shark generated enum");
        static_assert(std::is_same_v<uint8_t,underlying> ||
            std::is_same_v<int8_t,underlying> ||
            std::is_same_v<uint16_t,underlying> ||
            std::is_same_v<int16_t,underlying> ||
            std::is_same_v<uint32_t,underlying> ||
            std::is_same_v<int32_t,underlying>,
            "underlying must be one of `int8_|uint8_|int16_|uint16_|int32_t|uint32_t`");
        static_assert(sizeof(E) == sizeof(underlying), "enum size not match underlying size");

        static constexpr E enum_min = EnumTraitImpl<E>::enum_min;
        static constexpr E enum_max = EnumTraitImpl<E>::enum_max;

        static constexpr uint32_t value_count = EnumTraitImpl<E>::value_count;

        static constexpr underlying underlying_min = static_cast<underlying>(enum_min);
        static constexpr underlying underlying_max = static_cast<underlying>(enum_max);

        static constexpr bool has_descriptor = EnumTraitImpl<E>::has_descriptor;

        static constexpr bool fits_underlying(int64_t value) {
            return EnumTraitImpl<E>::fits_underlying(value);
        }

        static constexpr bool fits_wire(::uint64_t value) {
            return EnumTraitImpl<E>::fits_wire(value);
        }

        static constexpr underlying to_underlying(enum_type value) {
            return static_cast<underlying>(value);
        }

        static inline const std::string& enum_name(enum_type enum_t_value) {
            return EnumTraitImpl<E>::enum_name(enum_t_value);
        }

        template<typename T>
        static inline const std::string& enum_name(T enum_t_value) {
            static_assert(std::is_integral_v<T> || std::is_enum_v<T>,"must be integer or enum");
            return EnumTraitImpl<E>::enum_name(static_cast<E>(enum_t_value));
        }

        static inline bool enum_parse(ConstStringParam name, enum_type* value) {
            return EnumTraitImpl<E>::enum_parse(name, value);
        }

        static inline bool is_valid(int64_t value) {
            return fits_underlying(value) && EnumTraitImpl<E>::is_valid(value);
        }
    };


    template <typename E>
    struct IsSharkEnum: ::std::false_type {};

    template<typename E>
    constexpr bool is_shark_enum() {
        return IsSharkEnum<E>::value;
    }
} // namespace shark
