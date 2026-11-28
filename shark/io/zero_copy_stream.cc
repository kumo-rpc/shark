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

#include <shark/io/zero_copy_stream.h>
#include <turbo/log/logging.h>

namespace shark::io {


bool ZeroCopyInputStream::read_cord(turbo::Cord* cord, int count) {
  if (count <= 0) return true;

  turbo::CordBuffer cord_buffer = cord->get_append_buffer(count);
  turbo::Span<char> out = cord_buffer.available_up_to(count);

  // We need to distinguish between the case where the stream is exhausted and
  // the case where the stream returns an empty chunk. If the next chunk is
  // empty, call Next() till it returns a non-empty chunk or exhausted.
  auto FetchNextNonEmptyChunk = [&]() -> turbo::Span<const char> {
    const void* buffer;
    int size;
    do {
      if (!next(&buffer, &size)) return {};
    } while (size == 0);

    if (size > count) {
      back_up(size - count);
      size = count;
    }
    return turbo::make_const_span(static_cast<const char*>(buffer), size);
  };

  auto AppendFullBuffer = [&]() -> turbo::Span<char> {
    cord->append(std::move(cord_buffer));
    cord_buffer = turbo::CordBuffer::create_with_default_limit(count);
    return cord_buffer.available_up_to(count);
  };

  auto CopyBytes = [&](turbo::Span<char>& dst, turbo::Span<const char>& src,
                       size_t bytes) {
    memcpy(dst.data(), src.data(), bytes);
    dst.remove_prefix(bytes);
    src.remove_prefix(bytes);
    count -= bytes;
    cord_buffer.increase_length_by(bytes);
  };

  do {
    turbo::Span<const char> in = FetchNextNonEmptyChunk();
    if (in.empty()) {
      // Append whatever we have pending so far.
      cord->append(std::move(cord_buffer));
      return false;
    }

    if (out.empty()) out = AppendFullBuffer();

    while (in.size() > out.size()) {
      CopyBytes(out, in, out.size());
      out = AppendFullBuffer();
    }

    CopyBytes(out, in, in.size());
  } while (count > 0);

  cord->append(std::move(cord_buffer));
  return true;
}

bool ZeroCopyOutputStream::write_aliased_raw(const void*, int) {
    return false;
}

bool ZeroCopyOutputStream::write_cord(const turbo::Cord& cord) {
  if (cord.empty()) return true;

  void* buffer;
  int buffer_size = 0;
  if (!next(&buffer, &buffer_size)) return false;

  for (std::string_view fragment : cord.chunks()) {
    while (fragment.size() > static_cast<size_t>(buffer_size)) {
      std::memcpy(buffer, fragment.data(), buffer_size);

      fragment.remove_prefix(buffer_size);

      if (!next(&buffer, &buffer_size)) return false;
    }
    std::memcpy(buffer, fragment.data(), fragment.size());

    // Advance the buffer.
    buffer = static_cast<char*>(buffer) + fragment.size();
    buffer_size -= static_cast<int>(fragment.size());
  }
  back_up(buffer_size);
  return true;
}


    void ZeroCopyStreamByteSink::Append(const char* bytes, size_t len) {
    while (!failed_ && len > 0) {
        if (buffer_size_ == 0) {
            int size;
            if (!stream_->next(&buffer_, &size)) {
                // There isn't a way for ByteSink to report errors.
                buffer_size_ = 0;
                failed_ = true;
                return;
            }
            buffer_size_ = static_cast<unsigned int>(size);
        }

        auto to_write = std::min(len, buffer_size_);
        memcpy(buffer_, bytes, to_write);

        buffer_ = static_cast<char*>(buffer_) + to_write;
        buffer_size_ -= to_write;

        bytes += to_write;
        len -= to_write;

        bytes_written_ += to_write;
    }
}

}  // namespace shark::io
