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

// Author: kenton@google.com (Kenton Varda)

#include <shark/stubs/strutil.h>
#include <turbo/strings/str_cat.h>

#include <locale.h>

#include <shark/stubs/stl_util.h>
#include <tests/common/googletest.h>
#include <gtest/gtest.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif

namespace shark {
namespace {

// TODO(kenton):  Copy strutil tests from google3?

TEST(StringUtilityTest, ImmuneToLocales) {
  // Remember the old locale.
  char* old_locale_cstr = setlocale(LC_NUMERIC, nullptr);
  ASSERT_TRUE(old_locale_cstr != nullptr);
  std::string old_locale = old_locale_cstr;

  // Set the locale to "C".
  ASSERT_TRUE(setlocale(LC_NUMERIC, "C") != nullptr);

  EXPECT_EQ("1.5", turbo::str_cat(turbo::HighPrecision(static_cast<double>(1.5))));
  EXPECT_EQ("1.5", turbo::str_cat(turbo::HighPrecision(static_cast<float>(1.5))));

  if (setlocale(LC_NUMERIC, "es_ES") == nullptr &&
      setlocale(LC_NUMERIC, "es_ES.utf8") == nullptr) {
    // Some systems may not have the desired locale available.
    GOOGLE_LOG(WARNING)
      << "Couldn't set locale to es_ES.  Skipping this test.";
  } else {
    EXPECT_EQ("1.5", turbo::str_cat(turbo::HighPrecision(static_cast<double>(1.5))));
    EXPECT_EQ("1.5", turbo::str_cat(turbo::HighPrecision(static_cast<float>(1.5))));
  }

  // Return to original locale.
  setlocale(LC_NUMERIC, old_locale.c_str());
}

// Test StrCat of ints and longs of various sizes and signdedness.
TEST(StrCat, Ints) {
  const short s = -1;  // NOLINT(runtime/int)
  const uint16_t us = 2;
  const int i = -3;
  const unsigned int ui = 4;
  const long l = -5;                 // NOLINT(runtime/int)
  const unsigned long ul = 6;        // NOLINT(runtime/int)
  const long long ll = -7;           // NOLINT(runtime/int)
  const unsigned long long ull = 8;  // NOLINT(runtime/int)
  const ptrdiff_t ptrdiff = -9;
  const size_t size = 10;
  const intptr_t intptr = -12;
  const uintptr_t uintptr = 13;
  std::string answer;
  answer = turbo::str_cat(s, us);
  EXPECT_EQ(answer, "-12");
  answer = turbo::str_cat(i, ui);
  EXPECT_EQ(answer, "-34");
  answer = turbo::str_cat(l, ul);
  EXPECT_EQ(answer, "-56");
  answer = turbo::str_cat(ll, ull);
  EXPECT_EQ(answer, "-78");
  answer = turbo::str_cat(ptrdiff, size);
  EXPECT_EQ(answer, "-910");
  answer = turbo::str_cat(ptrdiff, intptr);
  EXPECT_EQ(answer, "-9-12");
  answer = turbo::str_cat(uintptr, 0);
  EXPECT_EQ(answer, "130");
}

}  // anonymous namespace
}  // namespace shark
