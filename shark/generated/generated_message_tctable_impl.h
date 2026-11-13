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

#include <cstdint>
#include <type_traits>

#include <shark/extension_set.h>
#include <shark/format/wired/wire_context.h>
#include <shark/format/wired/wire_format_lite.h>
#include <shark/generated/generated_message_tctable_decl.h>
#include <shark/generated/metadata_lite.h>
#include <shark/message_lite.h>
#include <shark/port.h>

// Must come last:
#include <shark/port_def.inc>

namespace shark {

class Message;
class UnknownFieldSet;

namespace internal {

// Field layout enums.
//
// Structural information about fields is packed into a 16-bit value. The enum
// types below represent bitwise fields, along with their respective widths,
// shifts, and masks.
//
//     Bit:
//     +-----------------------+-----------------------+
//     |15        ..          8|7         ..          0|
//     +-----------------------+-----------------------+
//     :  .  :  .  :  .  :  .  :  .  :  .  : 3|========| [3] SharkFieldType
//     :     :     :     :     :     : 5|=====|  :     : [2] FieldCardinality
//     :  .  :  .  :  .  :  . 8|========|  :  .  :  .  : [3] FieldRep
//     :     :     :   10|=====|     :     :     :     : [2] TransformValidation
//     :  .  :  .12|=====|  .  :  .  :  .  :  .  :  .  : [2] FormatDiscriminator
//     +-----------------------+-----------------------+
//     |15        ..          8|7         ..          0|
//     +-----------------------+-----------------------+
//
namespace field_layout {
// clang-format off

// Field kind (3 bits):
// These values broadly represent a wire type and an in-memory storage class.
enum FieldKind : uint16_t {
  kFkShift = 0,
  kFkBits = 3,
  kFkMask = ((1 << kFkBits) - 1) << kFkShift,

  kFkNone = 0,
  kFkVarint,        // WT=0     rep=8,32,64 bits
  kFkPackedVarint,  // WT=2     rep=8,32,64 bits
  kFkFixed,         // WT=1,5   rep=32,64 bits
  kFkPackedFixed,   // WT=2     rep=32,64 bits
  kFkString,        // WT=2     rep=various
  kFkMessage,       // WT=2,3,4 rep=MessageLite*
  // Maps are a special case of Message, but use different parsing logic.
  kFkMap,           // WT=2     rep=Map(Lite)<various, various>
};

static_assert(kFkMap < (1 << kFkBits), "too many types");

// Cardinality (2 bits):
// These values determine how many values a field can have and its presence.
// Packed fields are represented in SharkFieldType.
enum Cardinality : uint16_t {
  kFcShift    = kFkShift + kFkBits,
  kFcBits     = 2,
  kFcMask     = ((1 << kFcBits) - 1) << kFcShift,

  kFcSingular = 0,
  kFcOptional = 1 << kFcShift,
  kFcRepeated = 2 << kFcShift,
  kFcOneof    = 3 << kFcShift,
};

// Field representation (3 bits):
// These values are the specific refinements of storage classes in SharkFieldType.
enum FieldRep : uint16_t {
  kRepShift    = kFcShift + kFcBits,
  kRepBits     = 3,
  kRepMask     = ((1 << kRepBits) - 1) << kRepShift,

  // Numeric types (used for optional and repeated fields):
  kRep8Bits    = 0,
  kRep32Bits   = 2 << kRepShift,
  kRep64Bits   = 3 << kRepShift,
  // String types:
  kRepAString  = 0,               // ArenaStringPtr
  kRepIString  = 1 << kRepShift,  // InlinedString
  kRepCord     = 2 << kRepShift,  // absl::Cord
  kRepSPiece   = 3 << kRepShift,  // StringPieceField
  kRepSString  = 4 << kRepShift,  // std::string*
  // Message types (WT=2 unless otherwise noted):
  kRepMessage  = 0,               // MessageLite*
  kRepGroup    = 1 << kRepShift,  // MessageLite* (WT=3,4)
  kRepLazy     = 2 << kRepShift,  // LazyField*
  kRepIWeak    = 3 << kRepShift,  // ImplicitWeak
};

// Transform/validation (2 bits):
// These values determine transforms or validation to/from wire format.
enum TransformValidation : uint16_t {
  kTvShift     = kRepShift + kRepBits,
  kTvBits      = 2,
  kTvMask      = ((1 << kTvBits) - 1) << kTvShift,

  // Varint fields:
  kTvZigZag    = 1 << kTvShift,
  kTvEnum      = 2 << kTvShift,  // validate using generated _IsValid()
  kTvRange     = 3 << kTvShift,  // validate using FieldAux::enum_range
  // String fields:
  kTvUtf8Debug = 1 << kTvShift,  // proto2
  kTvUtf8      = 2 << kTvShift,  // proto3
};

static_assert((kTvEnum & kTvRange) != 0,
              "enum validation types must share a bit");
static_assert((kTvEnum & kTvRange & kTvZigZag) == 0,
              "zigzag encoding is not enum validation");

// Format discriminators (2 bits):
enum FormatDiscriminator : uint16_t {
  kFmtShift      = kTvShift + kTvBits,
  kFmtBits       = 2,
  kFmtMask       = ((1 << kFmtBits) - 1) << kFmtShift,

  // Numeric:
  kFmtUnsigned   = 1 << kFmtShift,  // fixed, varint
  kFmtSigned     = 2 << kFmtShift,  // fixed, varint
  kFmtFloating   = 3 << kFmtShift,  // fixed
  kFmtEnum       = 3 << kFmtShift,  // varint
  // Strings:
  kFmtUtf8       = 1 << kFmtShift,  // string (proto3, enforce_utf8=true)
  kFmtUtf8Escape = 2 << kFmtShift,  // string (proto2, enforce_utf8=false)
  // Bytes:
  kFmtArray      = 1 << kFmtShift,  // bytes
  // Messages:
  kFmtShow       = 1 << kFmtShift,  // message, map
};

// Update this assertion (and comments above) when adding or removing bits:
static_assert(kFmtShift + kFmtBits == 12, "number of bits changed");

// This assertion should not change unless the storage width changes:
static_assert(kFmtShift + kFmtBits <= 16, "too many bits");

// Convenience aliases (16 bits, with format):
enum SharkFieldType : uint16_t {
  // Numeric types:
  kBool            = kFkVarint | kRep8Bits,

  kFixed32         = kFkFixed  | kRep32Bits | kFmtUnsigned,
  kUInt32          = kFkVarint | kRep32Bits | kFmtUnsigned,
  kSFixed32        = kFkFixed  | kRep32Bits | kFmtSigned,
  kInt32           = kFkVarint | kRep32Bits | kFmtSigned,
  kSInt32          = kFkVarint | kRep32Bits | kFmtSigned | kTvZigZag,
  kFloat           = kFkFixed  | kRep32Bits | kFmtFloating,
  kEnum            = kFkVarint | kRep32Bits | kFmtEnum   | kTvEnum,
  kEnumRange       = kFkVarint | kRep32Bits | kFmtEnum   | kTvRange,
  kOpenEnum        = kFkVarint | kRep32Bits | kFmtEnum,

  kFixed64         = kFkFixed  | kRep64Bits | kFmtUnsigned,
  kUInt64          = kFkVarint | kRep64Bits | kFmtUnsigned,
  kSFixed64        = kFkFixed  | kRep64Bits | kFmtSigned,
  kInt64           = kFkVarint | kRep64Bits | kFmtSigned,
  kSInt64          = kFkVarint | kRep64Bits | kFmtSigned | kTvZigZag,
  kDouble          = kFkFixed  | kRep64Bits | kFmtFloating,

  kPackedBool      = kFkPackedVarint | kRep8Bits,

  kPackedFixed32   = kFkPackedFixed  | kRep32Bits | kFmtUnsigned,
  kPackedUInt32    = kFkPackedVarint | kRep32Bits | kFmtUnsigned,
  kPackedSFixed32  = kFkPackedFixed  | kRep32Bits | kFmtSigned,
  kPackedInt32     = kFkPackedVarint | kRep32Bits | kFmtSigned,
  kPackedSInt32    = kFkPackedVarint | kRep32Bits | kFmtSigned | kTvZigZag,
  kPackedFloat     = kFkPackedFixed  | kRep32Bits | kFmtFloating,
  kPackedEnum      = kFkPackedVarint | kRep32Bits | kFmtEnum   | kTvEnum,
  kPackedEnumRange = kFkPackedVarint | kRep32Bits | kFmtEnum   | kTvRange,
  kPackedOpenEnum  = kFkPackedVarint | kRep32Bits | kFmtEnum,

  kPackedFixed64   = kFkPackedFixed  | kRep64Bits | kFmtUnsigned,
  kPackedUInt64    = kFkPackedVarint | kRep64Bits | kFmtUnsigned,
  kPackedSFixed64  = kFkPackedFixed  | kRep64Bits | kFmtSigned,
  kPackedInt64     = kFkPackedVarint | kRep64Bits | kFmtSigned,
  kPackedSInt64    = kFkPackedVarint | kRep64Bits | kFmtSigned | kTvZigZag,
  kPackedDouble    = kFkPackedFixed  | kRep64Bits | kFmtFloating,

  // String types:
  kBytes           = kFkString | kFmtArray,
  kRawString       = kFkString | kFmtUtf8  | kTvUtf8Debug,
  kUtf8String      = kFkString | kFmtUtf8  | kTvUtf8,

  // Message types:
  kMessage         = kFkMessage,

  // Map types:
  kMap             = kFkMap,
};

// clang-format on
}  // namespace field_layout

// SHARK_TC_PARAM_DECL are the parameters for tailcall functions, it is
// defined in port_def.inc.
//
// Note that this is performance sensitive: changing the parameters will change
// the registers used by the ABI calling convention, which subsequently affects
// register selection logic inside the function.

// SHARK_TC_PARAM_PASS passes values to match SHARK_TC_PARAM_DECL.
#define SHARK_TC_PARAM_PASS msg, ptr, ctx, table, hasbits, data

#ifndef NDEBUG
template <size_t align>
#ifndef _MSC_VER
[[noreturn]]
#endif
void AlignFail(uintptr_t address) {
  GOOGLE_LOG(FATAL) << "Unaligned (" << align << ") access at " << address;
}

extern template void AlignFail<4>(uintptr_t);
extern template void AlignFail<8>(uintptr_t);
#endif

// TcParser implements most of the parsing logic for tailcall tables.
class SHARK_EXPORT TcParser final {
 public:
  static const char* GenericFallback(SHARK_TC_PARAM_DECL);
  static const char* GenericFallbackLite(SHARK_TC_PARAM_DECL);

  static const char* ParseLoop(MessageLite* msg, const char* ptr,
                               ParseContext* ctx,
                               const TcParseTableBase* table);

  // Functions referenced by generated fast tables (numeric types):
  //   F: fixed      V: varint     Z: zigzag
  //   8/32/64: storage type width (bits)
  //   S: singular   R: repeated   P: packed
  //   1/2: tag length (bytes)

  // Fixed:
  static const char* FastF32S1(SHARK_TC_PARAM_DECL);
  static const char* FastF32S2(SHARK_TC_PARAM_DECL);
  static const char* FastF32R1(SHARK_TC_PARAM_DECL);
  static const char* FastF32R2(SHARK_TC_PARAM_DECL);
  static const char* FastF32P1(SHARK_TC_PARAM_DECL);
  static const char* FastF32P2(SHARK_TC_PARAM_DECL);
  static const char* FastF64S1(SHARK_TC_PARAM_DECL);
  static const char* FastF64S2(SHARK_TC_PARAM_DECL);
  static const char* FastF64R1(SHARK_TC_PARAM_DECL);
  static const char* FastF64R2(SHARK_TC_PARAM_DECL);
  static const char* FastF64P1(SHARK_TC_PARAM_DECL);
  static const char* FastF64P2(SHARK_TC_PARAM_DECL);

  // Varint:
  static const char* FastV8S1(SHARK_TC_PARAM_DECL);
  static const char* FastV8S2(SHARK_TC_PARAM_DECL);
  static const char* FastV8R1(SHARK_TC_PARAM_DECL);
  static const char* FastV8R2(SHARK_TC_PARAM_DECL);
  static const char* FastV8P1(SHARK_TC_PARAM_DECL);
  static const char* FastV8P2(SHARK_TC_PARAM_DECL);
  static const char* FastN8S1(SHARK_TC_PARAM_DECL);
  static const char* FastN8S2(SHARK_TC_PARAM_DECL);
  static const char* FastU8S1(SHARK_TC_PARAM_DECL);
  static const char* FastU8S2(SHARK_TC_PARAM_DECL);
  static const char* FastN16S1(SHARK_TC_PARAM_DECL);
  static const char* FastN16S2(SHARK_TC_PARAM_DECL);
  static const char* FastU16S1(SHARK_TC_PARAM_DECL);
  static const char* FastU16S2(SHARK_TC_PARAM_DECL);
  static const char* FastV32S1(SHARK_TC_PARAM_DECL);
  static const char* FastV32S2(SHARK_TC_PARAM_DECL);
  static const char* FastV32R1(SHARK_TC_PARAM_DECL);
  static const char* FastV32R2(SHARK_TC_PARAM_DECL);
  static const char* FastV32P1(SHARK_TC_PARAM_DECL);
  static const char* FastV32P2(SHARK_TC_PARAM_DECL);
  static const char* FastV64S1(SHARK_TC_PARAM_DECL);
  static const char* FastV64S2(SHARK_TC_PARAM_DECL);
  static const char* FastV64R1(SHARK_TC_PARAM_DECL);
  static const char* FastV64R2(SHARK_TC_PARAM_DECL);
  static const char* FastV64P1(SHARK_TC_PARAM_DECL);
  static const char* FastV64P2(SHARK_TC_PARAM_DECL);

  // Varint (with zigzag):
  static const char* FastZ32S1(SHARK_TC_PARAM_DECL);
  static const char* FastZ32S2(SHARK_TC_PARAM_DECL);
  static const char* FastZ32R1(SHARK_TC_PARAM_DECL);
  static const char* FastZ32R2(SHARK_TC_PARAM_DECL);
  static const char* FastZ32P1(SHARK_TC_PARAM_DECL);
  static const char* FastZ32P2(SHARK_TC_PARAM_DECL);
  static const char* FastZ64S1(SHARK_TC_PARAM_DECL);
  static const char* FastZ64S2(SHARK_TC_PARAM_DECL);
  static const char* FastZ64R1(SHARK_TC_PARAM_DECL);
  static const char* FastZ64R2(SHARK_TC_PARAM_DECL);
  static const char* FastZ64P1(SHARK_TC_PARAM_DECL);
  static const char* FastZ64P2(SHARK_TC_PARAM_DECL);

  // Functions referenced by generated fast tables (closed enum):
  //   E: closed enum (N.B.: open enums use V32, above)
  //   r: enum range  v: enum validator (_IsValid function)
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastErS1(SHARK_TC_PARAM_DECL);
  static const char* FastErS2(SHARK_TC_PARAM_DECL);
  static const char* FastErR1(SHARK_TC_PARAM_DECL);
  static const char* FastErR2(SHARK_TC_PARAM_DECL);
  static const char* FastEvS1(SHARK_TC_PARAM_DECL);
  static const char* FastEvS2(SHARK_TC_PARAM_DECL);
  static const char* FastEvR1(SHARK_TC_PARAM_DECL);
  static const char* FastEvR2(SHARK_TC_PARAM_DECL);

  // Functions referenced by generated fast tables (string types):
  //   B: bytes      S: string     U: UTF-8 string
  //   (empty): ArenaStringPtr     i: InlinedString
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastBS1(SHARK_TC_PARAM_DECL);
  static const char* FastBS2(SHARK_TC_PARAM_DECL);
  static const char* FastBR1(SHARK_TC_PARAM_DECL);
  static const char* FastBR2(SHARK_TC_PARAM_DECL);
  static const char* FastSS1(SHARK_TC_PARAM_DECL);
  static const char* FastSS2(SHARK_TC_PARAM_DECL);
  static const char* FastSR1(SHARK_TC_PARAM_DECL);
  static const char* FastSR2(SHARK_TC_PARAM_DECL);
  static const char* FastUS1(SHARK_TC_PARAM_DECL);
  static const char* FastUS2(SHARK_TC_PARAM_DECL);
  static const char* FastUR1(SHARK_TC_PARAM_DECL);
  static const char* FastUR2(SHARK_TC_PARAM_DECL);

  static const char* FastBiS1(SHARK_TC_PARAM_DECL);
  static const char* FastBiS2(SHARK_TC_PARAM_DECL);
  static const char* FastSiS1(SHARK_TC_PARAM_DECL);
  static const char* FastSiS2(SHARK_TC_PARAM_DECL);
  static const char* FastUiS1(SHARK_TC_PARAM_DECL);
  static const char* FastUiS2(SHARK_TC_PARAM_DECL);

  // Functions referenced by generated fast tables (message types):
  //   M: message    G: group
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastMS1(SHARK_TC_PARAM_DECL);
  static const char* FastMS2(SHARK_TC_PARAM_DECL);
  static const char* FastMR1(SHARK_TC_PARAM_DECL);
  static const char* FastMR2(SHARK_TC_PARAM_DECL);
  static const char* FastGS1(SHARK_TC_PARAM_DECL);
  static const char* FastGS2(SHARK_TC_PARAM_DECL);
  static const char* FastGR1(SHARK_TC_PARAM_DECL);
  static const char* FastGR2(SHARK_TC_PARAM_DECL);

  template <typename T>
  static inline T& RefAt(void* x, size_t offset) {
    T* target = reinterpret_cast<T*>(static_cast<char*>(x) + offset);
#ifndef NDEBUG
    if (SHARK_PREDICT_FALSE(
            reinterpret_cast<uintptr_t>(target) % alignof(T) != 0)) {
      AlignFail<alignof(T)>(reinterpret_cast<uintptr_t>(target));
    }
#endif
    return *target;
  }

  template <typename T>
  static inline const T& RefAt(const void* x, size_t offset) {
    const T* target =
        reinterpret_cast<const T*>(static_cast<const char*>(x) + offset);
#ifndef NDEBUG
    if (SHARK_PREDICT_FALSE(
            reinterpret_cast<uintptr_t>(target) % alignof(T) != 0)) {
      AlignFail<alignof(T)>(reinterpret_cast<uintptr_t>(target));
    }
#endif
    return *target;
  }

  template <typename T>
  static inline T ReadAt(const void* x, size_t offset) {
    T out;
    memcpy(&out, static_cast<const char*>(x) + offset, sizeof(T));
    return out;
  }

  // Mini parsing:
  //
  // This function parses a field from incoming data based on metadata stored in
  // the message definition. If the field is not defined in the message, it is
  // stored in either the ExtensionSet (if applicable) or the UnknownFieldSet.
  //
  // NOTE: Currently, this function only calls the table-level fallback
  // function, so it should only be called as the fallback from fast table
  // parsing.
  static const char* MiniParse(SHARK_TC_PARAM_DECL);

 private:
  friend class GeneratedTcTableLiteTest;

  template <typename TagType, bool group_coding>
  static inline const char* SingularParseMessageAuxImpl(SHARK_TC_PARAM_DECL);
  template <typename TagType, bool group_coding>
  static inline const char* RepeatedParseMessageAuxImpl(SHARK_TC_PARAM_DECL);

  static inline SHARK_ALWAYS_INLINE void SyncHasbits(
      MessageLite* msg, uint64_t hasbits, const TcParseTableBase* table) {
    const uint32_t has_bits_offset = table->has_bits_offset;
    if (has_bits_offset) {
      // Only the first 32 has-bits are updated. Nothing above those is stored,
      // but e.g. messages without has-bits update the upper bits.
      RefAt<uint32_t>(msg, has_bits_offset) = static_cast<uint32_t>(hasbits);
    }
  }

  static const char* TagDispatch(SHARK_TC_PARAM_DECL);
  static const char* ToTagDispatch(SHARK_TC_PARAM_DECL);
  static const char* ToParseLoop(SHARK_TC_PARAM_DECL);
  static const char* Error(SHARK_TC_PARAM_DECL);

  static const char* FastUnknownEnumFallback(SHARK_TC_PARAM_DECL);

  class ScopedArenaSwap;

  template <class MessageBaseT, class UnknownFieldsT>
  static const char* GenericFallbackImpl(SHARK_TC_PARAM_DECL) {
#define CHK_(x) \
  if (SHARK_PREDICT_FALSE(!(x))) return nullptr /* NOLINT */

    SyncHasbits(msg, hasbits, table);
    CHK_(ptr);
    uint32_t tag = data.tag();
    if (tag == 0) {
      ctx->SetLastTag(tag);
      return ptr;
    }
    uint32_t num = tag >> 3;
    if (table->extension_range_low <= num &&
        num <= table->extension_range_high) {
      return RefAt<ExtensionSet>(msg, table->extension_offset)
          .ParseField(tag, ptr,
                      static_cast<const MessageBaseT*>(table->default_instance),
                      &msg->_internal_metadata_, ctx);
    }
    return UnknownFieldParse(
        tag, msg->_internal_metadata_.mutable_unknown_fields<UnknownFieldsT>(),
        ptr, ctx);
#undef CHK_
  }

  // Note: `inline` is needed on template function declarations below to avoid
  // -Wattributes diagnostic in GCC.

  // Implementations for fast fixed field parsing functions:
  template <typename LayoutType, typename TagType>
  static inline const char* SingularFixed(SHARK_TC_PARAM_DECL);
  template <typename LayoutType, typename TagType>
  static inline const char* RepeatedFixed(SHARK_TC_PARAM_DECL);
  template <typename LayoutType, typename TagType>
  static inline const char* PackedFixed(SHARK_TC_PARAM_DECL);

  // Implementations for fast varint field parsing functions:
  template <typename SharkFieldType, typename TagType, bool zigzag = false>
  static inline const char* SingularVarint(SHARK_TC_PARAM_DECL);
  template <typename SharkFieldType, typename TagType, bool zigzag = false>
  static inline const char* RepeatedVarint(SHARK_TC_PARAM_DECL);
  template <typename SharkFieldType, typename TagType, bool zigzag = false>
  static inline const char* PackedVarint(SHARK_TC_PARAM_DECL);

  // Helper for ints > 127:
  template <typename SharkFieldType, typename TagType, bool zigzag = false>
  static const char* SingularVarBigint(SHARK_TC_PARAM_DECL);

  // Implementations for fast enum field parsing functions:
  template <typename TagType, uint16_t xform_val>
  static inline const char* SingularEnum(SHARK_TC_PARAM_DECL);
  template <typename TagType, uint16_t xform_val>
  static inline const char* RepeatedEnum(SHARK_TC_PARAM_DECL);

  // Implementations for fast string field parsing functions:
  enum Utf8Type { kNoUtf8 = 0, kUtf8 = 1, kUtf8ValidateOnly = 2 };
  template <typename TagType, Utf8Type utf8>
  static inline const char* SingularString(SHARK_TC_PARAM_DECL);
  template <typename TagType, Utf8Type utf8>
  static inline const char* RepeatedString(SHARK_TC_PARAM_DECL);

  // Mini field lookup:
  static const TcParseTableBase::FieldEntry* FindFieldEntry(
      const TcParseTableBase* table, uint32_t field_num);
  static StringPiece MessageName(const TcParseTableBase* table);
  static StringPiece FieldName(const TcParseTableBase* table,
                                     const TcParseTableBase::FieldEntry*);
  static bool ChangeOneof(const TcParseTableBase* table,
                          const TcParseTableBase::FieldEntry& entry,
                          uint32_t field_num, ParseContext* ctx,
                          MessageLite* msg);

  // UTF-8 validation:
  static void ReportFastUtf8Error(uint32_t decoded_tag,
                                  const TcParseTableBase* table);
  static bool MpVerifyUtf8(StringPiece wire_bytes,
                           const TcParseTableBase* table,
                           const TcParseTableBase::FieldEntry& entry,
                           uint16_t xform_val);

  // For FindFieldEntry tests:
  friend class FindFieldEntryTest;
  static constexpr const uint32_t kMtSmallScanSize = 4;

  // Mini parsing:
  static const char* MpVarint(SHARK_TC_PARAM_DECL);
  static const char* MpRepeatedVarint(SHARK_TC_PARAM_DECL);
  static const char* MpPackedVarint(SHARK_TC_PARAM_DECL);
  static const char* MpFixed(SHARK_TC_PARAM_DECL);
  static const char* MpRepeatedFixed(SHARK_TC_PARAM_DECL);
  static const char* MpPackedFixed(SHARK_TC_PARAM_DECL);
  static const char* MpString(SHARK_TC_PARAM_DECL);
  static const char* MpRepeatedString(SHARK_TC_PARAM_DECL);
  static const char* MpMessage(SHARK_TC_PARAM_DECL);
  static const char* MpRepeatedMessage(SHARK_TC_PARAM_DECL);
  static const char* MpMap(SHARK_TC_PARAM_DECL);
};

}  // namespace internal
}  // namespace shark

#include <shark/port_undef.inc>

