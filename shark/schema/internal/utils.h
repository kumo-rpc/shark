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


#include <shark/descriptor.h>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <shark/types/any.h>
#include <shark/descriptor.skp.h>
#include <shark/descriptor_database.h>
#include <shark/dynamic_message.h>
#include <shark/generated/generated_message_util.h>
#include <shark/io/coded_stream.h>
#include <shark/io/strtod.h>
#include <shark/io/tokenizer.h>
#include <shark/io/zero_copy_stream_impl.h>
#include <shark/port.h>
#include <shark/stubs/casts.h>
#include <shark/stubs/string_util.h>
#include <shark/stubs/common.h>
#include <shark/stubs/hash.h>
#include <shark/stubs/logging.h>
#include <shark/stubs/map_util.h>
#include <shark/stubs/once.h>
#include <shark/stubs/stl_util.h>
#include <shark/stubs/stringprintf.h>
#include <shark/stubs/strutil.h>
#include <shark/stubs/substitute.h>
#include <shark/format/text/text_format.h>
#include <shark/unknown_field_set.h>

#undef PACKAGE // autoheader #defines this.  :(

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
    inline void Fold(std::initializer_list<ExpressionEater>) { }

    static constexpr int kPackageLimit = 100;

    template <int R>
    constexpr size_t RoundUpTo(size_t n) {
        static_assert((R & (R - 1)) == 0, "Must be power of two");
        return (n + (R - 1)) & ~(R - 1);
    }

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

        inline std::string EnumValueToPascalCase(const std::string& input) {
            bool next_upper = true;
            std::string result;
            result.reserve(input.size());

            for (char character : input) {
                if (character == '_') {
                    next_upper = true;
                } else {
                    if (next_upper) {
                        result.push_back(ToUpper(character));
                    } else {
                        result.push_back(ToLower(character));
                    }
                    next_upper = false;
                }
            }

            return result;
        }

        // Class to remove an enum prefix from enum values.
        class PrefixRemover {
        public:
            PrefixRemover(StringPiece prefix) {
                // Strip underscores and lower-case the prefix.
                for (char character : prefix) {
                    if (character != '_') {
                        prefix_ += ascii_tolower(character);
                    }
                }
            }

            // Tries to remove the enum prefix from this enum value.
            // If this is not possible, returns the input verbatim.
            std::string MaybeRemove(StringPiece str) {
                // We can't just lowercase and strip str and look for a prefix.
                // We need to properly recognize the difference between:
                //
                //   enum Foo {
                //     FOO_BAR_BAZ = 0;
                //     FOO_BARBAZ = 1;
                //   }
                //
                // This is acceptable (though perhaps not advisable) because even when
                // we PascalCase, these two will still be distinct (BarBaz vs. Barbaz).
                size_t i, j;

                // Skip past prefix_ in str if we can.
                for (i = 0, j = 0; i < str.size() && j < prefix_.size(); i++) {
                    if (str[i] == '_') {
                        continue;
                    }

                    if (ascii_tolower(str[i]) != prefix_[j++]) {
                        return std::string(str);
                    }
                }

                // If we didn't make it through the prefix, we've failed to strip the
                // prefix.
                if (j < prefix_.size()) {
                    return std::string(str);
                }

                // Skip underscores between prefix and further characters.
                while (i < str.size() && str[i] == '_') {
                    i++;
                }

                // Enum label can't be the empty string.
                if (i == str.size()) {
                    return std::string(str);
                }

                // We successfully stripped the prefix.
                str.remove_prefix(i);
                return std::string(str);
            }

        private:
            std::string prefix_;
        };

    ////////////////////////
    // A DescriptorPool contains a bunch of hash-maps to implement the
        // various Find*By*() methods.  Since hashtable lookups are O(1), it's
        // most efficient to construct a fixed set of large hash-maps used by
        // all objects in the pool rather than construct one or more small
        // hash-maps for each object.
        //
        // The keys to these hash-maps are (parent, name) or (parent, number) pairs.

        typedef std::pair<const void*, StringPiece> PointerStringPair;

        typedef std::pair<const Descriptor*, int> DescriptorIntPair;

#define HASH_MAP std::unordered_map
#define HASH_SET std::unordered_set
#define HASH_FXN hash

        template <typename PairType>
        struct PointerIntegerPairHash {
            size_t operator()(const PairType& p) const {
                static const size_t prime1 = 16777499;
                static const size_t prime2 = 16777619;
                return reinterpret_cast<size_t>(p.first) * prime1 ^ static_cast<size_t>(p.second) * prime2;
            }

#ifdef _MSC_VER
            // Used only by MSVC and platforms where hash_map is not available.
            static const size_t bucket_size = 4;
            static const size_t min_buckets = 8;
#endif
            inline bool operator()(const PairType& a, const PairType& b) const {
                return a < b;
            }
        };

        struct PointerStringPairHash {
            size_t operator()(const PointerStringPair& p) const {
                static const size_t prime = 16777619;
                hash<StringPiece> string_hash;
                return reinterpret_cast<size_t>(p.first) * prime ^ static_cast<size_t>(string_hash(p.second));
            }

#ifdef _MSC_VER
            // Used only by MSVC and platforms where hash_map is not available.
            static const size_t bucket_size = 4;
            static const size_t min_buckets = 8;
#endif
            inline bool operator()(const PointerStringPair& a,
                const PointerStringPair& b) const {
                return a < b;
            }
        };

        struct SymbolByFullNameHash {
            using is_transparent = void;

            template <typename T>
            size_t operator()(const T& s) const {
                return HASH_FXN<StringPiece> { }(s.full_name());
            }
        };
        struct SymbolByFullNameEq {
            using is_transparent = void;

            template <typename T, typename U>
            bool operator()(const T& a, const U& b) const {
                return a.full_name() == b.full_name();
            }
        };
        using SymbolsByNameSet = HASH_SET<Symbol, SymbolByFullNameHash, SymbolByFullNameEq>;

        struct SymbolByParentHash {
            using is_transparent = void;

            template <typename T>
            size_t operator()(const T& s) const {
                return PointerStringPairHash { }(s.parent_name_key());
            }
        };
        struct SymbolByParentEq {
            using is_transparent = void;

            template <typename T, typename U>
            bool operator()(const T& a, const U& b) const {
                return a.parent_name_key() == b.parent_name_key();
            }
        };
        using SymbolsByParentSet = HASH_SET<Symbol, SymbolByParentHash, SymbolByParentEq>;

        typedef HASH_MAP<StringPiece, const FileDescriptor*,
            HASH_FXN<StringPiece>>
            FilesByNameMap;

        typedef HASH_MAP<PointerStringPair, const FieldDescriptor*,
            PointerStringPairHash>
            FieldsByNameMap;

        struct FieldsByNumberHash {
            using is_transparent = void;

            template <typename T>
            size_t operator()(const T& s) const {
                return PointerIntegerPairHash<std::pair<const void*, int>> { }(
                    s.parent_number_key());
            }
        };
        struct FieldsByNumberEq {
            using is_transparent = void;

            template <typename T, typename U>
            bool operator()(const T& a, const U& b) const {
                return a.parent_number_key() == b.parent_number_key();
            }
        };
        using FieldsByNumberSet = HASH_SET<Symbol, FieldsByNumberHash, FieldsByNumberEq>;
        using EnumValuesByNumberSet = FieldsByNumberSet;

        // This is a map rather than a hash-map, since we use it to iterate
        // through all the extensions that extend a given Descriptor, and an
        // ordered data structure that implements lower_bound is convenient
        // for that.
        typedef std::map<DescriptorIntPair, const FieldDescriptor*>
            ExtensionsGroupedByDescriptorMap;
        typedef HASH_MAP<std::string, const SourceCodeInfo_Location*>
            LocationsByPathMap;

        //////////////////////////////////////////
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

        ////////////////////////////
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

    bool FormatLineOptions(int depth, const Message& options,
            const DescriptorPool* pool, std::string* output);

    bool RetrieveOptions(int depth, const Message& options,
            const DescriptorPool* pool,
            std::vector<std::string>* option_entries);
    bool RetrieveOptionsAssumingRightPool(
            int depth, const Message& options,
            std::vector<std::string>* option_entries);


        class SourceLocationCommentPrinter {
        public:
            template <typename DescType>
            SourceLocationCommentPrinter(const DescType* desc, const std::string& prefix,
                const DebugStringOptions& options)
                : options_(options)
                , prefix_(prefix) {
                // Perform the SourceLocation lookup only if we're including user comments,
                // because the lookup is fairly expensive.
                have_source_loc_ = options.include_comments && desc->GetSourceLocation(&source_loc_);
            }
            SourceLocationCommentPrinter(const FileDescriptor* file,
                const std::vector<int>& path,
                const std::string& prefix,
                const DebugStringOptions& options)
                : options_(options)
                , prefix_(prefix) {
                // Perform the SourceLocation lookup only if we're including user comments,
                // because the lookup is fairly expensive.
                have_source_loc_ = options.include_comments && file->GetSourceLocation(path, &source_loc_);
            }
            void AddPreComment(std::string* output) {
                if (have_source_loc_) {
                    // Detached leading comments.
                    for (const std::string& leading_detached_comment :
                        source_loc_.leading_detached_comments) {
                        *output += FormatComment(leading_detached_comment);
                        *output += "\n";
                    }
                    // Attached leading comments.
                    if (!source_loc_.leading_comments.empty()) {
                        *output += FormatComment(source_loc_.leading_comments);
                    }
                }
            }
            void AddPostComment(std::string* output) {
                if (have_source_loc_ && source_loc_.trailing_comments.size() > 0) {
                    *output += FormatComment(source_loc_.trailing_comments);
                }
            }

            // Format comment such that each line becomes a full-line C++-style comment in
            // the DebugString() output.
            std::string FormatComment(const std::string& comment_text) {
                std::string stripped_comment = comment_text;
                StripWhitespace(&stripped_comment);
                std::vector<std::string> lines = Split(stripped_comment, "\n");
                std::string output;
                for (const std::string& line : lines) {
                    strings::SubstituteAndAppend(&output, "$0// $1\n", prefix_, line);
                }
                return output;
            }

        private:
            bool have_source_loc_;
            SourceLocation source_loc_;
            DebugStringOptions options_;
            std::string prefix_;
        };

    // Formats options that all appear together in brackets. Does not include
    // brackets.
    bool FormatBracketedOptions(int depth, const Message& options,
        const DescriptorPool* pool, std::string* output) ;
} // namespace shark



#include <shark/port_undef.inc>
