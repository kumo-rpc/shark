// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <type_traits>

#include <shark/arena.h>
#include <shark/arenastring.h>
#include <shark/format/wired/wire_context.h>
#include <shark/format/wired/wire_format_lite.h>
#include <shark/io/coded_stream.h>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace shark::internal {

    // Used for compile time type selection. MapIf::type will be TrueType if Flag is
    // true and FalseType otherwise.
    template <bool Flag, typename TrueType, typename FalseType>
    struct MapIf;

    template <typename TrueType, typename FalseType>
    struct MapIf<true, TrueType, FalseType> {
        typedef TrueType type;
    };

    template <typename TrueType, typename FalseType>
    struct MapIf<false, TrueType, FalseType> {
        typedef FalseType type;
    };

    template <typename Type, bool is_arena_constructable>
    class MapArenaMessageCreator {
    public:
        // Use arena to create message if Type is arena constructable. Otherwise,
        // create the message on heap.
        static inline Type* CreateMessage(Arena* arena);
    };
    template <typename Type>
    class MapArenaMessageCreator<Type, true> {
    public:
        static inline Type* CreateMessage(Arena* arena) {
            return Arena::CreateMessage<Type>(arena);
        }
    };
    template <typename Type>
    class MapArenaMessageCreator<Type, false> {
    public:
        static inline Type* CreateMessage(Arena* arena) {
            return Arena::Create<Type>(arena);
        }
    };

    // Define constants for given wire field type
    template <SharkFieldType field_type, typename Type>
    class MapWireFieldTypeTraits { };

#define TYPE_TRAITS(TYPE, CType, WireFormatType, IsMessage, IsEnum)                             \
    template <typename Type>                                                                         \
    class MapWireFieldTypeTraits<SharkFieldType::TYPE, Type> {                  \
    public:                                                                                          \
        static const bool kIsMessage = IsMessage;                                                    \
        static const bool kIsEnum = IsEnum;                                                          \
        typedef typename MapIf<kIsMessage, Type*, CType>::type TypeOnMemory;                         \
        typedef typename MapIf<kIsEnum, int, Type>::type MapEntryAccessorType;                       \
        static const WireFormatLite::WireType kWireType = WireFormatLite::WIRETYPE_##WireFormatType; \
    };

    TYPE_TRAITS(MESSAGE, Type, LENGTH_DELIMITED, true, false)
    TYPE_TRAITS(STRING, ArenaStringPtr, LENGTH_DELIMITED, false, false)
    TYPE_TRAITS(BYTES, ArenaStringPtr, LENGTH_DELIMITED, false, false)
    TYPE_TRAITS(INT64, int64_t, VARINT, false, false)
    TYPE_TRAITS(UINT64, uint64_t, VARINT, false, false)
    TYPE_TRAITS(INT32, int32_t, VARINT, false, false)
    TYPE_TRAITS(INT8, int8_t, VARINT, false, false)
    TYPE_TRAITS(INT16, int16_t, VARINT, false, false)
    TYPE_TRAITS(UINT8, uint8_t, VARINT, false, false)
    TYPE_TRAITS(UINT16, uint16_t, VARINT, false, false)
    TYPE_TRAITS(UINT32, uint32_t, VARINT, false, false)
    TYPE_TRAITS(SINT64, int64_t, VARINT, false, false)
    TYPE_TRAITS(SINT32, int32_t, VARINT, false, false)
    TYPE_TRAITS(ENUM, int, VARINT, false, true)
    TYPE_TRAITS(DOUBLE, double, FIXED64, false, false)
    TYPE_TRAITS(FLOAT, float, FIXED32, false, false)
    TYPE_TRAITS(FIXED64, uint64_t, FIXED64, false, false)
    TYPE_TRAITS(FIXED32, uint32_t, FIXED32, false, false)
    TYPE_TRAITS(SFIXED64, int64_t, FIXED64, false, false)
    TYPE_TRAITS(SFIXED32, int32_t, FIXED32, false, false)
    TYPE_TRAITS(BOOL, bool, VARINT, false, false)

#undef TYPE_TRAITS

    template <SharkFieldType field_type, typename Type>
    class MapTypeHandler { };

    template <typename Type>
    class MapTypeHandler<SharkFieldType::MESSAGE, Type> {
    public:
        // Enum type cannot be used for MapTypeHandler::Read. Define a type which will
        // replace Enum with int.
        typedef typename MapWireFieldTypeTraits<SharkFieldType::MESSAGE,
            Type>::MapEntryAccessorType
            MapEntryAccessorType;
        // Internal stored type in MapEntryLite for given wire field type.
        typedef typename MapWireFieldTypeTraits<SharkFieldType::MESSAGE,
            Type>::TypeOnMemory TypeOnMemory;
        // Corresponding wire type for field type.
        static constexpr WireFormatLite::WireType kWireType = MapWireFieldTypeTraits<SharkFieldType::MESSAGE, Type>::kWireType;
        // Whether wire type is for message.
        static constexpr bool kIsMessage = MapWireFieldTypeTraits<SharkFieldType::MESSAGE, Type>::kIsMessage;
        // Whether wire type is for enum.
        static constexpr bool kIsEnum = MapWireFieldTypeTraits<SharkFieldType::MESSAGE, Type>::kIsEnum;

        // Functions used in parsing and serialization. ===================
        static inline size_t ByteSize(const MapEntryAccessorType& value);
        static inline int GetCachedSize(const MapEntryAccessorType& value);
        static inline bool Read(io::CodedInputStream* input,
            MapEntryAccessorType* value);
        static inline const char* Read(const char* ptr, ParseContext* ctx,
            MapEntryAccessorType* value);

        static inline uint8_t* Write(int field, const MapEntryAccessorType& value,
            uint8_t* ptr, io::EpsCopyOutputStream* stream);

        // Functions to manipulate data on memory. ========================
        static inline const Type& GetExternalReference(const Type* value);
        static inline void DeleteNoArena(const Type* x);
        static inline void Merge(const Type& from, Type** to, Arena* arena);
        static inline void Clear(Type** value, Arena* arena);
        static constexpr TypeOnMemory Constinit();

        static inline Type* EnsureMutable(Type** value, Arena* arena);
        // SpaceUsedInMapEntry: Return bytes used by value in MapEntry, excluding
        // those already calculate in sizeof(MapField).
        static inline size_t SpaceUsedInMapEntryLong(const Type* value);
        // Return default instance if value is not initialized when calling const
        // reference accessor.
        static inline const Type& DefaultIfNotInitialized(const Type* value);
        // Check if all required fields have values set.
        static inline bool IsInitialized(Type* value);
    };

#define MAP_HANDLER(TYPE)                                                                                              \
    template <typename Type>                                                                                                \
    class MapTypeHandler<SharkFieldType::TYPE, Type> {                                                 \
    public:                                                                                                                 \
        typedef typename MapWireFieldTypeTraits<SharkFieldType::TYPE,                                  \
            Type>::MapEntryAccessorType                                                                                     \
            MapEntryAccessorType;                                                                                           \
        typedef typename MapWireFieldTypeTraits<SharkFieldType::TYPE,                                  \
            Type>::TypeOnMemory TypeOnMemory;                                                                               \
        static const WireFormatLite::WireType kWireType = MapWireFieldTypeTraits<SharkFieldType::TYPE, \
            Type>::kWireType;                                                                                               \
        static const bool kIsMessage = MapWireFieldTypeTraits<SharkFieldType::TYPE,                    \
            Type>::kIsMessage;                                                                                              \
        static const bool kIsEnum = MapWireFieldTypeTraits<SharkFieldType::TYPE,                       \
            Type>::kIsEnum;                                                                                                 \
        static inline int ByteSize(const MapEntryAccessorType& value);                                                      \
        static inline int GetCachedSize(const MapEntryAccessorType& value);                                                 \
        static inline bool Read(io::CodedInputStream* input,                                                                \
            MapEntryAccessorType* value);                                                                                   \
        static inline const char* Read(const char* begin, ParseContext* ctx,                                                \
            MapEntryAccessorType* value);                                                                                   \
        static inline uint8_t* Write(int field, const MapEntryAccessorType& value,                                          \
            uint8_t* ptr,                                                                                                   \
            io::EpsCopyOutputStream* stream);                                                                               \
        static inline const MapEntryAccessorType& GetExternalReference(                                                     \
            const TypeOnMemory& value);                                                                                     \
        static inline void DeleteNoArena(const TypeOnMemory& x);                                                            \
        static inline void Merge(const MapEntryAccessorType& from,                                                          \
            TypeOnMemory* to, Arena* arena);                                                                                \
        static inline void Clear(TypeOnMemory* value, Arena* arena);                                                        \
        static inline size_t SpaceUsedInMapEntryLong(const TypeOnMemory& value);                                            \
        static inline const MapEntryAccessorType& DefaultIfNotInitialized(                                                  \
            const TypeOnMemory& value);                                                                                     \
        static inline bool IsInitialized(const TypeOnMemory& value);                                                        \
        static void DeleteNoArena(TypeOnMemory& value);                                                                     \
        static constexpr TypeOnMemory Constinit();                                                                          \
        static inline MapEntryAccessorType* EnsureMutable(TypeOnMemory* value,                                              \
            Arena* arena);                                                                                                  \
    };
    MAP_HANDLER(STRING)
    MAP_HANDLER(BYTES)
    MAP_HANDLER(INT64)
    MAP_HANDLER(UINT64)
    MAP_HANDLER(INT32)
    MAP_HANDLER(INT8)
    MAP_HANDLER(INT16)
    MAP_HANDLER(UINT8)
    MAP_HANDLER(UINT16)
    MAP_HANDLER(UINT32)
    MAP_HANDLER(SINT64)
    MAP_HANDLER(SINT32)
    MAP_HANDLER(ENUM)
    MAP_HANDLER(DOUBLE)
    MAP_HANDLER(FLOAT)
    MAP_HANDLER(FIXED64)
    MAP_HANDLER(FIXED32)
    MAP_HANDLER(SFIXED64)
    MAP_HANDLER(SFIXED32)
    MAP_HANDLER(BOOL)
#undef MAP_HANDLER

    template <typename Type>
    inline size_t MapTypeHandler<SharkFieldType::MESSAGE, Type>::ByteSize(
        const MapEntryAccessorType& value) {
        return WireFormatLite::MessageSizeNoVirtual(value);
    }

#define SHARK_BYTE_SIZE(TYPE, DeclaredType)                                          \
    template <typename Type>                                                              \
    inline int MapTypeHandler<SharkFieldType::TYPE, Type>::ByteSize( \
        const MapEntryAccessorType& value) {                                              \
        return static_cast<int>(WireFormatLite::DeclaredType##Size(value));               \
    }

    SHARK_BYTE_SIZE(STRING, String)
    SHARK_BYTE_SIZE(BYTES, Bytes)
    SHARK_BYTE_SIZE(INT64, Int64)
    SHARK_BYTE_SIZE(UINT64, UInt64)
    SHARK_BYTE_SIZE(INT32, Int32)
    SHARK_BYTE_SIZE(INT8, Int32)
    SHARK_BYTE_SIZE(INT16, Int32)
    SHARK_BYTE_SIZE(UINT8, UInt32)
    SHARK_BYTE_SIZE(UINT16, UInt32)
    SHARK_BYTE_SIZE(UINT32, UInt32)
    SHARK_BYTE_SIZE(SINT64, SInt64)
    SHARK_BYTE_SIZE(SINT32, SInt32)
    SHARK_BYTE_SIZE(ENUM, Enum)

#undef SHARK_BYTE_SIZE

#define FIXED_BYTE_SIZE(TYPE, DeclaredType)                                          \
    template <typename Type>                                                              \
    inline int MapTypeHandler<SharkFieldType::TYPE, Type>::ByteSize( \
        const MapEntryAccessorType& /* value */) {                                        \
        return WireFormatLite::k##DeclaredType##Size;                                     \
    }

    FIXED_BYTE_SIZE(DOUBLE, Double)
    FIXED_BYTE_SIZE(FLOAT, Float)
    FIXED_BYTE_SIZE(FIXED64, Fixed64)
    FIXED_BYTE_SIZE(FIXED32, Fixed32)
    FIXED_BYTE_SIZE(SFIXED64, SFixed64)
    FIXED_BYTE_SIZE(SFIXED32, SFixed32)
    FIXED_BYTE_SIZE(BOOL, Bool)

#undef FIXED_BYTE_SIZE

    template <typename Type>
    inline int MapTypeHandler<SharkFieldType::MESSAGE, Type>::GetCachedSize(
        const MapEntryAccessorType& value) {
        return static_cast<int>(WireFormatLite::LengthDelimitedSize(
            static_cast<size_t>(value.GetCachedSize())));
    }

#define GET_CACHED_SIZE(TYPE, DeclaredType)                                    \
    template <typename Type>                                                        \
    inline int                                                                      \
    MapTypeHandler<SharkFieldType::TYPE, Type>::GetCachedSize( \
        const MapEntryAccessorType& value) {                                        \
        return static_cast<int>(WireFormatLite::DeclaredType##Size(value));         \
    }

    GET_CACHED_SIZE(STRING, String)
    GET_CACHED_SIZE(BYTES, Bytes)
    GET_CACHED_SIZE(INT64, Int64)
    GET_CACHED_SIZE(UINT64, UInt64)
    GET_CACHED_SIZE(INT32, Int32)
    GET_CACHED_SIZE(INT8, Int32)
    GET_CACHED_SIZE(INT16, Int32)
    GET_CACHED_SIZE(UINT8, UInt32)
    GET_CACHED_SIZE(UINT16, UInt32)
    GET_CACHED_SIZE(UINT32, UInt32)
    GET_CACHED_SIZE(SINT64, SInt64)
    GET_CACHED_SIZE(SINT32, SInt32)
    GET_CACHED_SIZE(ENUM, Enum)

#undef GET_CACHED_SIZE

#define GET_FIXED_CACHED_SIZE(TYPE, DeclaredType)                              \
    template <typename Type>                                                        \
    inline int                                                                      \
    MapTypeHandler<SharkFieldType::TYPE, Type>::GetCachedSize( \
        const MapEntryAccessorType& /* value */) {                                  \
        return WireFormatLite::k##DeclaredType##Size;                               \
    }

    GET_FIXED_CACHED_SIZE(DOUBLE, Double)
    GET_FIXED_CACHED_SIZE(FLOAT, Float)
    GET_FIXED_CACHED_SIZE(FIXED64, Fixed64)
    GET_FIXED_CACHED_SIZE(FIXED32, Fixed32)
    GET_FIXED_CACHED_SIZE(SFIXED64, SFixed64)
    GET_FIXED_CACHED_SIZE(SFIXED32, SFixed32)
    GET_FIXED_CACHED_SIZE(BOOL, Bool)

#undef GET_FIXED_CACHED_SIZE

    template <typename Type>
    inline uint8_t* MapTypeHandler<SharkFieldType::MESSAGE, Type>::Write(
        int field, const MapEntryAccessorType& value, uint8_t* ptr,
        io::EpsCopyOutputStream* stream) {
        ptr = stream->EnsureSpace(ptr);
        return WireFormatLite::InternalWriteMessage(
            field, value, value.GetCachedSize(), ptr, stream);
    }

#define WRITE_METHOD(TYPE, DeclaredType)                               \
    template <typename Type>                                                \
    inline uint8_t*                                                         \
    MapTypeHandler<SharkFieldType::TYPE, Type>::Write( \
        int field, const MapEntryAccessorType& value, uint8_t* ptr,         \
        io::EpsCopyOutputStream* stream) {                                  \
        ptr = stream->EnsureSpace(ptr);                                     \
        return stream->Write##DeclaredType(field, value, ptr);              \
    }

    WRITE_METHOD(STRING, String)
    WRITE_METHOD(BYTES, Bytes)

#undef WRITE_METHOD
#define WRITE_METHOD(TYPE, DeclaredType)                                   \
    template <typename Type>                                                    \
    inline uint8_t*                                                             \
    MapTypeHandler<SharkFieldType::TYPE, Type>::Write(     \
        int field, const MapEntryAccessorType& value, uint8_t* ptr,             \
        io::EpsCopyOutputStream* stream) {                                      \
        ptr = stream->EnsureSpace(ptr);                                         \
        return WireFormatLite::Write##DeclaredType##ToArray(field, value, ptr); \
    }

    WRITE_METHOD(INT64, Int64)
    WRITE_METHOD(UINT64, UInt64)
    WRITE_METHOD(INT32, Int32)
    WRITE_METHOD(INT8, Int32)
    WRITE_METHOD(INT16, Int32)
    WRITE_METHOD(UINT8, UInt32)
    WRITE_METHOD(UINT16, UInt32)
    WRITE_METHOD(UINT32, UInt32)
    WRITE_METHOD(SINT64, SInt64)
    WRITE_METHOD(SINT32, SInt32)
    WRITE_METHOD(ENUM, Enum)
    WRITE_METHOD(DOUBLE, Double)
    WRITE_METHOD(FLOAT, Float)
    WRITE_METHOD(FIXED64, Fixed64)
    WRITE_METHOD(FIXED32, Fixed32)
    WRITE_METHOD(SFIXED64, SFixed64)
    WRITE_METHOD(SFIXED32, SFixed32)
    WRITE_METHOD(BOOL, Bool)

#undef WRITE_METHOD

    template <typename Type>
    inline bool MapTypeHandler<SharkFieldType::MESSAGE, Type>::Read(
        io::CodedInputStream* input, MapEntryAccessorType* value) {
        return WireFormatLite::ReadMessageNoVirtual(input, value);
    }

    template <typename Type>
    inline bool MapTypeHandler<SharkFieldType::STRING, Type>::Read(
        io::CodedInputStream* input, MapEntryAccessorType* value) {
        return WireFormatLite::ReadString(input, value);
    }

    template <typename Type>
    inline bool MapTypeHandler<SharkFieldType::BYTES, Type>::Read(
        io::CodedInputStream* input, MapEntryAccessorType* value) {
        return WireFormatLite::ReadBytes(input, value);
    }

    template <typename Type>
    const char* MapTypeHandler<SharkFieldType::MESSAGE, Type>::Read(
        const char* ptr, ParseContext* ctx, MapEntryAccessorType* value) {
        return ctx->ParseMessage(value, ptr);
    }

    template <typename Type>
    const char* MapTypeHandler<SharkFieldType::STRING, Type>::Read(
        const char* ptr, ParseContext* ctx, MapEntryAccessorType* value) {
        int size = ReadSize(&ptr);
        SHARK_PARSER_ASSERT(ptr);
        return ctx->ReadString(ptr, size, value);
    }

    template <typename Type>
    const char* MapTypeHandler<SharkFieldType::BYTES, Type>::Read(
        const char* ptr, ParseContext* ctx, MapEntryAccessorType* value) {
        int size = ReadSize(&ptr);
        SHARK_PARSER_ASSERT(ptr);
        return ctx->ReadString(ptr, size, value);
    }

    inline const char* ReadINT64(const char* ptr, int64_t* value) {
        return VarintParse(ptr, reinterpret_cast<uint64_t*>(value));
    }
    inline const char* ReadUINT64(const char* ptr, uint64_t* value) {
        return VarintParse(ptr, value);
    }
    inline const char* ReadINT32(const char* ptr, int32_t* value) {
        return VarintParse(ptr, reinterpret_cast<uint32_t*>(value));
    }
    inline const char* ReadINT8(const char* ptr, int8_t* value) {
        uint32_t temp;
        ptr = VarintParse(ptr, &temp);
        if (ptr != nullptr)
            *value = static_cast<int8_t>(static_cast<int32_t>(temp));
        return ptr;
    }
    inline const char* ReadINT16(const char* ptr, int16_t* value) {
        uint32_t temp;
        ptr = VarintParse(ptr, &temp);
        if (ptr != nullptr)
            *value = static_cast<int16_t>(static_cast<int32_t>(temp));
        return ptr;
    }
    inline const char* ReadUINT8(const char* ptr, uint8_t* value) {
        uint32_t temp;
        ptr = VarintParse(ptr, &temp);
        if (ptr != nullptr)
            *value = static_cast<uint8_t>(temp);
        return ptr;
    }
    inline const char* ReadUINT16(const char* ptr, uint16_t* value) {
        uint32_t temp;
        ptr = VarintParse(ptr, &temp);
        if (ptr != nullptr)
            *value = static_cast<uint16_t>(temp);
        return ptr;
    }
    inline const char* ReadUINT32(const char* ptr, uint32_t* value) {
        return VarintParse(ptr, value);
    }
    inline const char* ReadSINT64(const char* ptr, int64_t* value) {
        *value = ReadVarintZigZag64(&ptr);
        return ptr;
    }
    inline const char* ReadSINT32(const char* ptr, int32_t* value) {
        *value = ReadVarintZigZag32(&ptr);
        return ptr;
    }
    template <typename E>
    inline const char* ReadENUM(const char* ptr, E* value) {
        *value = static_cast<E>(ReadVarint32(&ptr));
        return ptr;
    }
    inline const char* ReadBOOL(const char* ptr, bool* value) {
        *value = static_cast<bool>(ReadVarint32(&ptr));
        return ptr;
    }

    template <typename F>
    inline const char* ReadUnaligned(const char* ptr, F* value) {
        *value = UnalignedLoad<F>(ptr);
        return ptr + sizeof(F);
    }
    inline const char* ReadFLOAT(const char* ptr, float* value) {
        return ReadUnaligned(ptr, value);
    }
    inline const char* ReadDOUBLE(const char* ptr, double* value) {
        return ReadUnaligned(ptr, value);
    }
    inline const char* ReadFIXED64(const char* ptr, uint64_t* value) {
        return ReadUnaligned(ptr, value);
    }
    inline const char* ReadFIXED32(const char* ptr, uint32_t* value) {
        return ReadUnaligned(ptr, value);
    }
    inline const char* ReadSFIXED64(const char* ptr, int64_t* value) {
        return ReadUnaligned(ptr, value);
    }
    inline const char* ReadSFIXED32(const char* ptr, int32_t* value) {
        return ReadUnaligned(ptr, value);
    }

#define READ_METHOD(TYPE)                                                         \
    template <typename Type>                                                           \
    inline bool MapTypeHandler<SharkFieldType::TYPE, Type>::Read( \
        io::CodedInputStream* input, MapEntryAccessorType* value) {                    \
        return WireFormatLite::ReadPrimitive<TypeOnMemory,                             \
            SharkFieldType::TYPE>(                                \
            input, value);                                                             \
    }                                                                                  \
    template <typename Type>                                                           \
    const char* MapTypeHandler<SharkFieldType::TYPE, Type>::Read( \
        const char* begin, ParseContext* ctx, MapEntryAccessorType* value) {           \
        (void)ctx;                                                                     \
        return Read##TYPE(begin, value);                                          \
    }

    READ_METHOD(INT64)
    READ_METHOD(UINT64)
    READ_METHOD(INT32)
    READ_METHOD(INT8)
    READ_METHOD(INT16)
    READ_METHOD(UINT8)
    READ_METHOD(UINT16)
    READ_METHOD(UINT32)
    READ_METHOD(SINT64)
    READ_METHOD(SINT32)
    READ_METHOD(ENUM)
    READ_METHOD(DOUBLE)
    READ_METHOD(FLOAT)
    READ_METHOD(FIXED64)
    READ_METHOD(FIXED32)
    READ_METHOD(SFIXED64)
    READ_METHOD(SFIXED32)
    READ_METHOD(BOOL)

#undef READ_METHOD

    // Definition for message handler

    template <typename Type>
    inline const Type&
    MapTypeHandler<SharkFieldType::MESSAGE, Type>::GetExternalReference(
        const Type* value) {
        return *value;
    }

    template <typename Type>
    inline size_t MapTypeHandler<SharkFieldType::MESSAGE,
        Type>::SpaceUsedInMapEntryLong(const Type* value) {
        return value->SpaceUsedLong();
    }

    template <typename Type>
    inline void MapTypeHandler<SharkFieldType::MESSAGE, Type>::Clear(
        Type** value, Arena* /* arena */) {
        if (*value != nullptr)
            (*value)->Clear();
    }
    template <typename Type>
    inline void MapTypeHandler<SharkFieldType::MESSAGE, Type>::Merge(
        const Type& from, Type** to, Arena* /* arena */) {
        (*to)->MergeFrom(from);
    }

    template <typename Type>
    void MapTypeHandler<SharkFieldType::MESSAGE, Type>::DeleteNoArena(
        const Type* ptr) {
        delete ptr;
    }

    template <typename Type>
    constexpr auto MapTypeHandler<SharkFieldType::MESSAGE, Type>::Constinit()
        -> TypeOnMemory {
        return nullptr;
    }

    template <typename Type>
    inline Type* MapTypeHandler<SharkFieldType::MESSAGE, Type>::EnsureMutable(
        Type** value, Arena* arena) {
        if (*value == nullptr) {
            *value = MapArenaMessageCreator<
                Type,
                Arena::is_arena_constructable<Type>::type::value>::CreateMessage(arena);
        }
        return *value;
    }

    template <typename Type>
    inline const Type&
    MapTypeHandler<SharkFieldType::MESSAGE, Type>::DefaultIfNotInitialized(
        const Type* value) {
        return value != nullptr ? *value : *Type::internal_default_instance();
    }

    template <typename Type>
    inline bool MapTypeHandler<SharkFieldType::MESSAGE, Type>::IsInitialized(
        Type* value) {
        return value ? value->IsInitialized() : false;
    }

    // Definition for string/bytes handler

#define STRING_OR_BYTES_HANDLER_FUNCTIONS(TYPE)                                     \
    template <typename Type>                                                             \
    inline const typename MapTypeHandler<SharkFieldType::TYPE,      \
        Type>::MapEntryAccessorType&                                                     \
    MapTypeHandler<SharkFieldType::TYPE,                            \
        Type>::GetExternalReference(const TypeOnMemory& value) {                         \
        return value.Get();                                                              \
    }                                                                                    \
    template <typename Type>                                                             \
    inline size_t                                                                        \
    MapTypeHandler<SharkFieldType::TYPE,                            \
        Type>::SpaceUsedInMapEntryLong(const TypeOnMemory& value) {                      \
        return sizeof(value);                                                            \
    }                                                                                    \
    template <typename Type>                                                             \
    inline void MapTypeHandler<SharkFieldType::TYPE, Type>::Clear(  \
        TypeOnMemory* value, Arena* /* arena */) {                                       \
        value->ClearToEmpty();                                                           \
    }                                                                                    \
    template <typename Type>                                                             \
    inline void MapTypeHandler<SharkFieldType::TYPE, Type>::Merge(  \
        const MapEntryAccessorType& from, TypeOnMemory* to, Arena* arena) {              \
        to->Set(from, arena);                                                            \
    }                                                                                    \
    template <typename Type>                                                             \
    void MapTypeHandler<SharkFieldType::TYPE, Type>::DeleteNoArena( \
        TypeOnMemory& value) {                                                           \
        value.Destroy();                                                                 \
    }                                                                                    \
    template <typename Type>                                                             \
    constexpr auto                                                                       \
    MapTypeHandler<SharkFieldType::TYPE, Type>::Constinit()         \
        -> TypeOnMemory {                                                                \
        return TypeOnMemory(&internal::fixed_address_empty_string,                       \
            ConstantInitialized { });                                                    \
    }                                                                                    \
    template <typename Type>                                                             \
    inline typename MapTypeHandler<SharkFieldType::TYPE,            \
        Type>::MapEntryAccessorType*                                                     \
    MapTypeHandler<SharkFieldType::TYPE, Type>::EnsureMutable(      \
        TypeOnMemory* value, Arena* arena) {                                             \
        return value->Mutable(arena);                                                    \
    }                                                                                    \
    template <typename Type>                                                             \
    inline const typename MapTypeHandler<SharkFieldType::TYPE,      \
        Type>::MapEntryAccessorType&                                                     \
    MapTypeHandler<SharkFieldType::TYPE,                            \
        Type>::DefaultIfNotInitialized(const TypeOnMemory& value) {                      \
        return value.Get();                                                              \
    }                                                                                    \
    template <typename Type>                                                             \
    inline bool                                                                          \
    MapTypeHandler<SharkFieldType::TYPE, Type>::IsInitialized(      \
        const TypeOnMemory& /* value */) {                                               \
        return true;                                                                     \
    }
    STRING_OR_BYTES_HANDLER_FUNCTIONS(STRING)
    STRING_OR_BYTES_HANDLER_FUNCTIONS(BYTES)
#undef STRING_OR_BYTES_HANDLER_FUNCTIONS

#define PRIMITIVE_HANDLER_FUNCTIONS(TYPE)                                          \
    template <typename Type>                                                            \
    inline const typename MapTypeHandler<SharkFieldType::TYPE,     \
        Type>::MapEntryAccessorType&                                                    \
    MapTypeHandler<SharkFieldType::TYPE,                           \
        Type>::GetExternalReference(const TypeOnMemory& value) {                        \
        return value;                                                                   \
    }                                                                                   \
    template <typename Type>                                                            \
    inline size_t MapTypeHandler<SharkFieldType::TYPE, Type>::     \
        SpaceUsedInMapEntryLong(const TypeOnMemory& /* value */) {                      \
        return 0;                                                                       \
    }                                                                                   \
    template <typename Type>                                                            \
    inline void MapTypeHandler<SharkFieldType::TYPE, Type>::Clear( \
        TypeOnMemory* value, Arena* /* arena */) {                                      \
        *value = 0;                                                                     \
    }                                                                                   \
    template <typename Type>                                                            \
    inline void MapTypeHandler<SharkFieldType::TYPE, Type>::Merge( \
        const MapEntryAccessorType& from, TypeOnMemory* to,                             \
        Arena* /* arena */) {                                                           \
        *to = from;                                                                     \
    }                                                                                   \
    template <typename Type>                                                            \
    inline void MapTypeHandler<SharkFieldType::TYPE,               \
        Type>::DeleteNoArena(TypeOnMemory& /* x */) { }                                 \
    template <typename Type>                                                            \
    constexpr auto                                                                      \
    MapTypeHandler<SharkFieldType::TYPE, Type>::Constinit()        \
        -> TypeOnMemory {                                                               \
        return 0;                                                                       \
    }                                                                                   \
    template <typename Type>                                                            \
    inline typename MapTypeHandler<SharkFieldType::TYPE,           \
        Type>::MapEntryAccessorType*                                                    \
    MapTypeHandler<SharkFieldType::TYPE, Type>::EnsureMutable(     \
        TypeOnMemory* value, Arena* /* arena */) {                                      \
        return value;                                                                   \
    }                                                                                   \
    template <typename Type>                                                            \
    inline const typename MapTypeHandler<SharkFieldType::TYPE,     \
        Type>::MapEntryAccessorType&                                                    \
    MapTypeHandler<SharkFieldType::TYPE,                           \
        Type>::DefaultIfNotInitialized(const TypeOnMemory& value) {                     \
        return value;                                                                   \
    }                                                                                   \
    template <typename Type>                                                            \
    inline bool                                                                         \
    MapTypeHandler<SharkFieldType::TYPE, Type>::IsInitialized(     \
        const TypeOnMemory& /* value */) {                                              \
        return true;                                                                    \
    }
    PRIMITIVE_HANDLER_FUNCTIONS(INT64)
    PRIMITIVE_HANDLER_FUNCTIONS(UINT64)
    PRIMITIVE_HANDLER_FUNCTIONS(INT32)
    PRIMITIVE_HANDLER_FUNCTIONS(INT8)
    PRIMITIVE_HANDLER_FUNCTIONS(INT16)
    PRIMITIVE_HANDLER_FUNCTIONS(UINT8)
    PRIMITIVE_HANDLER_FUNCTIONS(UINT16)
    PRIMITIVE_HANDLER_FUNCTIONS(UINT32)
    PRIMITIVE_HANDLER_FUNCTIONS(SINT64)
    PRIMITIVE_HANDLER_FUNCTIONS(SINT32)
    PRIMITIVE_HANDLER_FUNCTIONS(ENUM)
    PRIMITIVE_HANDLER_FUNCTIONS(DOUBLE)
    PRIMITIVE_HANDLER_FUNCTIONS(FLOAT)
    PRIMITIVE_HANDLER_FUNCTIONS(FIXED64)
    PRIMITIVE_HANDLER_FUNCTIONS(FIXED32)
    PRIMITIVE_HANDLER_FUNCTIONS(SFIXED64)
    PRIMITIVE_HANDLER_FUNCTIONS(SFIXED32)
    PRIMITIVE_HANDLER_FUNCTIONS(BOOL)
#undef PRIMITIVE_HANDLER_FUNCTIONS

    // Enum class values are stored as int. Other types must be passed by
    // reference: copying a message drops its cached size and writes an empty body.
    template <typename Accessor, typename T>
    typename std::enable_if<std::is_enum<T>::value, Accessor>::type
    MapEntryWireValue(const T& value) {
        return static_cast<Accessor>(value);
    }
    template <typename Accessor, typename T>
    typename std::enable_if<!std::is_enum<T>::value, const T&>::type
    MapEntryWireValue(const T& value) {
        return value;
    }

    // Functions for operating on a map entry using type handlers.
    //
    // Does not contain any representation (this class is not intended to be
    // instantiated).
    template <typename Key, typename Value, SharkFieldType kKeyFieldType,
        SharkFieldType kValueFieldType>
    struct MapEntryFuncs {
        typedef MapTypeHandler<kKeyFieldType, Key> KeyTypeHandler;
        typedef MapTypeHandler<kValueFieldType, Value> ValueTypeHandler;

        static constexpr int kKeyFieldNumber = 1;
        static constexpr int kValueFieldNumber = 2;

        static uint8_t* InternalSerialize(int field_number, const Key& key,
            const Value& value, uint8_t* ptr,
            io::EpsCopyOutputStream* stream) {
            ptr = stream->EnsureSpace(ptr);
            ptr = WireFormatLite::WriteTagToArray(
                field_number, WireFormatLite::WIRETYPE_LENGTH_DELIMITED, ptr);
            ptr = io::CodedOutputStream::WriteVarint32ToArray(GetCachedSize(key, value),
                ptr);

            ptr = KeyTypeHandler::Write(
                kKeyFieldNumber,
                MapEntryWireValue<typename KeyTypeHandler::MapEntryAccessorType>(key),
                ptr, stream);
            return ValueTypeHandler::Write(
                kValueFieldNumber,
                MapEntryWireValue<typename ValueTypeHandler::MapEntryAccessorType>(
                    value),
                ptr, stream);
        }

        static size_t ByteSizeLong(const Key& key, const Value& value) {
            // Tags for key and value will both be one byte (field numbers 1 and 2).
            size_t inner_length = 2 + KeyTypeHandler::ByteSize(MapEntryWireValue<typename KeyTypeHandler::MapEntryAccessorType>(key)) + ValueTypeHandler::ByteSize(MapEntryWireValue<typename ValueTypeHandler::MapEntryAccessorType>(value));
            return inner_length + io::CodedOutputStream::VarintSize32(static_cast<uint32_t>(inner_length));
        }

        static int GetCachedSize(const Key& key, const Value& value) {
            // Tags for key and value will both be one byte (field numbers 1 and 2).
            return 2 + KeyTypeHandler::GetCachedSize(MapEntryWireValue<typename KeyTypeHandler::MapEntryAccessorType>(key)) + ValueTypeHandler::GetCachedSize(MapEntryWireValue<typename ValueTypeHandler::MapEntryAccessorType>(value));
        }
    };

} // namespace shark::internal
