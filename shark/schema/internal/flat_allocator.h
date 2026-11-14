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

#include <cstdint>
#include <shark/schema/internal/file_descriptor_tables.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // The implementation for FlatAllocator below.
        // This separate class template makes it easier to have methods that fold on
        // `T...`.
        template <typename... T>
        class FlatAllocatorImpl {
        public:
            using Allocation = FlatAllocation<T...>;

            template <typename U>
            void PlanArray(int array_size) {
                // We can't call PlanArray after FinalizePlanning has been called.
                GOOGLE_CHECK(!has_allocated());
                if (std::is_trivially_destructible<U>::value) {
                    // Trivial types are aligned to 8 bytes.
                    static_assert(alignof(U) <= 8, "");
                    total_.template Get<char>() += RoundUpTo<8>(array_size * sizeof(U));
                } else {
                    // Since we can't use `if constexpr`, just make the expression compile
                    // when this path is not taken.
                    using TypeToUse =
                        typename std::conditional<std::is_trivially_destructible<U>::value,
                            char, U>::type;
                    total_.template Get<TypeToUse>() += array_size;
                }
            }

            template <typename U>
            U* AllocateArray(int array_size) {
                constexpr bool trivial = std::is_trivially_destructible<U>::value;
                using TypeToUse = typename std::conditional<trivial, char, U>::type;

                // We can only allocate after FinalizePlanning has been called.
                GOOGLE_CHECK(has_allocated());

                TypeToUse*& data = pointers_.template Get<TypeToUse>();
                int& used = used_.template Get<TypeToUse>();
                U* res = reinterpret_cast<U*>(data + used);
                used += trivial ? RoundUpTo<8>(array_size * sizeof(U)) : array_size;
                GOOGLE_CHECK_LE(used, total_.template Get<TypeToUse>());
                return res;
            }

            template <typename... In>
            const std::string* AllocateStrings(In&&... in) {
                std::string* strings = AllocateArray<std::string>(sizeof...(in));
                std::string* res = strings;
                Fold({ (*strings++ = std::string(std::forward<In>(in)))... });
                return res;
            }

            // Allocate all 5 names of the field:
            // name, full name, lowercase, camelcase and json.
            // It will dedup the strings when possible.
            // The resulting array contains `name` at index 0, `full_name` at index 1
            // and the other 3 indices are specified in the result.
            void PlanFieldNames(const std::string& name,
                const std::string* opt_json_name) {
                GOOGLE_CHECK(!has_allocated());

                // Fast path for snake_case names, which follow the style guide.
                if (opt_json_name == nullptr) {
                    switch (GetFieldNameCase(name)) {
                    case FieldNameCase::kAllLower:
                        // Case 1: they are all the same.
                        return PlanArray<std::string>(2);
                    case FieldNameCase::kSnakeCase:
                        // Case 2: name==lower, camel==json
                        return PlanArray<std::string>(3);
                    default:
                        break;
                    }
                }

                std::string lowercase_name = name;
                LowerString(&lowercase_name);

                std::string camelcase_name = ToCamelCase(name, /* lower_first = */ true);
                std::string json_name = opt_json_name != nullptr ? *opt_json_name : ToJsonName(name);

                StringPiece all_names[] = { name, lowercase_name, camelcase_name,
                    json_name };
                std::sort(all_names, all_names + 4);
                int unique = static_cast<int>(std::unique(all_names, all_names + 4) - all_names);

                PlanArray<std::string>(unique + 1);
            }

            struct FieldNamesResult {
                const std::string* array;
                int lowercase_index;
                int camelcase_index;
                int json_index;
            };
            FieldNamesResult AllocateFieldNames(const std::string& name,
                const std::string& scope,
                const std::string* opt_json_name) {
                GOOGLE_CHECK(has_allocated());

                std::string full_name = scope.empty() ? name : StrCat(scope, ".", name);

                // Fast path for snake_case names, which follow the style guide.
                if (opt_json_name == nullptr) {
                    switch (GetFieldNameCase(name)) {
                    case FieldNameCase::kAllLower:
                        // Case 1: they are all the same.
                        return { AllocateStrings(name, std::move(full_name)), 0, 0, 0 };
                    case FieldNameCase::kSnakeCase:
                        // Case 2: name==lower, camel==json
                        return { AllocateStrings(name, std::move(full_name),
                                     ToCamelCase(name, /* lower_first = */ true)),
                            0, 2, 2 };
                    default:
                        break;
                    }
                }

                std::vector<std::string> names;
                names.push_back(name);
                names.push_back(std::move(full_name));

                const auto push_name = [&](std::string new_name) {
                    for (size_t i = 0; i < names.size(); ++i) {
                        // Do not compare the full_name. It is unlikely to match, except in
                        // custom json_name. We are not taking this into account in
                        // PlanFieldNames so better to not try it.
                        if (i == 1)
                            continue;
                        if (names[i] == new_name)
                            return i;
                    }
                    names.push_back(std::move(new_name));
                    return names.size() - 1;
                };

                FieldNamesResult result { nullptr, 0, 0, 0 };

                std::string lowercase_name = name;
                LowerString(&lowercase_name);
                result.lowercase_index = push_name(std::move(lowercase_name));
                result.camelcase_index = push_name(ToCamelCase(name, /* lower_first = */ true));
                result.json_index = push_name(opt_json_name != nullptr ? *opt_json_name : ToJsonName(name));

                std::string* all_names = AllocateArray<std::string>(names.size());
                result.array = all_names;
                std::move(names.begin(), names.end(), all_names);

                return result;
            }

            template <typename Alloc>
            void FinalizePlanning(Alloc& alloc) {
                GOOGLE_CHECK(!has_allocated());

                pointers_ = alloc->CreateFlatAlloc(total_)->Pointers();

                GOOGLE_CHECK(has_allocated());
            }

            void ExpectConsumed() const {
                // We verify that we consumed all the memory requested if there was no
                // error in processing.
                Fold({ ExpectConsumed<T>()... });
            }

        private:
            bool has_allocated() const {
                return pointers_.template Get<char>() != nullptr;
            }

            static bool IsLower(char c) { return 'a' <= c && c <= 'z'; }
            static bool IsDigit(char c) { return '0' <= c && c <= '9'; }
            static bool IsLowerOrDigit(char c) { return IsLower(c) || IsDigit(c); }

            enum class FieldNameCase { kAllLower,
                kSnakeCase,
                kOther };
            FieldNameCase GetFieldNameCase(const std::string& name) {
                if (!IsLower(name[0]))
                    return FieldNameCase::kOther;
                FieldNameCase best = FieldNameCase::kAllLower;
                for (char c : name) {
                    if (IsLowerOrDigit(c)) {
                        // nothing to do
                    } else if (c == '_') {
                        best = FieldNameCase::kSnakeCase;
                    } else {
                        return FieldNameCase::kOther;
                    }
                }
                return best;
            }

            template <typename U>
            bool ExpectConsumed() const {
                GOOGLE_CHECK_EQ(total_.template Get<U>(), used_.template Get<U>());
                return true;
            }

            TypeMap<PointerT, T...> pointers_;
            TypeMap<IntT, T...> total_;
            TypeMap<IntT, T...> used_;
        };

    namespace internal {

        // Small sequential allocator to be used within a single file.
        // Most of the memory for a single FileDescriptor and everything under it is
        // allocated in a single block of memory, with the FlatAllocator giving it out
        // in parts later.
        // The code first plans the total number of bytes needed by calling PlanArray
        // with all the allocations that will happen afterwards, then calls
        // FinalizePlanning passing the underlying allocator (the DescriptorPool::Tables
        // instance), and then proceeds to get the memory via
        // `AllocateArray`/`AllocateString` calls. The calls to PlanArray and
        // The calls have to match between planning and allocating, though not
        // necessarily in the same order.
        class FlatAllocator
            : public decltype(ApplyTypeList<FlatAllocatorImpl>(
                  SortByAlignment<char, std::string, SourceCodeInfo,
                      FileDescriptorTables,
                      // Option types
                      MessageOptions, FieldOptions, EnumOptions,
                      EnumValueOptions, ExtensionRangeOptions, OneofOptions,
                      ServiceOptions, MethodOptions, FileOptions>())) { };

    } // namespace internal

} // namespace shark


#include <shark/port_undef.inc>
