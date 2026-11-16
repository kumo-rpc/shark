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

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <iosfwd>
#include <limits>
#include <string>

#if defined(__cpp_lib_string_view)
#include <string_view>
#endif

#include <shark/stubs/hash.h>

#include <shark/port_def.inc>

namespace shark {
namespace stringpiece_internal {

class SHARK_EXPORT StringPiece {
 public:
  using traits_type = std::char_traits<char>;
  using value_type = char;
  using pointer = char*;
  using const_pointer = const char*;
  using reference = char&;
  using const_reference = const char&;
  using const_iterator = const char*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

 private:
  const char* ptr_;
  size_type length_;

  static constexpr size_type kMaxSize =
      (std::numeric_limits<difference_type>::max)();

  static size_type CheckSize(size_type size) {
#if !defined(NDEBUG) || defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE > 0
    if (SHARK_PREDICT_FALSE(size > kMaxSize)) {
      // Some people grep for this message in logs
      // so take care if you ever change it.
      LogFatalSizeTooBig(size, "string length exceeds max size");
    }
#endif
    return size;
  }

  // Out-of-line error path.
  static void LogFatalSizeTooBig(size_type size, const char* details);

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  //
  // Style guide exception granted:
  // http://goto/style-guide-exception-20978288
  StringPiece() : ptr_(nullptr), length_(0) {}

  StringPiece(const char* str)  // NOLINT(runtime/explicit)
      : ptr_(str), length_(0) {
    if (str != nullptr) {
      length_ = CheckSize(strlen(str));
    }
  }

  template <class Allocator>
  StringPiece(  // NOLINT(runtime/explicit)
      const std::basic_string<char, std::char_traits<char>, Allocator>& str)
      : ptr_(str.data()), length_(0) {
    length_ = CheckSize(str.size());
  }

#if defined(__cpp_lib_string_view)
  StringPiece(  // NOLINT(runtime/explicit)
      std::string_view str)
      : ptr_(str.data()), length_(0) {
    length_ = CheckSize(str.size());
  }
#endif

  StringPiece(const char* offset, size_type len)
      : ptr_(offset), length_(CheckSize(len)) {}

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const_pointer data() const { return ptr_; }
  size_type size() const { return length_; }
  size_type length() const { return length_; }
  bool empty() const { return length_ == 0; }

  char operator[](size_type i) const {
    assert(i < length_);
    return ptr_[i];
  }

    operator std::string_view() const {
      return std::string_view{data(),size()};
  }
  void remove_prefix(size_type n) {
    assert(length_ >= n);
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_type n) {
    assert(length_ >= n);
    length_ -= n;
  }

  // returns {-1, 0, 1}
  int compare(StringPiece x) const {
    size_type min_size = length_ < x.length_ ? length_ : x.length_;
    int r = memcmp(ptr_, x.ptr_, static_cast<size_t>(min_size));
    if (r < 0) return -1;
    if (r > 0) return 1;
    if (length_ < x.length_) return -1;
    if (length_ > x.length_) return 1;
    return 0;
  }

  std::string as_string() const { return ToString(); }
  // We also define ToString() here, since many other string-like
  // interfaces name the routine that converts to a C++ string
  // "ToString", and it's confusing to have the method that does that
  // for a StringPiece be called "as_string()".  We also leave the
  // "as_string()" method defined here for existing code.
  std::string ToString() const {
    if (ptr_ == nullptr) return "";
    return std::string(data(), static_cast<size_type>(size()));
  }

  explicit operator std::string() const { return ToString(); }

  void CopyToString(std::string* target) const;
  void AppendToString(std::string* target) const;

  bool starts_with(StringPiece x) const {
    return (length_ >= x.length_) &&
           (memcmp(ptr_, x.ptr_, static_cast<size_t>(x.length_)) == 0);
  }

  bool ends_with(StringPiece x) const {
    return ((length_ >= x.length_) &&
            (memcmp(ptr_ + (length_-x.length_), x.ptr_,
                 static_cast<size_t>(x.length_)) == 0));
  }

  // Checks whether StringPiece starts with x and if so advances the beginning
  // of it to past the match.  It's basically a shortcut for starts_with
  // followed by remove_prefix.
  bool Consume(StringPiece x);
  // Like above but for the end of the string.
  bool ConsumeFromEnd(StringPiece x);

  // standard STL container boilerplate
  static const size_type npos;
  const_iterator begin() const { return ptr_; }
  const_iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }
  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  // cpplint.py emits a false positive [build/include_what_you_use]
  size_type copy(char* buf, size_type n, size_type pos = 0) const;  // NOLINT

  bool contains(StringPiece s) const;

  size_type find(StringPiece s, size_type pos = 0) const;
  size_type find(char c, size_type pos = 0) const;
  size_type rfind(StringPiece s, size_type pos = npos) const;
  size_type rfind(char c, size_type pos = npos) const;

  size_type find_first_of(StringPiece s, size_type pos = 0) const;
  size_type find_first_of(char c, size_type pos = 0) const {
    return find(c, pos);
  }
  size_type find_first_not_of(StringPiece s, size_type pos = 0) const;
  size_type find_first_not_of(char c, size_type pos = 0) const;
  size_type find_last_of(StringPiece s, size_type pos = npos) const;
  size_type find_last_of(char c, size_type pos = npos) const {
    return rfind(c, pos);
  }
  size_type find_last_not_of(StringPiece s, size_type pos = npos) const;
  size_type find_last_not_of(char c, size_type pos = npos) const;

  StringPiece substr(size_type pos, size_type n = npos) const;
};

// This large function is defined inline so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
inline bool operator==(StringPiece x, StringPiece y) {
  StringPiece::size_type len = x.size();
  if (len != y.size()) {
    return false;
  }

  return x.data() == y.data() || len <= 0 ||
      memcmp(x.data(), y.data(), static_cast<size_t>(len)) == 0;
}

inline bool operator!=(StringPiece x, StringPiece y) {
  return !(x == y);
}

inline bool operator<(StringPiece x, StringPiece y) {
  const StringPiece::size_type min_size =
      x.size() < y.size() ? x.size() : y.size();
  const int r = memcmp(x.data(), y.data(), static_cast<size_t>(min_size));
  return (r < 0) || (r == 0 && x.size() < y.size());
}

inline bool operator>(StringPiece x, StringPiece y) {
  return y < x;
}

inline bool operator<=(StringPiece x, StringPiece y) {
  return !(x > y);
}

inline bool operator>=(StringPiece x, StringPiece y) {
  return !(x < y);
}

// allow StringPiece to be logged
extern std::ostream& operator<<(std::ostream& o, StringPiece piece);

}  // namespace stringpiece_internal

using ::shark::stringpiece_internal::StringPiece;

}  // namespace shark

SHARK_HASH_NAMESPACE_DECLARATION_START
template<> struct hash<StringPiece> {
  size_t operator()(const StringPiece& s) const {
    size_t result = 0;
    for (const char *str = s.data(), *end = str + s.size(); str < end; str++) {
      result = 5 * result + static_cast<size_t>(*str);
    }
    return result;
  }
};
SHARK_HASH_NAMESPACE_DECLARATION_END

#include <shark/port_undef.inc>
