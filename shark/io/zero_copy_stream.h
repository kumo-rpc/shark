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

#include <cstdint>
#include <turbo/macros/macros.h>
#include <turbo/cord/cord.h>

namespace shark::io {


    // Defined in this file.
    class ZeroCopyInputStream;
    class ZeroCopyOutputStream;

    // Abstract interface similar to an input stream but designed to minimize
    // copying.
    class KUMO_DLL ZeroCopyInputStream {
    public:
        ZeroCopyInputStream()  = default;
        virtual ~ZeroCopyInputStream() = default;
        ZeroCopyInputStream(const ZeroCopyInputStream&) = delete;
        ZeroCopyInputStream& operator=(const ZeroCopyInputStream&) = delete;

        // Obtains a chunk of data from the stream.
        //
        // Preconditions:
        // * "size" and "data" are not NULL.
        //
        // Postconditions:
        // * If the returned value is false, there is no more data to return or
        //   an error occurred.  All errors are permanent.
        // * Otherwise, "size" points to the actual number of bytes read and "data"
        //   points to a pointer to a buffer containing these bytes.
        // * Ownership of this buffer remains with the stream, and the buffer
        //   remains valid only until some other method of the stream is called
        //   or the stream is destroyed.
        // * It is legal for the returned buffer to have zero size, as long
        //   as repeatedly calling next() eventually yields a buffer with non-zero
        //   size.
        virtual bool next(const void** data, int* size) = 0;

        // Backs up a number of bytes, so that the next call to next() returns
        // data again that was already returned by the last call to next().  This
        // is useful when writing procedures that are only supposed to read up
        // to a certain point in the input, then return.  If next() returns a
        // buffer that goes beyond what you wanted to read, you can use back_up()
        // to return to the point where you intended to finish.
        //
        // This method can be called with `count = 0` to finalize (flush) any
        // previously returned buffer. For example, a file output stream can
        // flush buffers returned from a previous call to next() upon such
        // back_up(0) invocations. ZeroCopyOutputStream callers should always
        // invoke back_up() after a final next() call, even if there is no
        // excess buffer data to be backed up to indicate a flush point.
        //
        // Preconditions:
        // * The last method called must have been next().
        // * count must be less than or equal to the size of the last buffer
        //   returned by next().
        //
        // Postconditions:
        // * The last "count" bytes of the last buffer returned by next() will be
        //   pushed back into the stream.  Subsequent calls to next() will return
        //   the same data again before producing new data.
        virtual void back_up(int count) = 0;

        // Skips a number of bytes.  Returns false if the end of the stream is
        // reached or some input error occurred.  In the end-of-stream case, the
        // stream is advanced to the end of the stream (so byte_count() will return
        // the total size of the stream).
        [[nodiscard]] virtual bool skip(int count) = 0;

        // Returns the total number of bytes read since this object was created.
        [[nodiscard]]  virtual int64_t byte_count() const = 0;
        [[nodiscard]] virtual bool read_cord(turbo::Cord* cord,
                                                            int count);
    };

    // Abstract interface similar to an output stream but designed to minimize
    // copying.
    class KUMO_DLL ZeroCopyOutputStream {
    public:
        ZeroCopyOutputStream()  = default;
        virtual ~ZeroCopyOutputStream()  = default;

        ZeroCopyOutputStream(const ZeroCopyOutputStream&) = delete;
        ZeroCopyOutputStream& operator=(const ZeroCopyOutputStream&) = delete;

        // Obtains a buffer into which data can be written.  Any data written
        // into this buffer will eventually (maybe instantly, maybe later on)
        // be written to the output.
        //
        // Preconditions:
        // * "size" and "data" are not NULL.
        //
        // Postconditions:
        // * If the returned value is false, an error occurred.  All errors are
        //   permanent.
        // * Otherwise, "size" points to the actual number of bytes in the buffer
        //   and "data" points to the buffer.
        // * Ownership of this buffer remains with the stream, and the buffer
        //   remains valid only until some other method of the stream is called
        //   or the stream is destroyed.
        // * Any data which the caller stores in this buffer will eventually be
        //   written to the output (unless back_up() is called).
        // * It is legal for the returned buffer to have zero size, as long
        //   as repeatedly calling next() eventually yields a buffer with non-zero
        //   size.
        virtual bool next(void** data, int* size) = 0;

        // Backs up a number of bytes, so that the end of the last buffer returned
        // by next() is not actually written.  This is needed when you finish
        // writing all the data you want to write, but the last buffer was bigger
        // than you needed.  You don't want to write a bunch of garbage after the
        // end of your data, so you use back_up() to back up.
        //
        // Preconditions:
        // * The last method called must have been next().
        // * count must be less than or equal to the size of the last buffer
        //   returned by next().
        // * The caller must not have written anything to the last "count" bytes
        //   of that buffer.
        //
        // Postconditions:
        // * The last "count" bytes of the last buffer returned by next() will be
        //   ignored.
        virtual void back_up(int count) = 0;

        // Returns the total number of bytes written since this object was created.
        [[nodiscard]] virtual int64_t byte_count() const = 0;

        // Write a given chunk of data to the output.  Some output streams may
        // implement this in a way that avoids copying. Check AllowsAliasing() before
        // calling write_aliased_raw(). It will SHARK_CHECK fail if write_aliased_raw() is
        // called on a stream that does not allow aliasing.
        //
        // NOTE: It is caller's responsibility to ensure that the chunk of memory
        // remains live until all of the data has been consumed from the stream.
        virtual bool write_aliased_raw(const void* data, int size);
         [[nodiscard]] virtual bool allows_aliasing() const { return false; }

        // Writes the given Cord to the output.
        //
        // The default implementation iterates over all Cord chunks copying all cord
        // data into the buffer(s) returned by the stream's `Next()` method.
        //
        // Some streams may implement this in a way that avoids copying the cord
        // data by copying and managing a copy of the provided cord instead.
        [[nodiscard]] virtual bool write_cord(
            const turbo::Cord& cord);

    };


    // Internal helper class, for turning a ZeroCopyOutputStream into a sink.
    class KUMO_DLL ZeroCopyStreamByteSink {
    public:
        explicit ZeroCopyStreamByteSink(io::ZeroCopyOutputStream* stream)
            : stream_(stream) {}
        ZeroCopyStreamByteSink(const ZeroCopyStreamByteSink&) = delete;
        ZeroCopyStreamByteSink& operator=(const ZeroCopyStreamByteSink&) = delete;

        ~ZeroCopyStreamByteSink() {
            if (buffer_size_ > 0) {
                stream_->back_up(buffer_size_);
            }
        }

        void Append(const char* bytes, size_t len);
        void Write(std::string_view str) { Append(str.data(), str.size()); }

        [[nodiscard]] size_t bytes_written() {
            return bytes_written_;
        }
        [[nodiscard]] bool failed() { return failed_; }

    private:
        io::ZeroCopyOutputStream* stream_;
        void* buffer_ = nullptr;
        size_t buffer_size_ = 0;
        size_t bytes_written_ = 0;
        bool failed_ = false;
    };


}  // namespace shark::io
