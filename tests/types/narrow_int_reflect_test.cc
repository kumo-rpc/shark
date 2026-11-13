#include <tests/sk/types/narrow_int.skp.h>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <shark/dynamic_message.h>
#include <shark/format/wired/wire_format.h>
#include <shark/io/coded_stream.h>
#include <shark/io/zero_copy_stream_impl.h>
#include <shark/format/text/text_format.h>
#include <shark/types/map_field.h>
#include <shark/util/message_differencer.h>

namespace shark {
namespace {

TEST(NarrowIntReflectTest, TextRoundTrip) {
  shark_unittest::NarrowInt message;
  message.set_i8(-128);
  message.set_u8(255);
  message.set_i16(-32768);
  message.set_u16(65535);

  std::string text;
  ASSERT_TRUE(TextFormat::PrintToString(message, &text));
  EXPECT_NE(text.find("i8: -128"), std::string::npos);
  EXPECT_NE(text.find("u8: 255"), std::string::npos);
  EXPECT_NE(text.find("i16: -32768"), std::string::npos);
  EXPECT_NE(text.find("u16: 65535"), std::string::npos);

  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(TextFormat::ParseFromString(text, &parsed));
  EXPECT_EQ(-128, parsed.i8());
  EXPECT_EQ(255, parsed.u8());
  EXPECT_EQ(-32768, parsed.i16());
  EXPECT_EQ(65535, parsed.u16());
}

TEST(NarrowIntReflectTest, TextFormatRepeatedRoundTrip) {
  shark_unittest::NarrowInt message;
  message.add_ri8(-1);
  message.add_ri8(127);
  message.add_ru16(0);
  message.add_ru16(65535);

  std::string text;
  ASSERT_TRUE(TextFormat::PrintToString(message, &text));
  EXPECT_NE(text.find("ri8: -1"), std::string::npos);
  EXPECT_NE(text.find("ri8: 127"), std::string::npos);
  EXPECT_NE(text.find("ru16: 0"), std::string::npos);
  EXPECT_NE(text.find("ru16: 65535"), std::string::npos);

  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(TextFormat::ParseFromString(text, &parsed));
  ASSERT_EQ(2, parsed.ri8_size());
  EXPECT_EQ(-1, parsed.ri8(0));
  EXPECT_EQ(127, parsed.ri8(1));
  ASSERT_EQ(2, parsed.ru16_size());
  EXPECT_EQ(0, parsed.ru16(0));
  EXPECT_EQ(65535, parsed.ru16(1));
}

TEST(NarrowIntReflectTest, TextFormatLowerBoundFails) {
  shark_unittest::NarrowInt parsed;
  EXPECT_FALSE(TextFormat::ParseFromString("i8: -129", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("i16: -32769", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("ri8: 128", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("ru16: 65536", &parsed));
}

TEST(NarrowIntReflectTest, TextFormatPrintsMapsInKeyOrder) {
  shark_unittest::NarrowMaps message;
  (*message.mutable_i8())[2] = 1;
  (*message.mutable_i8())[-5] = 2;
  (*message.mutable_u8())[200] = 1;
  (*message.mutable_u8())[1] = 2;
  (*message.mutable_i16())[300] = 1;
  (*message.mutable_i16())[-20] = 2;
  (*message.mutable_u16())[40000] = 1;
  (*message.mutable_u16())[3] = 2;

  std::string text;
  ASSERT_TRUE(TextFormat::PrintToString(message, &text));
  EXPECT_LT(text.find("key: -5\n"), text.find("key: 2\n"));
  EXPECT_LT(text.find("key: 1\n"), text.find("key: 200\n"));
  EXPECT_LT(text.find("key: -20\n"), text.find("key: 300\n"));
  EXPECT_LT(text.find("key: 3\n"), text.find("key: 40000\n"));

  shark_unittest::NarrowMaps parsed;
  ASSERT_TRUE(TextFormat::ParseFromString(text, &parsed));
  EXPECT_EQ(2, parsed.i8().at(-5));
  EXPECT_EQ(1, parsed.i8().at(2));
  EXPECT_EQ(2, parsed.u8().at(1));
  EXPECT_EQ(1, parsed.u8().at(200));
  EXPECT_EQ(2, parsed.i16().at(-20));
  EXPECT_EQ(1, parsed.i16().at(300));
  EXPECT_EQ(2, parsed.u16().at(3));
  EXPECT_EQ(1, parsed.u16().at(40000));
}

TEST(NarrowIntReflectTest, TextOutOfRangeFails) {
  shark_unittest::NarrowInt parsed;
  EXPECT_FALSE(TextFormat::ParseFromString("i8: 128", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("u8: 256", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("i16: 32768", &parsed));
  EXPECT_FALSE(TextFormat::ParseFromString("u16: 65536", &parsed));
}

TEST(NarrowIntReflectTest, ReflectionReadWrite) {
  shark_unittest::NarrowInt message;
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();
  const FieldDescriptor* i8 = descriptor->FindFieldByName("i8");
  const FieldDescriptor* u16 = descriptor->FindFieldByName("u16");
  reflection->SetInt32(&message, i8, -5);
  reflection->SetUInt32(&message, u16, 40000);
  EXPECT_EQ(-5, reflection->GetInt32(message, i8));
  EXPECT_EQ(40000u, reflection->GetUInt32(message, u16));
  EXPECT_EQ(-5, message.i8());
  EXPECT_EQ(40000, message.u16());
  EXPECT_TRUE(reflection->HasField(message, i8));
}

TEST(NarrowIntReflectTest, DynamicRoundTripAndByteSize) {
  shark_unittest::NarrowInt generated;
  generated.set_i8(-1);
  generated.set_u8(200);
  generated.set_i16(300);
  generated.set_u16(400);
  std::string bytes;
  ASSERT_TRUE(generated.SerializeToString(&bytes));
  EXPECT_EQ(bytes.size(),
            static_cast<size_t>(internal::WireFormat::ByteSize(generated)));

  DynamicMessageFactory factory;
  const Message* prototype =
      factory.GetPrototype(shark_unittest::NarrowInt::descriptor());
  std::unique_ptr<Message> dynamic(prototype->New());
  ASSERT_TRUE(dynamic->ParseFromString(bytes));
  std::string again;
  ASSERT_TRUE(dynamic->SerializeToString(&again));
  EXPECT_EQ(bytes, again);
}

TEST(NarrowIntReflectTest, MapValueSerializeMatchesGenerated) {
  shark_unittest::NarrowMaps generated;
  (*generated.mutable_vi8())[1] = -128;
  (*generated.mutable_vu8())[1] = 255;
  (*generated.mutable_vi16())[1] = -32768;
  (*generated.mutable_vu16())[1] = 65535;

  std::string generated_bytes;
  ASSERT_TRUE(generated.SerializeToString(&generated_bytes));

  std::string reflected_bytes;
  {
    io::StringOutputStream raw_output(&reflected_bytes);
    io::CodedOutputStream output(&raw_output);
    size_t size = internal::WireFormat::ByteSize(generated);
    internal::WireFormat::SerializeWithCachedSizes(generated, size, &output);
    ASSERT_FALSE(output.HadError());
  }
  EXPECT_EQ(generated_bytes, reflected_bytes);

  DynamicMessageFactory factory;
  std::unique_ptr<Message> dynamic(
      factory.GetPrototype(shark_unittest::NarrowMaps::descriptor())->New());
  ASSERT_TRUE(dynamic->ParseFromString(generated_bytes));
  std::string dynamic_bytes;
  ASSERT_TRUE(dynamic->SerializeToString(&dynamic_bytes));
  EXPECT_EQ(generated_bytes, dynamic_bytes);
}

TEST(NarrowIntReflectTest, ReflectionPackedMatchesGenerated) {
  shark_unittest::NarrowInt generated;
  const Reflection* reflection = generated.GetReflection();
  const Descriptor* descriptor = generated.GetDescriptor();
  const FieldDescriptor* ri8 = descriptor->FindFieldByName("ri8");
  const FieldDescriptor* ru16 = descriptor->FindFieldByName("ru16");
  reflection->AddInt32(&generated, ri8, -1);
  reflection->AddInt32(&generated, ri8, 127);
  reflection->AddUInt32(&generated, ru16, 0);
  reflection->AddUInt32(&generated, ru16, 65535);

  std::string bytes;
  ASSERT_TRUE(generated.SerializeToString(&bytes));
  EXPECT_EQ(bytes.size(),
            static_cast<size_t>(internal::WireFormat::ByteSize(generated)));

  DynamicMessageFactory factory;
  std::unique_ptr<Message> dynamic(
      factory.GetPrototype(descriptor)->New());
  ASSERT_TRUE(dynamic->ParseFromString(bytes));
  std::string again;
  ASSERT_TRUE(dynamic->SerializeToString(&again));
  EXPECT_EQ(bytes, again);
}

TEST(NarrowIntReflectTest, CopyMergeSwapAndDifferencer) {
  shark_unittest::NarrowInt original;
  original.set_i8(-5);
  original.add_ri8(1);
  original.add_ri8(2);

  shark_unittest::NarrowInt copied;
  copied.CopyFrom(original);
  EXPECT_TRUE(util::MessageDifferencer::Equals(original, copied));

  shark_unittest::NarrowInt merged;
  merged.MergeFrom(original);
  EXPECT_EQ(-5, merged.i8());
  EXPECT_EQ(2, merged.ri8_size());

  shark_unittest::NarrowInt other;
  other.set_i8(9);
  original.Swap(&other);
  EXPECT_EQ(9, original.i8());
  EXPECT_EQ(-5, other.i8());
  EXPECT_FALSE(util::MessageDifferencer::Equals(original, other));
}

TEST(NarrowIntReflectTest, ExtensionRoundTripAndOutOfWidth) {
  shark_unittest::NarrowInt message;
  const Reflection* reflection = message.GetReflection();
  const Descriptor* descriptor = message.GetDescriptor();
  const FieldDescriptor* ei8 =
      descriptor->file()->FindExtensionByName("ei8");
  const FieldDescriptor* eri8 =
      descriptor->file()->FindExtensionByName("eri8");
  ASSERT_NE(nullptr, ei8);
  ASSERT_NE(nullptr, eri8);

  reflection->SetInt32(&message, ei8, -4);
  reflection->AddInt32(&message, eri8, -1);
  reflection->AddInt32(&message, eri8, 7);
  EXPECT_EQ(-4, reflection->GetInt32(message, ei8));
  EXPECT_EQ(2, reflection->FieldSize(message, eri8));

  std::string bytes;
  ASSERT_TRUE(message.SerializeToString(&bytes));
  shark_unittest::NarrowInt parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_EQ(-4, parsed.GetReflection()->GetInt32(parsed, ei8));
  EXPECT_EQ(-1, parsed.GetReflection()->GetRepeatedInt32(parsed, eri8, 0));
  EXPECT_EQ(7, parsed.GetReflection()->GetRepeatedInt32(parsed, eri8, 1));

  reflection->SetInt32(&message, ei8, 128);
  EXPECT_EQ(-4, reflection->GetInt32(message, ei8));
}

TEST(NarrowIntReflectTest, DynamicMapSortsNarrowKeys) {
  shark_unittest::NarrowKeyList list;
  list.add_i8()->set_key(2);
  list.add_i8()->set_key(-5);
  list.add_u8()->set_key(200);
  list.add_u8()->set_key(1);
  list.add_i16()->set_key(300);
  list.add_i16()->set_key(-20);
  list.add_u16()->set_key(40000);
  list.add_u16()->set_key(3);

  const Reflection* reflection = list.GetReflection();
  const Descriptor* descriptor = list.GetDescriptor();
  auto sort_keys = [&](const char* name) {
    const FieldDescriptor* field = descriptor->FindFieldByName(name);
    std::vector<const Message*> sorted = DynamicMapSorter::Sort(
        list, reflection->FieldSize(list, field), reflection, field);
    std::vector<int32_t> keys;
    const FieldDescriptor* key =
        field->message_type()->FindFieldByName("key");
    for (const Message* entry : sorted) {
      if (key->cpp_type() == SharkFieldCppType::CPPTYPE_UINT8 ||
          key->cpp_type() == SharkFieldCppType::CPPTYPE_UINT16) {
        keys.push_back(
            static_cast<int32_t>(entry->GetReflection()->GetUInt32(*entry, key)));
      } else {
        keys.push_back(entry->GetReflection()->GetInt32(*entry, key));
      }
    }
    return keys;
  };

  EXPECT_EQ(std::vector<int32_t>({-5, 2}), sort_keys("i8"));
  EXPECT_EQ(std::vector<int32_t>({1, 200}), sort_keys("u8"));
  EXPECT_EQ(std::vector<int32_t>({-20, 300}), sort_keys("i16"));
  EXPECT_EQ(std::vector<int32_t>({3, 40000}), sort_keys("u16"));
}

TEST(NarrowIntReflectTest, MapKeyKeepsNarrowType) {
  MapKey key;
  key.SetInt8Value(-5);
  EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT8, key.type());
  EXPECT_EQ(-5, key.GetInt32Value());

  MapKey copy;
  copy.CopyFrom(key);
  EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT8, copy.type());
  EXPECT_TRUE(key == copy);

  key.SetUInt16Value(40000);
  EXPECT_EQ(SharkFieldCppType::CPPTYPE_UINT16, key.type());
  EXPECT_EQ(40000u, key.GetUInt32Value());
}

}  // namespace
}  // namespace shark
