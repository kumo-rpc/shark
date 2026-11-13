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

#include <shark/types/any.h>

#include <shark/arenastring.h>
#include <shark/descriptor.h>
#include <shark/generated/generated_message_util.h>
#include <shark/message.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark::internal {

bool AnyMetadata::PackFrom(Arena* arena, const Message& message) {
  return PackFrom(arena, message, kTypeGoogleApisComPrefix);
}

bool AnyMetadata::PackFrom(Arena* arena, const Message& message,
                           StringPiece type_url_prefix) {
  type_url_->Set(
      GetTypeUrl(message.GetDescriptor()->full_name(), type_url_prefix), arena);
  return message.SerializeToString(value_->Mutable(arena));
}

bool AnyMetadata::UnpackTo(Message* message) const {
  if (!InternalIs(message->GetDescriptor()->full_name())) {
    return false;
  }
  return message->ParseFromString(value_->Get());
}

bool GetAnyFieldDescriptors(const Message& message,
                            const FieldDescriptor** type_url_field,
                            const FieldDescriptor** value_field) {
  const Descriptor* descriptor = message.GetDescriptor();
  if (descriptor->full_name() != kAnyFullTypeName) {
    return false;
  }
  *type_url_field = descriptor->FindFieldByNumber(1);
  *value_field = descriptor->FindFieldByNumber(2);
  return (*type_url_field != nullptr &&
          (*type_url_field)->type() == SharkFieldType::STRING &&
          *value_field != nullptr &&
          (*value_field)->type() == SharkFieldType::BYTES);
}

}  // namespace shark::internal

#include <shark/port_undef.inc>
