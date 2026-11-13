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

#include <shark/parser/parser.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <shark/types/any.skp.h>
#include <tests/compiler/parser_test_fixture.h>
#include <tests/common/test_util2.h>
#include <tests/common/googletest.h>
#include <tests/sk/unittest.skp.h>
#include <tests/sk/lib/unittest_custom_options.skp.h>

namespace shark {
    namespace compiler {

        namespace {

            TEST_F(ParserTest, StopAfterSyntaxIdentifier) {
                SetupParser(
                    "// blah\n"
                    "syntax = \"foobar\";\n"
                    "this line will not be parsed\n");
                parser_->SetStopAfterSyntaxIdentifier(true);
                EXPECT_FALSE(parser_->Parse(input_.get(), nullptr));
                EXPECT_EQ(
                    "1:9: Unrecognized syntax identifier \"foobar\".  This parser "
                    "only recognizes \"proto2\".\n",
                    error_collector_.text_);
                EXPECT_EQ("foobar", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParserTest, StopAfterOmittedSyntaxIdentifier) {
                supply_syntax_ = false;
                SetupParser(
                    "// blah\n"
                    "this line will not be parsed\n");
                parser_->SetStopAfterSyntaxIdentifier(true);
                EXPECT_FALSE(parser_->Parse(input_.get(), nullptr));
                EXPECT_EQ(
                    "1:0: File must begin with a syntax statement, e.g. 'syntax = "
                    "\"proto2\";'.\n",
                    error_collector_.text_);
                EXPECT_EQ("", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParserTest, StopAfterSyntaxIdentifierWithErrors) {
                SetupParser(
                    "// blah\n"
                    "syntax = error;\n");
                parser_->SetStopAfterSyntaxIdentifier(true);
                EXPECT_FALSE(parser_->Parse(input_.get(), nullptr));
                EXPECT_EQ("1:9: Expected syntax identifier.\n", error_collector_.text_);
            }

            TEST_F(ParserTest, ErrorIfSyntaxIdentifierOmitted) {
                supply_syntax_ = false;
                SetupParser("message A {}");
                FileDescriptorProto file;
                EXPECT_FALSE(parser_->Parse(input_.get(), &file));
                EXPECT_EQ(
                    "0:0: File must begin with a syntax statement, e.g. 'syntax = "
                    "\"proto2\";'.\n",
                    error_collector_.text_);
            }

            TEST_F(ParserTest, WarnIfFieldNameIsNotUpperCamel) {
                SetupParser(
                    "syntax = \"proto2\";"
                    "message abc {}");
                FileDescriptorProto file;
                EXPECT_TRUE(parser_->Parse(input_.get(), &file));
                EXPECT_TRUE(error_collector_.warning_.find(
                                "Message name should be in UpperCamelCase. Found: abc.")
                    != std::string::npos);
            }

            TEST_F(ParserTest, WarnIfFieldNameIsNotLowerUnderscore) {
                SetupParser(
                    "syntax = \"proto2\";"
                    "message A {"
                    "  optional string SongName = 1;"
                    "}");
                FileDescriptorProto file;
                EXPECT_TRUE(parser_->Parse(input_.get(), &file));
                EXPECT_TRUE(error_collector_.warning_.find(
                                "Field name should be lowercase. Found: SongName")
                    != std::string::npos);
            }

            TEST_F(ParserTest, WarnIfFieldNameContainsNumberImmediatelyFollowUnderscore) {
                SetupParser(
                    "syntax = \"proto2\";"
                    "message A {"
                    "  optional string song_name_1 = 1;"
                    "}");
                FileDescriptorProto file;
                EXPECT_TRUE(parser_->Parse(input_.get(), &file));
                EXPECT_TRUE(error_collector_.warning_.find(
                                "Number should not come right after an underscore. Found: "
                                "song_name_1.")
                    != std::string::npos);
            }

            // ===================================================================

            typedef ParserTest ParseMessageTest;

            TEST_F(ParseMessageTest, IgnoreBOM) {
                char input[] = "   syntax = \"proto2\";\n"
                               "message TestMessage {\n"
                               "  required int32 foo = 1;\n"
                               "}\n";
                // Set UTF-8 BOM.
                input[0] = (char)0xEF;
                input[1] = (char)0xBB;
                input[2] = (char)0xBF;
                ExpectParsesTo(
                    input,
                    "syntax: 'proto2' "
                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:1 }"
                    "}");
            }

            TEST_F(ParseMessageTest, BOMError) {
                char input[] = "   syntax = \"proto2\";\n"
                               "message TestMessage {\n"
                               "  required int32 foo = 1;\n"
                               "}\n";
                input[0] = (char)0xEF;
                ExpectHasEarlyExitErrors(input,
                    "0:1: Proto file starts with 0xEF but not UTF-8 BOM. "
                    "Only UTF-8 is accepted for proto file.\n"
                    "0:0: File must begin with a syntax statement, e.g. "
                    "'syntax = \"proto2\";'.\n");
            }

            TEST_F(ParseMessageTest, SimpleMessage) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  required int32 foo = 1;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:1 }"
                    "}");
            }

            TEST_F(ParseMessageTest, ImplicitSyntaxIdentifier) {
                supply_syntax_ = false;
                ExpectHasEarlyExitErrors(
                    "message TestMessage {\n"
                    "  required int32 foo = 1;\n"
                    "}\n",
                    "0:0: File must begin with a syntax statement, e.g. 'syntax = "
                    "\"proto2\";'.\n");
                EXPECT_EQ("", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseMessageTest, ExplicitSyntaxIdentifier) {
                ExpectParsesTo(
                    "syntax = \"proto2\";\n"
                    "message TestMessage {\n"
                    "  required int32 foo = 1;\n"
                    "}\n",

                    "syntax: 'proto2' "
                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:1 }"
                    "}");
                EXPECT_EQ("proto2", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseMessageTest, ExplicitRequiredSyntaxIdentifier) {
                require_syntax_identifier_ = true;
                ExpectParsesTo(
                    "syntax = \"proto2\";\n"
                    "message TestMessage {\n"
                    "  required int32 foo = 1;\n"
                    "}\n",

                    "syntax: 'proto2' "
                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:1 }"
                    "}");
                EXPECT_EQ("proto2", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseMessageTest, SimpleFields) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  required int32 foo = 15;\n"
                    "  optional int32 bar = 34;\n"
                    "  list<int32> baz = 3;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:15 }"
                    "  field { name:\"bar\" label:OPTIONAL type:INT32 number:34 }"
                    "  field { name:\"baz\" label:REPEATED type:INT32 number:3  }"
                    "}");
            }

            TEST_F(ParseMessageTest, PrimitiveFieldTypes) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  required int32    foo = 1;\n"
                    "  required int64    foo = 1;\n"
                    "  required uint32   foo = 1;\n"
                    "  required uint64   foo = 1;\n"
                    "  required sint32   foo = 1;\n"
                    "  required sint64   foo = 1;\n"
                    "  required fixed32  foo = 1;\n"
                    "  required fixed64  foo = 1;\n"
                    "  required sfixed32 foo = 1;\n"
                    "  required sfixed64 foo = 1;\n"
                    "  required float    foo = 1;\n"
                    "  required double   foo = 1;\n"
                    "  required string   foo = 1;\n"
                    "  required bytes    foo = 1;\n"
                    "  required bool     foo = 1;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32    number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:INT64    number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:UINT32   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:UINT64   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:SINT32   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:SINT64   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:FIXED32  number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:FIXED64  number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:SFIXED32 number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:SFIXED64 number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:FLOAT    number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:DOUBLE   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:STRING   number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:BYTES    number:1 "
                    "}"
                    "  field { name:\"foo\" label:REQUIRED type:BOOL     number:1 "
                    "}"
                    "}");
            }

            TEST_F(ParseMessageTest, FieldDefaults) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  required int32  foo = 1 [default=  1  ];\n"
                    "  required int32  foo = 1 [default= -2  ];\n"
                    "  required int64  foo = 1 [default=  3  ];\n"
                    "  required int64  foo = 1 [default= -4  ];\n"
                    "  required uint32 foo = 1 [default=  5  ];\n"
                    "  required uint64 foo = 1 [default=  6  ];\n"
                    "  required float  foo = 1 [default=  7.5];\n"
                    "  required float  foo = 1 [default= -8.5];\n"
                    "  required float  foo = 1 [default=  9  ];\n"
                    "  required double foo = 1 [default= 10.5];\n"
                    "  required double foo = 1 [default=-11.5];\n"
                    "  required double foo = 1 [default= 12  ];\n"
                    "  required double foo = 1 [default= inf ];\n"
                    "  required double foo = 1 [default=-inf ];\n"
                    "  required double foo = 1 [default= nan ];\n"
                    "  required string foo = 1 [default='13\\001'];\n"
                    "  required string foo = 1 [default='a' \"b\" \n \"c\"];\n"
                    "  required bytes  foo = 1 [default='14\\002'];\n"
                    "  required bytes  foo = 1 [default='a' \"b\" \n 'c'];\n"
                    "  required bool   foo = 1 [default=true ];\n"
                    "  required Foo    foo = 1 [default=FOO  ];\n"

                    "  required int32  foo = 1 [default= 0x7FFFFFFF];\n"
                    "  required int32  foo = 1 [default=-0x80000000];\n"
                    "  required uint32 foo = 1 [default= 0xFFFFFFFF];\n"
                    "  required int64  foo = 1 [default= 0x7FFFFFFFFFFFFFFF];\n"
                    "  required int64  foo = 1 [default=-0x8000000000000000];\n"
                    "  required uint64 foo = 1 [default= 0xFFFFFFFFFFFFFFFF];\n"
                    "  required double foo = 1 [default= 0xabcd];\n"
                    "}\n",

#define ETC "name:\"foo\" label:REQUIRED number:1"
                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { type:INT32   default_value:\"1\"         " ETC
                    " }"
                    "  field { type:INT32   default_value:\"-2\"        " ETC
                    " }"
                    "  field { type:INT64   default_value:\"3\"         " ETC
                    " }"
                    "  field { type:INT64   default_value:\"-4\"        " ETC
                    " }"
                    "  field { type:UINT32  default_value:\"5\"         " ETC
                    " }"
                    "  field { type:UINT64  default_value:\"6\"         " ETC
                    " }"
                    "  field { type:FLOAT   default_value:\"7.5\"       " ETC
                    " }"
                    "  field { type:FLOAT   default_value:\"-8.5\"      " ETC
                    " }"
                    "  field { type:FLOAT   default_value:\"9\"         " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"10.5\"      " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"-11.5\"     " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"12\"        " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"inf\"       " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"-inf\"      " ETC
                    " }"
                    "  field { type:DOUBLE  default_value:\"nan\"       " ETC
                    " }"
                    "  field { type:STRING  default_value:\"13\\001\"   " ETC
                    " }"
                    "  field { type:STRING  default_value:\"abc\"       " ETC
                    " }"
                    "  field { type:BYTES   default_value:\"14\\\\002\" " ETC
                    " }"
                    "  field { type:BYTES   default_value:\"abc\"       " ETC
                    " }"
                    "  field { type:BOOL    default_value:\"true\"      " ETC
                    " }"
                    "  field { type_name:\"Foo\" default_value:\"FOO\"       " ETC
                    " }"

                    "  field {"
                    "    type:INT32   default_value:\"2147483647\"           " ETC
                    "  }"
                    "  field {"
                    "    type:INT32   default_value:\"-2147483648\"          " ETC
                    "  }"
                    "  field {"
                    "    type:UINT32  default_value:\"4294967295\"           " ETC
                    "  }"
                    "  field {"
                    "    type:INT64   default_value:\"9223372036854775807\"  " ETC
                    "  }"
                    "  field {"
                    "    type:INT64   default_value:\"-9223372036854775808\" " ETC
                    "  }"
                    "  field {"
                    "    type:UINT64  default_value:\"18446744073709551615\" " ETC
                    "  }"
                    "  field {"
                    "    type:DOUBLE  default_value:\"43981\"                " ETC
                    "  }"
                    "}");
#undef ETC
            }

            TEST_F(ParseMessageTest, FieldJsonName) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  optional string foo = 1 [json_name = \"@type\"];\n"
                    "}\n",
                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field {\n"
                    "    name: \"foo\" label: OPTIONAL type:STRING number: 1"
                    "    json_name: \"@type\"\n"
                    "  }\n"
                    "}\n");
            }

            TEST_F(ParseMessageTest, FieldOptions) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  optional string foo = 1\n"
                    "      [ctype=CORD, (foo)=7, foo.(.bar.baz).qux.quux.(corge)=-33, \n"
                    "       (quux)=\"x\040y\", (baz.qux)=hey];\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name: \"foo\" label: OPTIONAL type:STRING number: "
                    "1"
                    "          options { uninterpreted_option: { name { name_part: \"ctype\" "
                    "                                                   is_extension: false "
                    "} "
                    "                                            identifier_value: \"CORD\"  "
                    "}"
                    "                    uninterpreted_option: { name { name_part: \"foo\" "
                    "                                                   is_extension: true } "
                    "                                            positive_int_value: 7  }"
                    "                    uninterpreted_option: { name { name_part: \"foo\" "
                    "                                                   is_extension: false "
                    "} "
                    "                                            name { name_part: "
                    "\".bar.baz\""
                    "                                                   is_extension: true } "
                    "                                            name { name_part: \"qux\" "
                    "                                                   is_extension: false "
                    "} "
                    "                                            name { name_part: \"quux\" "
                    "                                                   is_extension: false "
                    "} "
                    "                                            name { name_part: \"corge\" "
                    "                                                   is_extension: true } "
                    "                                            negative_int_value: -33 }"
                    "                    uninterpreted_option: { name { name_part: \"quux\" "
                    "                                                   is_extension: true } "
                    "                                            string_value: \"x y\" }"
                    "                    uninterpreted_option: { name { name_part: "
                    "\"baz.qux\" "
                    "                                                   is_extension: true } "
                    "                                            identifier_value: \"hey\" }"
                    "          }"
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, Oneof) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  oneof foo {\n"
                    "    int32 a = 1;\n"
                    "    string b = 2;\n"
                    "    TestMessage c = 3;\n"
                    "  }\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"a\" label:OPTIONAL type:INT32 number:1 "
                    "          oneof_index:0 }"
                    "  field { name:\"b\" label:OPTIONAL type:STRING number:2 "
                    "          oneof_index:0 }"
                    "  field { name:\"c\" label:OPTIONAL type_name:\"TestMessage\" "
                    "          number:3 oneof_index:0 }"
                    "  oneof_decl {"
                    "    name: \"foo\""
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, MultipleOneofs) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  oneof foo {\n"
                    "    int32 a = 1;\n"
                    "    string b = 2;\n"
                    "  }\n"
                    "  oneof bar {\n"
                    "    int32 c = 3;\n"
                    "    string d = 4;\n"
                    "  }\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"a\" label:OPTIONAL type:INT32 number:1 "
                    "          oneof_index:0 }"
                    "  field { name:\"b\" label:OPTIONAL type:STRING number:2 "
                    "          oneof_index:0 }"
                    "  field { name:\"c\" label:OPTIONAL type:INT32 number:3 "
                    "          oneof_index:1 }"
                    "  field { name:\"d\" label:OPTIONAL type:STRING number:4 "
                    "          oneof_index:1 }"
                    "  oneof_decl {"
                    "    name: \"foo\""
                    "  }"
                    "  oneof_decl {"
                    "    name: \"bar\""
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, Maps) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  map<int32, string> primitive_type_map = 1;\n"
                    "  map<KeyType, ValueType> composite_type_map = 2;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  nested_type {"
                    "    name: \"PrimitiveTypeMapEntry\""
                    "    field { "
                    "       name: \"key\" number: 1 label:OPTIONAL"
                    "       type:INT32"
                    "    }"
                    "    field { "
                    "       name: \"value\" number: 2 label:OPTIONAL"
                    "       type:STRING"
                    "    }"
                    "    options { map_entry: true }"
                    "  }"
                    "  nested_type {"
                    "    name: \"CompositeTypeMapEntry\""
                    "    field { "
                    "       name: \"key\" number: 1 label:OPTIONAL"
                    "       type_name: \"KeyType\""
                    "    }"
                    "    field { "
                    "       name: \"value\" number: 2 label:OPTIONAL"
                    "       type_name: \"ValueType\""
                    "    }"
                    "    options { map_entry: true }"
                    "  }"
                    "  field {"
                    "    name: \"primitive_type_map\""
                    "    label: REPEATED"
                    "    type_name: \"PrimitiveTypeMapEntry\""
                    "    number: 1"
                    "  }"
                    "  field {"
                    "    name: \"composite_type_map\""
                    "    label: REPEATED"
                    "    type_name: \"CompositeTypeMapEntry\""
                    "    number: 2"
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, NestedMessage) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  message Nested {}\n"
                    "  optional Nested test_nested = 1;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  nested_type { name: \"Nested\" }"
                    "  field { name:\"test_nested\" label:OPTIONAL number:1"
                    "          type_name: \"Nested\" }"
                    "}");
            }

            TEST_F(ParseMessageTest, NestedEnum) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  enum NestedEnum {}\n"
                    "  optional NestedEnum test_enum = 1;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  enum_type {"
                    "    name: \"NestedEnum\""
                    "    options { underlying_type: ENUM_UNDERLYING_TYPE_INT32 }"
                    "  }"
                    "  field { name:\"test_enum\" label:OPTIONAL number:1"
                    "          type_name: \"NestedEnum\" }"
                    "}");
            }

            TEST_F(ParseMessageTest, ReservedRange) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  required int32 foo = 1;\n"
                    "  reserved 2, 15, 9 to 11, 3, 20 to max;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  field { name:\"foo\" label:REQUIRED type:INT32 number:1 }"
                    "  reserved_range { start:2   end:3         }"
                    "  reserved_range { start:15  end:16        }"
                    "  reserved_range { start:9   end:12        }"
                    "  reserved_range { start:3   end:4         }"
                    "  reserved_range { start:20  end:536870912 }"
                    "}");
            }

            TEST_F(ParseMessageTest, ReservedRangeOnMessageSet) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  option message_set_wire_format = true;\n"
                    "  reserved 20 to max;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  options {"
                    "    uninterpreted_option {"
                    "      name {"
                    "        name_part: \"message_set_wire_format\""
                    "        is_extension: false"
                    "      }"
                    "      identifier_value: \"true\""
                    "    }"
                    "  }"
                    "  reserved_range { start:20  end:2147483647 }"
                    "}");
            }

            TEST_F(ParseMessageTest, ReservedNames) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  reserved \"foo\", \"bar\";\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  reserved_name: \"foo\""
                    "  reserved_name: \"bar\""
                    "}");
            }

            TEST_F(ParseMessageTest, ExtensionRange) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extensions 10 to 19;\n"
                    "  extensions 30 to max;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  extension_range { start:10 end:20        }"
                    "  extension_range { start:30 end:536870912 }"
                    "}");
            }

            TEST_F(ParseMessageTest, ExtensionRangeWithOptions) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extensions 10 to 19 [(i) = 5];\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  extension_range {"
                    "    start:10"
                    "    end:20"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, CompoundExtensionRange) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extensions 2, 15, 9 to 11, 100 to max, 3;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  extension_range { start:2   end:3         }"
                    "  extension_range { start:15  end:16        }"
                    "  extension_range { start:9   end:12        }"
                    "  extension_range { start:100 end:536870912 }"
                    "  extension_range { start:3   end:4         }"
                    "}");
            }

            TEST_F(ParseMessageTest, CompoundExtensionRangeWithOptions) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extensions 2, 15, 9 to 11, 100 to max, 3 [(i) = 5];\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  extension_range {"
                    "    start:2"
                    "    end:3"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "  extension_range {"
                    "    start:15"
                    "    end:16"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "  extension_range {"
                    "    start:9"
                    "    end:12"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "  extension_range {"
                    "    start:100"
                    "    end:536870912"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "  extension_range {"
                    "    start:3"
                    "    end:4"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: \"i\""
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 5"
                    "      }"
                    "    }"
                    "  }"
                    "}");
            }

            TEST_F(ParseMessageTest, LargerMaxForMessageSetWireFormatMessages) {
                // Messages using the message_set_wire_format option can accept larger
                // extension numbers, as the numbers are not encoded as int32 field values
                // rather than tags.
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extensions 4 to max;\n"
                    "  option message_set_wire_format = true;\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "    extension_range { start:4 end: 0x7fffffff }"
                    "  options {\n"
                    "    uninterpreted_option { \n"
                    "      name {\n"
                    "        name_part: \"message_set_wire_format\"\n"
                    "        is_extension: false\n"
                    "      }\n"
                    "      identifier_value: \"true\"\n"
                    "    }\n"
                    "  }\n"
                    "}");
            }

            TEST_F(ParseMessageTest, Extensions) {
                ExpectParsesTo(
                    "extend Extendee1 { optional int32 foo = 12; }\n"
                    "extend Extendee2 { list<TestMessage> bar = 22; }\n",

                    "extension { name:\"foo\" label:OPTIONAL type:INT32 number:12"
                    "            extendee: \"Extendee1\" } "
                    "extension { name:\"bar\" label:REPEATED number:22"
                    "            type_name:\"TestMessage\" extendee: \"Extendee2\" }");
            }

            TEST_F(ParseMessageTest, ExtensionsInMessageScope) {
                ExpectParsesTo(
                    "message TestMessage {\n"
                    "  extend Extendee1 { optional int32 foo = 12; }\n"
                    "  extend Extendee2 { list<TestMessage> bar = 22; }\n"
                    "}\n",

                    "message_type {"
                    "  name: \"TestMessage\""
                    "  extension { name:\"foo\" label:OPTIONAL type:INT32 "
                    "number:12"
                    "              extendee: \"Extendee1\" }"
                    "  extension { name:\"bar\" label:REPEATED number:22"
                    "              type_name:\"TestMessage\" extendee: \"Extendee2\" }"
                    "}");
            }

            TEST_F(ParseMessageTest, MultipleExtensionsOneExtendee) {
                ExpectParsesTo(
                    "extend Extendee1 {\n"
                    "  optional int32 foo = 12;\n"
                    "  list<TestMessage> bar = 22;\n"
                    "}\n",

                    "extension { name:\"foo\" label:OPTIONAL type:INT32 number:12"
                    "            extendee: \"Extendee1\" } "
                    "extension { name:\"bar\" label:REPEATED number:22"
                    "            type_name:\"TestMessage\" extendee: \"Extendee1\" }");
            }

            TEST_F(ParseMessageTest, OptionalLabelProto3) {
                ExpectHasEarlyExitErrors(
                    "syntax = \"proto3\";\n"
                    "message TestMessage {\n"
                    "  int32 foo = 1;\n"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParseMessageTest, ExplicitOptionalLabelProto3) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message TestMessage {\n"
                    "  optional int32 foo = 1;\n"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            // ===================================================================

            typedef ParserTest ParseServiceTest;

            TEST_F(ParseServiceTest, SimpleService) {
                ExpectParsesTo(
                    "service TestService {\n"
                    "  rpc Foo(In) returns (Out);\n"
                    "}\n",

                    "service {"
                    "  name: \"TestService\""
                    "  method { name:\"Foo\" input_type:\"In\" output_type:\"Out\" }"
                    "}");
            }

            TEST_F(ParseServiceTest, MethodsAndStreams) {
                ExpectParsesTo(
                    "service TestService {\n"
                    "  rpc Foo(In1) returns (Out1);\n"
                    "  rpc Bar(In2) returns (Out2);\n"
                    "  rpc Baz(In3) returns (Out3);\n"
                    "}\n",

                    "service {"
                    "  name: \"TestService\""
                    "  method { name:\"Foo\" input_type:\"In1\" output_type:\"Out1\" }"
                    "  method { name:\"Bar\" input_type:\"In2\" output_type:\"Out2\" }"
                    "  method { name:\"Baz\" input_type:\"In3\" output_type:\"Out3\" }"
                    "}");
            }

            // ===================================================================
            // imports and packages

            typedef ParserTest ParseMiscTest;

            TEST_F(ParseMiscTest, ParseImport) {
                ExpectParsesTo("import \"foo/bar/baz.shark\";\n",
                    "dependency: \"foo/bar/baz.shark\"");
            }

            TEST_F(ParseMiscTest, ParseMultipleImports) {
                ExpectParsesTo(
                    "import \"foo.shark\";\n"
                    "import \"bar.shark\";\n"
                    "import \"baz.shark\";\n",
                    "dependency: \"foo.shark\""
                    "dependency: \"bar.shark\""
                    "dependency: \"baz.shark\"");
            }

            TEST_F(ParseMiscTest, ParsePublicImports) {
                ExpectParsesTo(
                    "import \"foo.shark\";\n"
                    "import public \"bar.shark\";\n"
                    "import \"baz.shark\";\n"
                    "import public \"qux.shark\";\n",
                    "dependency: \"foo.shark\""
                    "dependency: \"bar.shark\""
                    "dependency: \"baz.shark\""
                    "dependency: \"qux.shark\""
                    "public_dependency: 1 "
                    "public_dependency: 3 ");
            }

            TEST_F(ParseMiscTest, ParsePackage) {
                ExpectParsesTo("package foo.bar.baz;\n", "package: \"foo.bar.baz\"");
            }

            TEST_F(ParseMiscTest, ParsePackageWithSpaces) {
                ExpectParsesTo(
                    "package foo   .   bar.  \n"
                    "  baz;\n",
                    "package: \"foo.bar.baz\"");
            }

            // ===================================================================
            // options

            TEST_F(ParseMiscTest, ParseFileOptions) {
                ExpectParsesTo(
                    "option go_package = \"com.google.foo\";\n"
                    "option optimize_for = CODE_SIZE;",

                    "options {"
                    "uninterpreted_option { name { name_part: \"go_package\" "
                    "                              is_extension: false }"
                    "                       string_value: \"com.google.foo\"} "
                    "uninterpreted_option { name { name_part: \"optimize_for\" "
                    "                              is_extension: false }"
                    "                       identifier_value: \"CODE_SIZE\" } "
                    "}");
            }

            // ===================================================================
            // Error tests
            //
            // There are a very large number of possible errors that the parser could
            // report, so it's infeasible to test every single one of them.  Instead,
            // we test each unique call to AddError() in parser.h.  This does not mean
            // we are testing every possible error that Parser can generate because
            // each variant of the Consume() helper only counts as one unique call to
            // AddError().

            typedef ParserTest ParseErrorTest;

            TEST_F(ParseErrorTest, MissingSyntaxIdentifier) {
                supply_syntax_ = false;
                require_syntax_identifier_ = true;
                ExpectHasEarlyExitErrors("message TestMessage {}",
                    "0:0: File must begin with a syntax statement, e.g. "
                    "'syntax = \"proto2\";'.\n");
                EXPECT_EQ("", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseErrorTest, UnknownSyntaxIdentifier) {
                ExpectHasEarlyExitErrors(
                    "syntax = \"no_such_syntax\";",
                    "0:9: Unrecognized syntax identifier \"no_such_syntax\".  This parser "
                    "only recognizes \"proto2\".\n");
                EXPECT_EQ("no_such_syntax", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseErrorTest, SimpleSyntaxError) {
                ExpectHasErrors("message TestMessage @#$ { blah }",
                    "0:20: Expected \"{\".\n");
                EXPECT_EQ("proto2", parser_->GetSyntaxIdentifier());
            }

            TEST_F(ParseErrorTest, ExpectedTopLevel) {
                ExpectHasErrors("blah;",
                    "0:0: Expected top-level statement (e.g. \"message\").\n");
            }

            TEST_F(ParseErrorTest, UnmatchedCloseBrace) {
                // This used to cause an infinite loop.  Doh.
                ExpectHasErrors("}",
                    "0:0: Expected top-level statement (e.g. \"message\").\n"
                    "0:0: Unmatched \"}\".\n");
            }

            // -------------------------------------------------------------------
            // Message errors

            TEST_F(ParseErrorTest, MessageMissingName) {
                ExpectHasErrors("message {}", "0:8: Expected message name.\n");
            }

            TEST_F(ParseErrorTest, MessageMissingBody) {
                ExpectHasErrors("message TestMessage;", "0:19: Expected \"{\".\n");
            }

            TEST_F(ParseErrorTest, EofInMessage) {
                ExpectHasErrors(
                    "message TestMessage {",
                    "0:21: Reached end of input in message definition (missing '}').\n");
            }

            TEST_F(ParseErrorTest, MissingFieldNumber) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional int32 foo;\n"
                    "}\n",
                    "1:20: Missing field number.\n");
            }

            TEST_F(ParseErrorTest, ExpectedFieldNumber) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional int32 foo = ;\n"
                    "}\n",
                    "1:23: Expected field number.\n");
            }

            TEST_F(ParseErrorTest, FieldNumberOutOfRange) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional int32 foo = 0x100000000;\n"
                    "}\n",
                    "1:23: Integer out of range.\n");
            }

            TEST_F(ParseErrorTest, MissingLabel) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  int32 foo = 1;\n"
                    "}\n",
                    "1:2: Expected \"required\" or \"optional\".\n");
            }

            TEST_F(ParseErrorTest, ExpectedOptionName) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [];\n"
                    "}\n",
                    "1:27: Expected identifier.\n");
            }

            TEST_F(ParseErrorTest, NonExtensionOptionNameBeginningWithDot) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [.foo=1];\n"
                    "}\n",
                    "1:27: Expected identifier.\n");
            }

            TEST_F(ParseErrorTest, DefaultValueTypeMismatch) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [default=true];\n"
                    "}\n",
                    "1:35: Expected integer for field default value.\n");
            }

            TEST_F(ParseErrorTest, DefaultValueNotBoolean) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional bool foo = 1 [default=blah];\n"
                    "}\n",
                    "1:33: Expected \"true\" or \"false\".\n");
            }

            TEST_F(ParseErrorTest, DefaultValueNotString) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional string foo = 1 [default=1];\n"
                    "}\n",
                    "1:35: Expected string for field default value.\n");
            }

            TEST_F(ParseErrorTest, DefaultValueUnsignedNegative) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [default=-1];\n"
                    "}\n",
                    "1:36: Unsigned field can't have negative default value.\n");
            }

            TEST_F(ParseErrorTest, DefaultValueTooLarge) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional int32  foo = 1 [default= 0x80000000];\n"
                    "  optional int32  foo = 1 [default=-0x80000001];\n"
                    "  optional uint32 foo = 1 [default= 0x100000000];\n"
                    "  optional int64  foo = 1 [default= 0x80000000000000000];\n"
                    "  optional int64  foo = 1 [default=-0x80000000000000001];\n"
                    "  optional uint64 foo = 1 [default= 0x100000000000000000];\n"
                    "}\n",
                    "1:36: Integer out of range.\n"
                    "2:36: Integer out of range.\n"
                    "3:36: Integer out of range.\n"
                    "4:36: Integer out of range.\n"
                    "5:36: Integer out of range.\n"
                    "6:36: Integer out of range.\n");
            }

            TEST_F(ParseErrorTest, JsonNameNotString) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional string foo = 1 [json_name=1];\n"
                    "}\n",
                    "1:37: Expected string for JSON name.\n");
            }

            TEST_F(ParseErrorTest, DuplicateJsonName) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [json_name=\"a\",json_name=\"b\"];\n"
                    "}\n",
                    "1:41: Already set option \"json_name\".\n");
            }

            TEST_F(ParseErrorTest, DefaultValueMissing) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [default=];\n"
                    "}\n",
                    "1:35: Expected integer for field default value.\n");
            }

            TEST_F(ParseErrorTest, DuplicateDefaultValue) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional uint32 foo = 1 [default=1,default=2];\n"
                    "}\n",
                    "1:37: Already set option \"default\".\n");
            }

            TEST_F(ParseErrorTest, MissingOneofName) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  oneof {\n"
                    "    int32 bar = 1;\n"
                    "  }\n"
                    "}\n",
                    "1:8: Expected oneof name.\n");
            }

            TEST_F(ParseErrorTest, LabelInOneof) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  oneof foo {\n"
                    "    optional int32 bar = 1;\n"
                    "  }\n"
                    "}\n",
                    "2:4: Fields in oneofs must not have labels (required / optional).\n");
            }

            TEST_F(ParseErrorTest, MapInOneof) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  oneof foo {\n"
                    "    map<int32, int32> foo_map = 1;\n"
                    "    map message_field = 2;\n" // a normal message field is OK
                    "  }\n"
                    "}\n",
                    "2:7: Map fields are not allowed in oneofs.\n");
            }

            TEST_F(ParseErrorTest, LabelForMap) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional map<int32, int32> int_map = 1;\n"
                    "  required map<int32, int32> int_map2 = 2;\n"
                    "  optional map map_message = 4;\n" // a normal message field is OK
                    "}\n",
                    "1:14: Field labels (required/optional) are not allowed on map "
                    "fields.\n"
                    "2:14: Field labels (required/optional) are not allowed on map "
                    "fields.\n");
            }

            TEST_F(ParseErrorTest, MalformedMaps) {
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  map map_message = 1;\n" // a normal message field lacking label
                    "  map<string> str_map = 2;\n"
                    "  map<string,> str_map2 = 3;\n"
                    "  map<,string> str_map3 = 4;\n"
                    "  map<> empty_map = 5;\n"
                    "  map<string,string str_map6 = 6;\n"
                    "}"
                    "extend SomeMessage {\n"
                    "  map<int32, int32> int_map = 1;\n"
                    "}",
                    "1:6: Expected \"required\" or \"optional\".\n"
                    "2:12: Expected \",\".\n"
                    "3:13: Expected type name.\n"
                    "4:6: Expected type name.\n"
                    "5:6: Expected type name.\n"
                    "6:20: Expected \">\".\n"
                    "8:5: Map fields are not allowed to be extensions.\n");
            }

            TEST_F(ParseErrorTest, ExtendingPrimitive) {
                ExpectHasErrors("extend int32 { optional string foo = 4; }\n",
                    "0:7: Expected message type.\n");
            }

            TEST_F(ParseErrorTest, ErrorInExtension) {
                ExpectHasErrors(
                    "message Foo { extensions 100 to 199; }\n"
                    "extend Foo { optional string foo; }\n",
                    "1:32: Missing field number.\n");
            }

            TEST_F(ParseErrorTest, MultipleParseErrors) {
                // When a statement has a parse error, the parser should be able to continue
                // parsing at the next statement.
                ExpectHasErrors(
                    "message TestMessage {\n"
                    "  optional int32 foo;\n"
                    "  !invalid statement ending in a block { blah blah { blah } blah }\n"
                    "  optional int32 bar = 3 {}\n"
                    "}\n",
                    "1:20: Missing field number.\n"
                    "2:2: Expected \"required\" or \"optional\".\n"
                    "2:2: Expected type name.\n"
                    "3:25: Expected \";\".\n");
            }

            TEST_F(ParseErrorTest, EofInAggregateValue) {
                ExpectHasErrors(
                    "option (fileopt) = { i:100\n",
                    "1:0: Unexpected end of stream while parsing aggregate value.\n");
            }

            // -------------------------------------------------------------------
            // Reserved field number errors

            TEST_F(ParseErrorTest, ReservedStandaloneMaxNotAllowed) {
                ExpectHasErrors(
                    "message Foo {\n"
                    "  reserved max;\n"
                    "}\n",
                    "1:11: Expected field name or number range.\n");
            }

            TEST_F(ParseErrorTest, ReservedMixNameAndNumber) {
                ExpectHasErrors(
                    "message Foo {\n"
                    "  reserved 10, \"foo\";\n"
                    "}\n",
                    "1:15: Expected field number range.\n");
            }

            TEST_F(ParseErrorTest, ReservedMissingQuotes) {
                ExpectHasErrors(
                    "message Foo {\n"
                    "  reserved foo;\n"
                    "}\n",
                    "1:11: Expected field name or number range.\n");
            }

            TEST_F(ParseErrorTest, ReservedNegativeNumber) {
                ExpectHasErrors(
                    "message Foo {\n"
                    "  reserved -10;\n"
                    "}\n",
                    "1:11: Expected field name or number range.\n");
            }

            TEST_F(ParseErrorTest, ReservedNumberOutOfRange) {
                ExpectHasErrors(
                    "message Foo {\n"
                    "  reserved 2147483648;\n"
                    "}\n",
                    "1:11: Integer out of range.\n");
            }

            // -------------------------------------------------------------------
            // Service errors

            TEST_F(ParseErrorTest, EofInService) {
                ExpectHasErrors(
                    "service TestService {",
                    "0:21: Reached end of input in service definition (missing '}').\n");
            }

            TEST_F(ParseErrorTest, ServiceMethodPrimitiveParams) {
                ExpectHasErrors(
                    "service TestService {\n"
                    "  rpc Foo(int32) returns (string);\n"
                    "}\n",
                    "1:10: Expected message type.\n"
                    "1:26: Expected message type.\n");
            }

            TEST_F(ParseErrorTest, EofInMethodOptions) {
                ExpectHasErrors(
                    "service TestService {\n"
                    "  rpc Foo(Bar) returns(Bar) {",
                    "1:29: Reached end of input in method options (missing '}').\n"
                    "1:29: Reached end of input in service definition (missing '}').\n");
            }

            TEST_F(ParseErrorTest, PrimitiveMethodInput) {
                ExpectHasErrors(
                    "service TestService {\n"
                    "  rpc Foo(int32) returns(Bar);\n"
                    "}\n",
                    "1:10: Expected message type.\n");
            }

            TEST_F(ParseErrorTest, MethodOptionTypeError) {
                // This used to cause an infinite loop.
                ExpectHasErrors(
                    "message Baz {}\n"
                    "service Foo {\n"
                    "  rpc Bar(Baz) returns(Baz) { option invalid syntax; }\n"
                    "}\n",
                    "2:45: Expected \"=\".\n");
            }

            // -------------------------------------------------------------------
            // Import and package errors

            TEST_F(ParseErrorTest, ImportNotQuoted) {
                ExpectHasErrors("import foo;\n",
                    "0:7: Expected a string naming the file to import.\n");
            }

            TEST_F(ParseErrorTest, MultiplePackagesInFile) {
                ExpectHasErrors(
                    "package foo;\n"
                    "package bar;\n",
                    "1:0: Multiple package definitions.\n");
            }

            // ===================================================================
            // Test that errors detected by DescriptorPool correctly report line and
            // column numbers.  We have one test for every call to RecordLocation() in
            // parser.cc.

            typedef ParserTest ParserValidationErrorTest;

            TEST_F(ParserValidationErrorTest, PackageNameError) {
                // Create another file which defines symbol "foo".
                FileDescriptorProto other_file;
                other_file.set_name("bar.shark");
                other_file.add_message_type()->set_name("foo");
                EXPECT_TRUE(pool_.BuildFile(other_file) != nullptr);

                // Now try to define it as a package.
                ExpectHasValidationErrors(
                    "package foo.bar;",
                    "0:0: \"foo\" is already defined (as something other than a package) "
                    "in file \"bar.shark\".\n");
            }

            TEST_F(ParserValidationErrorTest, ImportUnloadedError) {
                ExpectHasValidationErrors(
                    "package test;\n"
                    "\n"
                    "import \"unloaded.shark\";",
                    "2:0: Import \"unloaded.shark\" has not been loaded.\n");
            }

            TEST_F(ParserValidationErrorTest, ImportTwice) {
                FileDescriptorProto other_file;
                other_file.set_name("bar.shark");
                other_file.add_message_type()->set_name("foo");
                EXPECT_TRUE(pool_.BuildFile(other_file) != nullptr);

                ExpectHasValidationErrors(
                    "package test;\n"
                    "\n"
                    "import \"bar.shark\";\n"
                    "  import \"bar.shark\";",
                    "3:2: Import \"bar.shark\" was listed twice.\n");
            }

            TEST_F(ParserValidationErrorTest, DuplicateFileError) {
                FileDescriptorProto other_file;
                other_file.set_name("foo.shark");
                EXPECT_TRUE(pool_.BuildFile(other_file) != nullptr);

                ExpectHasValidationErrors(
                    "syntax = \"proto2\";\n"
                    "package test;",
                    "0:0: A file with this name is already in the pool.\n");
            }

            TEST_F(ParserValidationErrorTest, MessageNameError) {
                ExpectHasValidationErrors(
                    "message Foo {}\n"
                    "message Foo {}\n",
                    "1:8: \"Foo\" is already defined.\n");
            }

            TEST_F(ParserValidationErrorTest, FieldNameError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  optional int32 bar = 1;\n"
                    "  optional int32 bar = 2;\n"
                    "}\n",
                    "2:17: \"bar\" is already defined in \"Foo\".\n");
            }

            TEST_F(ParserValidationErrorTest, FieldTypeError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  optional Baz bar = 1;\n"
                    "}\n",
                    "1:11: \"Baz\" is not defined.\n");
            }

            TEST_F(ParserValidationErrorTest, FieldNumberError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  optional int32 bar = 0;\n"
                    "}\n",
                    "1:23: Field numbers must be positive integers.\n"
                    "1:23: Suggested field numbers for Foo: 1\n");
            }

            TEST_F(ParserValidationErrorTest, FieldExtendeeError) {
                ExpectHasValidationErrors("extend Baz { optional int32 bar = 1; }\n",
                    "0:7: \"Baz\" is not defined.\n");
            }

            TEST_F(ParserValidationErrorTest, ExtensionJsonNameError) {
                ExpectHasValidationErrors(
                    "message TestMessage {\n"
                    "  extensions 1 to 100;\n"
                    "}\n"
                    "extend TestMessage {\n"
                    "  optional int32 foo = 12 [json_name = \"bar\"];\n"
                    "}",
                    "4:27: option json_name is not allowed on extension fields.\n");
            }

            TEST_F(ParserValidationErrorTest, FieldDefaultValueError) {
                ExpectHasValidationErrors(
                    "enum Baz { QUX = 1; }\n"
                    "message Foo {\n"
                    "  optional Baz bar = 1 [default=NO_SUCH_VALUE];\n"
                    "}\n",
                    "2:32: Enum type \"Baz\" has no value named \"NO_SUCH_VALUE\".\n");
            }

            TEST_F(ParserValidationErrorTest, FileOptionNameError) {
                ExpectHasValidationErrors(
                    "option foo = 5;",
                    "0:7: Option \"foo\" unknown. Ensure that your proto definition file "
                    "imports the proto which defines the option.\n");
            }

            TEST_F(ParserValidationErrorTest, FileOptionValueError) {
                ExpectHasValidationErrors(
                    "option go_package = 5;",
                    "0:20: Value must be quoted string for string option "
                    "\"shark.FileOptions.go_package\".\n");
            }

            TEST_F(ParserValidationErrorTest, FieldOptionNameError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  optional bool bar = 1 [foo=1];\n"
                    "}\n",
                    "1:25: Option \"foo\" unknown. Ensure that your proto definition file "
                    "imports the proto which defines the option.\n");
            }

            TEST_F(ParserValidationErrorTest, FieldOptionValueError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  optional int32 bar = 1 [ctype=1];\n"
                    "}\n",
                    "1:32: Value must be identifier for enum-valued option "
                    "\"shark.FieldOptions.ctype\".\n");
            }

            TEST_F(ParserValidationErrorTest, ExtensionRangeNumberError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  extensions 0;\n"
                    "}\n",
                    "1:13: Extension numbers must be positive integers.\n"
                    "1:13: Suggested field numbers for Foo: 1\n");
            }

            TEST_F(ParserValidationErrorTest, Proto3ExtensionError) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message Foo { \n"
                    "  extensions 100 to 199;\n"
                    "}\n"
                    "extend Foo { string foo = 101; }\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParserValidationErrorTest, Proto3MessageSet) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message Foo { \n"
                    "  option message_set_wire_format = true;\n"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParserValidationErrorTest, Proto3Required) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message Foo { \n"
                    "  required int32 field = 1;"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParserValidationErrorTest, Proto3Default) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message Foo { \n"
                    "  int32 field = 1 [default = 12];"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParserValidationErrorTest, Proto3JsonConflictError) {
                ExpectHasEarlyExitErrors(
                    "syntax = 'proto3';\n"
                    "message TestMessage {\n"
                    "  uint32 foo = 1;\n"
                    "  uint32 Foo = 2;\n"
                    "}\n",
                    "0:9: Unrecognized syntax identifier \"proto3\".  This parser "
                    "only recognizes \"proto2\".\n");
            }

            TEST_F(ParserValidationErrorTest, ExplicitlyMapEntryError) {
                ExpectHasValidationErrors(
                    "message Foo {\n"
                    "  message ValueEntry {\n"
                    "    option map_entry = true;\n"
                    "    optional int32 key = 1;\n"
                    "    optional int32 value = 2;\n"
                    "    extensions 99 to 999;\n"
                    "  }\n"
                    "  list<ValueEntry> value = 1;\n"
                    "}",
                    "7:2: map_entry should not be set explicitly. Use "
                    "map<KeyType, ValueType> instead.\n");
            }

            TEST_F(ParserValidationErrorTest, ServiceNameError) {
                ExpectHasValidationErrors(
                    "service Foo {}\n"
                    "service Foo {}\n",
                    "1:8: \"Foo\" is already defined.\n");
            }

            TEST_F(ParserValidationErrorTest, MethodNameError) {
                ExpectHasValidationErrors(
                    "message Baz {}\n"
                    "service Foo {\n"
                    "  rpc Bar(Baz) returns(Baz);\n"
                    "  rpc Bar(Baz) returns(Baz);\n"
                    "}\n",
                    "3:6: \"Bar\" is already defined in \"Foo\".\n");
            }

            TEST_F(ParserValidationErrorTest, MethodInputTypeError) {
                ExpectHasValidationErrors(
                    "message Baz {}\n"
                    "service Foo {\n"
                    "  rpc Bar(Qux) returns(Baz);\n"
                    "}\n",
                    "2:10: \"Qux\" is not defined.\n");
            }

            TEST_F(ParserValidationErrorTest, MethodOutputTypeError) {
                ExpectHasValidationErrors(
                    "message Baz {}\n"
                    "service Foo {\n"
                    "  rpc Bar(Baz) returns(Qux);\n"
                    "}\n",
                    "2:23: \"Qux\" is not defined.\n");
            }

            TEST_F(ParserValidationErrorTest, ResolvedUndefinedError) {
                // Create another file which defines symbol ".base.bar".
                FileDescriptorProto other_file;
                other_file.set_name("base.shark");
                other_file.set_package("base");
                other_file.add_message_type()->set_name("bar");
                EXPECT_TRUE(pool_.BuildFile(other_file) != nullptr);

                // Define "foo.base" and try "base.bar".
                // "base.bar" is resolved to "foo.base.bar" which is not defined.
                ExpectHasValidationErrors(
                    "package foo.base;\n"
                    "import \"base.shark\";\n"
                    "message qux {\n"
                    "  optional base.bar baz = 1;\n"
                    "  optional .base.bar quz = 2;\n"
                    "}\n",
                    "3:11: \"base.bar\" is resolved to \"foo.base.bar\","
                    " which is not defined. The innermost scope is searched first "
                    "in name resolution. Consider using a leading '.'(i.e., \".base.bar\")"
                    " to start from the outermost scope.\n");
            }

            TEST_F(ParserValidationErrorTest, ResovledUndefinedOptionError) {
                // Build descriptor message in test pool
                const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
                ASSERT_NE(meta_fd, nullptr);
                FileDescriptorProto meta_proto;
                meta_fd->CopyTo(&meta_proto);
                ASSERT_NE(pool_.BuildFile(meta_proto), nullptr);

                FileDescriptorProto descriptor_proto;
                DescriptorProto::descriptor()->file()->CopyTo(&descriptor_proto);
                ASSERT_TRUE(pool_.BuildFile(descriptor_proto) != nullptr);

                // base2.shark:
                //   package baz
                //   import net/proto2/proto/descriptor.shark
                //   message Bar { optional int32 foo = 1; }
                //   extend FileOptions { optional Bar bar = 7672757; }
                FileDescriptorProto other_file;
                other_file.set_name("base2.shark");
                other_file.set_package("baz");
                other_file.add_dependency();
                other_file.set_dependency(0, descriptor_proto.name());

                DescriptorProto* message(other_file.add_message_type());
                message->set_name("Bar");
                FieldDescriptorProto* field(message->add_field());
                field->set_name("foo");
                field->set_number(1);
                field->set_label(FieldCardinality::OPTIONAL);
                field->set_type(SharkType::INT32);

                FieldDescriptorProto* extension(other_file.add_extension());
                extension->set_name("bar");
                extension->set_number(7672757);
                extension->set_label(FieldCardinality::OPTIONAL);
                extension->set_type(SharkType::MESSAGE);
                extension->set_type_name("Bar");
                extension->set_extendee("shark.FileOptions");

                EXPECT_TRUE(pool_.BuildFile(other_file) != nullptr);

                // qux.shark:
                //   package qux.baz
                //   option (baz.bar).foo = 1;
                //
                // Although "baz.bar" is already defined, the lookup code will try
                // "qux.baz.bar", since it's the match from the innermost scope,
                // which will cause a symbol not defined error.
                ExpectHasValidationErrors(
                    "package qux.baz;\n"
                    "import \"base2.shark\";\n"
                    "option (baz.bar).foo = 1;\n",
                    "2:7: Option \"(baz.bar)\" is resolved to \"(qux.baz.bar)\","
                    " which is not defined. The innermost scope is searched first "
                    "in name resolution. Consider using a leading '.'(i.e., \"(.baz.bar)\")"
                    " to start from the outermost scope.\n");
            }

            // ===================================================================
            // Test that the output from FileDescriptor::DebugString() (and all other
            // descriptor types) is parseable, and results in the same Descriptor
            // definitions again afoter parsing (note, however, that the order of messages
            // cannot be guaranteed to be the same)

            typedef ParserTest ParseDescriptorDebugTest;

            class CompareDescriptorNames {
            public:
                bool operator()(const DescriptorProto* left,
                    const DescriptorProto* right) const {
                    return left->name() < right->name();
                }
            };

            // Sorts nested DescriptorProtos of a DescriptoProto, by name.
            void SortMessages(DescriptorProto* descriptor_proto) {
                int size = descriptor_proto->nested_type_size();
                // recursively sort; we can't guarantee the order of nested messages either
                for (int i = 0; i < size; ++i) {
                    SortMessages(descriptor_proto->mutable_nested_type(i));
                }
                auto* nested = descriptor_proto->mutable_nested_type();
                std::sort(nested->pointer_begin(), nested->pointer_end(),
                    CompareDescriptorNames());
            }

            // Sorts DescriptorProtos belonging to a FileDescriptorProto, by name.
            void SortMessages(FileDescriptorProto* file_descriptor_proto) {
                int size = file_descriptor_proto->message_type_size();
                // recursively sort; we can't guarantee the order of nested messages either
                for (int i = 0; i < size; ++i) {
                    SortMessages(file_descriptor_proto->mutable_message_type(i));
                }
                auto* messages = file_descriptor_proto->mutable_message_type();
                std::sort(messages->pointer_begin(), messages->pointer_end(),
                    CompareDescriptorNames());
            }

            // Strips the message and enum field type names for comparison purpose only.
            void StripFieldTypeName(DescriptorProto* proto) {
                for (int i = 0; i < proto->field_size(); ++i) {
                    std::string type_name = proto->field(i).type_name();
                    std::string::size_type pos = type_name.find_last_of('.');
                    if (pos != std::string::npos) {
                        proto->mutable_field(i)->mutable_type_name()->assign(
                            type_name.begin() + pos + 1, type_name.end());
                    }
                }
                for (int i = 0; i < proto->nested_type_size(); ++i) {
                    StripFieldTypeName(proto->mutable_nested_type(i));
                }
            }

            void StripFieldTypeName(FileDescriptorProto* file_proto) {
                for (int i = 0; i < file_proto->message_type_size(); ++i) {
                    StripFieldTypeName(file_proto->mutable_message_type(i));
                }
            }

            TEST_F(ParseDescriptorDebugTest, TestAllDescriptorTypes) {
                const FileDescriptor* original_file = sharkbuf_unittest::TestAllTypes::descriptor()->file();
                FileDescriptorProto expected;
                original_file->CopyTo(&expected);

                // Get the DebugString of the unittest.shark FileDecriptor, which includes
                // all other descriptor types
                std::string debug_string = original_file->DebugString();

                // Parse the debug string
                SetupParser(debug_string.c_str());
                FileDescriptorProto parsed;
                parser_->Parse(input_.get(), &parsed);
                EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
                ASSERT_EQ("", error_collector_.text_) << "Failed to parse:\n"
                                                      << debug_string;

                // We now have a FileDescriptorProto, but to compare with the expected we
                // need to link to a FileDecriptor, then output back to a proto. We'll
                // also need to give it the same name as the original.
                parsed.set_name(
                    TestUtil::MaybeTranslatePath("net/proto2/internal/sk/unittest.shark"));
                // We need the imported dependency before we can build our parsed proto
                const FileDescriptor* public_import = sharkbuf_unittest_import::PublicImportMessage::descriptor()->file();
                FileDescriptorProto public_import_proto;
                public_import->CopyTo(&public_import_proto);
                ASSERT_TRUE(pool_.BuildFile(public_import_proto) != nullptr);
                const FileDescriptor* import = sharkbuf_unittest_import::ImportMessage::descriptor()->file();
                FileDescriptorProto import_proto;
                import->CopyTo(&import_proto);
                ASSERT_TRUE(pool_.BuildFile(import_proto) != nullptr);
                const FileDescriptor* actual = pool_.BuildFile(parsed);
                parsed.Clear();
                ASSERT_TRUE(actual != nullptr) << "Failed to validate:\n"
                                               << debug_string;
                actual->CopyTo(&parsed);
                ASSERT_TRUE(actual != nullptr);

                // The messages might be in different orders, making them hard to compare.
                // So, sort the messages in the descriptor protos (including nested messages,
                // recursively).
                SortMessages(&expected);
                SortMessages(&parsed);

                // I really wanted to use StringDiff here for the debug output on fail,
                // but the strings are too long for it, and if I increase its max size,
                // we get a memory allocation failure :(
                EXPECT_EQ(expected.DebugString(), parsed.DebugString());
            }

            TEST_F(ParseDescriptorDebugTest, TestCustomOptions) {
                const FileDescriptor* original_file = sharkbuf_unittest::AggregateMessage::descriptor()->file();
                FileDescriptorProto expected;
                original_file->CopyTo(&expected);

                std::string debug_string = original_file->DebugString();

                // Parse the debug string
                SetupParser(debug_string.c_str());
                FileDescriptorProto parsed;
                parser_->Parse(input_.get(), &parsed);
                EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
                ASSERT_EQ("", error_collector_.text_);

                // We now have a FileDescriptorProto, but to compare with the expected we
                // need to link to a FileDecriptor, then output back to a proto. We'll
                // also need to give it the same name as the original.
                parsed.set_name(original_file->name());

                // unittest_custom_options.shark depends on descriptor.shark.

                const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
                ASSERT_NE(meta_fd, nullptr);
                FileDescriptorProto meta_proto;
                meta_fd->CopyTo(&meta_proto);
                ASSERT_NE(pool_.BuildFile(meta_proto), nullptr);

                const FileDescriptor* import = FileDescriptorProto::descriptor()->file();
                FileDescriptorProto import_proto;
                import->CopyTo(&import_proto);
                ASSERT_TRUE(pool_.BuildFile(import_proto) != nullptr);

                FileDescriptorProto any_import;
                shark::Any::descriptor()->file()->CopyTo(&any_import);
                ASSERT_TRUE(pool_.BuildFile(any_import) != nullptr);

                const FileDescriptor* actual = pool_.BuildFile(parsed);
                ASSERT_TRUE(actual != nullptr);
                parsed.Clear();
                actual->CopyTo(&parsed);

                // The messages might be in different orders, making them hard to compare.
                // So, sort the messages in the descriptor protos (including nested messages,
                // recursively).
                SortMessages(&expected);
                SortMessages(&parsed);

                EXPECT_EQ(expected.DebugString(), parsed.DebugString());
            }

            // Ensure that DebugStringWithOptions(), with |include_comments| set to true,
            // includes comments from the original parser input in all of the appropriate
            // places.
            TEST_F(ParseDescriptorDebugTest, TestCommentsInDebugString) {
                SetupParser(
                    "// Detached comment before syntax.\n"
                    "\n"
                    "// Syntax comment.\n"
                    "syntax = \"proto2\";\n"
                    "\n"
                    "// Detached comment before package.\n"
                    "\n"
                    "// Package comment.\n"
                    "package comment_test;\n"
                    "\n"
                    "// Detached comment before TestMessage1.\n"
                    "\n"
                    "// Message comment.\n"
                    "//\n"
                    "// More detail in message comment.\n"
                    "message TestMessage1 {\n"
                    "\n"
                    "  // Detached comment before foo.\n"
                    "\n"
                    "  // Field comment.\n"
                    "  optional int32 foo = 1;\n"
                    "\n"
                    "  // Detached comment before NestedMessage.\n"
                    "\n"
                    "  // Nested-message comment.\n"
                    "  message NestedMessage {\n"
                    "    optional int32 bar = 1;\n"
                    "  }\n"
                    "}\n"
                    "\n"
                    "// Detached comment before MyEnumType.\n"
                    "\n"
                    "// Enum comment.\n"
                    "enum MyEnumType {\n"
                    "\n"
                    "  // Detached comment before ASDF.\n"
                    "\n"
                    "  // Enum-value comment.\n"
                    "  ASDF = 1;\n"
                    "}\n"
                    "\n"
                    "// Detached comment before MyService.\n"
                    "\n"
                    "// Service comment.\n"
                    "service MyService {\n"
                    "\n"
                    "  // Detached comment before MyRPCCall.\n"
                    "\n"
                    "  // RPC comment.\n"
                    "  rpc MyRPCCall(TestMessage1) returns (TestMessage1) { }\n"
                    "}\n");

                FileDescriptorProto parsed_desc;
                parsed_desc.set_name("foo.shark");
                SourceLocationTable source_locations;
                parser_->RecordSourceLocationsTo(&source_locations);
                parser_->Parse(input_.get(), &parsed_desc);
                EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
                ASSERT_EQ("", error_collector_.text_);

                // We need to import the FileDescriptorProto to get a FileDescriptor.
                MockValidationErrorCollector collector(source_locations, &error_collector_);
                const FileDescriptor* descriptor = pool_.BuildFileCollectingErrors(parsed_desc, &collector);
                ASSERT_TRUE(descriptor != nullptr);

                // Ensure that each of the comments appears somewhere in the DebugString().
                // We don't test the exact comment placement or formatting, because we do not
                // want to be too fragile here.
                const char* expected_comments[] = {
                    "Detached comment before syntax.",
                    "Syntax comment.",
                    "Detached comment before package.",
                    "Package comment.",
                    "Detached comment before TestMessage1.",
                    "Message comment.",
                    "More detail in message comment.",
                    "Detached comment before foo.",
                    "Field comment",
                    "Detached comment before NestedMessage.",
                    "Nested-message comment",
                    "Detached comment before MyEnumType.",
                    "Enum comment",
                    "Detached comment before ASDF.",
                    "Enum-value comment",
                    "Detached comment before MyService.",
                    "Service comment",
                    "Detached comment before MyRPCCall.",
                    "RPC comment",
                };

                DebugStringOptions debug_string_options;
                debug_string_options.include_comments = true;

                {
                    const std::string debug_string = descriptor->DebugStringWithOptions(debug_string_options);

                    for (int i = 0; i < GOOGLE_ARRAYSIZE(expected_comments); ++i) {
                        std::string::size_type found_pos = debug_string.find(expected_comments[i]);
                        EXPECT_TRUE(found_pos != std::string::npos)
                            << "\"" << expected_comments[i] << "\" not found.";
                    }

                    // Result of DebugStringWithOptions should be parseable.
                    SetupParser(debug_string.c_str());
                    FileDescriptorProto parsed;
                    parser_->Parse(input_.get(), &parsed);
                    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
                    ASSERT_EQ("", error_collector_.text_) << "Failed to parse:\n"
                                                          << debug_string;
                }
            }

            TEST_F(ParseDescriptorDebugTest, TestMaps) {
                SetupParser(
                    "syntax = \"proto2\"; "
                    "message Foo { "
                    "  message Bar { } "
                    "  map<int32, Bar> enum_message_map = 1; "
                    "  map<string, float> primitive_map = 2; "
                    "} ");
                FileDescriptorProto original;
                EXPECT_TRUE(parser_->Parse(input_.get(), &original));
                original.set_name("foo.shark");
                const FileDescriptor* file = pool_.BuildFile(original);
                ASSERT_TRUE(file != nullptr);

                // Make sure the debug string uses map syntax and does not have the auto
                // generated entry.
                std::string debug_string = file->DebugString();
                EXPECT_TRUE(debug_string.find("map<") != std::string::npos);
                EXPECT_TRUE(debug_string.find("option map_entry") == std::string::npos);
                EXPECT_TRUE(debug_string.find("MapEntry") == std::string::npos);

                // Make sure the descriptor debug string is parsable.
                FileDescriptorProto parsed;
                SetupParser(debug_string.c_str());
                parsed.set_name("foo.shark");
                ASSERT_TRUE(parser_->Parse(input_.get(), &parsed));

                original.clear_source_code_info();
                parsed.clear_source_code_info();
                StripFieldTypeName(&original);
                StripFieldTypeName(&parsed);
                EXPECT_EQ(original.DebugString(), parsed.DebugString());
            }

            // ===================================================================
            // SourceCodeInfo tests.
            // (Fixture lives in parser_test_fixture.h; only tests below.)

            TEST_F(SourceInfoTest, BasicFileDecls) {
                EXPECT_TRUE(
                    Parse("$a$syntax = \"proto2\";$i$\n"
                          "$b$package foo.bar;$c$\n"
                          "$d$import \"baz.shark\";$e$\n"
                          "$f$import\"qux.shark\";$h$\n"
                          "$j$import $k$public$l$ \"bar.shark\";$m$\n"
                          "$n$import $o$weak$p$ \"bar.shark\";$q$\n"
                          "\n"
                          "// comment ignored\n"));

                EXPECT_TRUE(HasSpan('a', 'q', file_));
                EXPECT_TRUE(HasSpan('b', 'c', file_, "package"));
                EXPECT_TRUE(HasSpan('d', 'e', file_, "dependency", 0));
                EXPECT_TRUE(HasSpan('f', 'h', file_, "dependency", 1));
                EXPECT_TRUE(HasSpan('j', 'm', file_, "dependency", 2));
                EXPECT_TRUE(HasSpan('k', 'l', file_, "public_dependency", 0));
                EXPECT_TRUE(HasSpan('n', 'q', file_, "dependency", 3));
                EXPECT_TRUE(HasSpan('o', 'p', file_, "weak_dependency", 0));
                EXPECT_TRUE(HasSpan('a', 'i', file_, "syntax"));
            }

            TEST_F(SourceInfoTest, Messages) {
                EXPECT_TRUE(
                    Parse("$a$message $b$Foo$c$ {}$d$\n"
                          "$e$message $f$Bar$g$ {}$h$\n"));

                EXPECT_TRUE(HasSpan('a', 'd', file_.message_type(0)));
                EXPECT_TRUE(HasSpan('b', 'c', file_.message_type(0), "name"));
                EXPECT_TRUE(HasSpan('e', 'h', file_.message_type(1)));
                EXPECT_TRUE(HasSpan('f', 'g', file_.message_type(1), "name"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
            }

            TEST_F(SourceInfoTest, Fields) {
                EXPECT_TRUE(
                    Parse("message Foo {\n"
                          "  $a$optional$b$ $c$int32$d$ $e$bar$f$ = $g$1$h$;$i$\n"
                          "  $j$list$k$<$l$X.Y$m$> $n$baz$o$ = $p$2$q$;$r$\n"
                          "}\n"));

                const FieldDescriptorProto& field1 = file_.message_type(0).field(0);
                const FieldDescriptorProto& field2 = file_.message_type(0).field(1);

                EXPECT_TRUE(HasSpan('a', 'i', field1));
                EXPECT_TRUE(HasSpan('a', 'b', field1, "label"));
                EXPECT_TRUE(HasSpan('c', 'd', field1, "type"));
                EXPECT_TRUE(HasSpan('e', 'f', field1, "name"));
                EXPECT_TRUE(HasSpan('g', 'h', field1, "number"));

                EXPECT_TRUE(HasSpan('j', 'r', field2));
                EXPECT_TRUE(HasSpan('j', 'k', field2, "label"));
                EXPECT_TRUE(HasSpan('l', 'm', field2, "type_name"));
                EXPECT_TRUE(HasSpan('n', 'o', field2, "name"));
                EXPECT_TRUE(HasSpan('p', 'q', field2, "number"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
            }

            TEST_F(SourceInfoTest, Proto3Fields) {
                EXPECT_FALSE(
                    Parse("syntax = \"proto3\";\n"
                          "message Foo {\n"
                          "  $a$int32$b$ $c$bar$d$ = $e$1$f$;$g$\n"
                          "  $h$list$i$<$j$X.Y$k$> $l$baz$m$ = $n$2$o$;$p$\n"
                          "}\n"));
            }

            TEST_F(SourceInfoTest, Extensions) {
                EXPECT_TRUE(
                    Parse("$a$extend $b$Foo$c$ {\n"
                          "  $d$optional$e$ int32 bar = 1;$f$\n"
                          "  $g$list$h$<X.Y> baz = 2;$i$\n"
                          "}$j$\n"
                          "$k$extend $l$Bar$m$ {\n"
                          "  $n$optional int32 qux = 1;$o$\n"
                          "}$p$\n"));

                const FieldDescriptorProto& field1 = file_.extension(0);
                const FieldDescriptorProto& field2 = file_.extension(1);
                const FieldDescriptorProto& field3 = file_.extension(2);

                EXPECT_TRUE(HasSpan('a', 'j', file_, "extension"));
                EXPECT_TRUE(HasSpan('k', 'p', file_, "extension"));

                EXPECT_TRUE(HasSpan('d', 'f', field1));
                EXPECT_TRUE(HasSpan('d', 'e', field1, "label"));
                EXPECT_TRUE(HasSpan('b', 'c', field1, "extendee"));

                EXPECT_TRUE(HasSpan('g', 'i', field2));
                EXPECT_TRUE(HasSpan('g', 'h', field2, "label"));
                EXPECT_TRUE(HasSpan('b', 'c', field2, "extendee"));

                EXPECT_TRUE(HasSpan('n', 'o', field3));
                EXPECT_TRUE(HasSpan('l', 'm', field3, "extendee"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(field1, "type"));
                EXPECT_TRUE(HasSpan(field1, "name"));
                EXPECT_TRUE(HasSpan(field1, "number"));
                EXPECT_TRUE(HasSpan(field2, "type_name"));
                EXPECT_TRUE(HasSpan(field2, "name"));
                EXPECT_TRUE(HasSpan(field2, "number"));
                EXPECT_TRUE(HasSpan(field3, "label"));
                EXPECT_TRUE(HasSpan(field3, "type"));
                EXPECT_TRUE(HasSpan(field3, "name"));
                EXPECT_TRUE(HasSpan(field3, "number"));
            }

            TEST_F(SourceInfoTest, NestedExtensions) {
                EXPECT_TRUE(
                    Parse("message Message {\n"
                          "  $a$extend $b$Foo$c$ {\n"
                          "    $d$optional$e$ int32 bar = 1;$f$\n"
                          "    $g$list$h$<X.Y> baz = 2;$i$\n"
                          "  }$j$\n"
                          "  $k$extend $l$Bar$m$ {\n"
                          "    $n$optional int32 qux = 1;$o$\n"
                          "  }$p$\n"
                          "}\n"));

                const FieldDescriptorProto& field1 = file_.message_type(0).extension(0);
                const FieldDescriptorProto& field2 = file_.message_type(0).extension(1);
                const FieldDescriptorProto& field3 = file_.message_type(0).extension(2);

                EXPECT_TRUE(HasSpan('a', 'j', file_.message_type(0), "extension"));
                EXPECT_TRUE(HasSpan('k', 'p', file_.message_type(0), "extension"));

                EXPECT_TRUE(HasSpan('d', 'f', field1));
                EXPECT_TRUE(HasSpan('d', 'e', field1, "label"));
                EXPECT_TRUE(HasSpan('b', 'c', field1, "extendee"));

                EXPECT_TRUE(HasSpan('g', 'i', field2));
                EXPECT_TRUE(HasSpan('g', 'h', field2, "label"));
                EXPECT_TRUE(HasSpan('b', 'c', field2, "extendee"));

                EXPECT_TRUE(HasSpan('n', 'o', field3));
                EXPECT_TRUE(HasSpan('l', 'm', field3, "extendee"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
                EXPECT_TRUE(HasSpan(field1, "type"));
                EXPECT_TRUE(HasSpan(field1, "name"));
                EXPECT_TRUE(HasSpan(field1, "number"));
                EXPECT_TRUE(HasSpan(field2, "type_name"));
                EXPECT_TRUE(HasSpan(field2, "name"));
                EXPECT_TRUE(HasSpan(field2, "number"));
                EXPECT_TRUE(HasSpan(field3, "label"));
                EXPECT_TRUE(HasSpan(field3, "type"));
                EXPECT_TRUE(HasSpan(field3, "name"));
                EXPECT_TRUE(HasSpan(field3, "number"));
            }

            TEST_F(SourceInfoTest, ExtensionRanges) {
                EXPECT_TRUE(
                    Parse("message Message {\n"
                          "  $a$extensions $b$1$c$ to $d$4$e$, $f$6$g$;$h$\n"
                          "  $i$extensions $j$8$k$ to $l$max$m$;$n$\n"
                          "}\n"));

                const DescriptorProto::ExtensionRange& range1 = file_.message_type(0).extension_range(0);
                const DescriptorProto::ExtensionRange& range2 = file_.message_type(0).extension_range(1);
                const DescriptorProto::ExtensionRange& range3 = file_.message_type(0).extension_range(2);

                EXPECT_TRUE(HasSpan('a', 'h', file_.message_type(0), "extension_range"));
                EXPECT_TRUE(HasSpan('i', 'n', file_.message_type(0), "extension_range"));

                EXPECT_TRUE(HasSpan('b', 'e', range1));
                EXPECT_TRUE(HasSpan('b', 'c', range1, "start"));
                EXPECT_TRUE(HasSpan('d', 'e', range1, "end"));

                EXPECT_TRUE(HasSpan('f', 'g', range2));
                EXPECT_TRUE(HasSpan('f', 'g', range2, "start"));
                EXPECT_TRUE(HasSpan('f', 'g', range2, "end"));

                EXPECT_TRUE(HasSpan('j', 'm', range3));
                EXPECT_TRUE(HasSpan('j', 'k', range3, "start"));
                EXPECT_TRUE(HasSpan('l', 'm', range3, "end"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
            }

            TEST_F(SourceInfoTest, ReservedRanges) {
                EXPECT_TRUE(
                    Parse("message Message {\n"
                          "  $a$reserved $b$1$c$ to $d$4$e$, $f$6$g$;$h$\n"
                          "}\n"));

                const DescriptorProto::ReservedRange& range1 = file_.message_type(0).reserved_range(0);
                const DescriptorProto::ReservedRange& range2 = file_.message_type(0).reserved_range(1);

                EXPECT_TRUE(HasSpan('a', 'h', file_.message_type(0), "reserved_range"));

                EXPECT_TRUE(HasSpan('b', 'e', range1));
                EXPECT_TRUE(HasSpan('b', 'c', range1, "start"));
                EXPECT_TRUE(HasSpan('d', 'e', range1, "end"));

                EXPECT_TRUE(HasSpan('f', 'g', range2));
                EXPECT_TRUE(HasSpan('f', 'g', range2, "start"));
                EXPECT_TRUE(HasSpan('f', 'g', range2, "end"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
            }

            TEST_F(SourceInfoTest, Oneofs) {
                EXPECT_TRUE(
                    Parse("message Foo {\n"
                          "  $a$oneof $c$foo$d$ {\n"
                          "    $e$int32$f$ $g$a$h$ = $i$1$j$;$k$\n"
                          "  }$r$\n"
                          "}\n"));

                const OneofDescriptorProto& oneof_decl = file_.message_type(0).oneof_decl(0);
                const FieldDescriptorProto& field = file_.message_type(0).field(0);

                EXPECT_TRUE(HasSpan('a', 'r', oneof_decl));
                EXPECT_TRUE(HasSpan('c', 'd', oneof_decl, "name"));

                EXPECT_TRUE(HasSpan('e', 'k', field));
                EXPECT_TRUE(HasSpan('e', 'f', field, "type"));
                EXPECT_TRUE(HasSpan('g', 'h', field, "name"));
                EXPECT_TRUE(HasSpan('i', 'j', field, "number"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
            }

            TEST_F(SourceInfoTest, NestedMessages) {
                EXPECT_TRUE(
                    Parse("message Foo {\n"
                          "  $a$message $b$Bar$c$ {\n"
                          "    $d$message $e$Baz$f$ {}$g$\n"
                          "  }$h$\n"
                          "  $i$message $j$Qux$k$ {}$l$\n"
                          "}\n"));

                const DescriptorProto& bar = file_.message_type(0).nested_type(0);
                const DescriptorProto& baz = bar.nested_type(0);
                const DescriptorProto& qux = file_.message_type(0).nested_type(1);

                EXPECT_TRUE(HasSpan('a', 'h', bar));
                EXPECT_TRUE(HasSpan('b', 'c', bar, "name"));
                EXPECT_TRUE(HasSpan('d', 'g', baz));
                EXPECT_TRUE(HasSpan('e', 'f', baz, "name"));
                EXPECT_TRUE(HasSpan('i', 'l', qux));
                EXPECT_TRUE(HasSpan('j', 'k', qux, "name"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
            }

            TEST_F(SourceInfoTest, Services) {
                EXPECT_TRUE(
                    Parse("$a$service $b$Foo$c$ {}$d$\n"
                          "$e$service $f$Bar$g$ {}$h$\n"));

                EXPECT_TRUE(HasSpan('a', 'd', file_.service(0)));
                EXPECT_TRUE(HasSpan('b', 'c', file_.service(0), "name"));
                EXPECT_TRUE(HasSpan('e', 'h', file_.service(1)));
                EXPECT_TRUE(HasSpan('f', 'g', file_.service(1), "name"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
            }

            TEST_F(SourceInfoTest, MethodsAndStreams) {
                EXPECT_TRUE(
                    Parse("service Foo {\n"
                          "  $a$rpc $b$Bar$c$($d$X$e$) returns($f$Y$g$);$h$"
                          "  $i$rpc $j$Baz$k$($l$Z$m$) returns($n$W$o$);$p$"
                          "}"));

                const MethodDescriptorProto& bar = file_.service(0).method(0);
                const MethodDescriptorProto& baz = file_.service(0).method(1);

                EXPECT_TRUE(HasSpan('a', 'h', bar));
                EXPECT_TRUE(HasSpan('b', 'c', bar, "name"));
                EXPECT_TRUE(HasSpan('d', 'e', bar, "input_type"));
                EXPECT_TRUE(HasSpan('f', 'g', bar, "output_type"));

                EXPECT_TRUE(HasSpan('i', 'p', baz));
                EXPECT_TRUE(HasSpan('j', 'k', baz, "name"));
                EXPECT_TRUE(HasSpan('l', 'm', baz, "input_type"));
                EXPECT_TRUE(HasSpan('n', 'o', baz, "output_type"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.service(0)));
                EXPECT_TRUE(HasSpan(file_.service(0), "name"));
            }

            TEST_F(SourceInfoTest, Options) {
                EXPECT_TRUE(
                    Parse("$a$option $b$foo$c$.$d$($e$bar.baz$f$)$g$ = "
                          "$h$123$i$;$j$\n"
                          "$k$option qux = $l$-123$m$;$n$\n"
                          "$o$option corge = $p$abc$q$;$r$\n"
                          "$s$option grault = $t$'blah'$u$;$v$\n"
                          "$w$option garply = $x${ yadda yadda }$y$;$z$\n"
                          "$0$option waldo = $1$123.0$2$;$3$\n"));

                const UninterpretedOption& option1 = file_.options().uninterpreted_option(0);
                const UninterpretedOption& option2 = file_.options().uninterpreted_option(1);
                const UninterpretedOption& option3 = file_.options().uninterpreted_option(2);
                const UninterpretedOption& option4 = file_.options().uninterpreted_option(3);
                const UninterpretedOption& option5 = file_.options().uninterpreted_option(4);
                const UninterpretedOption& option6 = file_.options().uninterpreted_option(5);

                EXPECT_TRUE(HasSpan('a', 'j', file_.options()));
                EXPECT_TRUE(HasSpan('a', 'j', option1));
                EXPECT_TRUE(HasSpan('b', 'g', option1, "name"));
                EXPECT_TRUE(HasSpan('b', 'c', option1.name(0)));
                EXPECT_TRUE(HasSpan('b', 'c', option1.name(0), "name_part"));
                EXPECT_TRUE(HasSpan('d', 'g', option1.name(1)));
                EXPECT_TRUE(HasSpan('e', 'f', option1.name(1), "name_part"));
                EXPECT_TRUE(HasSpan('h', 'i', option1, "positive_int_value"));

                EXPECT_TRUE(HasSpan('k', 'n', file_.options()));
                EXPECT_TRUE(HasSpan('l', 'm', option2, "negative_int_value"));

                EXPECT_TRUE(HasSpan('o', 'r', file_.options()));
                EXPECT_TRUE(HasSpan('p', 'q', option3, "identifier_value"));

                EXPECT_TRUE(HasSpan('s', 'v', file_.options()));
                EXPECT_TRUE(HasSpan('t', 'u', option4, "string_value"));

                EXPECT_TRUE(HasSpan('w', 'z', file_.options()));
                EXPECT_TRUE(HasSpan('x', 'y', option5, "aggregate_value"));

                EXPECT_TRUE(HasSpan('0', '3', file_.options()));
                EXPECT_TRUE(HasSpan('1', '2', option6, "double_value"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(option2));
                EXPECT_TRUE(HasSpan(option3));
                EXPECT_TRUE(HasSpan(option4));
                EXPECT_TRUE(HasSpan(option5));
                EXPECT_TRUE(HasSpan(option6));
                EXPECT_TRUE(HasSpan(option2, "name"));
                EXPECT_TRUE(HasSpan(option3, "name"));
                EXPECT_TRUE(HasSpan(option4, "name"));
                EXPECT_TRUE(HasSpan(option5, "name"));
                EXPECT_TRUE(HasSpan(option6, "name"));
                EXPECT_TRUE(HasSpan(option2.name(0)));
                EXPECT_TRUE(HasSpan(option3.name(0)));
                EXPECT_TRUE(HasSpan(option4.name(0)));
                EXPECT_TRUE(HasSpan(option5.name(0)));
                EXPECT_TRUE(HasSpan(option6.name(0)));
                EXPECT_TRUE(HasSpan(option2.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option3.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option4.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option5.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option6.name(0), "name_part"));
            }

            TEST_F(SourceInfoTest, ScopedOptions) {
                EXPECT_TRUE(
                    Parse("message Foo {\n"
                          "  $a$option mopt = 1;$b$\n"
                          "}\n"
                          "enum Bar {\n"
                          "  $c$option eopt = 1;$d$\n"
                          "}\n"
                          "service Baz {\n"
                          "  $e$option sopt = 1;$f$\n"
                          "  rpc M(X) returns(Y) {\n"
                          "    $g$option mopt = 1;$h$\n"
                          "  }\n"
                          "  rpc MS4($1$stream$2$ X) returns($3$stream$4$ Y) {\n"
                          "    $k$option mopt = 1;$l$\n"
                          "  }\n"
                          "}\n"));

                EXPECT_TRUE(HasSpan('a', 'b', file_.message_type(0).options()));
                EXPECT_TRUE(HasSpan('c', 'd', file_.enum_type(0).options()));
                EXPECT_TRUE(HasSpan('e', 'f', file_.service(0).options()));
                EXPECT_TRUE(HasSpan('g', 'h', file_.service(0).method(0).options()));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
                EXPECT_TRUE(HasSpan(file_.message_type(0).options().uninterpreted_option(0)));
                EXPECT_TRUE(
                    HasSpan(file_.message_type(0).options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(
                    HasSpan(file_.message_type(0).options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(
                    HasSpan(file_.message_type(0).options().uninterpreted_option(0).name(0),
                        "name_part"));
                EXPECT_TRUE(HasSpan(file_.message_type(0).options().uninterpreted_option(0),
                    "positive_int_value"));
                EXPECT_TRUE(HasSpan(file_.enum_type(0)));
                EXPECT_TRUE(HasSpan(file_.enum_type(0), "name"));
                EXPECT_TRUE(HasSpan(file_.enum_type(0).options().uninterpreted_option(0)));
                EXPECT_TRUE(
                    HasSpan(file_.enum_type(0).options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(
                    HasSpan(file_.enum_type(0).options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(
                    HasSpan(file_.enum_type(0).options().uninterpreted_option(0).name(0),
                        "name_part"));
                EXPECT_TRUE(HasSpan(file_.enum_type(0).options().uninterpreted_option(0),
                    "positive_int_value"));
                EXPECT_TRUE(HasSpan(file_.service(0)));
                EXPECT_TRUE(HasSpan(file_.service(0), "name"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(0)));
                EXPECT_TRUE(HasSpan(file_.service(0).options().uninterpreted_option(0)));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).options().uninterpreted_option(0).name(0), "name_part"));
                EXPECT_TRUE(HasSpan(file_.service(0).options().uninterpreted_option(0),
                    "positive_int_value"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(0), "name"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(0), "input_type"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(0), "output_type"));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).method(0).options().uninterpreted_option(0)));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(0).options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(0).options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(0).options().uninterpreted_option(0).name(0),
                    "name_part"));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).method(0).options().uninterpreted_option(0),
                        "positive_int_value"));

                EXPECT_TRUE(HasSpan('k', 'l', file_.service(0).method(1).options()));
                EXPECT_TRUE(HasSpan(file_.service(0).method(1)));
                EXPECT_TRUE(HasSpan(file_.service(0).method(1), "name"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(1), "input_type"));
                EXPECT_TRUE(HasSpan(file_.service(0).method(1), "output_type"));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).method(1).options().uninterpreted_option(0)));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(1).options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(1).options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(HasSpan(
                    file_.service(0).method(1).options().uninterpreted_option(0).name(0),
                    "name_part"));
                EXPECT_TRUE(
                    HasSpan(file_.service(0).method(1).options().uninterpreted_option(0),
                        "positive_int_value"));
                EXPECT_TRUE(
                    HasSpan('1', '2', file_.service(0).method(1), "client_streaming"));
                EXPECT_TRUE(
                    HasSpan('3', '4', file_.service(0).method(1), "server_streaming"));
            }

            TEST_F(SourceInfoTest, FieldOptions) {
                // The actual "name = value" pairs are parsed by the same code as for
                // top-level options so we won't re-test that -- just make sure that the
                // syntax used for field options is understood.
                EXPECT_TRUE(
                    Parse("message Foo {"
                          "  optional int32 bar = 1 "
                          "$a$[default=$b$123$c$,$d$opt1=123$e$,"
                          "$f$opt2='hi'$g$]$h$;"
                          "}\n"));

                const FieldDescriptorProto& field = file_.message_type(0).field(0);
                const UninterpretedOption& option1 = field.options().uninterpreted_option(0);
                const UninterpretedOption& option2 = field.options().uninterpreted_option(1);

                EXPECT_TRUE(HasSpan('a', 'h', field.options()));
                EXPECT_TRUE(HasSpan('b', 'c', field, "default_value"));
                EXPECT_TRUE(HasSpan('d', 'e', option1));
                EXPECT_TRUE(HasSpan('f', 'g', option2));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(file_.message_type(0)));
                EXPECT_TRUE(HasSpan(file_.message_type(0), "name"));
                EXPECT_TRUE(HasSpan(field));
                EXPECT_TRUE(HasSpan(field, "label"));
                EXPECT_TRUE(HasSpan(field, "type"));
                EXPECT_TRUE(HasSpan(field, "name"));
                EXPECT_TRUE(HasSpan(field, "number"));
                EXPECT_TRUE(HasSpan(option1, "name"));
                EXPECT_TRUE(HasSpan(option2, "name"));
                EXPECT_TRUE(HasSpan(option1.name(0)));
                EXPECT_TRUE(HasSpan(option2.name(0)));
                EXPECT_TRUE(HasSpan(option1.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option2.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(option1, "positive_int_value"));
                EXPECT_TRUE(HasSpan(option2, "string_value"));
            }

            TEST_F(SourceInfoTest, DocComments) {
                EXPECT_TRUE(
                    Parse("// Foo leading\n"
                          "// line 2\n"
                          "$a$message Foo {\n"
                          "  // Foo trailing\n"
                          "  // line 2\n"
                          "\n"
                          "  // detached\n"
                          "\n"
                          "  // bar leading\n"
                          "  $b$optional int32 bar = 1;$c$\n"
                          "  // bar trailing\n"
                          "}$d$\n"
                          "// ignored\n"));

                const DescriptorProto& foo = file_.message_type(0);
                const FieldDescriptorProto& bar = foo.field(0);

                EXPECT_TRUE(HasSpanWithComment('a', 'd', foo, " Foo leading\n line 2\n",
                    " Foo trailing\n line 2\n", nullptr));
                EXPECT_TRUE(HasSpanWithComment('b', 'c', bar, " bar leading\n",
                    " bar trailing\n", " detached\n"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(foo, "name"));
                EXPECT_TRUE(HasSpan(bar, "label"));
                EXPECT_TRUE(HasSpan(bar, "type"));
                EXPECT_TRUE(HasSpan(bar, "name"));
                EXPECT_TRUE(HasSpan(bar, "number"));
            }

            TEST_F(SourceInfoTest, DocComments2) {
                EXPECT_TRUE(
                    Parse("// detached before message.\n"
                          "\n"
                          "// Foo leading\n"
                          "// line 2\n"
                          "$a$message Foo {\n"
                          "  /* Foo trailing\n"
                          "   * line 2 */\n"
                          "  // detached\n"
                          "  /* bar leading\n"
                          "   */"
                          "  $b$optional int32 bar = 1;$c$  // bar trailing\n"
                          "  // ignored detached\n"
                          "}$d$\n"
                          "// ignored\n"
                          "\n"
                          "// detached before option\n"
                          "\n"
                          "// option leading\n"
                          "$e$option baz = 123;$f$\n"
                          "// option trailing\n"));

                const DescriptorProto& foo = file_.message_type(0);
                const FieldDescriptorProto& bar = foo.field(0);
                const UninterpretedOption& baz = file_.options().uninterpreted_option(0);

                EXPECT_TRUE(HasSpanWithComment('a', 'd', foo, " Foo leading\n line 2\n",
                    " Foo trailing\n line 2 ",
                    " detached before message.\n"));
                EXPECT_TRUE(HasSpanWithComment('b', 'c', bar, " bar leading\n",
                    " bar trailing\n", " detached\n"));
                EXPECT_TRUE(HasSpanWithComment('e', 'f', baz, " option leading\n",
                    " option trailing\n",
                    " detached before option\n"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(foo, "name"));
                EXPECT_TRUE(HasSpan(bar, "label"));
                EXPECT_TRUE(HasSpan(bar, "type"));
                EXPECT_TRUE(HasSpan(bar, "name"));
                EXPECT_TRUE(HasSpan(bar, "number"));
                EXPECT_TRUE(HasSpan(file_.options()));
                EXPECT_TRUE(HasSpan(baz, "name"));
                EXPECT_TRUE(HasSpan(baz.name(0)));
                EXPECT_TRUE(HasSpan(baz.name(0), "name_part"));
                EXPECT_TRUE(HasSpan(baz, "positive_int_value"));
            }

            TEST_F(SourceInfoTest, DocComments3) {
                EXPECT_TRUE(
                    Parse("$a$message Foo {\n"
                          "  // bar leading\n"
                          "  $b$optional int32 bar = 1 [(baz.qux) = {}];$c$\n"
                          "  // bar trailing\n"
                          "}$d$\n"
                          "// ignored\n"));

                const DescriptorProto& foo = file_.message_type(0);
                const FieldDescriptorProto& bar = foo.field(0);

                EXPECT_TRUE(HasSpanWithComment('b', 'c', bar, " bar leading\n",
                    " bar trailing\n", nullptr));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(foo));
                EXPECT_TRUE(HasSpan(foo, "name"));
                EXPECT_TRUE(HasSpan(bar, "label"));
                EXPECT_TRUE(HasSpan(bar, "type"));
                EXPECT_TRUE(HasSpan(bar, "name"));
                EXPECT_TRUE(HasSpan(bar, "number"));
                EXPECT_TRUE(HasSpan(bar.options()));
                EXPECT_TRUE(HasSpan(bar.options().uninterpreted_option(0)));
                EXPECT_TRUE(HasSpan(bar.options().uninterpreted_option(0), "name"));
                EXPECT_TRUE(HasSpan(bar.options().uninterpreted_option(0).name(0)));
                EXPECT_TRUE(
                    HasSpan(bar.options().uninterpreted_option(0).name(0), "name_part"));
                EXPECT_TRUE(
                    HasSpan(bar.options().uninterpreted_option(0), "aggregate_value"));
            }

            TEST_F(SourceInfoTest, DocCommentsTopLevel) {
                EXPECT_TRUE(
                    Parse("// detached before syntax paragraph 1\n"
                          "\n"
                          "// detached before syntax paragraph 2\n"
                          "\n"
                          "// syntax leading\n"
                          "$a$syntax = \"proto2\";$b$\n"
                          "// syntax trailing\n"
                          "\n"
                          "// syntax-package detached comments\n"
                          "\n"
                          ";\n"
                          "\n"
                          "// detached after empty before package\n"
                          "\n"
                          "// package leading\n"
                          "$c$package foo;$d$\n"
                          "// package trailing\n"
                          "\n"
                          "// ignored detach\n"
                          "\n"));

                EXPECT_TRUE(HasSpan('a', 'b', file_, "syntax", -1, " syntax leading\n",
                    " syntax trailing\n",
                    " detached before syntax paragraph 1\n"
                    "\n"
                    " detached before syntax paragraph 2\n"));
                EXPECT_TRUE(HasSpan('c', 'd', file_, "package", -1, " package leading\n",
                    " package trailing\n",
                    " syntax-package detached comments\n"
                    "\n"
                    " detached after empty before package\n"));

                // ignore these.
                EXPECT_TRUE(HasSpan(file_));
            }

            TEST_F(SourceInfoTest, DocCommentsOneof) {
                EXPECT_TRUE(
                    Parse("// Foo leading\n"
                          "$a$message Foo {\n"
                          "  /* Foo trailing\n"
                          "   */\n"
                          "  // detached before oneof\n"
                          "  /* bar leading\n"
                          "   * line 2 */\n"
                          "  $b$oneof bar {\n"
                          "  /* bar trailing\n"
                          "   * line 2 */\n"
                          "  // detached before bar_int\n"
                          "  /* bar_int leading\n"
                          "   */\n"
                          "  $c$int32 bar_int = 1;$d$  // bar_int trailing\n"
                          "  // detach comment ignored\n"
                          "  }$e$\n"
                          "}$f$\n"));

                const DescriptorProto& foo = file_.message_type(0);
                const OneofDescriptorProto& bar = foo.oneof_decl(0);
                const FieldDescriptorProto& bar_int = foo.field(0);

                EXPECT_TRUE(HasSpanWithComment('a', 'f', foo, " Foo leading\n",
                    " Foo trailing\n", nullptr));
                EXPECT_TRUE(HasSpanWithComment('b', 'e', bar, " bar leading\n line 2 ",
                    " bar trailing\n line 2 ",
                    " detached before oneof\n"));
                EXPECT_TRUE(HasSpanWithComment('c', 'd', bar_int, " bar_int leading\n",
                    " bar_int trailing\n",
                    " detached before bar_int\n"));

                // Ignore these.
                EXPECT_TRUE(HasSpan(file_));
                EXPECT_TRUE(HasSpan(foo, "name"));
                EXPECT_TRUE(HasSpan(bar, "name"));
                EXPECT_TRUE(HasSpan(bar_int, "type"));
                EXPECT_TRUE(HasSpan(bar_int, "name"));
                EXPECT_TRUE(HasSpan(bar_int, "number"));
            }

            // ===================================================================

        } // anonymous namespace

    } // namespace compiler
} // namespace shark
