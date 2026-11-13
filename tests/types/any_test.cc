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

#include <tests/sk/types/any_test.skp.h>
#include <tests/sk/unittest.skp.h>
#include <gtest/gtest.h>


// Must be included last.
#include <shark/port_def.inc>

namespace shark {
namespace {

TEST(AnyMetadataTest, ConstInit) {
  SHARK_CONSTINIT static internal::AnyMetadata metadata(nullptr, nullptr);
  (void)metadata;
}

TEST(AnyTest, TestPackAndUnpack) {
  sharkbuf_unittest::TestAny submessage;
  submessage.set_int32_value(12345);
  sharkbuf_unittest::TestAny message;
  ASSERT_TRUE(message.mutable_any_value()->PackFrom(submessage));

  std::string data = message.SerializeAsString();

  ASSERT_TRUE(message.ParseFromString(data));
  EXPECT_TRUE(message.has_any_value());
  submessage.Clear();
  ASSERT_TRUE(message.any_value().UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestPackFromSerializationExceedsSizeLimit) {
  sharkbuf_unittest::TestAny submessage;
  submessage.mutable_text()->resize(INT_MAX, 'a');
  sharkbuf_unittest::TestAny message;
  EXPECT_FALSE(message.mutable_any_value()->PackFrom(submessage));
}

TEST(AnyTest, TestUnpackWithTypeMismatch) {
  sharkbuf_unittest::TestAny payload;
  payload.set_int32_value(13);
  shark::Any any;
  any.PackFrom(payload);

  // Attempt to unpack into the wrong type.
  sharkbuf_unittest::TestAllTypes dest;
  EXPECT_FALSE(any.UnpackTo(&dest));
}

TEST(AnyTest, TestPackAndUnpackAny) {
  // We can pack a Any message inside another Any message.
  sharkbuf_unittest::TestAny submessage;
  submessage.set_int32_value(12345);
  shark::Any any;
  any.PackFrom(submessage);
  sharkbuf_unittest::TestAny message;
  message.mutable_any_value()->PackFrom(any);

  std::string data = message.SerializeAsString();

  ASSERT_TRUE(message.ParseFromString(data));
  EXPECT_TRUE(message.has_any_value());
  any.Clear();
  submessage.Clear();
  ASSERT_TRUE(message.any_value().UnpackTo(&any));
  ASSERT_TRUE(any.UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestPackWithCustomTypeUrl) {
  sharkbuf_unittest::TestAny submessage;
  submessage.set_int32_value(12345);
  shark::Any any;
  // Pack with a custom type URL prefix.
  any.PackFrom(submessage, "type.myservice.com");
  EXPECT_EQ("type.myservice.com/sharkbuf_unittest.TestAny", any.type_url());
  // Pack with a custom type URL prefix ending with '/'.
  any.PackFrom(submessage, "type.myservice.com/");
  EXPECT_EQ("type.myservice.com/sharkbuf_unittest.TestAny", any.type_url());
  // Pack with an empty type URL prefix.
  any.PackFrom(submessage, "");
  EXPECT_EQ("/sharkbuf_unittest.TestAny", any.type_url());

  // Test unpacking the type.
  submessage.Clear();
  EXPECT_TRUE(any.UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestIs) {
  sharkbuf_unittest::TestAny submessage;
  submessage.set_int32_value(12345);
  shark::Any any;
  any.PackFrom(submessage);
  ASSERT_TRUE(any.ParseFromString(any.SerializeAsString()));
  EXPECT_TRUE(any.Is<sharkbuf_unittest::TestAny>());
  EXPECT_FALSE(any.Is<shark::Any>());

  sharkbuf_unittest::TestAny message;
  message.mutable_any_value()->PackFrom(any);
  ASSERT_TRUE(message.ParseFromString(message.SerializeAsString()));
  EXPECT_FALSE(message.any_value().Is<sharkbuf_unittest::TestAny>());
  EXPECT_TRUE(message.any_value().Is<shark::Any>());

  any.set_type_url("/sharkbuf_unittest.TestAny");
  EXPECT_TRUE(any.Is<sharkbuf_unittest::TestAny>());
  // The type URL must contain at least one "/".
  any.set_type_url("sharkbuf_unittest.TestAny");
  EXPECT_FALSE(any.Is<sharkbuf_unittest::TestAny>());
  // The type name after the slash must be fully qualified.
  any.set_type_url("/TestAny");
  EXPECT_FALSE(any.Is<sharkbuf_unittest::TestAny>());
}

TEST(AnyTest, MoveConstructor) {
  sharkbuf_unittest::TestAny payload;
  payload.set_int32_value(12345);

  shark::Any src;
  src.PackFrom(payload);

  const char* type_url = src.type_url().data();

  shark::Any dst(std::move(src));
  EXPECT_EQ(type_url, dst.type_url().data());
  payload.Clear();
  ASSERT_TRUE(dst.UnpackTo(&payload));
  EXPECT_EQ(12345, payload.int32_value());
}

TEST(AnyTest, MoveAssignment) {
  sharkbuf_unittest::TestAny payload;
  payload.set_int32_value(12345);

  shark::Any src;
  src.PackFrom(payload);

  const char* type_url = src.type_url().data();

  shark::Any dst;
  dst = std::move(src);
  EXPECT_EQ(type_url, dst.type_url().data());
  payload.Clear();
  ASSERT_TRUE(dst.UnpackTo(&payload));
  EXPECT_EQ(12345, payload.int32_value());
}

#ifdef SHARK_HAS_DEATH_TEST
#ifndef NDEBUG
TEST(AnyTest, PackSelfDeath) {
  shark::Any any;
  EXPECT_DEATH(any.PackFrom(any), "&message");
  EXPECT_DEATH(any.PackFrom(any, ""), "&message");
}
#endif  // !NDEBUG
#endif  // SHARK_HAS_DEATH_TEST


}  // namespace
}  // namespace shark

#include <shark/port_undef.inc>
