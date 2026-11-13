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
#include <functional>
#include <string>
#include <unordered_map>

#include <shark/stubs/status.h>

#include <shark/stubs/common.h>
#include <shark/types/type.skp.h>
#include <shark/stubs/statusor.h>
#include <shark/stubs/strutil.h>
#include <shark/transfer/json/object_source.h>
#include <shark/transfer/json/object_writer.h>
#include <shark/transfer/json/type_info.h>
#include <shark/transfer/json/type_resolver.h>
#include <shark/stubs/hash.h>
#include <shark/stubs/status.h>


// Must be included last.
#include <shark/port_def.inc>

namespace shark::util::converter {

class TypeInfo;

// An ObjectSource that can parse a stream of bytes as a protocol buffer.
// Its WriteTo() method can be given an ObjectWriter.
// This implementation uses a shark.Type for tag and name lookup.
// The field names are converted into lower camel-case when writing to the
// ObjectWriter.
//
// Sample usage: (suppose input is: string proto)
//   ArrayInputStream arr_stream(proto.data(), proto.size());
//   CodedInputStream in_stream(&arr_stream);
//   ProtoStreamObjectSource os(&in_stream, /*ServiceTypeInfo*/ typeinfo,
//                              <your message shark::Type>);
//
//   Status status = os.WriteTo(<some ObjectWriter>);
class SHARK_EXPORT ProtoStreamObjectSource : public ObjectSource {
 public:

  struct RenderOptions {
    RenderOptions() = default;
    RenderOptions(const RenderOptions&) = default;

    // Sets whether or not to use lowerCamelCase casing for enum values. If set
    // to false, enum values are output without any case conversions.
    //
    // For example, if we have an enum:
    // enum Type {
    //   ACTION_AND_ADVENTURE = 1;
    // }
    // Type type = 20;
    //
    // And this option is set to true. Then the rendered "type" field will have
    // the string "actionAndAdventure".
    // {
    //   ...
    //   "type": "actionAndAdventure",
    //   ...
    // }
    //
    // If set to false, the rendered "type" field will have the string
    // "ACTION_AND_ADVENTURE".
    // {
    //   ...
    //   "type": "ACTION_AND_ADVENTURE",
    //   ...
    // }
    bool use_lower_camel_for_enums = false;

    // Sets whether to always output enums as ints, by default this is off, and
    // enums are rendered as strings.
    bool use_ints_for_enums = false;

    // Whether to preserve proto field names
    bool preserve_proto_field_names = false;

  };

  ProtoStreamObjectSource(io::CodedInputStream* stream,
                          TypeResolver* type_resolver,
                          const shark::Type& type)
      : ProtoStreamObjectSource(stream, type_resolver, type, RenderOptions()) {}
  ProtoStreamObjectSource(io::CodedInputStream* stream,
                          TypeResolver* type_resolver,
                          const shark::Type& type,
                          const RenderOptions& render_options);

  ~ProtoStreamObjectSource() override;

  util::Status NamedWriteTo(StringPiece name,
                            ObjectWriter* ow) const override;

  // Sets the max recursion depth of proto message to be deserialized. Proto
  // messages over this depth will fail to be deserialized.
  // Default value is 64.
  void set_max_recursion_depth(int max_depth) {
    max_recursion_depth_ = max_depth;
  }

 protected:
  // Writes a proto2 Message to the ObjectWriter. When the given end_tag is
  // found this method will complete, allowing it to be used for parsing both
  // nested messages (end with 0) and nested groups (end with group end tag).
  // The include_start_and_end parameter allows this method to be called when
  // already inside of an object, and skip calling StartObject and EndObject.
  virtual util::Status WriteMessage(const shark::Type& type,
                                    StringPiece name,
                                    const uint32_t end_tag,
                                    bool include_start_and_end,
                                    ObjectWriter* ow) const;

  // Renders a repeating field (packed or unpacked).  Returns the next tag after
  // reading all sequential repeating elements. The caller should use this tag
  // before reading more tags from the stream.
  virtual util::StatusOr<uint32_t> RenderList(
      const shark::Field* field, StringPiece name,
      uint32_t list_tag, ObjectWriter* ow) const;

  // Looks up a field and verify its consistency with wire type in tag.
  const shark::Field* FindAndVerifyField(
      const shark::Type& type, uint32_t tag) const;

  // Renders a field value to the ObjectWriter.
  virtual util::Status RenderField(const shark::Field* field,
                                   StringPiece field_name,
                                   ObjectWriter* ow) const;

  // Reads field value according to Field spec in 'field' and returns the read
  // value as string. This only works for primitive datatypes (no message
  // types).
  const std::string ReadFieldValueAsString(
      const shark::Field& field) const;


  // Returns the input stream.
  io::CodedInputStream* stream() const { return stream_; }

 private:
  ProtoStreamObjectSource(io::CodedInputStream* stream,
                          const TypeInfo* typeinfo,
                          const shark::Type& type,
                          const RenderOptions& render_options);
  // Function that renders a well known type with a modified behavior.
  typedef util::Status (*TypeRenderer)(const ProtoStreamObjectSource*,
                                       const shark::Type&,
                                       StringPiece, ObjectWriter*);

  // TODO(skarvaje): Mark these methods as non-const as they modify internal
  // state (stream_).
  //
  // Renders a NWP map.
  // Returns the next tag after reading all map entries. The caller should use
  // this tag before reading more tags from the stream.
  util::StatusOr<uint32_t> RenderMap(const shark::Field* field,
                                     StringPiece name, uint32_t list_tag,
                                     ObjectWriter* ow) const;

  // Renders a packed repeating field. A packed field is stored as:
  // {tag length item1 item2 item3} instead of the less efficient
  // {tag item1 tag item2 tag item3}.
  util::Status RenderPacked(const shark::Field* field,
                            ObjectWriter* ow) const;

  // Renders a shark.Timestamp value to ObjectWriter
  static util::Status RenderTimestamp(const ProtoStreamObjectSource* os,
                                      const shark::Type& type,
                                      StringPiece name, ObjectWriter* ow);

  // Renders a shark.Duration value to ObjectWriter
  static util::Status RenderDuration(const ProtoStreamObjectSource* os,
                                     const shark::Type& type,
                                     StringPiece name, ObjectWriter* ow);

  // Following RenderTYPE functions render well known types in
  // shark/types/wrappers.shark corresponding to TYPE.
  static util::Status RenderDouble(const ProtoStreamObjectSource* os,
                                   const shark::Type& type,
                                   StringPiece name, ObjectWriter* ow);
  static util::Status RenderFloat(const ProtoStreamObjectSource* os,
                                  const shark::Type& type,
                                  StringPiece name, ObjectWriter* ow);
  static util::Status RenderInt64(const ProtoStreamObjectSource* os,
                                  const shark::Type& type,
                                  StringPiece name, ObjectWriter* ow);
  static util::Status RenderUInt64(const ProtoStreamObjectSource* os,
                                   const shark::Type& type,
                                   StringPiece name, ObjectWriter* ow);
  static util::Status RenderInt32(const ProtoStreamObjectSource* os,
                                  const shark::Type& type,
                                  StringPiece name, ObjectWriter* ow);
  static util::Status RenderUInt32(const ProtoStreamObjectSource* os,
                                   const shark::Type& type,
                                   StringPiece name, ObjectWriter* ow);
  static util::Status RenderBool(const ProtoStreamObjectSource* os,
                                 const shark::Type& type,
                                 StringPiece name, ObjectWriter* ow);
  static util::Status RenderString(const ProtoStreamObjectSource* os,
                                   const shark::Type& type,
                                   StringPiece name, ObjectWriter* ow);
  static util::Status RenderBytes(const ProtoStreamObjectSource* os,
                                  const shark::Type& type,
                                  StringPiece name, ObjectWriter* ow);

  // Renders a shark.Struct to ObjectWriter.
  static util::Status RenderStruct(const ProtoStreamObjectSource* os,
                                   const shark::Type& type,
                                   StringPiece name, ObjectWriter* ow);

  // Helper to render shark.Struct's Value fields to ObjectWriter.
  static util::Status RenderStructValue(const ProtoStreamObjectSource* os,
                                        const shark::Type& type,
                                        StringPiece name,
                                        ObjectWriter* ow);

  // Helper to render shark.Struct's ListValue fields to ObjectWriter.
  static util::Status RenderStructListValue(const ProtoStreamObjectSource* os,
                                            const shark::Type& type,
                                            StringPiece name,
                                            ObjectWriter* ow);

  // Render the "Any" type.
  static util::Status RenderAny(const ProtoStreamObjectSource* os,
                                const shark::Type& type,
                                StringPiece name, ObjectWriter* ow);

  // Render the "FieldMask" type.
  static util::Status RenderFieldMask(const ProtoStreamObjectSource* os,
                                      const shark::Type& type,
                                      StringPiece name, ObjectWriter* ow);

  static std::unordered_map<std::string, TypeRenderer>* renderers_;
  static void InitRendererMap();
  static void DeleteRendererMap();
  static TypeRenderer* FindTypeRenderer(const std::string& type_url);

  // Same as above but renders all non-message field types. Callers don't call
  // this function directly. They just use RenderField.
  util::Status RenderNonMessageField(const shark::Field* field,
                                     StringPiece field_name,
                                     ObjectWriter* ow) const;


  // Utility function to detect proto maps. The 'field' MUST be repeated.
  bool IsMap(const shark::Field& field) const;

  // Utility to read int64 and int32 values from a message type in stream_.
  // Used for reading shark.Timestamp and Duration messages.
  std::pair<int64_t, int32_t> ReadSecondsAndNanos(
      const shark::Type& type) const;

  // Helper function to check recursion depth and increment it. It will return
  // OkStatus() if the current depth is allowed. Otherwise an error is returned.
  // type_name and field_name are used for error reporting.
  util::Status IncrementRecursionDepth(StringPiece type_name,
                                       StringPiece field_name) const;

  // Input stream to read from. Ownership rests with the caller.
  mutable io::CodedInputStream* stream_;

  // Type information for all the types used in the descriptor. Used to find
  // shark::Type of nested messages/enums.
  const TypeInfo* typeinfo_;

  // Whether this class owns the typeinfo_ object. If true the typeinfo_ object
  // should be deleted in the destructor.
  bool own_typeinfo_;

  // shark::Type of the message source.
  const shark::Type& type_;


  const RenderOptions render_options_;

  // Tracks current recursion depth.
  mutable int recursion_depth_;

  // Maximum allowed recursion depth.
  int max_recursion_depth_;

  GOOGLE_DISALLOW_IMPLICIT_CONSTRUCTORS(ProtoStreamObjectSource);
};

}  // namespace shark::util::converter

#include <shark/port_undef.inc>

