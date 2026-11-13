#include <shark/descriptor.h>
#include <shark/format/wired/wire_format_lite.h>
#include <shark/io/coded_stream.h>
#include <shark/io/zero_copy_stream_impl_lite.h>

#include <tests/sk/unittest.skp.h>

#include <gtest/gtest.h>

namespace shark {
    namespace {

        using internal::WireFormatLite;

        TEST(FixedTagTest, FieldOneVarintBytes) {
            EXPECT_EQ(0x0fffffffu, static_cast<uint32_t>(FieldDescriptor::kMaxNumber));
            const uint32_t tag = WireFormatLite::MakeTag(1, WireFormatLite::WIRETYPE_VARINT);
            EXPECT_EQ(0x11u, tag);

            uint8_t bytes[8] = { };
            uint8_t* end = WireFormatLite::WriteInt32ToArray(1, 150, bytes);
            ASSERT_EQ(6, end - bytes);
            EXPECT_EQ(0x11, bytes[0]);
            EXPECT_EQ(0x00, bytes[1]);
            EXPECT_EQ(0x00, bytes[2]);
            EXPECT_EQ(0x00, bytes[3]);
            EXPECT_EQ(0x96, bytes[4]);
            EXPECT_EQ(0x01, bytes[5]);
        }

        TEST(FixedTagTest, FilledZeroConsumesOnlyTag) {
            const uint8_t bytes[] = { 0x20, 0x00, 0x00, 0x00, 0x7f };
            io::ArrayInputStream raw(bytes, 5);
            io::CodedInputStream input(&raw);
            uint32_t tag = input.ReadTag();
            EXPECT_EQ(0x20u, tag);
            EXPECT_EQ(2, WireFormatLite::GetTagFieldNumber(tag));
            EXPECT_FALSE(tag & 1u);
            EXPECT_TRUE(WireFormatLite::SkipField(&input, tag));
            EXPECT_EQ(4, input.CurrentPosition());
        }

        TEST(FixedTagTest, PresenceRoundTrip) {
            sharkbuf_unittest::TestAllTypes set_zero;
            set_zero.set_optional_int32(0);
            set_zero.set_optional_bool(false);
            set_zero.set_optional_string("");
            std::string bytes = set_zero.SerializeAsString();
            EXPECT_FALSE(bytes.empty());

            sharkbuf_unittest::TestAllTypes parsed;
            ASSERT_TRUE(parsed.ParseFromString(bytes));
            EXPECT_TRUE(parsed.has_optional_int32());
            EXPECT_EQ(0, parsed.optional_int32());
            EXPECT_TRUE(parsed.has_optional_bool());
            EXPECT_FALSE(parsed.optional_bool());
            EXPECT_TRUE(parsed.has_optional_string());
            EXPECT_EQ("", parsed.optional_string());

            sharkbuf_unittest::TestAllTypes unset;
            EXPECT_TRUE(unset.SerializeAsString().empty());
        }

        TEST(FixedTagTest, TruncatedTagFails) {
            const uint8_t bytes[] = { 0x11, 0x00 };
            sharkbuf_unittest::TestAllTypes message;
            EXPECT_FALSE(message.ParseFromArray(bytes, 2));
        }

        TEST(FixedTagTest, ReservedWireTypeFails) {
            const uint8_t bytes[] = { 0x1d, 0x00, 0x00, 0x00 };
            sharkbuf_unittest::TestAllTypes message;
            EXPECT_FALSE(message.ParseFromArray(bytes, 4));
        }

    } // namespace
} // namespace shark
