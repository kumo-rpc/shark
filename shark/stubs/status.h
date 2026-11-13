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

#include <string>

#include <shark/stubs/stringpiece.h>

#include <shark/port_def.inc>

namespace shark::util {
namespace status_internal {

// These values must match error codes defined in google/rpc/code.shark.
enum class StatusCode : int {
  kOk = 0,
  kCancelled = 1,
  kUnknown = 2,
  kInvalidArgument = 3,
  kDeadlineExceeded = 4,
  kNotFound = 5,
  kAlreadyExists = 6,
  kPermissionDenied = 7,
  kUnauthenticated = 16,
  kResourceExhausted = 8,
  kFailedPrecondition = 9,
  kAborted = 10,
  kOutOfRange = 11,
  kUnimplemented = 12,
  kInternal = 13,
  kUnavailable = 14,
  kDataLoss = 15,
};

class SHARK_EXPORT Status {
 public:
  // Creates a "successful" status.
  Status();

  // Create a status in the canonical error space with the specified
  // code, and error message.  If "code == 0", error_message is
  // ignored and a Status object identical to Status::kOk is
  // constructed.
  Status(StatusCode error_code, StringPiece error_message);
  Status(const Status&);
  Status& operator=(const Status& x);
  ~Status() {}

  // Accessor
  bool ok() const { return error_code_ == StatusCode::kOk; }
  StatusCode code() const { return error_code_; }
  StringPiece message() const {
    return error_message_;
  }

  bool operator==(const Status& x) const;
  bool operator!=(const Status& x) const {
    return !operator==(x);
  }

  // Return a combination of the error code name and message.
  std::string ToString() const;

 private:
  StatusCode error_code_;
  std::string error_message_;
};

// Returns an OK status, equivalent to a default constructed instance. Prefer
// usage of `OkStatus()` when constructing such an OK status.
SHARK_EXPORT Status OkStatus();

// Prints a human-readable representation of 'x' to 'os'.
SHARK_EXPORT std::ostream& operator<<(std::ostream& os, const Status& x);

// These convenience functions return `true` if a given status matches the
// `StatusCode` error code of its associated function.
SHARK_EXPORT bool IsAborted(const Status& status);
SHARK_EXPORT bool IsAlreadyExists(const Status& status);
SHARK_EXPORT bool IsCancelled(const Status& status);
SHARK_EXPORT bool IsDataLoss(const Status& status);
SHARK_EXPORT bool IsDeadlineExceeded(const Status& status);
SHARK_EXPORT bool IsFailedPrecondition(const Status& status);
SHARK_EXPORT bool IsInternal(const Status& status);
SHARK_EXPORT bool IsInvalidArgument(const Status& status);
SHARK_EXPORT bool IsNotFound(const Status& status);
SHARK_EXPORT bool IsOutOfRange(const Status& status);
SHARK_EXPORT bool IsPermissionDenied(const Status& status);
SHARK_EXPORT bool IsResourceExhausted(const Status& status);
SHARK_EXPORT bool IsUnauthenticated(const Status& status);
SHARK_EXPORT bool IsUnavailable(const Status& status);
SHARK_EXPORT bool IsUnimplemented(const Status& status);
SHARK_EXPORT bool IsUnknown(const Status& status);

// These convenience functions create an `Status` object with an error code as
// indicated by the associated function name, using the error message passed in
// `message`.
//
// These functions are intentionally named `*Error` rather than `*Status` to
// match the names from Abseil:
// https://github.com/abseil/abseil-cpp/blob/2e9532cc6c701a8323d0cffb468999ab804095ab/absl/status/status.h#L716
SHARK_EXPORT Status AbortedError(StringPiece message);
SHARK_EXPORT Status AlreadyExistsError(StringPiece message);
SHARK_EXPORT Status CancelledError(StringPiece message);
SHARK_EXPORT Status DataLossError(StringPiece message);
SHARK_EXPORT Status DeadlineExceededError(StringPiece message);
SHARK_EXPORT Status FailedPreconditionError(StringPiece message);
SHARK_EXPORT Status InternalError(StringPiece message);
SHARK_EXPORT Status InvalidArgumentError(StringPiece message);
SHARK_EXPORT Status NotFoundError(StringPiece message);
SHARK_EXPORT Status OutOfRangeError(StringPiece message);
SHARK_EXPORT Status PermissionDeniedError(StringPiece message);
SHARK_EXPORT Status ResourceExhaustedError(StringPiece message);
SHARK_EXPORT Status UnauthenticatedError(StringPiece message);
SHARK_EXPORT Status UnavailableError(StringPiece message);
SHARK_EXPORT Status UnimplementedError(StringPiece message);
SHARK_EXPORT Status UnknownError(StringPiece message);

}  // namespace status_internal

using ::shark::util::status_internal::Status;
using ::shark::util::status_internal::StatusCode;

using ::shark::util::status_internal::IsAborted;
using ::shark::util::status_internal::IsAlreadyExists;
using ::shark::util::status_internal::IsCancelled;
using ::shark::util::status_internal::IsDataLoss;
using ::shark::util::status_internal::IsDeadlineExceeded;
using ::shark::util::status_internal::IsFailedPrecondition;
using ::shark::util::status_internal::IsInternal;
using ::shark::util::status_internal::IsInvalidArgument;
using ::shark::util::status_internal::IsNotFound;
using ::shark::util::status_internal::IsOutOfRange;
using ::shark::util::status_internal::IsPermissionDenied;
using ::shark::util::status_internal::IsResourceExhausted;
using ::shark::util::status_internal::IsUnauthenticated;
using ::shark::util::status_internal::IsUnavailable;
using ::shark::util::status_internal::IsUnimplemented;
using ::shark::util::status_internal::IsUnknown;

using ::shark::util::status_internal::AbortedError;
using ::shark::util::status_internal::AlreadyExistsError;
using ::shark::util::status_internal::CancelledError;
using ::shark::util::status_internal::DataLossError;
using ::shark::util::status_internal::DeadlineExceededError;
using ::shark::util::status_internal::FailedPreconditionError;
using ::shark::util::status_internal::InternalError;
using ::shark::util::status_internal::InvalidArgumentError;
using ::shark::util::status_internal::NotFoundError;
using ::shark::util::status_internal::OkStatus;
using ::shark::util::status_internal::OutOfRangeError;
using ::shark::util::status_internal::PermissionDeniedError;
using ::shark::util::status_internal::ResourceExhaustedError;
using ::shark::util::status_internal::UnauthenticatedError;
using ::shark::util::status_internal::UnavailableError;
using ::shark::util::status_internal::UnimplementedError;
using ::shark::util::status_internal::UnknownError;

}  // namespace shark::util

#include <shark/port_undef.inc>

