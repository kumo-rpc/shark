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

#include <shark/io/memory_zero_copy_stream.h>
#include <turbo/log/logging.h>
#include <turbo/strings/internal/resize_uninitialized.h>

namespace shark::io {

    namespace {

        // Default block size for Copying{In,Out}putStreamAdaptor.
        static const int kDefaultBlockSize = 8192;

    } // namespace

    // ===================================================================

    ArrayInputStream::ArrayInputStream(const void* data, int size, int block_size)
        : data_(reinterpret_cast<const uint8_t*>(data))
        , size_(size)
        , block_size_(block_size > 0 ? block_size : size)
        , position_(0)
        , last_returned_size_(0) { }

    bool ArrayInputStream::next(const void** data, int* size) {
        if (position_ < size_) {
            last_returned_size_ = std::min(block_size_, size_ - position_);
            *data = data_ + position_;
            *size = last_returned_size_;
            position_ += last_returned_size_;
            return true;
        } else {
            // We're at the end of the array.
            last_returned_size_ = 0; // Don't let caller back up.
            return false;
        }
    }

    void ArrayInputStream::back_up(int count) {
        KCHECK_GT(last_returned_size_, 0)
            << "back_up() can only be called after a successful next().";
        KCHECK_LE(count, last_returned_size_);
        KCHECK_GE(count, 0);
        position_ -= count;
        last_returned_size_ = 0; // Don't let caller back up further.
    }

    bool ArrayInputStream::skip(int count) {
        KCHECK_GE(count, 0);
        last_returned_size_ = 0; // Don't let caller back up.
        if (count > size_ - position_) {
            position_ = size_;
            return false;
        } else {
            position_ += count;
            return true;
        }
    }

    int64_t ArrayInputStream::byte_count() const {
        return position_;
    }

    // ===================================================================

    ArrayOutputStream::ArrayOutputStream(void* data, int size, int block_size)
        : data_(reinterpret_cast<uint8_t*>(data))
        , size_(size)
        , block_size_(block_size > 0 ? block_size : size)
        , position_(0)
        , last_returned_size_(0) { }

    bool ArrayOutputStream::next(void** data, int* size) {
        if (position_ < size_) {
            last_returned_size_ = std::min(block_size_, size_ - position_);
            *data = data_ + position_;
            *size = last_returned_size_;
            position_ += last_returned_size_;
            return true;
        } else {
            // We're at the end of the array.
            last_returned_size_ = 0; // Don't let caller back up.
            return false;
        }
    }

    void ArrayOutputStream::back_up(int count) {
        KCHECK_LE(count, last_returned_size_)
            << "back_up() can not exceed the size of the last next() call.";
        KCHECK_GE(count, 0);
        position_ -= count;
        last_returned_size_ -= count;
    }

    int64_t ArrayOutputStream::byte_count() const {
        return position_;
    }

    // ===================================================================

    StringOutputStream::StringOutputStream(std::string* target)
        : target_(target) { }

    bool StringOutputStream::next(void** data, int* size) {
        KCHECK(target_ != nullptr);
        size_t old_size = target_->size();

        // Grow the string.
        size_t new_size;
        if (old_size < target_->capacity()) {
            // Resize the string to match its capacity, since we can get away
            // without a memory allocation this way.
            new_size = target_->capacity();
        } else {
            // Size has reached capacity, try to double it.
            new_size = old_size * 2;
        }
        // Avoid integer overflow in returned '*size'.
        new_size = std::min(new_size, old_size + std::numeric_limits<int>::max());
        // Increase the size, also make sure that it is at least kMinimumSize.
        turbo::strings_internal::STLStringResizeUninitialized(
            target_,
            std::max(new_size,
                kMinimumSize + 0)); // "+ 0" works around GCC4 weirdness.

        *data = mutable_string_data(target_) + old_size;
        *size = target_->size() - old_size;
        return true;
    }

    void StringOutputStream::back_up(int count) {
        KCHECK_GE(count, 0);
        KCHECK(target_ != nullptr);
        KCHECK_LE(static_cast<size_t>(count), target_->size());
        target_->resize(target_->size() - count);
    }

    int64_t StringOutputStream::byte_count() const {
        KCHECK(target_ != nullptr);
        return target_->size();
    }

    // ===================================================================

    int CopyingInputStream::skip(int count) {
        char junk[4096];
        int skipped = 0;
        while (skipped < count) {
            int bytes = Read(junk, std::min(count - skipped, turbo::implicit_cast<int>(sizeof(junk))));
            if (bytes <= 0) {
                // EOF or read error.
                return skipped;
            }
            skipped += bytes;
        }
        return skipped;
    }

    CopyingInputStreamAdaptor::CopyingInputStreamAdaptor(
        CopyingInputStream* copying_stream, int block_size)
        : copying_stream_(copying_stream)
        , owns_copying_stream_(false)
        , failed_(false)
        , position_(0)
        , buffer_size_(block_size > 0 ? block_size : kDefaultBlockSize)
        , buffer_used_(0)
        , backup_bytes_(0) { }

    CopyingInputStreamAdaptor::~CopyingInputStreamAdaptor() {
        if (owns_copying_stream_) {
            delete copying_stream_;
        }
    }

    bool CopyingInputStreamAdaptor::next(const void** data, int* size) {
        if (failed_) {
            // Already failed on a previous read.
            return false;
        }

        AllocateBufferIfNeeded();

        if (backup_bytes_ > 0) {
            // We have data left over from a previous back_up(), so just return that.
            *data = buffer_.get() + buffer_used_ - backup_bytes_;
            *size = backup_bytes_;
            backup_bytes_ = 0;
            return true;
        }

        // Read new data into the buffer.
        buffer_used_ = copying_stream_->Read(buffer_.get(), buffer_size_);
        if (buffer_used_ <= 0) {
            // EOF or read error.  We don't need the buffer anymore.
            if (buffer_used_ < 0) {
                // Read error (not EOF).
                failed_ = true;
            }
            FreeBuffer();
            return false;
        }
        position_ += buffer_used_;

        *size = buffer_used_;
        *data = buffer_.get();
        return true;
    }

    void CopyingInputStreamAdaptor::back_up(int count) {
        KCHECK(backup_bytes_ == 0 && buffer_ != nullptr)
            << " back_up() can only be called after next().";
        KCHECK_LE(count, buffer_used_)
            << " Can't back up over more bytes than were returned by the last call"
               " to next().";
        KCHECK_GE(count, 0) << " Parameter to back_up() can't be negative.";

        backup_bytes_ = count;
    }

    bool CopyingInputStreamAdaptor::skip(int count) {
        KCHECK_GE(count, 0);

        if (failed_) {
            // Already failed on a previous read.
            return false;
        }

        // First skip any bytes left over from a previous back_up().
        if (backup_bytes_ >= count) {
            // We have more data left over than we're trying to skip.  Just chop it.
            backup_bytes_ -= count;
            return true;
        }

        count -= backup_bytes_;
        backup_bytes_ = 0;

        int skipped = copying_stream_->skip(count);
        position_ += skipped;
        return skipped == count;
    }

    int64_t CopyingInputStreamAdaptor::byte_count() const {
        return position_ - backup_bytes_;
    }

    void CopyingInputStreamAdaptor::AllocateBufferIfNeeded() {
        if (buffer_ == nullptr) {
            buffer_.reset(new uint8_t[buffer_size_]);
        }
    }

    void CopyingInputStreamAdaptor::FreeBuffer() {
        KCHECK_EQ(backup_bytes_, 0);
        buffer_used_ = 0;
        buffer_.reset();
    }

    // ===================================================================

    CopyingOutputStreamAdaptor::CopyingOutputStreamAdaptor(
        CopyingOutputStream* copying_stream, int block_size)
        : copying_stream_(copying_stream)
        , owns_copying_stream_(false)
        , failed_(false)
        , position_(0)
        , buffer_size_(block_size > 0 ? block_size : kDefaultBlockSize)
        , buffer_used_(0) { }

    CopyingOutputStreamAdaptor::~CopyingOutputStreamAdaptor() {
        WriteBuffer();
        if (owns_copying_stream_) {
            delete copying_stream_;
        }
    }

    bool CopyingOutputStreamAdaptor::Flush() {
        return WriteBuffer();
    }

    bool CopyingOutputStreamAdaptor::next(void** data, int* size) {
        if (buffer_used_ == buffer_size_) {
            if (!WriteBuffer())
                return false;
        }

        AllocateBufferIfNeeded();

        *data = buffer_.get() + buffer_used_;
        *size = buffer_size_ - buffer_used_;
        buffer_used_ = buffer_size_;
        return true;
    }

    void CopyingOutputStreamAdaptor::back_up(int count) {
        if (count == 0) {
            // TODO: Remove this suppression.
            (void)Flush();
            return;
        }
        KCHECK_GE(count, 0);
        KCHECK_EQ(buffer_used_, buffer_size_)
            << " back_up() can only be called after next().";
        KCHECK_LE(count, buffer_used_)
            << " Can't back up over more bytes than were returned by the last call"
               " to next().";

        buffer_used_ -= count;
    }

    int64_t CopyingOutputStreamAdaptor::byte_count() const {
        return position_ + buffer_used_;
    }

    bool CopyingOutputStreamAdaptor::write_aliased_raw(const void* data, int size) {
        if (size >= buffer_size_) {
            if (!Flush() || !copying_stream_->Write(data, size)) {
                return false;
            }
            DKCHECK_EQ(buffer_used_, 0);
            position_ += size;
            return true;
        }

        void* out;
        int out_size;
        while (true) {
            if (!next(&out, &out_size)) {
                return false;
            }

            if (size <= out_size) {
                std::memcpy(out, data, size);
                back_up(out_size - size);
                break;
            }

            std::memcpy(out, data, out_size);
            data = static_cast<const char*>(data) + out_size;
            size -= out_size;
        }
        return true;
    }

    bool CopyingOutputStreamAdaptor::write_cord(const turbo::Cord& cord) {
        for (std::string_view chunk : cord.chunks()) {
            if (!write_aliased_raw(chunk.data(), chunk.size())) {
                return false;
            }
        }
        return true;
    }

    bool CopyingOutputStreamAdaptor::WriteBuffer() {
        if (failed_) {
            // Already failed on a previous write.
            return false;
        }

        if (buffer_used_ == 0)
            return true;

        if (copying_stream_->Write(buffer_.get(), buffer_used_)) {
            position_ += buffer_used_;
            buffer_used_ = 0;
            return true;
        } else {
            failed_ = true;
            FreeBuffer();
            return false;
        }
    }

    void CopyingOutputStreamAdaptor::AllocateBufferIfNeeded() {
        if (buffer_ == nullptr) {
            buffer_.reset(new uint8_t[buffer_size_]);
        }
    }

    void CopyingOutputStreamAdaptor::FreeBuffer() {
        buffer_used_ = 0;
        buffer_.reset();
    }

    // ===================================================================

    LimitingInputStream::LimitingInputStream(ZeroCopyInputStream* input,
        int64_t limit)
        : input_(input)
        , limit_(limit) {
        prior_bytes_read_ = input_->byte_count();
    }

    LimitingInputStream::~LimitingInputStream() {
        // If we overshot the limit, back up.
        if (limit_ < 0)
            input_->back_up(-limit_);
    }

    bool LimitingInputStream::next(const void** data, int* size) {
        if (limit_ <= 0)
            return false;
        if (!input_->next(data, size))
            return false;

        limit_ -= *size;
        if (limit_ < 0) {
            // We overshot the limit.  Reduce *size to hide the rest of the buffer.
            *size += limit_;
        }
        return true;
    }

    void LimitingInputStream::back_up(int count) {
        if (limit_ < 0) {
            input_->back_up(count - limit_);
            limit_ = count;
        } else {
            input_->back_up(count);
            limit_ += count;
        }
    }

    bool LimitingInputStream::skip(int count) {
        if (count > limit_) {
            if (limit_ < 0)
                return false;
            // TODO: Remove this suppression.
            (void)input_->skip(limit_);
            limit_ = 0;
            return false;
        } else {
            if (!input_->skip(count))
                return false;
            limit_ -= count;
            return true;
        }
    }

    int64_t LimitingInputStream::byte_count() const {
        if (limit_ < 0) {
            return input_->byte_count() + limit_ - prior_bytes_read_;
        } else {
            return input_->byte_count() - prior_bytes_read_;
        }
    }

    bool LimitingInputStream::read_cord(turbo::Cord* cord, int count) {
        if (count <= 0)
            return true;
        if (count <= limit_) {
            if (!input_->read_cord(cord, count))
                return false;
            limit_ -= count;
            return true;
        }
        if (limit_ > 0) {
            // TODO: Remove this suppression.
            (void)input_->read_cord(cord, limit_);
            limit_ = 0;
        }
        return false;
    }

    // ===================================================================
    CordInputStream::CordInputStream(const turbo::Cord* cord)
        : it_(cord->char_begin())
        , length_(cord->size())
        , bytes_remaining_(length_) {
        LoadChunkData();
    }

    bool CordInputStream::LoadChunkData() {
        if (bytes_remaining_ != 0) {
            std::string_view sv = turbo::Cord::chunk_remaining(it_);
            data_ = sv.data();
            size_ = available_ = sv.size();
            return true;
        }
        size_ = available_ = 0;
        return false;
    }

    bool CordInputStream::NextChunk(size_t skip) {
        // `size_ == 0` indicates we're at EOF.
        if (size_ == 0)
            return false;

        // The caller consumed 'size_ - available_' bytes that are not yet accounted
        // for in the iterator position to get to the start of the next chunk.
        const size_t distance = size_ - available_ + skip;
        turbo::Cord::advance(&it_, distance);
        bytes_remaining_ -= skip;

        return LoadChunkData();
    }

    bool CordInputStream::next(const void** data, int* size) {
        if (available_ > 0 || NextChunk(0)) {
            *data = data_ + size_ - available_;
            size_t consumed = std::min(available_, size_t { std::numeric_limits<int>::max() });
            *size = consumed;
            bytes_remaining_ -= consumed;
            available_ -= consumed;
            return true;
        }
        return false;
    }

    void CordInputStream::back_up(int count) {
        // Backup is only allowed on last returned chunk from `next()`.
        KCHECK_LE(static_cast<size_t>(count), size_ - available_);

        available_ += count;
        bytes_remaining_ += count;
    }

    bool CordInputStream::skip(int count) {
        // Short circuit if we stay inside the current chunk.
        if (static_cast<size_t>(count) <= available_) {
            available_ -= count;
            bytes_remaining_ -= count;
            return true;
        }

        // Sanity check the skip count.
        if (static_cast<size_t>(count) <= bytes_remaining_) {
            // skip to end: do not return EOF condition: skipping into EOF is ok.
            NextChunk(count);
            return true;
        }
        NextChunk(bytes_remaining_);
        return false;
    }

    int64_t CordInputStream::byte_count() const {
        return length_ - bytes_remaining_;
    }

    bool CordInputStream::read_cord(turbo::Cord* cord, int count) {
        if (count <= 0)
            return true;
        // advance the iterator to the current position
        const size_t used = size_ - available_;
        turbo::Cord::advance(&it_, used);

        // Read the cord, adjusting the iterator position.
        // Make sure to cap at available bytes to avoid hard crashes.
        const size_t n = std::min(static_cast<size_t>(count), bytes_remaining_);
        cord->append(turbo::Cord::advance_and_read(&it_, n));

        // Update current chunk data.
        bytes_remaining_ -= n;
        LoadChunkData();

        return n == static_cast<size_t>(count);
    }

    CordOutputStream::CordOutputStream(size_t size_hint)
        : size_hint_(size_hint) { }

    CordOutputStream::CordOutputStream(turbo::Cord cord, size_t size_hint)
        : cord_(std::move(cord))
        , size_hint_(size_hint)
        , state_(cord_.empty() ? State::kEmpty : State::kSteal) { }

    CordOutputStream::CordOutputStream(turbo::CordBuffer buffer, size_t size_hint)
        : size_hint_(size_hint)
        , state_(buffer.length() < buffer.capacity() ? State::kPartial
                                                     : State::kFull)
        , buffer_(std::move(buffer)) { }

    CordOutputStream::CordOutputStream(turbo::Cord cord, turbo::CordBuffer buffer,
        size_t size_hint)
        : cord_(std::move(cord))
        , size_hint_(size_hint)
        , state_(buffer.length() < buffer.capacity() ? State::kPartial
                                                     : State::kFull)
        , buffer_(std::move(buffer)) { }

    bool CordOutputStream::next(void** data, int* size) {
        // Use 128 bytes as a minimum buffer size if we don't have any application
        // provided size hints. This number is picked somewhat arbitrary as 'small
        // enough to avoid excessive waste on small data, and large enough to not
        // waste CPU and memory on tiny buffer overhead'.
        // It is worth noting that absent size hints, we pick 'current size' as
        // the default buffer size (capped at max flat size), which means we quickly
        // double the buffer size. This is in contrast to `Cord::append()` functions
        // accepting strings which use a conservative 10% growth.
        static const size_t kMinBlockSize = 128;

        size_t desired_size, max_size;
        const size_t cord_size = cord_.size() + buffer_.length();
        if (size_hint_ > cord_size) {
            // Try to hit size_hint_ exactly so the caller doesn't receive a larger
            // buffer than indicated, requiring a non-zero call to back_up() to undo
            // the buffer capacity we returned beyond the indicated size hint.
            desired_size = size_hint_ - cord_size;
            max_size = desired_size;
        } else {
            // We're past the size hint or don't have a size hint.  Try to allocate a
            // block as large as what we have so far, or at least kMinBlockSize bytes.
            // CordBuffer will truncate this to an appropriate size if it is too large.
            desired_size = std::max(cord_size, kMinBlockSize);
            max_size = std::numeric_limits<size_t>::max();
        }

        switch (state_) {
        case State::kSteal:
            // Steal last buffer from Cord if available.
            assert(buffer_.length() == 0);
            buffer_ = cord_.get_append_buffer(desired_size);
            break;
        case State::kPartial:
            // Use existing capacity in 'buffer_`
            assert(buffer_.length() < buffer_.capacity());
            break;
        case State::kFull:
            assert(buffer_.length() > 0);
            cord_.append(std::move(buffer_));
            [[fallthrough]];
        case State::kEmpty:
            assert(buffer_.length() == 0);
            buffer_ = turbo::CordBuffer::create_with_default_limit(desired_size);
            break;
        }

        // Get all available capacity from the buffer.
        turbo::Span<char> span = buffer_.available();
        assert(!span.empty());
        *data = span.data();

        // Only hand out up to 'max_size', which is limited if there is a size hint
        // specified, and we have more available than the size hint.
        if (span.size() > max_size) {
            *size = static_cast<int>(max_size);
            buffer_.increase_length_by(max_size);
            state_ = State::kPartial;
        } else {
            *size = static_cast<int>(span.size());
            buffer_.increase_length_by(span.size());
            state_ = State::kFull;
        }

        return true;
    }

    void CordOutputStream::back_up(int count) {
        // Check if something to do, else state remains unchanged.
        assert(0 <= count && count <= byte_count());
        if (count == 0)
            return;

        // Backup() is not supposed to backup beyond last next() call
        const int buffer_length = static_cast<int>(buffer_.length());
        assert(count <= buffer_length);
        if (count <= buffer_length) {
            buffer_.set_length(static_cast<size_t>(buffer_length - count));
            state_ = State::kPartial;
        } else {
            buffer_ = { };
            cord_.remove_suffix(static_cast<size_t>(count));
            state_ = State::kSteal;
        }
    }

    int64_t CordOutputStream::byte_count() const {
        return static_cast<int64_t>(cord_.size() + buffer_.length());
    }

    bool CordOutputStream::write_cord(const turbo::Cord& cord) {
        cord_.append(std::move(buffer_));
        cord_.append(cord);
        state_ = State::kSteal; // Attempt to utilize existing capacity in `cord'
        return true;
    }

    turbo::Cord CordOutputStream::consume() {
        cord_.append(std::move(buffer_));
        state_ = State::kEmpty;
        return std::move(cord_);
    }

} // namespace shark::io
