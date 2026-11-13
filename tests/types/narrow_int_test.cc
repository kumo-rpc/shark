#include <tests/sk/types/narrow_int.skp.h>

#include <string>

#include <gtest/gtest.h>

namespace shark {
namespace {

TEST(NarrowIntTest, RoundTripEdges) {
  shark_unittest::NarrowInt message;
  message.set_i8(-128);
  message.set_u8(255);
  message.set_i16(-32768);
  message.set_u16(65535);
  message.add_ri8(-1);
  message.add_ri8(127);
  message.add_ru16(0);
  message.add_ru16(65535);

  std::string bytes;
  ASSERT_TRUE(message.SerializeToString(&bytes));

  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_EQ(-128, parsed.i8());
  EXPECT_EQ(255, parsed.u8());
  EXPECT_EQ(-32768, parsed.i16());
  EXPECT_EQ(65535, parsed.u16());
  ASSERT_EQ(2, parsed.ri8_size());
  EXPECT_EQ(-1, parsed.ri8(0));
  EXPECT_EQ(127, parsed.ri8(1));
  ASSERT_EQ(2, parsed.ru16_size());
  EXPECT_EQ(0, parsed.ru16(0));
  EXPECT_EQ(65535, parsed.ru16(1));
}

TEST(NarrowIntTest, OutOfWidthGoesToUnknown) {
  // Field 1, wire type varint, value 200. 200 does not fit in int8.
  const std::string bytes("\x08\xC8\x01", 3);
  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_FALSE(parsed.has_i8());
  ASSERT_EQ(1, parsed.unknown_fields().field_count());
  EXPECT_EQ(1, parsed.unknown_fields().field(0).number());
  EXPECT_EQ(200u, parsed.unknown_fields().field(0).varint());
}

TEST(NarrowIntTest, PackedOutOfWidthGoesToUnknown) {
  // Packed field 5: length 3, values 1 and 200. Only 1 is stored.
  const std::string bytes("\x2a\x03\x01\xc8\x01", 5);
  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  ASSERT_EQ(1, parsed.ri8_size());
  EXPECT_EQ(1, parsed.ri8(0));
  ASSERT_EQ(1, parsed.unknown_fields().field_count());
  EXPECT_EQ(5, parsed.unknown_fields().field(0).number());
  EXPECT_EQ(200u, parsed.unknown_fields().field(0).varint());
}

TEST(NarrowIntTest, NegativeInt8UsesSignExtendedVarint) {
  shark_unittest::NarrowInt message;
  message.set_i8(-1);
  std::string bytes;
  ASSERT_TRUE(message.SerializeToString(&bytes));
  // Tag plus the 10-byte sign-extended varint used by int32.
  EXPECT_EQ(11u, bytes.size());

  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_EQ(-1, parsed.i8());
}

}  // namespace
}  // namespace shark
