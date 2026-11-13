// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <string>
#include <vector>

#include <shark/schema/fwd.h>
#include <shark/stubs/strutil.h>
#include <shark/stubs/string_util.h>
#include <shark/port.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // Backport of fold expressions for the comma operator to C++11.
    // Usage:  Fold({expr...});
    // Guaranteed to evaluate left-to-right
    struct ExpressionEater {
        template <typename T>
        ExpressionEater(T&&) { } // NOLINT
    };
    void Fold(std::initializer_list<ExpressionEater>) { }

    template <int R>
    constexpr size_t RoundUpTo(size_t n) {
        static_assert((R & (R - 1)) == 0, "Must be power of two");
        return (n + (R - 1)) & ~(R - 1);
    }

      const int kPackageLimit = 100;



        constexpr size_t Max(size_t a, size_t b) {
            return a > b ? a : b;
        }
        template <typename T, typename... Ts>
        constexpr size_t Max(T a, Ts... b) {
            return Max(a, Max(b...));
        }

        template <typename T>
        constexpr size_t EffectiveAlignof() {
            // `char` is special in that it gets aligned to 8. It is where we drop the
            // trivial structs.
            return std::is_same<T, char>::value ? 8 : alignof(T);
        }

        template <int align, typename U, typename... T>
        using AppendIfAlign =
            typename std::conditional<EffectiveAlignof<U>() == align, void (*)(T..., U),
                void (*)(T...)>::type;

        // Metafunction to sort types in descending order of alignment.
        // Useful for the flat allocator to ensure proper alignment of all elements
        // without having to add padding.
        // Instead of implementing a proper sort metafunction we just do a
        // filter+merge, which is much simpler to write as a metafunction.
        // We have a fixed set of alignments we can filter on.
        // For simplicity we use a function pointer as a type list.
        template <typename In, typename T16, typename T8, typename T4, typename T2,
            typename T1>
        struct TypeListSortImpl;

        template <typename... T16, typename... T8, typename... T4, typename... T2,
            typename... T1>
        struct TypeListSortImpl<void (*)(), void (*)(T16...), void (*)(T8...),
            void (*)(T4...), void (*)(T2...), void (*)(T1...)> {
            using type = void (*)(T16..., T8..., T4..., T2..., T1...);
        };

        template <typename First, typename... Rest, typename... T16, typename... T8,
            typename... T4, typename... T2, typename... T1>
        struct TypeListSortImpl<void (*)(First, Rest...), void (*)(T16...),
            void (*)(T8...), void (*)(T4...), void (*)(T2...),
            void (*)(T1...)> {
            using type = typename TypeListSortImpl<
                void (*)(Rest...), AppendIfAlign<16, First, T16...>,
                AppendIfAlign<8, First, T8...>, AppendIfAlign<4, First, T4...>,
                AppendIfAlign<2, First, T2...>, AppendIfAlign<1, First, T1...>>::type;
        };

        template <typename... T>
        using SortByAlignment =
            typename TypeListSortImpl<void (*)(T...), void (*)(), void (*)(),
                void (*)(), void (*)(), void (*)()>::type;

        template <template <typename...> class C, typename... T>
        auto ApplyTypeList(void (*)(T...)) -> C<T...>;

        template <typename T>
        constexpr int FindTypeIndex() {
            return -1;
        }

        template <typename T, typename T1, typename... Ts>
        constexpr int FindTypeIndex() {
            return std::is_same<T, T1>::value ? 0 : FindTypeIndex<T, Ts...>() + 1;
        }

        // A type to value map, where the possible keys as specified in `Keys...`.
        // The values for key `K` is `ValueT<K>`
        template <template <typename> class ValueT, typename... Keys>
        class TypeMap {
        public:
            template <typename K>
            ValueT<K>& Get() {
                return static_cast<Base<K>&>(payload_).value;
            }

            template <typename K>
            const ValueT<K>& Get() const {
                return static_cast<const Base<K>&>(payload_).value;
            }

        private:
            template <typename K>
            struct Base {
                ValueT<K> value { };
            };
            struct Payload : Base<Keys>... { };
            Payload payload_;
        };

        template <typename T>
        using IntT = int;
        template <typename T>
        using PointerT = T*;

        // Manages an allocation of sequential arrays of type `T...`.
        // It is more space efficient than storing N (ptr, size) pairs, by storing only
        // the pointer to the head and the boundaries between the arrays.
        template <typename... T>
        class FlatAllocation {
        public:
            static constexpr size_t kMaxAlign = Max(alignof(T)...);

            FlatAllocation(const TypeMap<IntT, T...>& ends)
                : ends_(ends) {
                // The arrays start just after FlatAllocation, so adjust the ends.
                Fold({ (ends_.template Get<T>() += RoundUpTo<kMaxAlign>(sizeof(FlatAllocation)))... });
                Fold({ Init<T>()... });
            }

            void Destroy() {
                Fold({ Destroy<T>()... });
                internal::SizedDelete(this, total_bytes());
            }

            template <int I>
            using type = typename std::tuple_element<I, std::tuple<T...>>::type;

            // Gets a tuple of the head pointers for the arrays
            TypeMap<PointerT, T...> Pointers() const {
                TypeMap<PointerT, T...> out;
                Fold({ (out.template Get<T>() = Begin<T>())... });
                return out;
            }

        private:
            // Total number of bytes used by all arrays.
            int total_bytes() const {
                // Get the last end.
                return ends_.template Get<typename std::tuple_element<
                    sizeof...(T) - 1, std::tuple<T...>>::type>();
            }

            template <typename U>
            int BeginOffset() const {
                constexpr int type_index = FindTypeIndex<U, T...>();
                // Avoid a negative value here to keep it compiling when type_index == 0
                constexpr int prev_type_index = type_index == 0 ? 0 : type_index - 1;
                using PrevType =
                    typename std::tuple_element<prev_type_index, std::tuple<T...>>::type;
                // Ensure the types are properly aligned.
                static_assert(EffectiveAlignof<PrevType>() >= EffectiveAlignof<U>(), "");
                return type_index == 0 ? RoundUpTo<kMaxAlign>(sizeof(FlatAllocation))
                                       : ends_.template Get<PrevType>();
            }

            template <typename U>
            int EndOffset() const {
                return ends_.template Get<U>();
            }

            // Avoid the reinterpret_cast if the array is empty.
            // Clang's Control Flow Integrity does not like the cast pointing to memory
            // that is not yet initialized to be of that type.
            // (from -fsanitize=cfi-unrelated-cast)
            template <typename U>
            U* Begin() const {
                int begin = BeginOffset<U>(), end = EndOffset<U>();
                if (begin == end)
                    return nullptr;
                return reinterpret_cast<U*>(data() + begin);
            }

            template <typename U>
            U* End() const {
                int begin = BeginOffset<U>(), end = EndOffset<U>();
                if (begin == end)
                    return nullptr;
                return reinterpret_cast<U*>(data() + end);
            }

            template <typename U>
            bool Init() {
                // Skip for the `char` block. No need to zero initialize it.
                if (std::is_same<U, char>::value)
                    return true;
                for (char *p = data() + BeginOffset<U>(), *end = data() + EndOffset<U>();
                    p != end; p += sizeof(U)) {
                    ::new (p) U { };
                }
                return true;
            }

            template <typename U>
            bool Destroy() {
                if (std::is_trivially_destructible<U>::value)
                    return true;
                for (U *it = Begin<U>(), *end = End<U>(); it != end; ++it) {
                    it->~U();
                }
                return true;
            }

            char* data() const {
                return const_cast<char*>(reinterpret_cast<const char*>(this));
            }

            TypeMap<IntT, T...> ends_;
        };

        template <typename... T>
        TypeMap<IntT, T...> CalculateEnds(const TypeMap<IntT, T...>& sizes) {
            int total = 0;
            TypeMap<IntT, T...> out;
            Fold({ (out.template Get<T>() = total += sizeof(T) * sizes.template Get<T>())... });
            return out;
        }

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
}  // namespace shark

#include <shark/port_undef.inc>
