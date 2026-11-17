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

#include <shark/stubs/common.h>

// This file contains constants used by //net/proto2/util/converter.

namespace shark::util::converter {
// Prefix for type URLs.
const char kTypeServiceBaseUrl[] = "type.googleapis.com";

// Format string for RFC3339 timestamp formatting.
const char kRfc3339TimeFormat[] = "%E4Y-%m-%dT%H:%M:%S";

// Same as above, but the year value is not zero-padded i.e. this accepts
// timestamps like "1-01-0001T23:59:59Z" instead of "0001-01-0001T23:59:59Z".
const char kRfc3339TimeFormatNoPadding[] = "%Y-%m-%dT%H:%M:%S";

// Minimum seconds allowed in a shark.Timestamp value.
const int64_t kTimestampMinSeconds = -62135596800LL;

// Maximum seconds allowed in a shark.Timestamp value.
const int64_t kTimestampMaxSeconds = 253402300799LL;

// Minimum seconds allowed in a shark.Duration value.
const int64_t kDurationMinSeconds = -315576000000LL;

// Maximum seconds allowed in a shark.Duration value.
const int64_t kDurationMaxSeconds = 315576000000LL;

// Nano seconds in a second.
const int32_t kNanosPerSecond = 1000000000;

// Type url representing NULL values in shark.Struct type.
const char kStructNullValueTypeUrl[] =
    "type.googleapis.com/shark.NullValue";

// Type string for shark.Struct
const char kStructType[] = "shark.Struct";

// Type string for struct.shark's shark.Value value type.
const char kStructValueType[] = "shark.Value";

// Type string for struct.shark's shark.ListValue value type.
const char kStructListValueType[] = "shark.ListValue";

// Type string for shark.Timestamp
const char kTimestampType[] = "shark.Timestamp";

// Type string for shark.Duration
const char kDurationType[] = "shark.Duration";

// Type URL for struct value type shark.Value
const char kStructValueTypeUrl[] = "type.googleapis.com/shark.Value";

// Type string for shark.Any
const char kAnyType[] = "shark.Any";

// The protobuf option name of jspb.message_id;
const char kOptionJspbMessageId[] = "jspb.message_id";

}  // namespace shark::util::converter
