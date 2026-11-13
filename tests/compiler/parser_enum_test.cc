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
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Enum-related tests split out of parser_unittest.cc. Fixtures live in
// parser_test_fixture.h.

#include <tests/compiler/parser_test_fixture.h>

namespace shark {
namespace compiler {

// ===================================================================
// Enum parsing.

typedef ParserTest ParseEnumTest;
typedef ParserTest ParseErrorTest;
typedef ParserTest ParserValidationErrorTest;

TEST_F(ParseEnumTest, EnumClassDefaultInt32) {
  ExpectParsesTo(
      "enum class TestEnum {\n"
      "  FOO = 1;\n"
      "  BAR = -2;\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"FOO\" number:1 }"
      "  value { name:\"BAR\" number:-2 }"
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

TEST_F(ParseEnumTest, EnumClassUint8) {
  ExpectParsesTo(
      "enum class Perm : uint8 {\n"
      "  option enable_operator = true;\n"
      "  READ = 1;\n"
      "  WRITE = 2;\n"
      "}\n",

      "enum_type {"
      "  name: \"Perm\""
      "  value { name:\"READ\" number:1 }"
      "  value { name:\"WRITE\" number:2 }"
      "  options {"
      "    underlying_type: ENUM_UNDERLYING_TYPE_UINT8"
      "    uninterpreted_option {"
      "      name { name_part: \"enable_operator\" is_extension: false }"
      "      identifier_value: \"true\""
      "    }"
      "  }"
      "}");
}

TEST_F(ParseEnumTest, EnumClassUint32HighBit) {
  ExpectParsesTo(
      "enum class Mask : uint32 {\n"
      "  TOP = 4294967295;\n"
      "}\n",

      "enum_type {"
      "  name: \"Mask\""
      "  value { name:\"TOP\" number:-1 }"
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_UINT32 }"
      "}");
}

TEST_F(ParseEnumTest, Proto3EnumClass) {
  ExpectHasEarlyExitErrors(
      "syntax = \"proto3\";\n"
      "enum class Color : uint16 {\n"
      "  COLOR_UNSPECIFIED = 0;\n"
      "  RED = 1;\n"
      "}\n",
      "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
      "only recognizes \"proto2\".\n");
}

TEST_F(ParseEnumTest, EnumWithUnderlyingType) {
  // Plain enums accept an underlying type, same as enum class.
  ExpectParsesTo(
      "enum Perm : uint8 {\n"
      "  option enable_operator = true;\n"
      "  READ = 1;\n"
      "  WRITE = 2;\n"
      "}\n",

      "enum_type {"
      "  name: \"Perm\""
      "  value { name:\"READ\" number:1 }"
      "  value { name:\"WRITE\" number:2 }"
      "  options {"
      "    underlying_type: ENUM_UNDERLYING_TYPE_UINT8"
      "    uninterpreted_option {"
      "      name { name_part: \"enable_operator\" is_extension: false }"
      "      identifier_value: \"true\""
      "    }"
      "  }"
      "}");
}

TEST_F(ParseEnumTest, EnumEnableOperator) {
  // enable_operator is no longer restricted to enum class.
  ExpectParsesTo(
      "enum Foo {\n"
      "  option enable_operator = true;\n"
      "  BAR = 1;\n"
      "}\n",

      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:1 }"
      "  options {"
      "    underlying_type: ENUM_UNDERLYING_TYPE_INT32"
      "    uninterpreted_option {"
      "      name { name_part: \"enable_operator\" is_extension: false }"
      "      identifier_value: \"true\""
      "    }"
      "  }"
      "}");
}

TEST_F(ParseEnumTest, EnumAndEnumClassEquivalent) {
  // "enum X" and "enum class X" produce byte-identical descriptors.
  FileDescriptorProto plain, enum_class;
  {
    SetupParser(
        "enum Perm {\n"
        "  READ = 1;\n"
        "  WRITE = 2;\n"
        "}\n");
    parser_->Parse(input_.get(), &plain);
    ASSERT_EQ("", error_collector_.text_);
  }
  {
    SetupParser(
        "enum class Perm {\n"
        "  READ = 1;\n"
        "  WRITE = 2;\n"
        "}\n");
    parser_->Parse(input_.get(), &enum_class);
    ASSERT_EQ("", error_collector_.text_);
  }
  plain.clear_source_code_info();
  enum_class.clear_source_code_info();
  EXPECT_EQ(plain.DebugString(), enum_class.DebugString());
}

TEST_F(ParseEnumTest, SimpleEnum) {
  ExpectParsesTo(
      "enum TestEnum {\n"
      "  FOO = 0;\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"FOO\" number:0 }"
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

TEST_F(ParseEnumTest, Values) {
  ExpectParsesTo(
      "enum TestEnum {\n"
      "  FOO = 13;\n"
      "  BAR = -10;\n"
      "  BAZ = 500;\n"
      "  HEX_MAX = 0x7FFFFFFF;\n"
      "  HEX_MIN = -0x80000000;\n"
      "  INT_MAX = 2147483647;\n"
      "  INT_MIN = -2147483648;\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"FOO\" number:13 }"
      "  value { name:\"BAR\" number:-10 }"
      "  value { name:\"BAZ\" number:500 }"
      "  value { name:\"HEX_MAX\" number:2147483647 }"
      "  value { name:\"HEX_MIN\" number:-2147483648 }"
      "  value { name:\"INT_MAX\" number:2147483647 }"
      "  value { name:\"INT_MIN\" number:-2147483648 }"
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

TEST_F(ParseEnumTest, ValueOptions) {
  ExpectParsesTo(
      "enum TestEnum {\n"
      "  FOO = 13;\n"
      "  BAR = -10 [ (something.text) = 'abc' ];\n"
      "  BAZ = 500 [ (something.text) = 'def', other = 1 ];\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name: \"FOO\" number: 13 }"
      "  value { name: \"BAR\" number: -10 "
      "    options { "
      "      uninterpreted_option { "
      "        name { name_part: \"something.text\" is_extension: true } "
      "        string_value: \"abc\" "
      "      } "
      "    } "
      "  } "
      "  value { name: \"BAZ\" number: 500 "
      "    options { "
      "      uninterpreted_option { "
      "        name { name_part: \"something.text\" is_extension: true } "
      "        string_value: \"def\" "
      "      } "
      "      uninterpreted_option { "
      "        name { name_part: \"other\" is_extension: false } "
      "        positive_int_value: 1 "
      "      } "
      "    } "
      "  } "
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

TEST_F(ParseEnumTest, ReservedRange) {
  ExpectParsesTo(
      "enum TestEnum {\n"
      "  FOO = 0;\n"
      "  reserved -2147483648, -6 to -4, -1 to 1, 2, 15, 9 to 11, 3, 20 to "
      "max;\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"FOO\" number:0 }"
      "  reserved_range { start:-2147483648  end:-2147483648 }"
      "  reserved_range { start:-6           end:-4          }"
      "  reserved_range { start:-1           end:1           }"
      "  reserved_range { start:2            end:2           }"
      "  reserved_range { start:15           end:15          }"
      "  reserved_range { start:9            end:11          }"
      "  reserved_range { start:3            end:3           }"
      "  reserved_range { start:20           end:2147483647  }"
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

TEST_F(ParseEnumTest, ReservedNames) {
  ExpectParsesTo(
      "enum TestEnum {\n"
      "  FOO = 0;\n"
      "  reserved \"foo\", \"bar\";\n"
      "}\n",

      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"FOO\" number:0 }"
      "  reserved_name: \"foo\""
      "  reserved_name: \"bar\""
      "  options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
      "}");
}

// ===================================================================
// Enum errors.

TEST_F(ParseErrorTest, EnumClassUnderlyingType) {
  ExpectHasErrors(
      "enum class Perm : string {\n"
      "  READ = 1;\n"
      "}\n",
      "0:25: Unknown enum underlying type \"string\".\n");
  ExpectHasErrors(
      "enum class Perm : uint8 {\n"
      "  READ = 256;\n"
      "}\n",
      "1:9: Integer out of range.\n");
  ExpectHasErrors(
      "enum class Perm : uint8 {\n"
      "  READ = -1;\n"
      "}\n",
      "1:10: Unsigned enum underlying type cannot be negative.\n");
}

TEST_F(ParseErrorTest, EnumValueOutOfRange) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "  HEX_TOO_BIG   =  0x80000000;\n"
      "  HEX_TOO_SMALL = -0x80000001;\n"
      "  INT_TOO_BIG   =  2147483648;\n"
      "  INT_TOO_SMALL = -2147483649;\n"
      "}\n",
      "1:19: Integer out of range.\n"
      "2:19: Integer out of range.\n"
      "3:19: Integer out of range.\n"
      "4:19: Integer out of range.\n");
}

TEST_F(ParseErrorTest, EnumAllowAliasFalse) {
  ExpectHasErrors(
      "enum Foo {\n"
      "  option allow_alias = false;\n"
      "  BAR = 1;\n"
      "  BAZ = 2;\n"
      "}\n",
      "5:0: \"Foo\" declares 'option allow_alias = false;' which has no "
      "effect. "
      "Please remove the declaration.\n");
}

TEST_F(ParseErrorTest, UnnecessaryEnumAllowAlias) {
  ExpectHasErrors(
      "enum Foo {\n"
      "  option allow_alias = true;\n"
      "  BAR = 1;\n"
      "  BAZ = 2;\n"
      "}\n",
      "5:0: \"Foo\" declares support for enum aliases but no enum values share "
      "field numbers. Please remove the unnecessary 'option allow_alias = "
      "true;' "
      "declaration.\n");
}

// -------------------------------------------------------------------
// Enum errors

TEST_F(ParseErrorTest, EofInEnum) {
  ExpectHasErrors(
      "enum TestEnum {",
      "0:15: Reached end of input in enum definition (missing '}').\n");
}

TEST_F(ParseErrorTest, EnumValueMissingNumber) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "  FOO;\n"
      "}\n",
      "1:5: Missing numeric value for enum constant.\n");
}

TEST_F(ParseErrorTest, EnumReservedStandaloneMaxNotAllowed) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "  FOO = 1;\n"
      "  reserved max;\n"
      "}\n",
      "2:11: Expected enum value or number range.\n");
}

TEST_F(ParseErrorTest, EnumReservedMixNameAndNumber) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "  FOO = 1;\n"
      "  reserved 10, \"foo\";\n"
      "}\n",
      "2:15: Expected enum number range.\n");
}

TEST_F(ParseErrorTest, EnumReservedPositiveNumberOutOfRange) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "FOO = 1;\n"
      "  reserved 2147483648;\n"
      "}\n",
      "2:11: Integer out of range.\n");
}

TEST_F(ParseErrorTest, EnumReservedNegativeNumberOutOfRange) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "FOO = 1;\n"
      "  reserved -2147483649;\n"
      "}\n",
      "2:12: Integer out of range.\n");
}

TEST_F(ParseErrorTest, EnumReservedMissingQuotes) {
  ExpectHasErrors(
      "enum TestEnum {\n"
      "  FOO = 1;\n"
      "  reserved foo;\n"
      "}\n",
      "2:11: Expected enum value or number range.\n");
}

// ===================================================================
// Enum validation errors.

TEST_F(ParserValidationErrorTest, EnumNameError) {
  ExpectHasValidationErrors(
      "enum Foo {A = 1;}\n"
      "enum Foo {B = 1;}\n",
      "1:5: \"Foo\" is already defined.\n");
}

TEST_F(ParserValidationErrorTest, Proto3EnumClassRequiresZero) {
  ExpectHasEarlyExitErrors(
      "syntax = 'proto3';\n"
      "enum class Foo : uint8 { A = 1; }\n",
      "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
      "only recognizes \"proto2\".\n");
}

TEST_F(ParserValidationErrorTest, Proto3EnumError) {
  ExpectHasEarlyExitErrors(
      "syntax = 'proto3';\n"
      "enum Foo {A = 1;}\n",
      "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
      "only recognizes \"proto2\".\n");
}

TEST_F(ParserValidationErrorTest, EnumValueNameError) {
  ExpectHasValidationErrors(
      "enum Foo {\n"
      "  BAR = 1;\n"
      "  BAR = 1;\n"
      "}\n",
      "2:2: \"BAR\" is already defined in \"Foo\".\n");
}

TEST_F(ParserValidationErrorTest, EnumValueAliasError) {
  ExpectHasValidationErrors(
      "enum Foo {\n"
      "  BAR = 1;\n"
      "  BAZ = 1;\n"
      "}\n",
      "2:8: \"Foo.BAZ\" uses the same enum value as \"Foo.BAR\". If this is "
      "intended, set 'option allow_alias = true;' to the enum "
      "definition.\n");
}

// ===================================================================
// Enum source-info tests.

TEST_F(SourceInfoTest, Enums) {
  EXPECT_TRUE(
      Parse("$a$enum $b$Foo$c$ {}$d$\n"
            "$e$enum $f$Bar$g$ {}$h$\n"));

  EXPECT_TRUE(HasSpan('a', 'd', file_.enum_type(0)));
  EXPECT_TRUE(HasSpan('b', 'c', file_.enum_type(0), "name"));
  EXPECT_TRUE(HasSpan('e', 'h', file_.enum_type(1)));
  EXPECT_TRUE(HasSpan('f', 'g', file_.enum_type(1), "name"));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
}

TEST_F(SourceInfoTest, EnumValues) {
  EXPECT_TRUE(
      Parse("enum Foo {\n"
            "  $a$BAR$b$ = $c$1$d$;$e$\n"
            "  $f$BAZ$g$ = $h$2$i$;$j$\n"
            "}"));

  const EnumValueDescriptorProto& bar = file_.enum_type(0).value(0);
  const EnumValueDescriptorProto& baz = file_.enum_type(0).value(1);

  EXPECT_TRUE(HasSpan('a', 'e', bar));
  EXPECT_TRUE(HasSpan('a', 'b', bar, "name"));
  EXPECT_TRUE(HasSpan('c', 'd', bar, "number"));
  EXPECT_TRUE(HasSpan('f', 'j', baz));
  EXPECT_TRUE(HasSpan('f', 'g', baz, "name"));
  EXPECT_TRUE(HasSpan('h', 'i', baz, "number"));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
  EXPECT_TRUE(HasSpan(file_.enum_type(0)));
  EXPECT_TRUE(HasSpan(file_.enum_type(0), "name"));
}

TEST_F(SourceInfoTest, EnumReservedRange) {
  EXPECT_TRUE(
      Parse("enum TestEnum {\n"
            "  $a$reserved $b$1$c$ to $d$10$e$;$f$\n"
            "}"));

  const EnumDescriptorProto::EnumReservedRange& bar =
      file_.enum_type(0).reserved_range(0);

  EXPECT_TRUE(HasSpan('a', 'f', file_.enum_type(0), "reserved_range"));
  EXPECT_TRUE(HasSpan('b', 'e', bar));
  EXPECT_TRUE(HasSpan('b', 'c', bar, "start"));
  EXPECT_TRUE(HasSpan('d', 'e', bar, "end"));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
  EXPECT_TRUE(HasSpan(file_.enum_type(0)));
  EXPECT_TRUE(HasSpan(file_.enum_type(0), "name"));
}

TEST_F(SourceInfoTest, EnumReservedName) {
  EXPECT_TRUE(
      Parse("enum TestEnum {\n"
            "  $a$reserved $b$'foo'$c$;$d$\n"
            "}"));

  const EnumDescriptorProto& bar = file_.enum_type(0);

  EXPECT_TRUE(HasSpan('a', 'd', bar, "reserved_name"));
  EXPECT_TRUE(HasSpan('b', 'c', bar, "reserved_name", 0));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
  EXPECT_TRUE(HasSpan(file_.enum_type(0)));
  EXPECT_TRUE(HasSpan(file_.enum_type(0), "name"));
}

TEST_F(SourceInfoTest, NestedEnums) {
  EXPECT_TRUE(
      Parse("message Foo {\n"
            "  $a$enum $b$Bar$c$ {}$d$\n"
            "  $e$enum $f$Baz$g$ {}$h$\n"
            "}\n"));

  const EnumDescriptorProto& bar = file_.message_type(0).enum_type(0);
  const EnumDescriptorProto& baz = file_.message_type(0).enum_type(1);

  EXPECT_TRUE(HasSpan('a', 'd', bar));
  EXPECT_TRUE(HasSpan('b', 'c', bar, "name"));
  EXPECT_TRUE(HasSpan('e', 'h', baz));
  EXPECT_TRUE(HasSpan('f', 'g', baz, "name"));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
  EXPECT_TRUE(HasSpan(file_.message_type(0)));
  EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
}

TEST_F(SourceInfoTest, EnumValueOptions) {
  // The actual "name = value" pairs are parsed by the same code as for
  // top-level options so we won't re-test that -- just make sure that the
  // syntax used for enum options is understood.
  EXPECT_TRUE(
      Parse("enum Foo {"
            "  BAR = 1 $a$[$b$opt1=123$c$,$d$opt2='hi'$e$]$f$;"
            "}\n"));

  const EnumValueDescriptorProto& value = file_.enum_type(0).value(0);
  const UninterpretedOption& option1 = value.options().uninterpreted_option(0);
  const UninterpretedOption& option2 = value.options().uninterpreted_option(1);

  EXPECT_TRUE(HasSpan('a', 'f', value.options()));
  EXPECT_TRUE(HasSpan('b', 'c', option1));
  EXPECT_TRUE(HasSpan('d', 'e', option2));

  // Ignore these.
  EXPECT_TRUE(HasSpan(file_));
  EXPECT_TRUE(HasSpan(file_.enum_type(0)));
  EXPECT_TRUE(HasSpan(file_.enum_type(0), "name"));
  EXPECT_TRUE(HasSpan(value));
  EXPECT_TRUE(HasSpan(value, "name"));
  EXPECT_TRUE(HasSpan(value, "number"));
  EXPECT_TRUE(HasSpan(option1, "name"));
  EXPECT_TRUE(HasSpan(option2, "name"));
  EXPECT_TRUE(HasSpan(option1.name(0)));
  EXPECT_TRUE(HasSpan(option2.name(0)));
  EXPECT_TRUE(HasSpan(option1.name(0), "name_part"));
  EXPECT_TRUE(HasSpan(option2.name(0), "name_part"));
  EXPECT_TRUE(HasSpan(option1, "positive_int_value"));
  EXPECT_TRUE(HasSpan(option2, "string_value"));
}

}  // namespace compiler
}  // namespace shark
