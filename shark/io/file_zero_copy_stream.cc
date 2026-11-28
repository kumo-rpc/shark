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

#include <shark/io/file_zero_copy_stream.h>
#include <turbo/log/logging.h>
#ifndef _MSC_VER
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <cerrno>

#include <algorithm>
#include <istream>
#include <ostream>

namespace shark::io {

#ifdef _WIN32
// Win32 lseek is broken:  If invoked on a non-seekable file descriptor, its
// return value is undefined.  We re-define it to always produce an error.
#define lseek(fd, offset, origin) ((off_t) - 1)
    // DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
    // them like we do below.
    using google::protobuf::io::win32::access;
    using google::protobuf::io::win32::close;
    using google::protobuf::io::win32::open;
    using google::protobuf::io::win32::read;
    using google::protobuf::io::win32::write;
#endif

    namespace {

        // If close(fd) returns an error, there is really nothing special to do --
        // except on *some* systems in case `errno == EINTR`. Unfortunately, that case
        // is also a huge mess and the question whether fd was closed in this case
        // depends on the system. Linux *did* close `fd` in this case. POSIX.1-2024
        // introduced posix_close (and did other changes) which helps fix this mess.
        // The POSIX.1-2024 documentation for unistd.h specifies that
        // POSIX_CLOSE_RESTART needs to be provided (and the value defines the behavior
        // of posix_close). We use it to decide if we can use posix_close.
        int robust_close(int fd) {
#if defined(POSIX_CLOSE_RESTART)
            return posix_close(fd, 0);
#else
            return close(fd);
#endif
        }

    } // namespace

    // ===================================================================

    FileInputStream::FileInputStream(int file_descriptor, int block_size)
        : copying_input_(file_descriptor)
        , impl_(&copying_input_, block_size) { }

    bool FileInputStream::Close() {
        return copying_input_.Close();
    }

    bool FileInputStream::next(const void** data, int* size) {
        return impl_.next(data, size);
    }

    void FileInputStream::back_up(int count) {
        impl_.back_up(count);
    }

    bool FileInputStream::skip(int count) {
        return impl_.skip(count);
    }

    int64_t FileInputStream::byte_count() const {
        return impl_.byte_count();
    }

    FileInputStream::CopyingFileInputStream::CopyingFileInputStream(
        int file_descriptor)
        : file_(file_descriptor)
        , close_on_delete_(false)
        , is_closed_(false)
        , errno_(0)
        , previous_seek_failed_(false) {
#ifndef _WIN32
        int flags = fcntl(file_, F_GETFL);
        flags &= ~O_NONBLOCK;
        fcntl(file_, F_SETFL, flags);
#endif
    }

    FileInputStream::CopyingFileInputStream::~CopyingFileInputStream() {
        if (close_on_delete_) {
            if (!Close()) {
                KLOG(ERROR) << "close() failed: " << strerror(errno_);
            }
        }
    }

    bool FileInputStream::CopyingFileInputStream::Close() {
        KCHECK(!is_closed_);

        is_closed_ = true;
        if (robust_close(file_) != 0) {
            errno_ = errno;
            return false;
        }

        return true;
    }

    int FileInputStream::CopyingFileInputStream::Read(void* buffer, int size) {
        KCHECK(!is_closed_);

        int result;
        do {
            result = read(file_, buffer, size);
        } while (result < 0 && errno == EINTR);

        if (result < 0) {
            // Read error (not EOF).
            errno_ = errno;
        }

        return result;
    }

    int FileInputStream::CopyingFileInputStream::skip(int count) {
        KCHECK(!is_closed_);

        if (!previous_seek_failed_ && lseek(file_, count, SEEK_CUR) != (off_t)-1) {
            // Seek succeeded.
            return count;
        } else {
            // Failed to seek.

            // Note to self:  Don't seek again.  This file descriptor doesn't
            // support it.
            previous_seek_failed_ = true;

            // Use the default implementation.
            return CopyingInputStream::skip(count);
        }
    }

    // ===================================================================

    FileOutputStream::FileOutputStream(int file_descriptor, int block_size)
        : CopyingOutputStreamAdaptor(&copying_output_, block_size)
        , copying_output_(file_descriptor) { }

    bool FileOutputStream::Close() {
        bool flush_succeeded = Flush();
        return copying_output_.Close() && flush_succeeded;
    }

    FileOutputStream::CopyingFileOutputStream::CopyingFileOutputStream(
        int file_descriptor)
        : file_(file_descriptor)
        , close_on_delete_(false)
        , is_closed_(false)
        , errno_(0) { }

    FileOutputStream::~FileOutputStream() {
        (void)Flush();
    }

    FileOutputStream::CopyingFileOutputStream::~CopyingFileOutputStream() {
        if (close_on_delete_) {
            if (!Close()) {
                KLOG(ERROR) << "close() failed: " << strerror(errno_);
            }
        }
    }

    bool FileOutputStream::CopyingFileOutputStream::Close() {
        KCHECK(!is_closed_);

        is_closed_ = true;
        if (robust_close(file_) != 0) {
            errno_ = errno;
            return false;
        }

        return true;
    }

    bool FileOutputStream::CopyingFileOutputStream::Write(const void* buffer,
        int size) {
        KCHECK(!is_closed_);
        int total_written = 0;

        const uint8_t* buffer_base = reinterpret_cast<const uint8_t*>(buffer);

        while (total_written < size) {
            int bytes;
            do {
                bytes = write(file_, buffer_base + total_written, size - total_written);
            } while (bytes < 0 && errno == EINTR);

            if (bytes <= 0) {
                // Write error.

                // FIXME(kenton):  According to the man page, if write() returns zero,
                //   there was no error; write() simply did not write anything.  It's
                //   unclear under what circumstances this might happen, but presumably
                //   errno won't be set in this case.  I am confused as to how such an
                //   event should be handled.  For now I'm treating it as an error, since
                //   retrying seems like it could lead to an infinite loop.  I suspect
                //   this never actually happens anyway.

                if (bytes < 0) {
                    errno_ = errno;
                }
                return false;
            }
            total_written += bytes;
        }

        return true;
    }

    // ===================================================================

    IstreamInputStream::IstreamInputStream(std::istream* input, int block_size)
        : copying_input_(input)
        , impl_(&copying_input_, block_size) { }

    bool IstreamInputStream::next(const void** data, int* size) {
        return impl_.next(data, size);
    }

    void IstreamInputStream::back_up(int count) {
        impl_.back_up(count);
    }

    bool IstreamInputStream::skip(int count) {
        return impl_.skip(count);
    }

    int64_t IstreamInputStream::byte_count() const {
        return impl_.byte_count();
    }

    IstreamInputStream::CopyingIstreamInputStream::CopyingIstreamInputStream(
        std::istream* input)
        : input_(input) { }

    IstreamInputStream::CopyingIstreamInputStream::~CopyingIstreamInputStream() = default;

    int IstreamInputStream::CopyingIstreamInputStream::Read(void* buffer,
        int size) {
        input_->read(reinterpret_cast<char*>(buffer), size);
        int result = input_->gcount();
        if (result == 0 && input_->fail() && !input_->eof()) {
            return -1;
        }
        return result;
    }

    // ===================================================================

    OstreamOutputStream::OstreamOutputStream(std::ostream* output, int block_size)
        : copying_output_(output)
        , impl_(&copying_output_, block_size) { }

    OstreamOutputStream::~OstreamOutputStream() {
        (void)impl_.Flush();
    }

    bool OstreamOutputStream::next(void** data, int* size) {
        return impl_.next(data, size);
    }

    void OstreamOutputStream::back_up(int count) {
        impl_.back_up(count);
    }

    int64_t OstreamOutputStream::byte_count() const {
        return impl_.byte_count();
    }

    OstreamOutputStream::CopyingOstreamOutputStream::CopyingOstreamOutputStream(
        std::ostream* output)
        : output_(output) { }

    OstreamOutputStream::CopyingOstreamOutputStream::~CopyingOstreamOutputStream() = default;

    bool OstreamOutputStream::CopyingOstreamOutputStream::Write(const void* buffer,
        int size) {
        output_->write(reinterpret_cast<const char*>(buffer), size);
        return output_->good();
    }

    // ===================================================================

    ConcatenatingInputStream::ConcatenatingInputStream(
        ZeroCopyInputStream* const streams[], int count)
        : streams_(streams)
        , stream_count_(count)
        , bytes_retired_(0) {
    }

    bool ConcatenatingInputStream::next(const void** data, int* size) {
        while (stream_count_ > 0) {
            if (streams_[0]->next(data, size))
                return true;

            // That stream is done.  Advance to the next one.
            bytes_retired_ += streams_[0]->byte_count();
            ++streams_;
            --stream_count_;
        }

        // No more streams.
        return false;
    }

    void ConcatenatingInputStream::back_up(int count) {
        if (stream_count_ > 0) {
            streams_[0]->back_up(count);
        } else {
            DKLOG(FATAL) << "Can't back_up() after failed next().";
        }
    }

    bool ConcatenatingInputStream::skip(int count) {
        while (stream_count_ > 0) {
            // Assume that byte_count() can be used to find out how much we actually
            // skipped when skip() fails.
            int64_t target_byte_count = streams_[0]->byte_count() + count;
            if (streams_[0]->skip(count))
                return true;

            // Hit the end of the stream.  Figure out how many more bytes we still have
            // to skip.
            int64_t final_byte_count = streams_[0]->byte_count();
            DKCHECK_LT(final_byte_count, target_byte_count);
            count = target_byte_count - final_byte_count;

            // That stream is done.  Advance to the next one.
            bytes_retired_ += final_byte_count;
            ++streams_;
            --stream_count_;
        }

        return false;
    }

    int64_t ConcatenatingInputStream::byte_count() const {
        if (stream_count_ == 0) {
            return bytes_retired_;
        } else {
            return bytes_retired_ + streams_[0]->byte_count();
        }
    }

} // namespace shark::io
