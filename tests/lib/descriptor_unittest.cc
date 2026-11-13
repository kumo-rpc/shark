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
// This file makes extensive use of RFC 3092.  :)

#include <limits>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <shark/parser/importer.h>
#include <shark/parser/parser.h>
#include <shark/descriptor.h>
#include <shark/descriptor.skp.h>
#include <shark/descriptor_database.h>
#include <shark/dynamic_message.h>
#include <shark/io/tokenizer.h>
#include <shark/io/zero_copy_stream_impl.h>
#include <shark/stubs/common.h>
#include <shark/stubs/logging.h>
#include <shark/stubs/stringprintf.h>
#include <shark/stubs/strutil.h>
#include <shark/stubs/substitute.h>
#include <shark/format/text/text_format.h>
#include <shark/types/any.skp.h>
#include <tests/common/googletest.h>
#include <tests/sk/unittest.skp.h>
#include <tests/sk/lib/unittest_custom_options.skp.h>
#include <tests/sk/lib/unittest_lazy_dependencies.skp.h>

// Must be included last.
#include <shark/port_def.inc>

using ::testing::AnyOf;

namespace shark {

    // Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
    namespace descriptor_unittest {

        // Some helpers to make assembling descriptors faster.
        DescriptorProto* AddMessage(FileDescriptorProto* file,
            const std::string& name) {
            DescriptorProto* result = file->add_message_type();
            result->set_name(name);
            return result;
        }

        DescriptorProto* AddNestedMessage(DescriptorProto* parent,
            const std::string& name) {
            DescriptorProto* result = parent->add_nested_type();
            result->set_name(name);
            return result;
        }

        EnumDescriptorProto* AddEnum(FileDescriptorProto* file,
            const std::string& name) {
            EnumDescriptorProto* result = file->add_enum_type();
            result->set_name(name);
            return result;
        }

        EnumDescriptorProto* AddNestedEnum(DescriptorProto* parent,
            const std::string& name) {
            EnumDescriptorProto* result = parent->add_enum_type();
            result->set_name(name);
            return result;
        }

        ServiceDescriptorProto* AddService(FileDescriptorProto* file,
            const std::string& name) {
            ServiceDescriptorProto* result = file->add_service();
            result->set_name(name);
            return result;
        }

        FieldDescriptorProto* AddField(DescriptorProto* parent, const std::string& name,
            int number, FieldCardinality label,
            SharkType type) {
            FieldDescriptorProto* result = parent->add_field();
            result->set_name(name);
            result->set_number(number);
            result->set_label(label);
            result->set_type(type);
            return result;
        }

        FieldDescriptorProto* AddExtension(FileDescriptorProto* file,
            const std::string& extendee,
            const std::string& name, int number,
            FieldCardinality label,
            SharkType type) {
            FieldDescriptorProto* result = file->add_extension();
            result->set_name(name);
            result->set_number(number);
            result->set_label(label);
            result->set_type(type);
            result->set_extendee(extendee);
            return result;
        }

        FieldDescriptorProto* AddNestedExtension(DescriptorProto* parent,
            const std::string& extendee,
            const std::string& name, int number,
            FieldCardinality label,
            SharkType type) {
            FieldDescriptorProto* result = parent->add_extension();
            result->set_name(name);
            result->set_number(number);
            result->set_label(label);
            result->set_type(type);
            result->set_extendee(extendee);
            return result;
        }

        DescriptorProto::ExtensionRange* AddExtensionRange(DescriptorProto* parent,
            int start, int end) {
            DescriptorProto::ExtensionRange* result = parent->add_extension_range();
            result->set_start(start);
            result->set_end(end);
            return result;
        }

        DescriptorProto::ReservedRange* AddReservedRange(DescriptorProto* parent,
            int start, int end) {
            DescriptorProto::ReservedRange* result = parent->add_reserved_range();
            result->set_start(start);
            result->set_end(end);
            return result;
        }

        EnumDescriptorProto::EnumReservedRange* AddReservedRange(
            EnumDescriptorProto* parent, int start, int end) {
            EnumDescriptorProto::EnumReservedRange* result = parent->add_reserved_range();
            result->set_start(start);
            result->set_end(end);
            return result;
        }

        EnumValueDescriptorProto* AddEnumValue(EnumDescriptorProto* enum_proto,
            const std::string& name, int number) {
            EnumValueDescriptorProto* result = enum_proto->add_value();
            result->set_name(name);
            result->set_number(number);
            return result;
        }

        MethodDescriptorProto* AddMethod(ServiceDescriptorProto* service,
            const std::string& name,
            const std::string& input_type,
            const std::string& output_type) {
            MethodDescriptorProto* result = service->add_method();
            result->set_name(name);
            result->set_input_type(input_type);
            result->set_output_type(output_type);
            return result;
        }

        // Empty enums technically aren't allowed.  We need to insert a dummy value
        // into them.
        void AddEmptyEnum(FileDescriptorProto* file, const std::string& name) {
            AddEnumValue(AddEnum(file, name), name + "_DUMMY", 1);
        }

        class MockErrorCollector : public DescriptorPool::ErrorCollector {
        public:
            MockErrorCollector() { }
            ~MockErrorCollector() override { }

            std::string text_;
            std::string warning_text_;

            // implements ErrorCollector ---------------------------------------
            void AddError(const std::string& filename, const std::string& element_name,
                const Message* descriptor, ErrorLocation location,
                const std::string& message) override {
                const char* location_name = nullptr;
                switch (location) {
                case NAME:
                    location_name = "NAME";
                    break;
                case NUMBER:
                    location_name = "NUMBER";
                    break;
                case TYPE:
                    location_name = "TYPE";
                    break;
                case EXTENDEE:
                    location_name = "EXTENDEE";
                    break;
                case DEFAULT_VALUE:
                    location_name = "DEFAULT_VALUE";
                    break;
                case OPTION_NAME:
                    location_name = "OPTION_NAME";
                    break;
                case OPTION_VALUE:
                    location_name = "OPTION_VALUE";
                    break;
                case INPUT_TYPE:
                    location_name = "INPUT_TYPE";
                    break;
                case OUTPUT_TYPE:
                    location_name = "OUTPUT_TYPE";
                    break;
                case IMPORT:
                    location_name = "IMPORT";
                    break;
                case OTHER:
                    location_name = "OTHER";
                    break;
                }

                strings::SubstituteAndAppend(&text_, "$0: $1: $2: $3\n", filename,
                    element_name, location_name, message);
            }

            // implements ErrorCollector ---------------------------------------
            void AddWarning(const std::string& filename, const std::string& element_name,
                const Message* descriptor, ErrorLocation location,
                const std::string& message) override {
                const char* location_name = nullptr;
                switch (location) {
                case NAME:
                    location_name = "NAME";
                    break;
                case NUMBER:
                    location_name = "NUMBER";
                    break;
                case TYPE:
                    location_name = "TYPE";
                    break;
                case EXTENDEE:
                    location_name = "EXTENDEE";
                    break;
                case DEFAULT_VALUE:
                    location_name = "DEFAULT_VALUE";
                    break;
                case OPTION_NAME:
                    location_name = "OPTION_NAME";
                    break;
                case OPTION_VALUE:
                    location_name = "OPTION_VALUE";
                    break;
                case INPUT_TYPE:
                    location_name = "INPUT_TYPE";
                    break;
                case OUTPUT_TYPE:
                    location_name = "OUTPUT_TYPE";
                    break;
                case IMPORT:
                    location_name = "IMPORT";
                    break;
                case OTHER:
                    location_name = "OTHER";
                    break;
                }

                strings::SubstituteAndAppend(&warning_text_, "$0: $1: $2: $3\n", filename,
                    element_name, location_name, message);
            }
        };

        // ===================================================================

        // Test simple files.
        class FileDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   // in "foo.shark"
                //   message FooMessage { extensions 1; }
                //   enum FooEnum {FOO_ENUM_VALUE = 1;}
                //   service FooService {}
                //   extend FooMessage { optional int32 foo_extension = 1; }
                //
                //   // in "bar.shark"
                //   package bar_package;
                //   message BarMessage { extensions 1; }
                //   enum BarEnum {BAR_ENUM_VALUE = 1;}
                //   service BarService {}
                //   extend BarMessage { optional int32 bar_extension = 1; }
                //
                // Also, we have an empty file "baz.shark".  This file's purpose is to
                // make sure that even though it has the same package as foo.shark,
                // searching it for members of foo.shark won't work.

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");
                AddExtensionRange(AddMessage(&foo_file, "FooMessage"), 1, 2);
                AddEnumValue(AddEnum(&foo_file, "FooEnum"), "FOO_ENUM_VALUE", 1);
                AddService(&foo_file, "FooService");
                AddExtension(&foo_file, "FooMessage", "foo_extension", 1,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                FileDescriptorProto bar_file;
                bar_file.set_name("bar.shark");
                bar_file.set_package("bar_package");
                bar_file.add_dependency("foo.shark");
                AddExtensionRange(AddMessage(&bar_file, "BarMessage"), 1, 2);
                AddEnumValue(AddEnum(&bar_file, "BarEnum"), "BAR_ENUM_VALUE", 1);
                AddService(&bar_file, "BarService");
                AddExtension(&bar_file, "bar_package.BarMessage", "bar_extension", 1,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                FileDescriptorProto baz_file;
                baz_file.set_name("baz.shark");

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                bar_file_ = pool_.BuildFile(bar_file);
                ASSERT_TRUE(bar_file_ != nullptr);

                baz_file_ = pool_.BuildFile(baz_file);
                ASSERT_TRUE(baz_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->message_type_count());
                foo_message_ = foo_file_->message_type(0);
                ASSERT_EQ(1, foo_file_->enum_type_count());
                foo_enum_ = foo_file_->enum_type(0);
                ASSERT_EQ(1, foo_enum_->value_count());
                foo_enum_value_ = foo_enum_->value(0);
                ASSERT_EQ(1, foo_file_->service_count());
                foo_service_ = foo_file_->service(0);
                ASSERT_EQ(1, foo_file_->extension_count());
                foo_extension_ = foo_file_->extension(0);

                ASSERT_EQ(1, bar_file_->message_type_count());
                bar_message_ = bar_file_->message_type(0);
                ASSERT_EQ(1, bar_file_->enum_type_count());
                bar_enum_ = bar_file_->enum_type(0);
                ASSERT_EQ(1, bar_enum_->value_count());
                bar_enum_value_ = bar_enum_->value(0);
                ASSERT_EQ(1, bar_file_->service_count());
                bar_service_ = bar_file_->service(0);
                ASSERT_EQ(1, bar_file_->extension_count());
                bar_extension_ = bar_file_->extension(0);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;
            const FileDescriptor* bar_file_;
            const FileDescriptor* baz_file_;

            const Descriptor* foo_message_;
            const EnumDescriptor* foo_enum_;
            const EnumValueDescriptor* foo_enum_value_;
            const ServiceDescriptor* foo_service_;
            const FieldDescriptor* foo_extension_;

            const Descriptor* bar_message_;
            const EnumDescriptor* bar_enum_;
            const EnumValueDescriptor* bar_enum_value_;
            const ServiceDescriptor* bar_service_;
            const FieldDescriptor* bar_extension_;
        };

        TEST_F(FileDescriptorTest, Name) {
            EXPECT_EQ("foo.shark", foo_file_->name());
            EXPECT_EQ("bar.shark", bar_file_->name());
            EXPECT_EQ("baz.shark", baz_file_->name());
        }

        TEST_F(FileDescriptorTest, Package) {
            EXPECT_EQ("", foo_file_->package());
            EXPECT_EQ("bar_package", bar_file_->package());
        }

        TEST_F(FileDescriptorTest, Dependencies) {
            EXPECT_EQ(0, foo_file_->dependency_count());
            EXPECT_EQ(1, bar_file_->dependency_count());
            EXPECT_EQ(foo_file_, bar_file_->dependency(0));
        }

        TEST_F(FileDescriptorTest, FindMessageTypeByName) {
            EXPECT_EQ(foo_message_, foo_file_->FindMessageTypeByName("FooMessage"));
            EXPECT_EQ(bar_message_, bar_file_->FindMessageTypeByName("BarMessage"));

            EXPECT_TRUE(foo_file_->FindMessageTypeByName("BarMessage") == nullptr);
            EXPECT_TRUE(bar_file_->FindMessageTypeByName("FooMessage") == nullptr);
            EXPECT_TRUE(baz_file_->FindMessageTypeByName("FooMessage") == nullptr);

            EXPECT_TRUE(foo_file_->FindMessageTypeByName("NoSuchMessage") == nullptr);
            EXPECT_TRUE(foo_file_->FindMessageTypeByName("FooEnum") == nullptr);
        }

        TEST_F(FileDescriptorTest, FindEnumTypeByName) {
            EXPECT_EQ(foo_enum_, foo_file_->FindEnumTypeByName("FooEnum"));
            EXPECT_EQ(bar_enum_, bar_file_->FindEnumTypeByName("BarEnum"));

            EXPECT_TRUE(foo_file_->FindEnumTypeByName("BarEnum") == nullptr);
            EXPECT_TRUE(bar_file_->FindEnumTypeByName("FooEnum") == nullptr);
            EXPECT_TRUE(baz_file_->FindEnumTypeByName("FooEnum") == nullptr);

            EXPECT_TRUE(foo_file_->FindEnumTypeByName("NoSuchEnum") == nullptr);
            EXPECT_TRUE(foo_file_->FindEnumTypeByName("FooMessage") == nullptr);
        }

        TEST_F(FileDescriptorTest, FindEnumValueByName) {
            EXPECT_EQ(foo_enum_value_, foo_file_->FindEnumValueByName("FooEnum.FOO_ENUM_VALUE"));
            EXPECT_EQ(bar_enum_value_, bar_file_->FindEnumValueByName("BarEnum.BAR_ENUM_VALUE"));

            EXPECT_TRUE(foo_file_->FindEnumValueByName("BAR_ENUM_VALUE") == nullptr);
            EXPECT_TRUE(bar_file_->FindEnumValueByName("FOO_ENUM_VALUE") == nullptr);
            EXPECT_TRUE(baz_file_->FindEnumValueByName("FOO_ENUM_VALUE") == nullptr);

            EXPECT_TRUE(foo_file_->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(foo_file_->FindEnumValueByName("FooMessage") == nullptr);
        }

        TEST_F(FileDescriptorTest, FindServiceByName) {
            EXPECT_EQ(foo_service_, foo_file_->FindServiceByName("FooService"));
            EXPECT_EQ(bar_service_, bar_file_->FindServiceByName("BarService"));

            EXPECT_TRUE(foo_file_->FindServiceByName("BarService") == nullptr);
            EXPECT_TRUE(bar_file_->FindServiceByName("FooService") == nullptr);
            EXPECT_TRUE(baz_file_->FindServiceByName("FooService") == nullptr);

            EXPECT_TRUE(foo_file_->FindServiceByName("NoSuchService") == nullptr);
            EXPECT_TRUE(foo_file_->FindServiceByName("FooMessage") == nullptr);
        }

        TEST_F(FileDescriptorTest, FindExtensionByName) {
            EXPECT_EQ(foo_extension_, foo_file_->FindExtensionByName("foo_extension"));
            EXPECT_EQ(bar_extension_, bar_file_->FindExtensionByName("bar_extension"));

            EXPECT_TRUE(foo_file_->FindExtensionByName("bar_extension") == nullptr);
            EXPECT_TRUE(bar_file_->FindExtensionByName("foo_extension") == nullptr);
            EXPECT_TRUE(baz_file_->FindExtensionByName("foo_extension") == nullptr);

            EXPECT_TRUE(foo_file_->FindExtensionByName("no_such_extension") == nullptr);
            EXPECT_TRUE(foo_file_->FindExtensionByName("FooMessage") == nullptr);
        }

        TEST_F(FileDescriptorTest, FindExtensionByNumber) {
            EXPECT_EQ(foo_extension_, pool_.FindExtensionByNumber(foo_message_, 1));
            EXPECT_EQ(bar_extension_, pool_.FindExtensionByNumber(bar_message_, 1));

            EXPECT_TRUE(pool_.FindExtensionByNumber(foo_message_, 2) == nullptr);
        }

        TEST_F(FileDescriptorTest, BuildAgain) {
            // Test that if we call BuildFile again on the same input we get the same
            // FileDescriptor back.
            FileDescriptorProto file;
            foo_file_->CopyTo(&file);
            EXPECT_EQ(foo_file_, pool_.BuildFile(file));

            // But if we change the file then it won't work.
            file.set_package("some.other.package");
            EXPECT_TRUE(pool_.BuildFile(file) == nullptr);
        }

        TEST_F(FileDescriptorTest, BuildAgainWithSyntax) {
            // Test that if we call BuildFile again on the same input we get the same
            // FileDescriptor back even if syntax param is specified.
            FileDescriptorProto proto_syntax2;
            proto_syntax2.set_name("foo_syntax2");
            proto_syntax2.set_syntax("proto2");

            const FileDescriptor* proto2_descriptor = pool_.BuildFile(proto_syntax2);
            EXPECT_TRUE(proto2_descriptor != nullptr);
            EXPECT_EQ(proto2_descriptor, pool_.BuildFile(proto_syntax2));

            FileDescriptorProto implicit_proto2;
            implicit_proto2.set_name("foo_implicit_syntax2");

            const FileDescriptor* implicit_proto2_descriptor = pool_.BuildFile(implicit_proto2);
            EXPECT_TRUE(implicit_proto2_descriptor != nullptr);
            // We get the same FileDescriptor back if syntax param is explicitly
            // specified.
            implicit_proto2.set_syntax("proto2");
            EXPECT_EQ(implicit_proto2_descriptor, pool_.BuildFile(implicit_proto2));

            FileDescriptorProto proto_syntax3;
            proto_syntax3.set_name("foo_syntax3");
            proto_syntax3.set_syntax("proto3");

            EXPECT_TRUE(pool_.BuildFile(proto_syntax3) == nullptr);
            EXPECT_EQ(3, static_cast<int>(SyntaxEdition::SYNTAX_PROTO3));
        }

        TEST_F(FileDescriptorTest, Syntax) {
            FileDescriptorProto proto;
            proto.set_name("foo");
            // Enable the test when we also populate the syntax for proto2.
#if 0
  {
    proto.set_syntax("proto2");
    DescriptorPool pool;
    const FileDescriptor* file = pool.BuildFile(proto);
    EXPECT_TRUE(file != nullptr);
    EXPECT_EQ(FileDescriptor::SYNTAX_PROTO2, file->syntax());
    FileDescriptorProto other;
    file->CopyTo(&other);
    EXPECT_EQ("proto2", other.syntax());
  }
#endif
            {
                proto.set_syntax("proto3");
                DescriptorPool pool;
                EXPECT_TRUE(pool.BuildFile(proto) == nullptr);
                EXPECT_EQ(3, static_cast<int>(SyntaxEdition::SYNTAX_PROTO3));
            }
        }

        void ExtractDebugString(
            const FileDescriptor* file, std::set<std::string>* visited,
            std::vector<std::pair<std::string, std::string>>* debug_strings) {
            if (!visited->insert(file->name()).second) {
                return;
            }
            for (int i = 0; i < file->dependency_count(); ++i) {
                ExtractDebugString(file->dependency(i), visited, debug_strings);
            }
            debug_strings->push_back(std::make_pair(file->name(), file->DebugString()));
        }

        class SimpleErrorCollector : public io::ErrorCollector {
        public:
            // implements ErrorCollector ---------------------------------------
            void AddError(int line, int column, const std::string& message) override {
                last_error_ = StringPrintf("%d:%d:", line, column) + message;
            }

            const std::string& last_error() { return last_error_; }

        private:
            std::string last_error_;
        };
        // Test that the result of FileDescriptor::DebugString() can be used to create
        // the original descriptors.
        TEST_F(FileDescriptorTest, DebugStringRoundTrip) {
            std::set<std::string> visited;
            std::vector<std::pair<std::string, std::string>> debug_strings;
            ExtractDebugString(sharkbuf_unittest::TestAllTypes::descriptor()->file(),
                &visited, &debug_strings);
            ExtractDebugString(
                sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file(),
                &visited, &debug_strings);
            ASSERT_GE(debug_strings.size(), 2);

            DescriptorPool pool;
            for (int i = 0; i < debug_strings.size(); ++i) {
                const std::string& name = debug_strings[i].first;
                const std::string& content = debug_strings[i].second;
                io::ArrayInputStream input_stream(content.data(), content.size());
                SimpleErrorCollector error_collector;
                io::Tokenizer tokenizer(&input_stream, &error_collector);
                compiler::Parser parser;
                parser.RecordErrorsTo(&error_collector);
                FileDescriptorProto proto;
                ASSERT_TRUE(parser.Parse(&tokenizer, &proto))
                    << error_collector.last_error() << "\n"
                    << content;
                ASSERT_EQ("", error_collector.last_error());
                proto.set_name(name);
                const FileDescriptor* descriptor = pool.BuildFile(proto);
                ASSERT_TRUE(descriptor != nullptr) << proto.DebugString();
                EXPECT_EQ(content, descriptor->DebugString());
            }
        }

        // ===================================================================

        // Test simple flat messages and fields.
        class DescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   // in "foo.shark"
                //   message TestForeign {}
                //   enum TestEnum {}
                //
                //   message TestMessage {
                //     required string      foo = 1;
                //     optional TestEnum    bar = 6;
                //     repeated TestForeign baz = 500000000;
                //     optional group       moo = 15 {}
                //   }
                //
                //   // in "bar.shark"
                //   package corge.grault;
                //   message TestMessage2 {
                //     required string foo = 1;
                //     required string bar = 2;
                //     required string mooo = 6;
                //   }
                //
                //   // in "map.shark"
                //   message TestMessage3 {
                //     map<int32, int32> map_int32_int32 = 1;
                //   }
                //
                //   // in "json.shark"
                //   message TestMessage4 {
                //     optional int32 field_name1 = 1;
                //     optional int32 fieldName2 = 2;
                //     optional int32 FieldName3 = 3;
                //     optional int32 _field_name4 = 4;
                //     optional int32 FIELD_NAME5 = 5;
                //     optional int32 field_name6 = 6 [json_name = "@type"];
                //   }
                //
                // We cheat and use TestForeign as the type for moo rather than create
                // an actual nested type.
                //
                // Since all primitive types (including string) use the same building
                // code, there's no need to test each one individually.
                //
                // TestMessage2 is primarily here to test FindFieldByName and friends.
                // All messages created from the same DescriptorPool share the same lookup
                // table, so we need to insure that they don't interfere.

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");
                AddMessage(&foo_file, "TestForeign");
                AddEmptyEnum(&foo_file, "TestEnum");

                DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
                AddField(message, "foo", 1, FieldCardinality::REQUIRED,
                    SharkType::STRING);
                AddField(message, "bar", 6, FieldCardinality::OPTIONAL,
                    SharkType::ENUM)
                    ->set_type_name("TestEnum");
                AddField(message, "baz", 500000000, FieldCardinality::REPEATED,
                    SharkType::MESSAGE)
                    ->set_type_name("TestForeign");
                AddField(message, "moo", 15, FieldCardinality::OPTIONAL,
                    SharkType::MESSAGE)
                    ->set_type_name("TestForeign");

                FileDescriptorProto bar_file;
                bar_file.set_name("bar.shark");
                bar_file.set_package("corge.grault");

                DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
                AddField(message2, "foo", 1, FieldCardinality::REQUIRED,
                    SharkType::STRING);
                AddField(message2, "bar", 2, FieldCardinality::REQUIRED,
                    SharkType::STRING);
                AddField(message2, "mooo", 6, FieldCardinality::REQUIRED,
                    SharkType::STRING);

                FileDescriptorProto map_file;
                map_file.set_name("map.shark");
                DescriptorProto* message3 = AddMessage(&map_file, "TestMessage3");

                DescriptorProto* entry = AddNestedMessage(message3, "MapInt32Int32Entry");
                AddField(entry, "key", 1, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(entry, "value", 2, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                entry->mutable_options()->set_map_entry(true);

                AddField(message3, "map_int32_int32", 1,
                    FieldCardinality::REPEATED,
                    SharkType::MESSAGE)
                    ->set_type_name("MapInt32Int32Entry");

                FileDescriptorProto json_file;
                json_file.set_name("json.shark");
                json_file.set_syntax("proto2");
                DescriptorProto* message4 = AddMessage(&json_file, "TestMessage4");
                AddField(message4, "field_name1", 1, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message4, "fieldName2", 2, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message4, "FieldName3", 3, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message4, "_field_name4", 4, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message4, "FIELD_NAME5", 5, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message4, "field_name6", 6, FieldCardinality::OPTIONAL,
                    SharkType::INT32)
                    ->set_json_name("@type");
                AddField(message4, "fieldname7", 7, FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                bar_file_ = pool_.BuildFile(bar_file);
                ASSERT_TRUE(bar_file_ != nullptr);

                map_file_ = pool_.BuildFile(map_file);
                ASSERT_TRUE(map_file_ != nullptr);

                json_file_ = pool_.BuildFile(json_file);
                ASSERT_TRUE(json_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->enum_type_count());
                enum_ = foo_file_->enum_type(0);

                ASSERT_EQ(2, foo_file_->message_type_count());
                foreign_ = foo_file_->message_type(0);
                message_ = foo_file_->message_type(1);

                ASSERT_EQ(4, message_->field_count());
                foo_ = message_->field(0);
                bar_ = message_->field(1);
                baz_ = message_->field(2);
                moo_ = message_->field(3);

                ASSERT_EQ(1, bar_file_->message_type_count());
                message2_ = bar_file_->message_type(0);

                ASSERT_EQ(3, message2_->field_count());
                foo2_ = message2_->field(0);
                bar2_ = message2_->field(1);
                mooo2_ = message2_->field(2);

                ASSERT_EQ(1, map_file_->message_type_count());
                message3_ = map_file_->message_type(0);

                ASSERT_EQ(1, message3_->field_count());
                map_ = message3_->field(0);

                ASSERT_EQ(1, json_file_->message_type_count());
                message4_ = json_file_->message_type(0);
            }

            void CopyWithJsonName(const Descriptor* message, DescriptorProto* proto) {
                message->CopyTo(proto);
                message->CopyJsonNameTo(proto);
            }

            const EnumValueDescriptor* FindValueByNumberCreatingIfUnknown(
                const EnumDescriptor* desc, int number) {
                return desc->FindValueByNumberCreatingIfUnknown(number);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;
            const FileDescriptor* bar_file_;
            const FileDescriptor* map_file_;
            const FileDescriptor* json_file_;

            const Descriptor* message_;
            const Descriptor* message2_;
            const Descriptor* message3_;
            const Descriptor* message4_;
            const Descriptor* foreign_;
            const EnumDescriptor* enum_;

            const FieldDescriptor* foo_;
            const FieldDescriptor* bar_;
            const FieldDescriptor* baz_;
            const FieldDescriptor* moo_;

            const FieldDescriptor* foo2_;
            const FieldDescriptor* bar2_;
            const FieldDescriptor* mooo2_;

            const FieldDescriptor* map_;
        };

        TEST_F(DescriptorTest, Name) {
            EXPECT_EQ("TestMessage", message_->name());
            EXPECT_EQ("TestMessage", message_->full_name());
            EXPECT_EQ(foo_file_, message_->file());

            EXPECT_EQ("TestMessage2", message2_->name());
            EXPECT_EQ("corge.grault.TestMessage2", message2_->full_name());
            EXPECT_EQ(bar_file_, message2_->file());
        }

        TEST_F(DescriptorTest, ContainingType) {
            EXPECT_TRUE(message_->containing_type() == nullptr);
            EXPECT_TRUE(message2_->containing_type() == nullptr);
        }

        TEST_F(DescriptorTest, FieldNamesDedup) {
            const auto collect_unique_names = [](const FieldDescriptor* field) {
                std::set<std::string> names { field->name(), field->lowercase_name(),
                    field->camelcase_name(), field->json_name() };
                // Verify that we have the same number of string objects as we have string
                // values. That is, duplicate names use the same std::string object.
                // This is for memory efficiency.
                EXPECT_EQ(names.size(), (std::set<const std::string*> { &field->name(), &field->lowercase_name(), &field->camelcase_name(), &field->json_name() }.size()))
                    << testing::PrintToString(names);
                return names;
            };

            using testing::ElementsAre;
            // field_name1
            EXPECT_THAT(collect_unique_names(message4_->field(0)),
                ElementsAre("fieldName1", "field_name1"));
            // fieldName2
            EXPECT_THAT(collect_unique_names(message4_->field(1)),
                ElementsAre("fieldName2", "fieldname2"));
            // FieldName3
            EXPECT_THAT(collect_unique_names(message4_->field(2)),
                ElementsAre("FieldName3", "fieldName3", "fieldname3"));
            // _field_name4
            EXPECT_THAT(collect_unique_names(message4_->field(3)),
                ElementsAre("FieldName4", "_field_name4", "fieldName4"));
            // FIELD_NAME5
            EXPECT_THAT(
                collect_unique_names(message4_->field(4)),
                ElementsAre("FIELDNAME5", "FIELD_NAME5", "fIELDNAME5", "field_name5"));
            // field_name6, with json name @type
            EXPECT_THAT(collect_unique_names(message4_->field(5)),
                ElementsAre("@type", "fieldName6", "field_name6"));
            // fieldname7
            EXPECT_THAT(collect_unique_names(message4_->field(6)),
                ElementsAre("fieldname7"));
        }

        TEST_F(DescriptorTest, FieldNameDedupJsonEqFull) {
            // Test a regression where json_name == full_name
            FileDescriptorProto proto;
            proto.set_name("file");
            auto* message = AddMessage(&proto, "Name1");
            auto* field = AddField(message, "Name2", 1, FieldCardinality::OPTIONAL,
                SharkType::INT32);
            field->set_json_name("Name1.Name2");
            auto* file = pool_.BuildFile(proto);
            EXPECT_EQ(file->message_type(0)->name(), "Name1");
            EXPECT_EQ(file->message_type(0)->field(0)->name(), "Name2");
            EXPECT_EQ(file->message_type(0)->field(0)->full_name(), "Name1.Name2");
            EXPECT_EQ(file->message_type(0)->field(0)->json_name(), "Name1.Name2");
        }

        TEST_F(DescriptorTest, FieldsByIndex) {
            ASSERT_EQ(4, message_->field_count());
            EXPECT_EQ(foo_, message_->field(0));
            EXPECT_EQ(bar_, message_->field(1));
            EXPECT_EQ(baz_, message_->field(2));
            EXPECT_EQ(moo_, message_->field(3));
        }

        TEST_F(DescriptorTest, FindFieldByName) {
            // All messages in the same DescriptorPool share a single lookup table for
            // fields.  So, in addition to testing that FindFieldByName finds the fields
            // of the message, we need to test that it does *not* find the fields of
            // *other* messages.

            EXPECT_EQ(foo_, message_->FindFieldByName("foo"));
            EXPECT_EQ(bar_, message_->FindFieldByName("bar"));
            EXPECT_EQ(baz_, message_->FindFieldByName("baz"));
            EXPECT_EQ(moo_, message_->FindFieldByName("moo"));
            EXPECT_TRUE(message_->FindFieldByName("no_such_field") == nullptr);
            EXPECT_TRUE(message_->FindFieldByName("mooo") == nullptr);

            EXPECT_EQ(foo2_, message2_->FindFieldByName("foo"));
            EXPECT_EQ(bar2_, message2_->FindFieldByName("bar"));
            EXPECT_EQ(mooo2_, message2_->FindFieldByName("mooo"));
            EXPECT_TRUE(message2_->FindFieldByName("baz") == nullptr);
            EXPECT_TRUE(message2_->FindFieldByName("moo") == nullptr);
        }

        TEST_F(DescriptorTest, FindFieldByNumber) {
            EXPECT_EQ(foo_, message_->FindFieldByNumber(1));
            EXPECT_EQ(bar_, message_->FindFieldByNumber(6));
            EXPECT_EQ(baz_, message_->FindFieldByNumber(500000000));
            EXPECT_EQ(moo_, message_->FindFieldByNumber(15));
            EXPECT_TRUE(message_->FindFieldByNumber(837592) == nullptr);
            EXPECT_TRUE(message_->FindFieldByNumber(2) == nullptr);

            EXPECT_EQ(foo2_, message2_->FindFieldByNumber(1));
            EXPECT_EQ(bar2_, message2_->FindFieldByNumber(2));
            EXPECT_EQ(mooo2_, message2_->FindFieldByNumber(6));
            EXPECT_TRUE(message2_->FindFieldByNumber(15) == nullptr);
            EXPECT_TRUE(message2_->FindFieldByNumber(500000000) == nullptr);
        }

        TEST_F(DescriptorTest, FieldName) {
            EXPECT_EQ("foo", foo_->name());
            EXPECT_EQ("bar", bar_->name());
            EXPECT_EQ("baz", baz_->name());
            EXPECT_EQ("moo", moo_->name());
        }

        TEST_F(DescriptorTest, FieldFullName) {
            EXPECT_EQ("TestMessage.foo", foo_->full_name());
            EXPECT_EQ("TestMessage.bar", bar_->full_name());
            EXPECT_EQ("TestMessage.baz", baz_->full_name());
            EXPECT_EQ("TestMessage.moo", moo_->full_name());

            EXPECT_EQ("corge.grault.TestMessage2.foo", foo2_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.bar", bar2_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.mooo", mooo2_->full_name());
        }

        TEST_F(DescriptorTest, PrintableNameIsFullNameForNonExtensionFields) {
            EXPECT_EQ("TestMessage.foo", foo_->PrintableNameForExtension());
            EXPECT_EQ("TestMessage.bar", bar_->PrintableNameForExtension());
            EXPECT_EQ("TestMessage.baz", baz_->PrintableNameForExtension());
            EXPECT_EQ("TestMessage.moo", moo_->PrintableNameForExtension());

            EXPECT_EQ("corge.grault.TestMessage2.foo",
                foo2_->PrintableNameForExtension());
            EXPECT_EQ("corge.grault.TestMessage2.bar",
                bar2_->PrintableNameForExtension());
            EXPECT_EQ("corge.grault.TestMessage2.mooo",
                mooo2_->PrintableNameForExtension());
        }

        TEST_F(DescriptorTest, PrintableNameIsFullNameForNonMessageSetExtension) {
            EXPECT_EQ("sharkbuf_unittest.Aggregate.nested",
                sharkbuf_unittest::Aggregate::descriptor()
                    ->FindExtensionByName("nested")
                    ->PrintableNameForExtension());
        }

        TEST_F(DescriptorTest, PrintableNameIsExtendingTypeForMessageSetExtension) {
            EXPECT_EQ("sharkbuf_unittest.AggregateMessageSetElement",
                sharkbuf_unittest::AggregateMessageSetElement::descriptor()
                    ->FindExtensionByName("message_set_extension")
                    ->PrintableNameForExtension());
        }

        TEST_F(DescriptorTest, FieldJsonName) {
            EXPECT_EQ("fieldName1", message4_->field(0)->json_name());
            EXPECT_EQ("fieldName2", message4_->field(1)->json_name());
            EXPECT_EQ("FieldName3", message4_->field(2)->json_name());
            EXPECT_EQ("FieldName4", message4_->field(3)->json_name());
            EXPECT_EQ("FIELDNAME5", message4_->field(4)->json_name());
            EXPECT_EQ("@type", message4_->field(5)->json_name());

            DescriptorProto proto;
            message4_->CopyTo(&proto);
            ASSERT_EQ(7, proto.field_size());
            EXPECT_FALSE(proto.field(0).has_json_name());
            EXPECT_FALSE(proto.field(1).has_json_name());
            EXPECT_FALSE(proto.field(2).has_json_name());
            EXPECT_FALSE(proto.field(3).has_json_name());
            EXPECT_FALSE(proto.field(4).has_json_name());
            EXPECT_EQ("@type", proto.field(5).json_name());
            EXPECT_FALSE(proto.field(6).has_json_name());

            proto.Clear();
            CopyWithJsonName(message4_, &proto);
            ASSERT_EQ(7, proto.field_size());
            EXPECT_EQ("fieldName1", proto.field(0).json_name());
            EXPECT_EQ("fieldName2", proto.field(1).json_name());
            EXPECT_EQ("FieldName3", proto.field(2).json_name());
            EXPECT_EQ("FieldName4", proto.field(3).json_name());
            EXPECT_EQ("FIELDNAME5", proto.field(4).json_name());
            EXPECT_EQ("@type", proto.field(5).json_name());
            EXPECT_EQ("fieldname7", proto.field(6).json_name());

            // Test generated descriptor.
            const Descriptor* generated = sharkbuf_unittest::TestJsonName::descriptor();
            ASSERT_EQ(7, generated->field_count());
            EXPECT_EQ("fieldName1", generated->field(0)->json_name());
            EXPECT_EQ("fieldName2", generated->field(1)->json_name());
            EXPECT_EQ("FieldName3", generated->field(2)->json_name());
            EXPECT_EQ("FieldName4", generated->field(3)->json_name());
            EXPECT_EQ("FIELDNAME5", generated->field(4)->json_name());
            EXPECT_EQ("@type", generated->field(5)->json_name());
            EXPECT_EQ("fieldname7", generated->field(6)->json_name());
        }

        TEST_F(DescriptorTest, FieldFile) {
            EXPECT_EQ(foo_file_, foo_->file());
            EXPECT_EQ(foo_file_, bar_->file());
            EXPECT_EQ(foo_file_, baz_->file());
            EXPECT_EQ(foo_file_, moo_->file());

            EXPECT_EQ(bar_file_, foo2_->file());
            EXPECT_EQ(bar_file_, bar2_->file());
            EXPECT_EQ(bar_file_, mooo2_->file());
        }

        TEST_F(DescriptorTest, FieldIndex) {
            EXPECT_EQ(0, foo_->index());
            EXPECT_EQ(1, bar_->index());
            EXPECT_EQ(2, baz_->index());
            EXPECT_EQ(3, moo_->index());
        }

        TEST_F(DescriptorTest, FieldNumber) {
            EXPECT_EQ(1, foo_->number());
            EXPECT_EQ(6, bar_->number());
            EXPECT_EQ(500000000, baz_->number());
            EXPECT_EQ(15, moo_->number());
        }

        TEST_F(DescriptorTest, SharkFieldType) {
            EXPECT_EQ(SharkFieldType::STRING, foo_->type());
            EXPECT_EQ(SharkFieldType::ENUM, bar_->type());
            EXPECT_EQ(SharkFieldType::MESSAGE, baz_->type());
            EXPECT_EQ(SharkFieldType::MESSAGE, moo_->type());
        }

        TEST_F(DescriptorTest, FieldLabel) {
            EXPECT_EQ(FieldLabel::REQUIRED, foo_->label());
            EXPECT_EQ(FieldLabel::OPTIONAL, bar_->label());
            EXPECT_EQ(FieldLabel::REPEATED, baz_->label());
            EXPECT_EQ(FieldLabel::OPTIONAL, moo_->label());

            EXPECT_TRUE(foo_->is_required());
            EXPECT_FALSE(foo_->is_optional());
            EXPECT_FALSE(foo_->is_repeated());

            EXPECT_FALSE(bar_->is_required());
            EXPECT_TRUE(bar_->is_optional());
            EXPECT_FALSE(bar_->is_repeated());

            EXPECT_FALSE(baz_->is_required());
            EXPECT_FALSE(baz_->is_optional());
            EXPECT_TRUE(baz_->is_repeated());
        }

        TEST_F(DescriptorTest, IsMap) {
            EXPECT_TRUE(map_->is_map());
            EXPECT_FALSE(baz_->is_map());
            EXPECT_TRUE(map_->message_type()->options().map_entry());
        }

        TEST_F(DescriptorTest, GetMap) {
            const Descriptor* map_desc = map_->message_type();
            const FieldDescriptor* map_key = map_desc->map_key();
            ASSERT_TRUE(map_key != nullptr);
            EXPECT_EQ(map_key->name(), "key");
            EXPECT_EQ(map_key->number(), 1);

            const FieldDescriptor* map_value = map_desc->map_value();
            ASSERT_TRUE(map_value != nullptr);
            EXPECT_EQ(map_value->name(), "value");
            EXPECT_EQ(map_value->number(), 2);

            EXPECT_EQ(message_->map_key(), nullptr);
            EXPECT_EQ(message_->map_value(), nullptr);
        }

        TEST_F(DescriptorTest, FieldHasDefault) {
            EXPECT_FALSE(foo_->has_default_value());
            EXPECT_FALSE(bar_->has_default_value());
            EXPECT_FALSE(baz_->has_default_value());
            EXPECT_FALSE(moo_->has_default_value());
        }

        TEST_F(DescriptorTest, FieldContainingType) {
            EXPECT_EQ(message_, foo_->containing_type());
            EXPECT_EQ(message_, bar_->containing_type());
            EXPECT_EQ(message_, baz_->containing_type());
            EXPECT_EQ(message_, moo_->containing_type());

            EXPECT_EQ(message2_, foo2_->containing_type());
            EXPECT_EQ(message2_, bar2_->containing_type());
            EXPECT_EQ(message2_, mooo2_->containing_type());
        }

        TEST_F(DescriptorTest, FieldMessageType) {
            EXPECT_TRUE(foo_->message_type() == nullptr);
            EXPECT_TRUE(bar_->message_type() == nullptr);

            EXPECT_EQ(foreign_, baz_->message_type());
            EXPECT_EQ(foreign_, moo_->message_type());
        }

        TEST_F(DescriptorTest, FieldEnumType) {
            EXPECT_TRUE(foo_->enum_type() == nullptr);
            EXPECT_TRUE(baz_->enum_type() == nullptr);
            EXPECT_TRUE(moo_->enum_type() == nullptr);

            EXPECT_EQ(enum_, bar_->enum_type());
        }

        // ===================================================================

        // Test simple flat messages and fields.
        class OneofDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   package garply;
                //   message TestOneof {
                //     optional int32 a = 1;
                //     oneof foo {
                //       string b = 2;
                //       TestOneof c = 3;
                //     }
                //     oneof bar {
                //       float d = 4;
                //     }
                //   }

                FileDescriptorProto baz_file;
                baz_file.set_name("baz.shark");
                baz_file.set_package("garply");

                DescriptorProto* oneof_message = AddMessage(&baz_file, "TestOneof");
                oneof_message->add_oneof_decl()->set_name("foo");
                oneof_message->add_oneof_decl()->set_name("bar");

                AddField(oneof_message, "a", 1, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(oneof_message, "b", 2, FieldCardinality::OPTIONAL,
                    SharkType::STRING);
                oneof_message->mutable_field(1)->set_oneof_index(0);
                AddField(oneof_message, "c", 3, FieldCardinality::OPTIONAL,
                    SharkType::MESSAGE);
                oneof_message->mutable_field(2)->set_oneof_index(0);
                oneof_message->mutable_field(2)->set_type_name("TestOneof");

                AddField(oneof_message, "d", 4, FieldCardinality::OPTIONAL,
                    SharkType::FLOAT);
                oneof_message->mutable_field(3)->set_oneof_index(1);

                // Build the descriptors and get the pointers.
                baz_file_ = pool_.BuildFile(baz_file);
                ASSERT_TRUE(baz_file_ != nullptr);

                ASSERT_EQ(1, baz_file_->message_type_count());
                oneof_message_ = baz_file_->message_type(0);

                ASSERT_EQ(2, oneof_message_->oneof_decl_count());
                oneof_ = oneof_message_->oneof_decl(0);
                oneof2_ = oneof_message_->oneof_decl(1);

                ASSERT_EQ(4, oneof_message_->field_count());
                a_ = oneof_message_->field(0);
                b_ = oneof_message_->field(1);
                c_ = oneof_message_->field(2);
                d_ = oneof_message_->field(3);
            }

            DescriptorPool pool_;

            const FileDescriptor* baz_file_;

            const Descriptor* oneof_message_;

            const OneofDescriptor* oneof_;
            const OneofDescriptor* oneof2_;
            const FieldDescriptor* a_;
            const FieldDescriptor* b_;
            const FieldDescriptor* c_;
            const FieldDescriptor* d_;
        };

        TEST_F(OneofDescriptorTest, Normal) {
            EXPECT_EQ("foo", oneof_->name());
            EXPECT_EQ("garply.TestOneof.foo", oneof_->full_name());
            EXPECT_EQ(0, oneof_->index());
            ASSERT_EQ(2, oneof_->field_count());
            EXPECT_EQ(b_, oneof_->field(0));
            EXPECT_EQ(c_, oneof_->field(1));
            EXPECT_TRUE(a_->containing_oneof() == nullptr);
            EXPECT_EQ(oneof_, b_->containing_oneof());
            EXPECT_EQ(oneof_, c_->containing_oneof());
        }

        TEST_F(OneofDescriptorTest, FindByName) {
            EXPECT_EQ(oneof_, oneof_message_->FindOneofByName("foo"));
            EXPECT_EQ(oneof2_, oneof_message_->FindOneofByName("bar"));
            EXPECT_TRUE(oneof_message_->FindOneofByName("no_such_oneof") == nullptr);
        }

        // ===================================================================

        class StylizedFieldNamesTest : public testing::Test {
        protected:
            void SetUp() override {
                FileDescriptorProto file;
                file.set_name("foo.shark");

                AddExtensionRange(AddMessage(&file, "ExtendableMessage"), 1, 1000);

                DescriptorProto* message = AddMessage(&file, "TestMessage");
                AddField(message, "foo_foo", 1, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message, "FooBar", 2, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message, "fooBaz", 3, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message, "fooFoo", 4, // Camel-case conflict with foo_foo.
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddField(message, "foobar", 5, // Lower-case conflict with FooBar.
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                AddNestedExtension(message, "ExtendableMessage", "bar_foo", 1,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddNestedExtension(message, "ExtendableMessage", "BarBar", 2,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddNestedExtension(message, "ExtendableMessage", "BarBaz", 3,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddNestedExtension(message, "ExtendableMessage", "barFoo", 4, // Conflict
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddNestedExtension(message, "ExtendableMessage", "barbar", 5, // Conflict
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                AddExtension(&file, "ExtendableMessage", "baz_foo", 11,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddExtension(&file, "ExtendableMessage", "BazBar", 12,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddExtension(&file, "ExtendableMessage", "BazBaz", 13,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddExtension(&file, "ExtendableMessage", "bazFoo", 14, // Conflict
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddExtension(&file, "ExtendableMessage", "bazbar", 15, // Conflict
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);

                file_ = pool_.BuildFile(file);
                ASSERT_TRUE(file_ != nullptr);
                ASSERT_EQ(2, file_->message_type_count());
                message_ = file_->message_type(1);
                ASSERT_EQ("TestMessage", message_->name());
                ASSERT_EQ(5, message_->field_count());
                ASSERT_EQ(5, message_->extension_count());
                ASSERT_EQ(5, file_->extension_count());
            }

            DescriptorPool pool_;
            const FileDescriptor* file_;
            const Descriptor* message_;
        };

        TEST_F(StylizedFieldNamesTest, LowercaseName) {
            EXPECT_EQ("foo_foo", message_->field(0)->lowercase_name());
            EXPECT_EQ("foobar", message_->field(1)->lowercase_name());
            EXPECT_EQ("foobaz", message_->field(2)->lowercase_name());
            EXPECT_EQ("foofoo", message_->field(3)->lowercase_name());
            EXPECT_EQ("foobar", message_->field(4)->lowercase_name());

            EXPECT_EQ("bar_foo", message_->extension(0)->lowercase_name());
            EXPECT_EQ("barbar", message_->extension(1)->lowercase_name());
            EXPECT_EQ("barbaz", message_->extension(2)->lowercase_name());
            EXPECT_EQ("barfoo", message_->extension(3)->lowercase_name());
            EXPECT_EQ("barbar", message_->extension(4)->lowercase_name());

            EXPECT_EQ("baz_foo", file_->extension(0)->lowercase_name());
            EXPECT_EQ("bazbar", file_->extension(1)->lowercase_name());
            EXPECT_EQ("bazbaz", file_->extension(2)->lowercase_name());
            EXPECT_EQ("bazfoo", file_->extension(3)->lowercase_name());
            EXPECT_EQ("bazbar", file_->extension(4)->lowercase_name());
        }

        TEST_F(StylizedFieldNamesTest, CamelcaseName) {
            EXPECT_EQ("fooFoo", message_->field(0)->camelcase_name());
            EXPECT_EQ("fooBar", message_->field(1)->camelcase_name());
            EXPECT_EQ("fooBaz", message_->field(2)->camelcase_name());
            EXPECT_EQ("fooFoo", message_->field(3)->camelcase_name());
            EXPECT_EQ("foobar", message_->field(4)->camelcase_name());

            EXPECT_EQ("barFoo", message_->extension(0)->camelcase_name());
            EXPECT_EQ("barBar", message_->extension(1)->camelcase_name());
            EXPECT_EQ("barBaz", message_->extension(2)->camelcase_name());
            EXPECT_EQ("barFoo", message_->extension(3)->camelcase_name());
            EXPECT_EQ("barbar", message_->extension(4)->camelcase_name());

            EXPECT_EQ("bazFoo", file_->extension(0)->camelcase_name());
            EXPECT_EQ("bazBar", file_->extension(1)->camelcase_name());
            EXPECT_EQ("bazBaz", file_->extension(2)->camelcase_name());
            EXPECT_EQ("bazFoo", file_->extension(3)->camelcase_name());
            EXPECT_EQ("bazbar", file_->extension(4)->camelcase_name());
        }

        TEST_F(StylizedFieldNamesTest, FindByLowercaseName) {
            EXPECT_EQ(message_->field(0), message_->FindFieldByLowercaseName("foo_foo"));
            EXPECT_THAT(message_->FindFieldByLowercaseName("foobar"),
                AnyOf(message_->field(1), message_->field(4)));
            EXPECT_EQ(message_->field(2), message_->FindFieldByLowercaseName("foobaz"));
            EXPECT_TRUE(message_->FindFieldByLowercaseName("FooBar") == nullptr);
            EXPECT_TRUE(message_->FindFieldByLowercaseName("fooBaz") == nullptr);
            EXPECT_TRUE(message_->FindFieldByLowercaseName("bar_foo") == nullptr);
            EXPECT_TRUE(message_->FindFieldByLowercaseName("nosuchfield") == nullptr);

            EXPECT_EQ(message_->extension(0),
                message_->FindExtensionByLowercaseName("bar_foo"));
            EXPECT_THAT(message_->FindExtensionByLowercaseName("barbar"),
                AnyOf(message_->extension(1), message_->extension(4)));
            EXPECT_EQ(message_->extension(2),
                message_->FindExtensionByLowercaseName("barbaz"));
            EXPECT_TRUE(message_->FindExtensionByLowercaseName("BarBar") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByLowercaseName("barBaz") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByLowercaseName("foo_foo") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByLowercaseName("nosuchfield") == nullptr);

            EXPECT_EQ(file_->extension(0),
                file_->FindExtensionByLowercaseName("baz_foo"));
            EXPECT_THAT(file_->FindExtensionByLowercaseName("bazbar"),
                AnyOf(file_->extension(1), file_->extension(4)));
            EXPECT_EQ(file_->extension(2), file_->FindExtensionByLowercaseName("bazbaz"));
            EXPECT_TRUE(file_->FindExtensionByLowercaseName("BazBar") == nullptr);
            EXPECT_TRUE(file_->FindExtensionByLowercaseName("bazBaz") == nullptr);
            EXPECT_TRUE(file_->FindExtensionByLowercaseName("nosuchfield") == nullptr);
        }

        TEST_F(StylizedFieldNamesTest, FindByCamelcaseName) {
            EXPECT_THAT(message_->FindFieldByCamelcaseName("fooFoo"),
                AnyOf(message_->field(0), message_->field(3)));
            EXPECT_EQ(message_->field(1), message_->FindFieldByCamelcaseName("fooBar"));
            EXPECT_EQ(message_->field(2), message_->FindFieldByCamelcaseName("fooBaz"));
            EXPECT_TRUE(message_->FindFieldByCamelcaseName("foo_foo") == nullptr);
            EXPECT_TRUE(message_->FindFieldByCamelcaseName("FooBar") == nullptr);
            EXPECT_TRUE(message_->FindFieldByCamelcaseName("barFoo") == nullptr);
            EXPECT_TRUE(message_->FindFieldByCamelcaseName("nosuchfield") == nullptr);

            EXPECT_THAT(message_->FindExtensionByCamelcaseName("barFoo"),
                AnyOf(message_->extension(0), message_->extension(3)));
            EXPECT_EQ(message_->extension(1),
                message_->FindExtensionByCamelcaseName("barBar"));
            EXPECT_EQ(message_->extension(2),
                message_->FindExtensionByCamelcaseName("barBaz"));
            EXPECT_TRUE(message_->FindExtensionByCamelcaseName("bar_foo") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByCamelcaseName("BarBar") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByCamelcaseName("fooFoo") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByCamelcaseName("nosuchfield") == nullptr);

            EXPECT_THAT(file_->FindExtensionByCamelcaseName("bazFoo"),
                AnyOf(file_->extension(0), file_->extension(3)));
            EXPECT_EQ(file_->extension(1), file_->FindExtensionByCamelcaseName("bazBar"));
            EXPECT_EQ(file_->extension(2), file_->FindExtensionByCamelcaseName("bazBaz"));
            EXPECT_TRUE(file_->FindExtensionByCamelcaseName("baz_foo") == nullptr);
            EXPECT_TRUE(file_->FindExtensionByCamelcaseName("BazBar") == nullptr);
            EXPECT_TRUE(file_->FindExtensionByCamelcaseName("nosuchfield") == nullptr);
        }

        // ===================================================================

        // Test enum descriptors.
        class EnumDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   // in "foo.shark"
                //   enum TestEnum {
                //     FOO = 1;
                //     BAR = 2;
                //   }
                //
                //   // in "bar.shark"
                //   package corge.grault;
                //   enum TestEnum2 {
                //     FOO = 1;
                //     BAZ = 3;
                //   }
                //
                // TestEnum2 is primarily here to test FindValueByName and friends.
                // All enums created from the same DescriptorPool share the same lookup
                // table, so we need to insure that they don't interfere.

                // TestEnum
                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                EnumDescriptorProto* enum_proto = AddEnum(&foo_file, "TestEnum");
                AddEnumValue(enum_proto, "FOO", 1);
                AddEnumValue(enum_proto, "BAR", 2);

                // TestEnum2
                FileDescriptorProto bar_file;
                bar_file.set_name("bar.shark");
                bar_file.set_package("corge.grault");

                EnumDescriptorProto* enum2_proto = AddEnum(&bar_file, "TestEnum2");
                AddEnumValue(enum2_proto, "FOO", 1);
                AddEnumValue(enum2_proto, "BAZ", 3);

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                bar_file_ = pool_.BuildFile(bar_file);
                ASSERT_TRUE(bar_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->enum_type_count());
                enum_ = foo_file_->enum_type(0);

                ASSERT_EQ(2, enum_->value_count());
                foo_ = enum_->value(0);
                bar_ = enum_->value(1);

                ASSERT_EQ(1, bar_file_->enum_type_count());
                enum2_ = bar_file_->enum_type(0);

                ASSERT_EQ(2, enum2_->value_count());
                foo2_ = enum2_->value(0);
                baz2_ = enum2_->value(1);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;
            const FileDescriptor* bar_file_;

            const EnumDescriptor* enum_;
            const EnumDescriptor* enum2_;

            const EnumValueDescriptor* foo_;
            const EnumValueDescriptor* bar_;

            const EnumValueDescriptor* foo2_;
            const EnumValueDescriptor* baz2_;
        };

        TEST_F(EnumDescriptorTest, Name) {
            EXPECT_EQ("TestEnum", enum_->name());
            EXPECT_EQ("TestEnum", enum_->full_name());
            EXPECT_EQ(foo_file_, enum_->file());

            EXPECT_EQ("TestEnum2", enum2_->name());
            EXPECT_EQ("corge.grault.TestEnum2", enum2_->full_name());
            EXPECT_EQ(bar_file_, enum2_->file());
        }

        TEST_F(EnumDescriptorTest, ContainingType) {
            EXPECT_TRUE(enum_->containing_type() == nullptr);
            EXPECT_TRUE(enum2_->containing_type() == nullptr);
        }

        TEST_F(EnumDescriptorTest, ValuesByIndex) {
            ASSERT_EQ(2, enum_->value_count());
            EXPECT_EQ(foo_, enum_->value(0));
            EXPECT_EQ(bar_, enum_->value(1));
        }

        TEST_F(EnumDescriptorTest, FindValueByName) {
            EXPECT_EQ(foo_, enum_->FindValueByName("FOO"));
            EXPECT_EQ(bar_, enum_->FindValueByName("BAR"));
            EXPECT_EQ(foo2_, enum2_->FindValueByName("FOO"));
            EXPECT_EQ(baz2_, enum2_->FindValueByName("BAZ"));

            EXPECT_TRUE(enum_->FindValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(enum_->FindValueByName("BAZ") == nullptr);
            EXPECT_TRUE(enum2_->FindValueByName("BAR") == nullptr);
        }

        TEST_F(EnumDescriptorTest, FindValueByNumber) {
            EXPECT_EQ(foo_, enum_->FindValueByNumber(1));
            EXPECT_EQ(bar_, enum_->FindValueByNumber(2));
            EXPECT_EQ(foo2_, enum2_->FindValueByNumber(1));
            EXPECT_EQ(baz2_, enum2_->FindValueByNumber(3));

            EXPECT_TRUE(enum_->FindValueByNumber(416) == nullptr);
            EXPECT_TRUE(enum_->FindValueByNumber(3) == nullptr);
            EXPECT_TRUE(enum2_->FindValueByNumber(2) == nullptr);
        }

        TEST_F(EnumDescriptorTest, ValueName) {
            EXPECT_EQ("FOO", foo_->name());
            EXPECT_EQ("BAR", bar_->name());
        }

        TEST_F(EnumDescriptorTest, ValueFullName) {
            EXPECT_EQ("TestEnum.FOO", foo_->full_name());
            EXPECT_EQ("TestEnum.BAR", bar_->full_name());
            EXPECT_EQ("corge.grault.TestEnum2.FOO", foo2_->full_name());
            EXPECT_EQ("corge.grault.TestEnum2.BAZ", baz2_->full_name());
        }

        TEST_F(EnumDescriptorTest, ValueIndex) {
            EXPECT_EQ(0, foo_->index());
            EXPECT_EQ(1, bar_->index());
        }

        TEST_F(EnumDescriptorTest, ValueNumber) {
            EXPECT_EQ(1, foo_->number());
            EXPECT_EQ(2, bar_->number());
        }

        TEST_F(EnumDescriptorTest, ValueType) {
            EXPECT_EQ(enum_, foo_->type());
            EXPECT_EQ(enum_, bar_->type());
            EXPECT_EQ(enum2_, foo2_->type());
            EXPECT_EQ(enum2_, baz2_->type());
        }

        // ===================================================================

        // Test service descriptors.
        class ServiceDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following messages and service:
                //    // in "foo.shark"
                //    message FooRequest  {}
                //    message FooResponse {}
                //    message BarRequest  {}
                //    message BarResponse {}
                //    message BazRequest  {}
                //    message BazResponse {}
                //
                //    service TestService {
                //      rpc Foo(FooRequest) returns (FooResponse);
                //      rpc Bar(BarRequest) returns (BarResponse);
                //    }
                //
                //    // in "bar.shark"
                //    package corge.grault
                //    service TestService2 {
                //      rpc Foo(FooRequest) returns (FooResponse);
                //      rpc Baz(BazRequest) returns (BazResponse);
                //    }

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                AddMessage(&foo_file, "FooRequest");
                AddMessage(&foo_file, "FooResponse");
                AddMessage(&foo_file, "BarRequest");
                AddMessage(&foo_file, "BarResponse");
                AddMessage(&foo_file, "BazRequest");
                AddMessage(&foo_file, "BazResponse");

                ServiceDescriptorProto* service = AddService(&foo_file, "TestService");
                AddMethod(service, "Foo", "FooRequest", "FooResponse");
                AddMethod(service, "Bar", "BarRequest", "BarResponse");

                FileDescriptorProto bar_file;
                bar_file.set_name("bar.shark");
                bar_file.set_package("corge.grault");
                bar_file.add_dependency("foo.shark");

                ServiceDescriptorProto* service2 = AddService(&bar_file, "TestService2");
                AddMethod(service2, "Foo", "FooRequest", "FooResponse");
                AddMethod(service2, "Baz", "BazRequest", "BazResponse");

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                bar_file_ = pool_.BuildFile(bar_file);
                ASSERT_TRUE(bar_file_ != nullptr);

                ASSERT_EQ(6, foo_file_->message_type_count());
                foo_request_ = foo_file_->message_type(0);
                foo_response_ = foo_file_->message_type(1);
                bar_request_ = foo_file_->message_type(2);
                bar_response_ = foo_file_->message_type(3);
                baz_request_ = foo_file_->message_type(4);
                baz_response_ = foo_file_->message_type(5);

                ASSERT_EQ(1, foo_file_->service_count());
                service_ = foo_file_->service(0);

                ASSERT_EQ(2, service_->method_count());
                foo_ = service_->method(0);
                bar_ = service_->method(1);

                ASSERT_EQ(1, bar_file_->service_count());
                service2_ = bar_file_->service(0);

                ASSERT_EQ(2, service2_->method_count());
                foo2_ = service2_->method(0);
                baz2_ = service2_->method(1);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;
            const FileDescriptor* bar_file_;

            const Descriptor* foo_request_;
            const Descriptor* foo_response_;
            const Descriptor* bar_request_;
            const Descriptor* bar_response_;
            const Descriptor* baz_request_;
            const Descriptor* baz_response_;

            const ServiceDescriptor* service_;
            const ServiceDescriptor* service2_;

            const MethodDescriptor* foo_;
            const MethodDescriptor* bar_;

            const MethodDescriptor* foo2_;
            const MethodDescriptor* baz2_;
        };

        TEST_F(ServiceDescriptorTest, Name) {
            EXPECT_EQ("TestService", service_->name());
            EXPECT_EQ("TestService", service_->full_name());
            EXPECT_EQ(foo_file_, service_->file());

            EXPECT_EQ("TestService2", service2_->name());
            EXPECT_EQ("corge.grault.TestService2", service2_->full_name());
            EXPECT_EQ(bar_file_, service2_->file());
        }

        TEST_F(ServiceDescriptorTest, MethodsByIndex) {
            ASSERT_EQ(2, service_->method_count());
            EXPECT_EQ(foo_, service_->method(0));
            EXPECT_EQ(bar_, service_->method(1));
        }

        TEST_F(ServiceDescriptorTest, FindMethodByName) {
            EXPECT_EQ(foo_, service_->FindMethodByName("Foo"));
            EXPECT_EQ(bar_, service_->FindMethodByName("Bar"));
            EXPECT_EQ(foo2_, service2_->FindMethodByName("Foo"));
            EXPECT_EQ(baz2_, service2_->FindMethodByName("Baz"));

            EXPECT_TRUE(service_->FindMethodByName("NoSuchMethod") == nullptr);
            EXPECT_TRUE(service_->FindMethodByName("Baz") == nullptr);
            EXPECT_TRUE(service2_->FindMethodByName("Bar") == nullptr);
        }

        TEST_F(ServiceDescriptorTest, MethodName) {
            EXPECT_EQ("Foo", foo_->name());
            EXPECT_EQ("Bar", bar_->name());
        }
        TEST_F(ServiceDescriptorTest, MethodFullName) {
            EXPECT_EQ("TestService.Foo", foo_->full_name());
            EXPECT_EQ("TestService.Bar", bar_->full_name());
            EXPECT_EQ("corge.grault.TestService2.Foo", foo2_->full_name());
            EXPECT_EQ("corge.grault.TestService2.Baz", baz2_->full_name());
        }

        TEST_F(ServiceDescriptorTest, MethodIndex) {
            EXPECT_EQ(0, foo_->index());
            EXPECT_EQ(1, bar_->index());
        }

        TEST_F(ServiceDescriptorTest, MethodParent) {
            EXPECT_EQ(service_, foo_->service());
            EXPECT_EQ(service_, bar_->service());
        }

        TEST_F(ServiceDescriptorTest, MethodInputType) {
            EXPECT_EQ(foo_request_, foo_->input_type());
            EXPECT_EQ(bar_request_, bar_->input_type());
        }

        TEST_F(ServiceDescriptorTest, MethodOutputType) {
            EXPECT_EQ(foo_response_, foo_->output_type());
            EXPECT_EQ(bar_response_, bar_->output_type());
        }

        // ===================================================================

        // Test nested types.
        class NestedDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   // in "foo.shark"
                //   message TestMessage {
                //     message Foo {}
                //     message Bar {}
                //     enum Baz { A = 1; }
                //     enum Moo { B = 1; }
                //   }
                //
                //   // in "bar.shark"
                //   package corge.grault;
                //   message TestMessage2 {
                //     message Foo {}
                //     message Baz {}
                //     enum Moo  { A = 1; }
                //     enum Mooo { C = 1; }
                //   }
                //
                // TestMessage2 is primarily here to test FindNestedTypeByName and friends.
                // All messages created from the same DescriptorPool share the same lookup
                // table, so we need to insure that they don't interfere.
                //
                // We add enum values to the enums in order to test searching for enum
                // values across a message's scope.

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
                AddNestedMessage(message, "Foo");
                AddNestedMessage(message, "Bar");
                EnumDescriptorProto* baz = AddNestedEnum(message, "Baz");
                AddEnumValue(baz, "A", 1);
                EnumDescriptorProto* moo = AddNestedEnum(message, "Moo");
                AddEnumValue(moo, "B", 1);

                FileDescriptorProto bar_file;
                bar_file.set_name("bar.shark");
                bar_file.set_package("corge.grault");

                DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
                AddNestedMessage(message2, "Foo");
                AddNestedMessage(message2, "Baz");
                EnumDescriptorProto* moo2 = AddNestedEnum(message2, "Moo");
                AddEnumValue(moo2, "A", 1);
                EnumDescriptorProto* mooo2 = AddNestedEnum(message2, "Mooo");
                AddEnumValue(mooo2, "C", 1);

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                bar_file_ = pool_.BuildFile(bar_file);
                ASSERT_TRUE(bar_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->message_type_count());
                message_ = foo_file_->message_type(0);

                ASSERT_EQ(2, message_->nested_type_count());
                foo_ = message_->nested_type(0);
                bar_ = message_->nested_type(1);

                ASSERT_EQ(2, message_->enum_type_count());
                baz_ = message_->enum_type(0);
                moo_ = message_->enum_type(1);

                ASSERT_EQ(1, baz_->value_count());
                a_ = baz_->value(0);
                ASSERT_EQ(1, moo_->value_count());
                b_ = moo_->value(0);

                ASSERT_EQ(1, bar_file_->message_type_count());
                message2_ = bar_file_->message_type(0);

                ASSERT_EQ(2, message2_->nested_type_count());
                foo2_ = message2_->nested_type(0);
                baz2_ = message2_->nested_type(1);

                ASSERT_EQ(2, message2_->enum_type_count());
                moo2_ = message2_->enum_type(0);
                mooo2_ = message2_->enum_type(1);

                ASSERT_EQ(1, moo2_->value_count());
                a2_ = moo2_->value(0);
                ASSERT_EQ(1, mooo2_->value_count());
                c2_ = mooo2_->value(0);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;
            const FileDescriptor* bar_file_;

            const Descriptor* message_;
            const Descriptor* message2_;

            const Descriptor* foo_;
            const Descriptor* bar_;
            const EnumDescriptor* baz_;
            const EnumDescriptor* moo_;
            const EnumValueDescriptor* a_;
            const EnumValueDescriptor* b_;

            const Descriptor* foo2_;
            const Descriptor* baz2_;
            const EnumDescriptor* moo2_;
            const EnumDescriptor* mooo2_;
            const EnumValueDescriptor* a2_;
            const EnumValueDescriptor* c2_;
        };

        TEST_F(NestedDescriptorTest, MessageName) {
            EXPECT_EQ("Foo", foo_->name());
            EXPECT_EQ("Bar", bar_->name());
            EXPECT_EQ("Foo", foo2_->name());
            EXPECT_EQ("Baz", baz2_->name());

            EXPECT_EQ("TestMessage.Foo", foo_->full_name());
            EXPECT_EQ("TestMessage.Bar", bar_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.Foo", foo2_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.Baz", baz2_->full_name());
        }

        TEST_F(NestedDescriptorTest, MessageContainingType) {
            EXPECT_EQ(message_, foo_->containing_type());
            EXPECT_EQ(message_, bar_->containing_type());
            EXPECT_EQ(message2_, foo2_->containing_type());
            EXPECT_EQ(message2_, baz2_->containing_type());
        }

        TEST_F(NestedDescriptorTest, NestedMessagesByIndex) {
            ASSERT_EQ(2, message_->nested_type_count());
            EXPECT_EQ(foo_, message_->nested_type(0));
            EXPECT_EQ(bar_, message_->nested_type(1));
        }

        TEST_F(NestedDescriptorTest, FindFieldByNameDoesntFindNestedTypes) {
            EXPECT_TRUE(message_->FindFieldByName("Foo") == nullptr);
            EXPECT_TRUE(message_->FindFieldByName("Moo") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByName("Foo") == nullptr);
            EXPECT_TRUE(message_->FindExtensionByName("Moo") == nullptr);
        }

        TEST_F(NestedDescriptorTest, FindNestedTypeByName) {
            EXPECT_EQ(foo_, message_->FindNestedTypeByName("Foo"));
            EXPECT_EQ(bar_, message_->FindNestedTypeByName("Bar"));
            EXPECT_EQ(foo2_, message2_->FindNestedTypeByName("Foo"));
            EXPECT_EQ(baz2_, message2_->FindNestedTypeByName("Baz"));

            EXPECT_TRUE(message_->FindNestedTypeByName("NoSuchType") == nullptr);
            EXPECT_TRUE(message_->FindNestedTypeByName("Baz") == nullptr);
            EXPECT_TRUE(message2_->FindNestedTypeByName("Bar") == nullptr);

            EXPECT_TRUE(message_->FindNestedTypeByName("Moo") == nullptr);
        }

        TEST_F(NestedDescriptorTest, EnumName) {
            EXPECT_EQ("Baz", baz_->name());
            EXPECT_EQ("Moo", moo_->name());
            EXPECT_EQ("Moo", moo2_->name());
            EXPECT_EQ("Mooo", mooo2_->name());

            EXPECT_EQ("TestMessage.Baz", baz_->full_name());
            EXPECT_EQ("TestMessage.Moo", moo_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.Moo", moo2_->full_name());
            EXPECT_EQ("corge.grault.TestMessage2.Mooo", mooo2_->full_name());
        }

        TEST_F(NestedDescriptorTest, EnumContainingType) {
            EXPECT_EQ(message_, baz_->containing_type());
            EXPECT_EQ(message_, moo_->containing_type());
            EXPECT_EQ(message2_, moo2_->containing_type());
            EXPECT_EQ(message2_, mooo2_->containing_type());
        }

        TEST_F(NestedDescriptorTest, NestedEnumsByIndex) {
            ASSERT_EQ(2, message_->nested_type_count());
            EXPECT_EQ(foo_, message_->nested_type(0));
            EXPECT_EQ(bar_, message_->nested_type(1));
        }

        TEST_F(NestedDescriptorTest, FindEnumTypeByName) {
            EXPECT_EQ(baz_, message_->FindEnumTypeByName("Baz"));
            EXPECT_EQ(moo_, message_->FindEnumTypeByName("Moo"));
            EXPECT_EQ(moo2_, message2_->FindEnumTypeByName("Moo"));
            EXPECT_EQ(mooo2_, message2_->FindEnumTypeByName("Mooo"));

            EXPECT_TRUE(message_->FindEnumTypeByName("NoSuchType") == nullptr);
            EXPECT_TRUE(message_->FindEnumTypeByName("Mooo") == nullptr);
            EXPECT_TRUE(message2_->FindEnumTypeByName("Baz") == nullptr);

            EXPECT_TRUE(message_->FindEnumTypeByName("Foo") == nullptr);
        }

        TEST_F(NestedDescriptorTest, FindEnumValueByName) {
            EXPECT_EQ(a_, message_->FindEnumValueByName("Baz.A"));
            EXPECT_EQ(b_, message_->FindEnumValueByName("Moo.B"));
            EXPECT_EQ(a2_, message2_->FindEnumValueByName("Moo.A"));
            EXPECT_EQ(c2_, message2_->FindEnumValueByName("Mooo.C"));

            EXPECT_TRUE(message_->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(message_->FindEnumValueByName("C") == nullptr);
            EXPECT_TRUE(message2_->FindEnumValueByName("B") == nullptr);

            EXPECT_TRUE(message_->FindEnumValueByName("Foo") == nullptr);
        }

        // ===================================================================

        // Test extensions.
        class ExtensionDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   enum Baz {}
                //   message Moo {}
                //
                //   message Foo {
                //     extensions 10 to 19;
                //     extensions 30 to 39;
                //   }
                //   extend Foo {
                //     optional int32 foo_int32 = 10;
                //   }
                //   extend Foo {
                //     repeated TestEnum foo_enum = 19;
                //   }
                //   message Bar {
                //     optional int32 non_ext_int32 = 1;
                //     extend Foo {
                //       optional Moo foo_message = 30;
                //       repeated Moo foo_group = 39;
                //     }
                //   }

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                AddEmptyEnum(&foo_file, "Baz");
                AddMessage(&foo_file, "Moo");

                DescriptorProto* foo = AddMessage(&foo_file, "Foo");
                AddExtensionRange(foo, 10, 20);
                AddExtensionRange(foo, 30, 40);

                AddExtension(&foo_file, "Foo", "foo_int32", 10,
                    FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddExtension(&foo_file, "Foo", "foo_enum", 19,
                    FieldCardinality::REPEATED,
                    SharkType::ENUM)
                    ->set_type_name("Baz");

                DescriptorProto* bar = AddMessage(&foo_file, "Bar");
                AddField(bar, "non_ext_int32", 1, FieldCardinality::OPTIONAL,
                    SharkType::INT32);
                AddNestedExtension(bar, "Foo", "foo_message", 30,
                    FieldCardinality::OPTIONAL,
                    SharkType::MESSAGE)
                    ->set_type_name("Moo");
                AddNestedExtension(bar, "Foo", "foo_group", 39,
                    FieldCardinality::REPEATED,
                    SharkType::MESSAGE)
                    ->set_type_name("Moo");

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->enum_type_count());
                baz_ = foo_file_->enum_type(0);

                ASSERT_EQ(3, foo_file_->message_type_count());
                moo_ = foo_file_->message_type(0);
                foo_ = foo_file_->message_type(1);
                bar_ = foo_file_->message_type(2);
            }

            DescriptorPool pool_;

            const FileDescriptor* foo_file_;

            const Descriptor* foo_;
            const Descriptor* bar_;
            const EnumDescriptor* baz_;
            const Descriptor* moo_;
        };

        TEST_F(ExtensionDescriptorTest, ExtensionRanges) {
            EXPECT_EQ(0, bar_->extension_range_count());
            ASSERT_EQ(2, foo_->extension_range_count());

            EXPECT_EQ(10, foo_->extension_range(0)->start);
            EXPECT_EQ(30, foo_->extension_range(1)->start);

            EXPECT_EQ(20, foo_->extension_range(0)->end);
            EXPECT_EQ(40, foo_->extension_range(1)->end);
        }

        TEST_F(ExtensionDescriptorTest, Extensions) {
            EXPECT_EQ(0, foo_->extension_count());
            ASSERT_EQ(2, foo_file_->extension_count());
            ASSERT_EQ(2, bar_->extension_count());

            EXPECT_TRUE(foo_file_->extension(0)->is_extension());
            EXPECT_TRUE(foo_file_->extension(1)->is_extension());
            EXPECT_TRUE(bar_->extension(0)->is_extension());
            EXPECT_TRUE(bar_->extension(1)->is_extension());

            EXPECT_EQ("foo_int32", foo_file_->extension(0)->name());
            EXPECT_EQ("foo_enum", foo_file_->extension(1)->name());
            EXPECT_EQ("foo_message", bar_->extension(0)->name());
            EXPECT_EQ("foo_group", bar_->extension(1)->name());

            EXPECT_EQ(10, foo_file_->extension(0)->number());
            EXPECT_EQ(19, foo_file_->extension(1)->number());
            EXPECT_EQ(30, bar_->extension(0)->number());
            EXPECT_EQ(39, bar_->extension(1)->number());

            EXPECT_EQ(SharkFieldType::INT32, foo_file_->extension(0)->type());
            EXPECT_EQ(SharkFieldType::ENUM, foo_file_->extension(1)->type());
            EXPECT_EQ(SharkFieldType::MESSAGE, bar_->extension(0)->type());
            EXPECT_EQ(SharkFieldType::MESSAGE, bar_->extension(1)->type());

            EXPECT_EQ(baz_, foo_file_->extension(1)->enum_type());
            EXPECT_EQ(moo_, bar_->extension(0)->message_type());
            EXPECT_EQ(moo_, bar_->extension(1)->message_type());

            EXPECT_EQ(FieldLabel::OPTIONAL, foo_file_->extension(0)->label());
            EXPECT_EQ(FieldLabel::REPEATED, foo_file_->extension(1)->label());
            EXPECT_EQ(FieldLabel::OPTIONAL, bar_->extension(0)->label());
            EXPECT_EQ(FieldLabel::REPEATED, bar_->extension(1)->label());

            EXPECT_EQ(foo_, foo_file_->extension(0)->containing_type());
            EXPECT_EQ(foo_, foo_file_->extension(1)->containing_type());
            EXPECT_EQ(foo_, bar_->extension(0)->containing_type());
            EXPECT_EQ(foo_, bar_->extension(1)->containing_type());

            EXPECT_TRUE(foo_file_->extension(0)->extension_scope() == nullptr);
            EXPECT_TRUE(foo_file_->extension(1)->extension_scope() == nullptr);
            EXPECT_EQ(bar_, bar_->extension(0)->extension_scope());
            EXPECT_EQ(bar_, bar_->extension(1)->extension_scope());
        }

        TEST_F(ExtensionDescriptorTest, IsExtensionNumber) {
            EXPECT_FALSE(foo_->IsExtensionNumber(9));
            EXPECT_TRUE(foo_->IsExtensionNumber(10));
            EXPECT_TRUE(foo_->IsExtensionNumber(19));
            EXPECT_FALSE(foo_->IsExtensionNumber(20));
            EXPECT_FALSE(foo_->IsExtensionNumber(29));
            EXPECT_TRUE(foo_->IsExtensionNumber(30));
            EXPECT_TRUE(foo_->IsExtensionNumber(39));
            EXPECT_FALSE(foo_->IsExtensionNumber(40));
        }

        TEST_F(ExtensionDescriptorTest, FindExtensionByName) {
            // Note that FileDescriptor::FindExtensionByName() is tested by
            // FileDescriptorTest.
            ASSERT_EQ(2, bar_->extension_count());

            EXPECT_EQ(bar_->extension(0), bar_->FindExtensionByName("foo_message"));
            EXPECT_EQ(bar_->extension(1), bar_->FindExtensionByName("foo_group"));

            EXPECT_TRUE(bar_->FindExtensionByName("no_such_extension") == nullptr);
            EXPECT_TRUE(foo_->FindExtensionByName("foo_int32") == nullptr);
            EXPECT_TRUE(foo_->FindExtensionByName("foo_message") == nullptr);
        }

        TEST_F(ExtensionDescriptorTest, FieldVsExtension) {
            EXPECT_EQ(foo_->FindFieldByName("foo_message"), nullptr);
            EXPECT_EQ(bar_->FindFieldByName("foo_message"), nullptr);
            EXPECT_NE(bar_->FindFieldByName("non_ext_int32"), nullptr);
            EXPECT_EQ(foo_->FindExtensionByName("foo_message"), nullptr);
            EXPECT_NE(bar_->FindExtensionByName("foo_message"), nullptr);
            EXPECT_EQ(bar_->FindExtensionByName("non_ext_int32"), nullptr);
        }

        TEST_F(ExtensionDescriptorTest, FindExtensionByPrintableName) {
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(foo_, "no_such_extension") == nullptr);
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "no_such_extension") == nullptr);

            ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "Bar.foo_message") == nullptr);
            ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "Bar.foo_group") == nullptr);
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_message") == nullptr);
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_group") == nullptr);
            EXPECT_EQ(bar_->FindExtensionByName("foo_message"),
                pool_.FindExtensionByPrintableName(foo_, "Bar.foo_message"));
            EXPECT_EQ(bar_->FindExtensionByName("foo_group"),
                pool_.FindExtensionByPrintableName(foo_, "Bar.foo_group"));

            ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "foo_int32") == nullptr);
            ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "foo_enum") == nullptr);
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_int32") == nullptr);
            EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_enum") == nullptr);
            EXPECT_EQ(foo_file_->FindExtensionByName("foo_int32"),
                pool_.FindExtensionByPrintableName(foo_, "foo_int32"));
            EXPECT_EQ(foo_file_->FindExtensionByName("foo_enum"),
                pool_.FindExtensionByPrintableName(foo_, "foo_enum"));
        }

        TEST_F(ExtensionDescriptorTest, FindAllExtensions) {
            std::vector<const FieldDescriptor*> extensions;
            pool_.FindAllExtensions(foo_, &extensions);
            ASSERT_EQ(4, extensions.size());
            EXPECT_EQ(10, extensions[0]->number());
            EXPECT_EQ(19, extensions[1]->number());
            EXPECT_EQ(30, extensions[2]->number());
            EXPECT_EQ(39, extensions[3]->number());
        }

        TEST_F(ExtensionDescriptorTest, DuplicateFieldNumber) {
            DescriptorPool pool;
            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            // Add "shark/descriptor.shark".
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
            // Add "foo.shark":
            //   import "shark/descriptor.shark";
            //   extend shark.FieldOptions {
            //     optional int32 option1 = 1000;
            //   }
            file_proto.Clear();
            file_proto.set_name("foo.shark");
            file_proto.add_dependency("shark/descriptor.shark");
            AddExtension(&file_proto, "shark.FieldOptions", "option1", 1000,
                FieldCardinality::OPTIONAL,
                SharkType::INT32);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
            // Add "bar.shark":
            //   import "shark/descriptor.shark";
            //   extend shark.FieldOptions {
            //     optional int32 option2 = 1000;
            //   }
            file_proto.Clear();
            file_proto.set_name("bar.shark");
            file_proto.add_dependency("shark/descriptor.shark");
            AddExtension(&file_proto, "shark.FieldOptions", "option2", 1000,
                FieldCardinality::OPTIONAL,
                SharkType::INT32);
            // Currently we only generate a warning for conflicting extension numbers.
            // TODO(xiaofeng): Change it to an error.
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
        }

        // ===================================================================

        // Ensure that overlapping extension ranges are not allowed.
        TEST(OverlappingExtensionRangeTest, ExtensionRangeInternal) {
            // Build descriptors for the following definitions:
            //
            //   message Foo {
            //     extensions 10 to 19;
            //     extensions 15;
            //   }
            FileDescriptorProto foo_file;
            foo_file.set_name("foo.shark");

            DescriptorProto* foo = AddMessage(&foo_file, "Foo");
            AddExtensionRange(foo, 10, 20);
            AddExtensionRange(foo, 15, 16);

            DescriptorPool pool;
            MockErrorCollector error_collector;
            // The extensions ranges are invalid, so the proto shouldn't build.
            ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) == nullptr);
            ASSERT_EQ(
                "foo.shark: Foo: NUMBER: Extension range 15 to 15 overlaps with "
                "already-defined range 10 to 19.\n",
                error_collector.text_);
        }

        TEST(OverlappingExtensionRangeTest, ExtensionRangeAfter) {
            // Build descriptors for the following definitions:
            //
            //   message Foo {
            //     extensions 10 to 19;
            //     extensions 15 to 24;
            //   }
            FileDescriptorProto foo_file;
            foo_file.set_name("foo.shark");

            DescriptorProto* foo = AddMessage(&foo_file, "Foo");
            AddExtensionRange(foo, 10, 20);
            AddExtensionRange(foo, 15, 25);

            DescriptorPool pool;
            MockErrorCollector error_collector;
            // The extensions ranges are invalid, so the proto shouldn't build.
            ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) == nullptr);
            ASSERT_EQ(
                "foo.shark: Foo: NUMBER: Extension range 15 to 24 overlaps with "
                "already-defined range 10 to 19.\n",
                error_collector.text_);
        }

        TEST(OverlappingExtensionRangeTest, ExtensionRangeBefore) {
            // Build descriptors for the following definitions:
            //
            //   message Foo {
            //     extensions 10 to 19;
            //     extensions 5 to 14;
            //   }
            FileDescriptorProto foo_file;
            foo_file.set_name("foo.shark");

            DescriptorProto* foo = AddMessage(&foo_file, "Foo");
            AddExtensionRange(foo, 10, 20);
            AddExtensionRange(foo, 5, 15);

            DescriptorPool pool;
            MockErrorCollector error_collector;
            // The extensions ranges are invalid, so the proto shouldn't build.
            ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) == nullptr);
            ASSERT_EQ(
                "foo.shark: Foo: NUMBER: Extension range 5 to 14 overlaps with "
                "already-defined range 10 to 19.\n",
                error_collector.text_);
        }

        // ===================================================================

        // Test reserved fields.
        class ReservedDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   message Foo {
                //     reserved 2, 9 to 11, 15;
                //     reserved "foo", "bar";
                //   }

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                DescriptorProto* foo = AddMessage(&foo_file, "Foo");
                AddReservedRange(foo, 2, 3);
                AddReservedRange(foo, 9, 12);
                AddReservedRange(foo, 15, 16);

                foo->add_reserved_name("foo");
                foo->add_reserved_name("bar");

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->message_type_count());
                foo_ = foo_file_->message_type(0);
            }

            DescriptorPool pool_;
            const FileDescriptor* foo_file_;
            const Descriptor* foo_;
        };

        TEST_F(ReservedDescriptorTest, ReservedRanges) {
            ASSERT_EQ(3, foo_->reserved_range_count());

            EXPECT_EQ(2, foo_->reserved_range(0)->start);
            EXPECT_EQ(3, foo_->reserved_range(0)->end);

            EXPECT_EQ(9, foo_->reserved_range(1)->start);
            EXPECT_EQ(12, foo_->reserved_range(1)->end);

            EXPECT_EQ(15, foo_->reserved_range(2)->start);
            EXPECT_EQ(16, foo_->reserved_range(2)->end);
        }

        TEST_F(ReservedDescriptorTest, IsReservedNumber) {
            EXPECT_FALSE(foo_->IsReservedNumber(1));
            EXPECT_TRUE(foo_->IsReservedNumber(2));
            EXPECT_FALSE(foo_->IsReservedNumber(3));
            EXPECT_FALSE(foo_->IsReservedNumber(8));
            EXPECT_TRUE(foo_->IsReservedNumber(9));
            EXPECT_TRUE(foo_->IsReservedNumber(10));
            EXPECT_TRUE(foo_->IsReservedNumber(11));
            EXPECT_FALSE(foo_->IsReservedNumber(12));
            EXPECT_FALSE(foo_->IsReservedNumber(13));
            EXPECT_FALSE(foo_->IsReservedNumber(14));
            EXPECT_TRUE(foo_->IsReservedNumber(15));
            EXPECT_FALSE(foo_->IsReservedNumber(16));
        }

        TEST_F(ReservedDescriptorTest, ReservedNames) {
            ASSERT_EQ(2, foo_->reserved_name_count());

            EXPECT_EQ("foo", foo_->reserved_name(0));
            EXPECT_EQ("bar", foo_->reserved_name(1));
        }

        TEST_F(ReservedDescriptorTest, IsReservedName) {
            EXPECT_TRUE(foo_->IsReservedName("foo"));
            EXPECT_TRUE(foo_->IsReservedName("bar"));
            EXPECT_FALSE(foo_->IsReservedName("baz"));
        }

        // ===================================================================

        // Test reserved enum fields.
        class ReservedEnumDescriptorTest : public testing::Test {
        protected:
            void SetUp() override {
                // Build descriptors for the following definitions:
                //
                //   enum Foo {
                //     BAR = 1;
                //     reserved 2, 9 to 11, 15;
                //     reserved "foo", "bar";
                //   }

                FileDescriptorProto foo_file;
                foo_file.set_name("foo.shark");

                EnumDescriptorProto* foo = AddEnum(&foo_file, "Foo");
                EnumDescriptorProto* edge1 = AddEnum(&foo_file, "Edge1");
                EnumDescriptorProto* edge2 = AddEnum(&foo_file, "Edge2");

                AddEnumValue(foo, "BAR", 4);
                AddReservedRange(foo, -5, -3);
                AddReservedRange(foo, -2, 1);
                AddReservedRange(foo, 2, 3);
                AddReservedRange(foo, 9, 12);
                AddReservedRange(foo, 15, 16);

                foo->add_reserved_name("foo");
                foo->add_reserved_name("bar");

                // Some additional edge cases that cover most or all of the range of enum
                // values

                // Note: We use INT_MAX as the maximum reserved range upper bound,
                // inclusive.
                AddEnumValue(edge1, "EDGE1", 1);
                AddReservedRange(edge1, 10, INT_MAX);
                AddEnumValue(edge2, "EDGE2", 15);
                AddReservedRange(edge2, INT_MIN, 10);

                // Build the descriptors and get the pointers.
                foo_file_ = pool_.BuildFile(foo_file);
                ASSERT_TRUE(foo_file_ != nullptr);

                ASSERT_EQ(3, foo_file_->enum_type_count());
                foo_ = foo_file_->enum_type(0);
                edge1_ = foo_file_->enum_type(1);
                edge2_ = foo_file_->enum_type(2);
            }

            DescriptorPool pool_;
            const FileDescriptor* foo_file_;
            const EnumDescriptor* foo_;
            const EnumDescriptor* edge1_;
            const EnumDescriptor* edge2_;
        };

        TEST_F(ReservedEnumDescriptorTest, ReservedRanges) {
            ASSERT_EQ(5, foo_->reserved_range_count());

            EXPECT_EQ(-5, foo_->reserved_range(0)->start);
            EXPECT_EQ(-3, foo_->reserved_range(0)->end);

            EXPECT_EQ(-2, foo_->reserved_range(1)->start);
            EXPECT_EQ(1, foo_->reserved_range(1)->end);

            EXPECT_EQ(2, foo_->reserved_range(2)->start);
            EXPECT_EQ(3, foo_->reserved_range(2)->end);

            EXPECT_EQ(9, foo_->reserved_range(3)->start);
            EXPECT_EQ(12, foo_->reserved_range(3)->end);

            EXPECT_EQ(15, foo_->reserved_range(4)->start);
            EXPECT_EQ(16, foo_->reserved_range(4)->end);

            ASSERT_EQ(1, edge1_->reserved_range_count());
            EXPECT_EQ(10, edge1_->reserved_range(0)->start);
            EXPECT_EQ(INT_MAX, edge1_->reserved_range(0)->end);

            ASSERT_EQ(1, edge2_->reserved_range_count());
            EXPECT_EQ(INT_MIN, edge2_->reserved_range(0)->start);
            EXPECT_EQ(10, edge2_->reserved_range(0)->end);
        }

        TEST_F(ReservedEnumDescriptorTest, IsReservedNumber) {
            EXPECT_TRUE(foo_->IsReservedNumber(-5));
            EXPECT_TRUE(foo_->IsReservedNumber(-4));
            EXPECT_TRUE(foo_->IsReservedNumber(-3));
            EXPECT_TRUE(foo_->IsReservedNumber(-2));
            EXPECT_TRUE(foo_->IsReservedNumber(-1));
            EXPECT_TRUE(foo_->IsReservedNumber(0));
            EXPECT_TRUE(foo_->IsReservedNumber(1));
            EXPECT_TRUE(foo_->IsReservedNumber(2));
            EXPECT_TRUE(foo_->IsReservedNumber(3));
            EXPECT_FALSE(foo_->IsReservedNumber(8));
            EXPECT_TRUE(foo_->IsReservedNumber(9));
            EXPECT_TRUE(foo_->IsReservedNumber(10));
            EXPECT_TRUE(foo_->IsReservedNumber(11));
            EXPECT_TRUE(foo_->IsReservedNumber(12));
            EXPECT_FALSE(foo_->IsReservedNumber(13));
            EXPECT_FALSE(foo_->IsReservedNumber(13));
            EXPECT_FALSE(foo_->IsReservedNumber(14));
            EXPECT_TRUE(foo_->IsReservedNumber(15));
            EXPECT_TRUE(foo_->IsReservedNumber(16));
            EXPECT_FALSE(foo_->IsReservedNumber(17));

            EXPECT_FALSE(edge1_->IsReservedNumber(9));
            EXPECT_TRUE(edge1_->IsReservedNumber(10));
            EXPECT_TRUE(edge1_->IsReservedNumber(INT_MAX - 1));
            EXPECT_TRUE(edge1_->IsReservedNumber(INT_MAX));

            EXPECT_TRUE(edge2_->IsReservedNumber(INT_MIN));
            EXPECT_TRUE(edge2_->IsReservedNumber(9));
            EXPECT_TRUE(edge2_->IsReservedNumber(10));
            EXPECT_FALSE(edge2_->IsReservedNumber(11));
        }

        TEST_F(ReservedEnumDescriptorTest, ReservedNames) {
            ASSERT_EQ(2, foo_->reserved_name_count());

            EXPECT_EQ("foo", foo_->reserved_name(0));
            EXPECT_EQ("bar", foo_->reserved_name(1));
        }

        TEST_F(ReservedEnumDescriptorTest, IsReservedName) {
            EXPECT_TRUE(foo_->IsReservedName("foo"));
            EXPECT_TRUE(foo_->IsReservedName("bar"));
            EXPECT_FALSE(foo_->IsReservedName("baz"));
        }

        // ===================================================================

        class MiscTest : public testing::Test {
        protected:
            // Function which makes a field descriptor of the given type.
            const FieldDescriptor* GetFieldDescriptorOfType(SharkFieldType type) {
                FileDescriptorProto file_proto;
                file_proto.set_name("foo.shark");
                AddEmptyEnum(&file_proto, "DummyEnum");

                DescriptorProto* message = AddMessage(&file_proto, "TestMessage");
                FieldDescriptorProto* field = AddField(
                    message, "foo", 1, FieldCardinality::OPTIONAL,
                    static_cast<SharkType>(static_cast<int>(type)));

                if (type == SharkFieldType::MESSAGE) {
                    field->set_type_name("TestMessage");
                } else if (type == SharkFieldType::ENUM) {
                    field->set_type_name("DummyEnum");
                }

                // Build the descriptors and get the pointers.
                pool_.reset(new DescriptorPool());
                const FileDescriptor* file = pool_->BuildFile(file_proto);

                if (file != nullptr && file->message_type_count() == 1 && file->message_type(0)->field_count() == 1) {
                    return file->message_type(0)->field(0);
                } else {
                    return nullptr;
                }
            }

            const char* GetTypeNameForFieldType(SharkFieldType type) {
                const FieldDescriptor* field = GetFieldDescriptorOfType(type);
                return field != nullptr ? field->type_name() : "";
            }

            SharkFieldCppType GetCppTypeForFieldType(SharkFieldType type) {
                const FieldDescriptor* field = GetFieldDescriptorOfType(type);
                return field != nullptr ? field->cpp_type()
                                        : static_cast<SharkFieldCppType>(0);
            }

            const char* GetCppTypeNameForFieldType(SharkFieldType type) {
                const FieldDescriptor* field = GetFieldDescriptorOfType(type);
                return field != nullptr ? field->cpp_type_name() : "";
            }

            const Descriptor* GetMessageDescriptorForFieldType(
                SharkFieldType type) {
                const FieldDescriptor* field = GetFieldDescriptorOfType(type);
                return field != nullptr ? field->message_type() : nullptr;
            }

            const EnumDescriptor* GetEnumDescriptorForFieldType(
                SharkFieldType type) {
                const FieldDescriptor* field = GetFieldDescriptorOfType(type);
                return field != nullptr ? field->enum_type() : nullptr;
            }

            std::unique_ptr<DescriptorPool> pool_;
        };

        TEST_F(MiscTest, TypeNames) {
            // Test that correct type names are returned.

            EXPECT_STREQ("double", GetTypeNameForFieldType(SharkFieldType::DOUBLE));
            EXPECT_STREQ("float", GetTypeNameForFieldType(SharkFieldType::FLOAT));
            EXPECT_STREQ("int64", GetTypeNameForFieldType(SharkFieldType::INT64));
            EXPECT_STREQ("uint64", GetTypeNameForFieldType(SharkFieldType::UINT64));
            EXPECT_STREQ("int32", GetTypeNameForFieldType(SharkFieldType::INT32));
            EXPECT_STREQ("fixed64", GetTypeNameForFieldType(SharkFieldType::FIXED64));
            EXPECT_STREQ("fixed32", GetTypeNameForFieldType(SharkFieldType::FIXED32));
            EXPECT_STREQ("bool", GetTypeNameForFieldType(SharkFieldType::BOOL));
            EXPECT_STREQ("string", GetTypeNameForFieldType(SharkFieldType::STRING));
            EXPECT_STREQ("message", GetTypeNameForFieldType(SharkFieldType::MESSAGE));
            EXPECT_STREQ("bytes", GetTypeNameForFieldType(SharkFieldType::BYTES));
            EXPECT_STREQ("uint32", GetTypeNameForFieldType(SharkFieldType::UINT32));
            EXPECT_STREQ("enum", GetTypeNameForFieldType(SharkFieldType::ENUM));
            EXPECT_STREQ("sfixed32", GetTypeNameForFieldType(SharkFieldType::SFIXED32));
            EXPECT_STREQ("sfixed64", GetTypeNameForFieldType(SharkFieldType::SFIXED64));
            EXPECT_STREQ("sint32", GetTypeNameForFieldType(SharkFieldType::SINT32));
            EXPECT_STREQ("sint64", GetTypeNameForFieldType(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, StaticTypeNames) {
            // Test that correct type names are returned.

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_STREQ("double", FD::TypeName(SharkFieldType::DOUBLE));
            EXPECT_STREQ("float", FD::TypeName(SharkFieldType::FLOAT));
            EXPECT_STREQ("int64", FD::TypeName(SharkFieldType::INT64));
            EXPECT_STREQ("uint64", FD::TypeName(SharkFieldType::UINT64));
            EXPECT_STREQ("int32", FD::TypeName(SharkFieldType::INT32));
            EXPECT_STREQ("fixed64", FD::TypeName(SharkFieldType::FIXED64));
            EXPECT_STREQ("fixed32", FD::TypeName(SharkFieldType::FIXED32));
            EXPECT_STREQ("bool", FD::TypeName(SharkFieldType::BOOL));
            EXPECT_STREQ("string", FD::TypeName(SharkFieldType::STRING));
            EXPECT_STREQ("message", FD::TypeName(SharkFieldType::MESSAGE));
            EXPECT_STREQ("bytes", FD::TypeName(SharkFieldType::BYTES));
            EXPECT_STREQ("uint32", FD::TypeName(SharkFieldType::UINT32));
            EXPECT_STREQ("enum", FD::TypeName(SharkFieldType::ENUM));
            EXPECT_STREQ("sfixed32", FD::TypeName(SharkFieldType::SFIXED32));
            EXPECT_STREQ("sfixed64", FD::TypeName(SharkFieldType::SFIXED64));
            EXPECT_STREQ("sint32", FD::TypeName(SharkFieldType::SINT32));
            EXPECT_STREQ("sint64", FD::TypeName(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, CppTypes) {
            // Test that CPP types are assigned correctly.

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_EQ(SharkFieldCppType::CPPTYPE_DOUBLE, GetCppTypeForFieldType(SharkFieldType::DOUBLE));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_FLOAT, GetCppTypeForFieldType(SharkFieldType::FLOAT));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT64, GetCppTypeForFieldType(SharkFieldType::INT64));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_UINT64, GetCppTypeForFieldType(SharkFieldType::UINT64));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT32, GetCppTypeForFieldType(SharkFieldType::INT32));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_UINT64, GetCppTypeForFieldType(SharkFieldType::FIXED64));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_UINT32, GetCppTypeForFieldType(SharkFieldType::FIXED32));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_BOOL, GetCppTypeForFieldType(SharkFieldType::BOOL));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_STRING, GetCppTypeForFieldType(SharkFieldType::STRING));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_MESSAGE, GetCppTypeForFieldType(SharkFieldType::MESSAGE));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_STRING, GetCppTypeForFieldType(SharkFieldType::BYTES));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_UINT32, GetCppTypeForFieldType(SharkFieldType::UINT32));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_ENUM, GetCppTypeForFieldType(SharkFieldType::ENUM));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT32, GetCppTypeForFieldType(SharkFieldType::SFIXED32));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT64, GetCppTypeForFieldType(SharkFieldType::SFIXED64));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT32, GetCppTypeForFieldType(SharkFieldType::SINT32));
            EXPECT_EQ(SharkFieldCppType::CPPTYPE_INT64, GetCppTypeForFieldType(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, CppTypeNames) {
            // Test that correct CPP type names are returned.

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_STREQ("double", GetCppTypeNameForFieldType(SharkFieldType::DOUBLE));
            EXPECT_STREQ("float", GetCppTypeNameForFieldType(SharkFieldType::FLOAT));
            EXPECT_STREQ("int64", GetCppTypeNameForFieldType(SharkFieldType::INT64));
            EXPECT_STREQ("uint64", GetCppTypeNameForFieldType(SharkFieldType::UINT64));
            EXPECT_STREQ("int32", GetCppTypeNameForFieldType(SharkFieldType::INT32));
            EXPECT_STREQ("uint64", GetCppTypeNameForFieldType(SharkFieldType::FIXED64));
            EXPECT_STREQ("uint32", GetCppTypeNameForFieldType(SharkFieldType::FIXED32));
            EXPECT_STREQ("bool", GetCppTypeNameForFieldType(SharkFieldType::BOOL));
            EXPECT_STREQ("string", GetCppTypeNameForFieldType(SharkFieldType::STRING));
            EXPECT_STREQ("message", GetCppTypeNameForFieldType(SharkFieldType::MESSAGE));
            EXPECT_STREQ("string", GetCppTypeNameForFieldType(SharkFieldType::BYTES));
            EXPECT_STREQ("uint32", GetCppTypeNameForFieldType(SharkFieldType::UINT32));
            EXPECT_STREQ("enum", GetCppTypeNameForFieldType(SharkFieldType::ENUM));
            EXPECT_STREQ("int32", GetCppTypeNameForFieldType(SharkFieldType::SFIXED32));
            EXPECT_STREQ("int64", GetCppTypeNameForFieldType(SharkFieldType::SFIXED64));
            EXPECT_STREQ("int32", GetCppTypeNameForFieldType(SharkFieldType::SINT32));
            EXPECT_STREQ("int64", GetCppTypeNameForFieldType(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, StaticCppTypeNames) {
            // Test that correct CPP type names are returned.

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_STREQ("int32", FD::CppTypeName(SharkFieldCppType::CPPTYPE_INT32));
            EXPECT_STREQ("int64", FD::CppTypeName(SharkFieldCppType::CPPTYPE_INT64));
            EXPECT_STREQ("uint32", FD::CppTypeName(SharkFieldCppType::CPPTYPE_UINT32));
            EXPECT_STREQ("uint64", FD::CppTypeName(SharkFieldCppType::CPPTYPE_UINT64));
            EXPECT_STREQ("double", FD::CppTypeName(SharkFieldCppType::CPPTYPE_DOUBLE));
            EXPECT_STREQ("float", FD::CppTypeName(SharkFieldCppType::CPPTYPE_FLOAT));
            EXPECT_STREQ("bool", FD::CppTypeName(SharkFieldCppType::CPPTYPE_BOOL));
            EXPECT_STREQ("enum", FD::CppTypeName(SharkFieldCppType::CPPTYPE_ENUM));
            EXPECT_STREQ("string", FD::CppTypeName(SharkFieldCppType::CPPTYPE_STRING));
            EXPECT_STREQ("message", FD::CppTypeName(SharkFieldCppType::CPPTYPE_MESSAGE));
        }

        TEST_F(MiscTest, MessageType) {
            // Test that message_type() is nullptr for non-aggregate fields

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::DOUBLE));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::FLOAT));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::INT64));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::UINT64));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::INT32));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::FIXED64));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::FIXED32));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::BOOL));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::STRING));
            EXPECT_TRUE(nullptr != GetMessageDescriptorForFieldType(SharkFieldType::MESSAGE));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::BYTES));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::UINT32));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::ENUM));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::SFIXED32));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::SFIXED64));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::SINT32));
            EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, EnumType) {
            // Test that enum_type() is nullptr for non-enum fields

            typedef FieldDescriptor FD; // avoid ugly line wrapping

            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::DOUBLE));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::FLOAT));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::INT64));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::UINT64));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::INT32));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::FIXED64));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::FIXED32));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::BOOL));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::STRING));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::MESSAGE));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::BYTES));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::UINT32));
            EXPECT_TRUE(nullptr != GetEnumDescriptorForFieldType(SharkFieldType::ENUM));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::SFIXED32));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::SFIXED64));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::SINT32));
            EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(SharkFieldType::SINT64));
        }

        TEST_F(MiscTest, DefaultValues) {
            // Test that setting default values works.
            FileDescriptorProto file_proto;
            file_proto.set_name("foo.shark");

            EnumDescriptorProto* enum_type_proto = AddEnum(&file_proto, "DummyEnum");
            AddEnumValue(enum_type_proto, "A", 1);
            AddEnumValue(enum_type_proto, "B", 2);

            DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");

            typedef FieldDescriptorProto FD; // avoid ugly line wrapping
            const FieldCardinality label = FieldCardinality::OPTIONAL;

            // Create fields of every CPP type with default values.
            AddField(message_proto, "int32", 1, label, SharkType::INT32)
                ->set_default_value("-1");
            AddField(message_proto, "int64", 2, label, SharkType::INT64)
                ->set_default_value("-1000000000000");
            AddField(message_proto, "uint32", 3, label, SharkType::UINT32)
                ->set_default_value("42");
            AddField(message_proto, "uint64", 4, label, SharkType::UINT64)
                ->set_default_value("2000000000000");
            AddField(message_proto, "float", 5, label, SharkType::FLOAT)
                ->set_default_value("4.5");
            AddField(message_proto, "double", 6, label, SharkType::DOUBLE)
                ->set_default_value("10e100");
            AddField(message_proto, "bool", 7, label, SharkType::BOOL)
                ->set_default_value("true");
            AddField(message_proto, "string", 8, label, SharkType::STRING)
                ->set_default_value("hello");
            AddField(message_proto, "data", 9, label, SharkType::BYTES)
                ->set_default_value("\\001\\002\\003");

            FieldDescriptorProto* enum_field = AddField(message_proto, "enum", 10, label, SharkType::ENUM);
            enum_field->set_type_name("DummyEnum");
            enum_field->set_default_value("B");

            // Strings are allowed to have empty defaults.  (At one point, due to
            // a bug, empty defaults for strings were rejected.  Oops.)
            AddField(message_proto, "empty_string", 11, label, SharkType::STRING)
                ->set_default_value("");

            // Add a second set of fields with implicit default values.
            AddField(message_proto, "implicit_int32", 21, label, SharkType::INT32);
            AddField(message_proto, "implicit_int64", 22, label, SharkType::INT64);
            AddField(message_proto, "implicit_uint32", 23, label, SharkType::UINT32);
            AddField(message_proto, "implicit_uint64", 24, label, SharkType::UINT64);
            AddField(message_proto, "implicit_float", 25, label, SharkType::FLOAT);
            AddField(message_proto, "implicit_double", 26, label, SharkType::DOUBLE);
            AddField(message_proto, "implicit_bool", 27, label, SharkType::BOOL);
            AddField(message_proto, "implicit_string", 28, label, SharkType::STRING);
            AddField(message_proto, "implicit_data", 29, label, SharkType::BYTES);
            AddField(message_proto, "implicit_enum", 30, label, SharkType::ENUM)
                ->set_type_name("DummyEnum");

            // Build it.
            DescriptorPool pool;
            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);

            ASSERT_EQ(1, file->enum_type_count());
            const EnumDescriptor* enum_type = file->enum_type(0);
            ASSERT_EQ(2, enum_type->value_count());
            const EnumValueDescriptor* enum_value_a = enum_type->value(0);
            const EnumValueDescriptor* enum_value_b = enum_type->value(1);

            ASSERT_EQ(1, file->message_type_count());
            const Descriptor* message = file->message_type(0);

            ASSERT_EQ(21, message->field_count());

            // Check the default values.
            ASSERT_TRUE(message->field(0)->has_default_value());
            ASSERT_TRUE(message->field(1)->has_default_value());
            ASSERT_TRUE(message->field(2)->has_default_value());
            ASSERT_TRUE(message->field(3)->has_default_value());
            ASSERT_TRUE(message->field(4)->has_default_value());
            ASSERT_TRUE(message->field(5)->has_default_value());
            ASSERT_TRUE(message->field(6)->has_default_value());
            ASSERT_TRUE(message->field(7)->has_default_value());
            ASSERT_TRUE(message->field(8)->has_default_value());
            ASSERT_TRUE(message->field(9)->has_default_value());
            ASSERT_TRUE(message->field(10)->has_default_value());

            EXPECT_EQ(-1, message->field(0)->default_value_int32());
            EXPECT_EQ(int64_t { -1000000000000 }, message->field(1)->default_value_int64());
            EXPECT_EQ(42, message->field(2)->default_value_uint32());
            EXPECT_EQ(uint64_t { 2000000000000 }, message->field(3)->default_value_uint64());
            EXPECT_EQ(4.5, message->field(4)->default_value_float());
            EXPECT_EQ(10e100, message->field(5)->default_value_double());
            EXPECT_TRUE(message->field(6)->default_value_bool());
            EXPECT_EQ("hello", message->field(7)->default_value_string());
            EXPECT_EQ("\001\002\003", message->field(8)->default_value_string());
            EXPECT_EQ(enum_value_b, message->field(9)->default_value_enum());
            EXPECT_EQ("", message->field(10)->default_value_string());

            ASSERT_FALSE(message->field(11)->has_default_value());
            ASSERT_FALSE(message->field(12)->has_default_value());
            ASSERT_FALSE(message->field(13)->has_default_value());
            ASSERT_FALSE(message->field(14)->has_default_value());
            ASSERT_FALSE(message->field(15)->has_default_value());
            ASSERT_FALSE(message->field(16)->has_default_value());
            ASSERT_FALSE(message->field(17)->has_default_value());
            ASSERT_FALSE(message->field(18)->has_default_value());
            ASSERT_FALSE(message->field(19)->has_default_value());
            ASSERT_FALSE(message->field(20)->has_default_value());

            EXPECT_EQ(0, message->field(11)->default_value_int32());
            EXPECT_EQ(0, message->field(12)->default_value_int64());
            EXPECT_EQ(0, message->field(13)->default_value_uint32());
            EXPECT_EQ(0, message->field(14)->default_value_uint64());
            EXPECT_EQ(0.0f, message->field(15)->default_value_float());
            EXPECT_EQ(0.0, message->field(16)->default_value_double());
            EXPECT_FALSE(message->field(17)->default_value_bool());
            EXPECT_EQ("", message->field(18)->default_value_string());
            EXPECT_EQ("", message->field(19)->default_value_string());
            EXPECT_EQ(enum_value_a, message->field(20)->default_value_enum());
        }

        TEST_F(MiscTest, FieldOptions) {
            // Try setting field options.

            FileDescriptorProto file_proto;
            file_proto.set_name("foo.shark");

            DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");
            AddField(message_proto, "foo", 1, FieldCardinality::OPTIONAL,
                SharkType::INT32);
            FieldDescriptorProto* bar_proto = AddField(message_proto, "bar", 2, FieldCardinality::OPTIONAL,
                SharkType::INT32);

            FieldOptions* options = bar_proto->mutable_options();
            options->set_ctype(FieldOptions_CType::CORD);

            // Build the descriptors and get the pointers.
            DescriptorPool pool;
            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);

            ASSERT_EQ(1, file->message_type_count());
            const Descriptor* message = file->message_type(0);

            ASSERT_EQ(2, message->field_count());
            const FieldDescriptor* foo = message->field(0);
            const FieldDescriptor* bar = message->field(1);

            // "foo" had no options set, so it should return the default options.
            EXPECT_EQ(&FieldOptions::default_instance(), &foo->options());

            // "bar" had options set.
            EXPECT_NE(&FieldOptions::default_instance(), options);
            EXPECT_TRUE(bar->options().has_ctype());
            EXPECT_EQ(FieldOptions_CType::CORD, bar->options().ctype());
        }

        // ===================================================================
        enum DescriptorPoolMode { NO_DATABASE,
            FALLBACK_DATABASE };

        class AllowUnknownDependenciesTest
            : public testing::TestWithParam<
                  std::tuple<DescriptorPoolMode, const char*>> {
        protected:
            DescriptorPoolMode mode() { return std::get<0>(GetParam()); }
            const char* syntax() { return std::get<1>(GetParam()); }

            void SetUp() override {
                FileDescriptorProto foo_proto, bar_proto;

                switch (mode()) {
                case NO_DATABASE:
                    pool_.reset(new DescriptorPool);
                    break;
                case FALLBACK_DATABASE:
                    pool_.reset(new DescriptorPool(&db_));
                    break;
                }

                pool_->AllowUnknownDependencies();

                ASSERT_TRUE(TextFormat::ParseFromString(
                    "name: 'foo.shark'"
                    "dependency: 'bar.shark'"
                    "dependency: 'baz.shark'"
                    "message_type {"
                    "  name: 'Foo'"
                    "  field { name:'bar' number:1 label:OPTIONAL type_name:'Bar' }"
                    "  field { name:'baz' number:2 label:OPTIONAL type_name:'Baz' }"
                    "  field { name:'moo' number:3 label:OPTIONAL"
                    "    type_name: '.corge.Moo'"
                    "    type:ENUM"
                    "    options {"
                    "      uninterpreted_option {"
                    "        name {"
                    "          name_part: 'grault'"
                    "          is_extension: true"
                    "        }"
                    "        positive_int_value: 1234"
                    "      }"
                    "    }"
                    "  }"
                    "}",
                    &foo_proto));
                foo_proto.set_syntax(syntax());

                ASSERT_TRUE(
                    TextFormat::ParseFromString("name: 'bar.shark'"
                                                "message_type { name: 'Bar' }",
                        &bar_proto));
                bar_proto.set_syntax(syntax());

                // Collect pointers to stuff.
                bar_file_ = BuildFile(bar_proto);
                ASSERT_TRUE(bar_file_ != nullptr);

                ASSERT_EQ(1, bar_file_->message_type_count());
                bar_type_ = bar_file_->message_type(0);

                foo_file_ = BuildFile(foo_proto);
                ASSERT_TRUE(foo_file_ != nullptr);

                ASSERT_EQ(1, foo_file_->message_type_count());
                foo_type_ = foo_file_->message_type(0);

                ASSERT_EQ(3, foo_type_->field_count());
                bar_field_ = foo_type_->field(0);
                baz_field_ = foo_type_->field(1);
                moo_field_ = foo_type_->field(2);
            }

            const FileDescriptor* BuildFile(const FileDescriptorProto& proto) {
                switch (mode()) {
                case NO_DATABASE:
                    return pool_->BuildFile(proto);
                    break;
                case FALLBACK_DATABASE: {
                    EXPECT_TRUE(db_.Add(proto));
                    return pool_->FindFileByName(proto.name());
                }
                }
                GOOGLE_LOG(FATAL) << "Can't get here.";
                return nullptr;
            }

            const FileDescriptor* bar_file_;
            const Descriptor* bar_type_;
            const FileDescriptor* foo_file_;
            const Descriptor* foo_type_;
            const FieldDescriptor* bar_field_;
            const FieldDescriptor* baz_field_;
            const FieldDescriptor* moo_field_;

            SimpleDescriptorDatabase db_; // used if in FALLBACK_DATABASE mode.
            std::unique_ptr<DescriptorPool> pool_;
        };

        TEST_P(AllowUnknownDependenciesTest, PlaceholderFile) {
            ASSERT_EQ(2, foo_file_->dependency_count());
            EXPECT_EQ(bar_file_, foo_file_->dependency(0));
            EXPECT_FALSE(bar_file_->is_placeholder());

            const FileDescriptor* baz_file = foo_file_->dependency(1);
            EXPECT_EQ("baz.shark", baz_file->name());
            EXPECT_EQ(0, baz_file->message_type_count());
            EXPECT_TRUE(baz_file->is_placeholder());

            // Placeholder files should not be findable.
            EXPECT_EQ(bar_file_, pool_->FindFileByName(bar_file_->name()));
            EXPECT_TRUE(pool_->FindFileByName(baz_file->name()) == nullptr);

            // Copy*To should not crash for placeholder files.
            FileDescriptorProto baz_file_proto;
            baz_file->CopyTo(&baz_file_proto);
            baz_file->CopySourceCodeInfoTo(&baz_file_proto);
            EXPECT_FALSE(baz_file_proto.has_source_code_info());
        }

        TEST_P(AllowUnknownDependenciesTest, PlaceholderTypes) {
            ASSERT_EQ(SharkFieldType::MESSAGE, bar_field_->type());
            EXPECT_EQ(bar_type_, bar_field_->message_type());
            EXPECT_FALSE(bar_type_->is_placeholder());

            ASSERT_EQ(SharkFieldType::MESSAGE, baz_field_->type());
            const Descriptor* baz_type = baz_field_->message_type();
            EXPECT_EQ("Baz", baz_type->name());
            EXPECT_EQ("Baz", baz_type->full_name());
            EXPECT_EQ(0, baz_type->extension_range_count());
            EXPECT_TRUE(baz_type->is_placeholder());

            ASSERT_EQ(SharkFieldType::ENUM, moo_field_->type());
            const EnumDescriptor* moo_type = moo_field_->enum_type();
            EXPECT_EQ("Moo", moo_type->name());
            EXPECT_EQ("corge.Moo", moo_type->full_name());
            EXPECT_TRUE(moo_type->is_placeholder());
            // Placeholder enum values should not be findable.
            EXPECT_EQ(moo_type->FindValueByNumber(0), nullptr);

            // Placeholder types should not be findable.
            EXPECT_EQ(bar_type_, pool_->FindMessageTypeByName(bar_type_->full_name()));
            EXPECT_TRUE(pool_->FindMessageTypeByName(baz_type->full_name()) == nullptr);
            EXPECT_TRUE(pool_->FindEnumTypeByName(moo_type->full_name()) == nullptr);
        }

        TEST_P(AllowUnknownDependenciesTest, CopyTo) {
            // FieldDescriptor::CopyTo() should write non-fully-qualified type names
            // for placeholder types which were not originally fully-qualified.
            FieldDescriptorProto proto;

            // Bar is not a placeholder, so it is fully-qualified.
            bar_field_->CopyTo(&proto);
            EXPECT_EQ(".Bar", proto.type_name());
            EXPECT_EQ(SharkType::MESSAGE, proto.type());

            // Baz is an unqualified placeholder.
            proto.Clear();
            baz_field_->CopyTo(&proto);
            EXPECT_EQ("Baz", proto.type_name());
            EXPECT_FALSE(proto.has_type());

            // Moo is a fully-qualified placeholder.
            proto.Clear();
            moo_field_->CopyTo(&proto);
            EXPECT_EQ(".corge.Moo", proto.type_name());
            EXPECT_EQ(SharkType::ENUM, proto.type());
        }

        TEST_P(AllowUnknownDependenciesTest, CustomOptions) {
            // Moo should still have the uninterpreted option attached.
            ASSERT_EQ(1, moo_field_->options().uninterpreted_option_size());
            const UninterpretedOption& option = moo_field_->options().uninterpreted_option(0);
            ASSERT_EQ(1, option.name_size());
            EXPECT_EQ("grault", option.name(0).name_part());
        }

        TEST_P(AllowUnknownDependenciesTest, UnknownExtendee) {
            // Test that we can extend an unknown type.  This is slightly tricky because
            // it means that the placeholder type must have an extension range.

            FileDescriptorProto extension_proto;

            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: 'extension.shark'"
                "extension { extendee: 'UnknownType' name:'some_extension' number:123"
                "            label:OPTIONAL type:INT32 }",
                &extension_proto));
            const FileDescriptor* file = BuildFile(extension_proto);

            ASSERT_TRUE(file != nullptr);

            ASSERT_EQ(1, file->extension_count());
            const Descriptor* extendee = file->extension(0)->containing_type();
            EXPECT_EQ("UnknownType", extendee->name());
            EXPECT_TRUE(extendee->is_placeholder());
            ASSERT_EQ(1, extendee->extension_range_count());
            EXPECT_EQ(1, extendee->extension_range(0)->start);
            EXPECT_EQ(FieldDescriptor::kMaxNumber + 1, extendee->extension_range(0)->end);
        }

        TEST_P(AllowUnknownDependenciesTest, CustomOption) {
            // Test that we can use a custom option without having parsed
            // descriptor.shark.

            FileDescriptorProto option_proto;

            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"unknown_custom_options.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { "
                "  extendee: \"shark.FileOptions\" "
                "  name: \"some_option\" "
                "  number: 123456 "
                "  label: OPTIONAL "
                "  type:INT32 "
                "} "
                "options { "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"some_option\" "
                "      is_extension: true "
                "    } "
                "    positive_int_value: 1234 "
                "  } "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"unknown_option\" "
                "      is_extension: true "
                "    } "
                "    positive_int_value: 1234 "
                "  } "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"optimize_for\" "
                "      is_extension: false "
                "    } "
                "    identifier_value: \"SPEED\" "
                "  } "
                "}",
                &option_proto));

            const FileDescriptor* file = BuildFile(option_proto);
            ASSERT_TRUE(file != nullptr);

            // Verify that no extension options were set, but they were left as
            // uninterpreted_options.
            std::vector<const FieldDescriptor*> fields;
            file->options().GetReflection()->ListFields(file->options(), &fields);
            ASSERT_EQ(2, fields.size());
            EXPECT_TRUE(file->options().has_optimize_for());
            EXPECT_EQ(2, file->options().uninterpreted_option_size());
        }

        TEST_P(AllowUnknownDependenciesTest,
            UndeclaredDependencyTriggersBuildOfDependency) {
            // Crazy case: suppose foo.shark refers to a symbol without declaring the
            // dependency that finds it. In the event that the pool is backed by a
            // DescriptorDatabase, the pool will attempt to find the symbol in the
            // database. If successful, it will build the undeclared dependency to verify
            // that the file does indeed contain the symbol. If that file fails to build,
            // then its descriptors must be rolled back. However, we still want foo.shark
            // to build successfully, since we are allowing unknown dependencies.

            FileDescriptorProto undeclared_dep_proto;
            // We make this file fail to build by giving it two fields with tag 1.
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"invalid_file_as_undeclared_dep.shark\" "
                "package: \"undeclared\" "
                "message_type: {  "
                "  name: \"Mooo\"  "
                "  field { "
                "    name:'moo' number:1 label:OPTIONAL type:INT32 "
                "  }"
                "  field { "
                "    name:'mooo' number:1 label:OPTIONAL type:INT64 "
                "  }"
                "}",
                &undeclared_dep_proto));
            // We can't use the BuildFile() helper because we don't actually want to build
            // it into the descriptor pool in the fallback database case: it just needs to
            // be sitting in the database so that it gets built during the building of
            // test.shark below.
            switch (mode()) {
            case NO_DATABASE: {
                ASSERT_TRUE(pool_->BuildFile(undeclared_dep_proto) == nullptr);
                break;
            }
            case FALLBACK_DATABASE: {
                ASSERT_TRUE(db_.Add(undeclared_dep_proto));
            }
            }

            FileDescriptorProto test_proto;
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"test.shark\" "
                "message_type: { "
                "  name: \"Corge\" "
                "  field { "
                "    name:'mooo' number:1 label: OPTIONAL "
                "    type_name:'undeclared.Mooo' type:MESSAGE "
                "  }"
                "}",
                &test_proto));

            const FileDescriptor* file = BuildFile(test_proto);
            ASSERT_TRUE(file != nullptr);
            GOOGLE_LOG(INFO) << file->DebugString();

            EXPECT_EQ(0, file->dependency_count());
            ASSERT_EQ(1, file->message_type_count());
            const Descriptor* corge_desc = file->message_type(0);
            ASSERT_EQ("Corge", corge_desc->name());
            ASSERT_EQ(1, corge_desc->field_count());
            EXPECT_FALSE(corge_desc->is_placeholder());

            const FieldDescriptor* mooo_field = corge_desc->field(0);
            ASSERT_EQ(SharkFieldType::MESSAGE, mooo_field->type());
            ASSERT_EQ("Mooo", mooo_field->message_type()->name());
            ASSERT_EQ("undeclared.Mooo", mooo_field->message_type()->full_name());
            EXPECT_TRUE(mooo_field->message_type()->is_placeholder());
            // The place holder type should not be findable.
            ASSERT_TRUE(pool_->FindMessageTypeByName("undeclared.Mooo") == nullptr);
        }

        INSTANTIATE_TEST_SUITE_P(DatabaseSource, AllowUnknownDependenciesTest,
            testing::Combine(testing::Values(NO_DATABASE,
                                 FALLBACK_DATABASE),
                testing::Values("proto2")));

        // ===================================================================

        TEST(CustomOptions, OptionLocations) {
            const Descriptor* message = sharkbuf_unittest::TestMessageWithCustomOptions::descriptor();
            const FileDescriptor* file = message->file();
            const FieldDescriptor* field = message->FindFieldByName("field1");
            const OneofDescriptor* oneof = message->FindOneofByName("AnOneof");
            const FieldDescriptor* map_field = message->FindFieldByName("map_field");
            const EnumDescriptor* enm = message->FindEnumTypeByName("AnEnum");
            // TODO(benjy): Support EnumValue options, once the compiler does.
            const ServiceDescriptor* service = file->FindServiceByName("TestServiceWithCustomOptions");
            const MethodDescriptor* method = service->FindMethodByName("Foo");

            EXPECT_EQ(int64_t { 9876543210 },
                file->options().GetExtension(sharkbuf_unittest::file_opt1));
            EXPECT_EQ(-56,
                message->options().GetExtension(sharkbuf_unittest::message_opt1));
            EXPECT_EQ(int64_t { 8765432109 },
                field->options().GetExtension(sharkbuf_unittest::field_opt1));
            EXPECT_EQ(42, // Check that we get the default for an option we don't set.
                field->options().GetExtension(sharkbuf_unittest::field_opt2));
            EXPECT_EQ(-99, oneof->options().GetExtension(sharkbuf_unittest::oneof_opt1));
            EXPECT_EQ(int64_t { 12345 },
                map_field->options().GetExtension(sharkbuf_unittest::field_opt1));
            EXPECT_EQ(-789, enm->options().GetExtension(sharkbuf_unittest::enum_opt1));
            EXPECT_EQ(123, enm->value(1)->options().GetExtension(sharkbuf_unittest::enum_value_opt1));
            EXPECT_EQ(int64_t { -9876543210 },
                service->options().GetExtension(sharkbuf_unittest::service_opt1));
            EXPECT_EQ(sharkbuf_unittest::MethodOpt1::METHODOPT1_VAL2,
                method->options().GetExtension(sharkbuf_unittest::method_opt1));

            // See that the regular options went through unscathed.
            EXPECT_TRUE(message->options().has_message_set_wire_format());
            EXPECT_EQ(FieldOptions_CType::CORD, field->options().ctype());
        }

        TEST(CustomOptions, OptionTypes) {
            const MessageOptions* options = nullptr;

            constexpr int32_t kint32min = std::numeric_limits<int32_t>::min();
            constexpr int32_t kint32max = std::numeric_limits<int32_t>::max();
            constexpr uint32_t kuint32max = std::numeric_limits<uint32_t>::max();
            constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();
            constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
            constexpr uint64_t kuint64max = std::numeric_limits<uint64_t>::max();

            options = &sharkbuf_unittest::CustomOptionMinIntegerValues::descriptor()->options();
            EXPECT_EQ(false, options->GetExtension(sharkbuf_unittest::bool_opt));
            EXPECT_EQ(kint32min, options->GetExtension(sharkbuf_unittest::int32_opt));
            EXPECT_EQ(kint64min, options->GetExtension(sharkbuf_unittest::int64_opt));
            EXPECT_EQ(0, options->GetExtension(sharkbuf_unittest::uint32_opt));
            EXPECT_EQ(0, options->GetExtension(sharkbuf_unittest::uint64_opt));
            EXPECT_EQ(kint32min, options->GetExtension(sharkbuf_unittest::sint32_opt));
            EXPECT_EQ(kint64min, options->GetExtension(sharkbuf_unittest::sint64_opt));
            EXPECT_EQ(0, options->GetExtension(sharkbuf_unittest::fixed32_opt));
            EXPECT_EQ(0, options->GetExtension(sharkbuf_unittest::fixed64_opt));
            EXPECT_EQ(kint32min, options->GetExtension(sharkbuf_unittest::sfixed32_opt));
            EXPECT_EQ(kint64min, options->GetExtension(sharkbuf_unittest::sfixed64_opt));

            options = &sharkbuf_unittest::CustomOptionMaxIntegerValues::descriptor()->options();
            EXPECT_EQ(true, options->GetExtension(sharkbuf_unittest::bool_opt));
            EXPECT_EQ(kint32max, options->GetExtension(sharkbuf_unittest::int32_opt));
            EXPECT_EQ(kint64max, options->GetExtension(sharkbuf_unittest::int64_opt));
            EXPECT_EQ(kuint32max, options->GetExtension(sharkbuf_unittest::uint32_opt));
            EXPECT_EQ(kuint64max, options->GetExtension(sharkbuf_unittest::uint64_opt));
            EXPECT_EQ(kint32max, options->GetExtension(sharkbuf_unittest::sint32_opt));
            EXPECT_EQ(kint64max, options->GetExtension(sharkbuf_unittest::sint64_opt));
            EXPECT_EQ(kuint32max, options->GetExtension(sharkbuf_unittest::fixed32_opt));
            EXPECT_EQ(kuint64max, options->GetExtension(sharkbuf_unittest::fixed64_opt));
            EXPECT_EQ(kint32max, options->GetExtension(sharkbuf_unittest::sfixed32_opt));
            EXPECT_EQ(kint64max, options->GetExtension(sharkbuf_unittest::sfixed64_opt));

            options = &sharkbuf_unittest::CustomOptionOtherValues::descriptor()->options();
            EXPECT_EQ(-100, options->GetExtension(sharkbuf_unittest::int32_opt));
            EXPECT_FLOAT_EQ(12.3456789,
                options->GetExtension(sharkbuf_unittest::float_opt));
            EXPECT_DOUBLE_EQ(1.234567890123456789,
                options->GetExtension(sharkbuf_unittest::double_opt));
            EXPECT_EQ("Hello, \"World\"",
                options->GetExtension(sharkbuf_unittest::string_opt));

            EXPECT_EQ(std::string("Hello\0World", 11),
                options->GetExtension(sharkbuf_unittest::bytes_opt));

            EXPECT_EQ(sharkbuf_unittest::DummyMessageContainingEnum_TestEnumType::TEST_OPTION_ENUM_TYPE2,
                options->GetExtension(sharkbuf_unittest::enum_opt));

            options = &sharkbuf_unittest::SettingRealsFromPositiveInts::descriptor()->options();
            EXPECT_FLOAT_EQ(12, options->GetExtension(sharkbuf_unittest::float_opt));
            EXPECT_DOUBLE_EQ(154, options->GetExtension(sharkbuf_unittest::double_opt));

            options = &sharkbuf_unittest::SettingRealsFromNegativeInts::descriptor()->options();
            EXPECT_FLOAT_EQ(-12, options->GetExtension(sharkbuf_unittest::float_opt));
            EXPECT_DOUBLE_EQ(-154, options->GetExtension(sharkbuf_unittest::double_opt));
        }

        TEST(CustomOptions, ComplexExtensionOptions) {
            const MessageOptions* options = &sharkbuf_unittest::VariousComplexOptions::descriptor()->options();
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt1).foo(), 42);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt1)
                          .GetExtension(sharkbuf_unittest::mooo),
                324);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt1)
                          .GetExtension(sharkbuf_unittest::corge)
                          .moo(),
                876);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2).baz(), 987);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .GetExtension(sharkbuf_unittest::grault),
                654);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2).bar().foo(),
                743);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .bar()
                          .GetExtension(sharkbuf_unittest::mooo),
                1999);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .bar()
                          .GetExtension(sharkbuf_unittest::corge)
                          .moo(),
                2008);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .GetExtension(sharkbuf_unittest::garply)
                          .foo(),
                741);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .GetExtension(sharkbuf_unittest::garply)
                          .GetExtension(sharkbuf_unittest::mooo),
                1998);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2)
                          .GetExtension(sharkbuf_unittest::garply)
                          .GetExtension(sharkbuf_unittest::corge)
                          .moo(),
                2121);
            EXPECT_EQ(options
                          ->GetExtension(sharkbuf_unittest::ComplexOptionType2::
                                  ComplexOptionType4::complex_opt4)
                          .waldo(),
                1971);
            EXPECT_EQ(options->GetExtension(sharkbuf_unittest::complex_opt2).fred().waldo(),
                321);
            EXPECT_EQ(9, options->GetExtension(sharkbuf_unittest::complex_opt3).moo());
        }

        TEST(CustomOptions, OptionsFromOtherFile) {
            // Test that to use a custom option, we only need to import the file
            // defining the option; we do not also have to import descriptor.shark.
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // We have to import the Any dependency.
            FileDescriptorProto any_proto;
            shark::Any::descriptor()->file()->CopyTo(&any_proto);
            ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);

            sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
                &file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"custom_options_import.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"tests/sk/lib/unittest_custom_options.shark\" "
                "options { "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"file_opt1\" "
                "      is_extension: true "
                "    } "
                "    positive_int_value: 1234 "
                "  } "
                // Test a non-extension option too.  (At one point this failed due to a
                // bug.)
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"go_package\" "
                "      is_extension: false "
                "    } "
                "    string_value: \"foo\" "
                "  } "
                // Test that enum-typed options still work too.  (At one point this also
                // failed due to a bug.)
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"optimize_for\" "
                "      is_extension: false "
                "    } "
                "    identifier_value: \"SPEED\" "
                "  } "
                "}",
                &file_proto));

            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);
            EXPECT_EQ(1234, file->options().GetExtension(sharkbuf_unittest::file_opt1));
            EXPECT_TRUE(file->options().has_go_package());
            EXPECT_EQ("foo", file->options().go_package());
            EXPECT_TRUE(file->options().has_optimize_for());
            EXPECT_EQ(FileOptions_OptimizeMode::SPEED, file->options().optimize_for());
        }

        TEST(CustomOptions, MessageOptionThreeFieldsSet) {
            // This tests a bug which previously existed in custom options parsing.  The
            // bug occurred when you defined a custom option with message type and then
            // set three fields of that option on a single definition (see the example
            // below).  The bug is a bit hard to explain, so check the change history if
            // you want to know more.
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            FileDescriptorProto any_proto;
            shark::Any::descriptor()->file()->CopyTo(&any_proto);
            ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);

            sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
                &file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // The following represents the definition:
            //
            //   import "tests/sk/lib/unittest_custom_options.shark"
            //   package sharkbuf_unittest;
            //   message Foo {
            //     option (complex_opt1).foo  = 1234;
            //     option (complex_opt1).foo2 = 1234;
            //     option (complex_opt1).foo3 = 1234;
            //   }
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"custom_options_import.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"tests/sk/lib/unittest_custom_options.shark\" "
                "message_type { "
                "  name: \"Foo\" "
                "  options { "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 1234 "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo2\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 1234 "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo3\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 1234 "
                "    } "
                "  } "
                "}",
                &file_proto));

            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(1, file->message_type_count());

            const MessageOptions& options = file->message_type(0)->options();
            EXPECT_EQ(1234, options.GetExtension(sharkbuf_unittest::complex_opt1).foo());
        }

        TEST(CustomOptions, MessageOptionRepeatedLeafFieldSet) {
            // This test verifies that repeated fields in custom options can be
            // given multiple values by repeating the option with a different value.
            // This test checks repeated leaf values. Each repeated custom value
            // appears in a different uninterpreted_option, which will be concatenated
            // when they are merged into the final option value.
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            FileDescriptorProto any_proto;
            shark::Any::descriptor()->file()->CopyTo(&any_proto);
            ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);

            sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
                &file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // The following represents the definition:
            //
            //   import "tests/sk/lib/unittest_custom_options.shark"
            //   package sharkbuf_unittest;
            //   message Foo {
            //     option (complex_opt1).foo4 = 12;
            //     option (complex_opt1).foo4 = 34;
            //     option (complex_opt1).foo4 = 56;
            //   }
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"custom_options_import.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"tests/sk/lib/unittest_custom_options.shark\" "
                "message_type { "
                "  name: \"Foo\" "
                "  options { "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo4\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 12 "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo4\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 34 "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt1\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"foo4\" "
                "        is_extension: false "
                "      } "
                "      positive_int_value: 56 "
                "    } "
                "  } "
                "}",
                &file_proto));

            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(1, file->message_type_count());

            const MessageOptions& options = file->message_type(0)->options();
            EXPECT_EQ(3, options.GetExtension(sharkbuf_unittest::complex_opt1).foo4_size());
            EXPECT_EQ(12, options.GetExtension(sharkbuf_unittest::complex_opt1).foo4(0));
            EXPECT_EQ(34, options.GetExtension(sharkbuf_unittest::complex_opt1).foo4(1));
            EXPECT_EQ(56, options.GetExtension(sharkbuf_unittest::complex_opt1).foo4(2));
        }

        TEST(CustomOptions, MessageOptionRepeatedMsgFieldSet) {
            // This test verifies that repeated fields in custom options can be
            // given multiple values by repeating the option with a different value.
            // This test checks repeated message values. Each repeated custom value
            // appears in a different uninterpreted_option, which will be concatenated
            // when they are merged into the final option value.
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            FileDescriptorProto any_proto;
            shark::Any::descriptor()->file()->CopyTo(&any_proto);
            ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);

            sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
                &file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // The following represents the definition:
            //
            //   import "tests/sk/lib/unittest_custom_options.shark"
            //   package sharkbuf_unittest;
            //   message Foo {
            //     option (complex_opt2).barney = {waldo: 1};
            //     option (complex_opt2).barney = {waldo: 10};
            //     option (complex_opt2).barney = {waldo: 100};
            //   }
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"custom_options_import.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"tests/sk/lib/unittest_custom_options.shark\" "
                "message_type { "
                "  name: \"Foo\" "
                "  options { "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt2\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"barney\" "
                "        is_extension: false "
                "      } "
                "      aggregate_value: \"waldo: 1\" "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt2\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"barney\" "
                "        is_extension: false "
                "      } "
                "      aggregate_value: \"waldo: 10\" "
                "    } "
                "    uninterpreted_option { "
                "      name { "
                "        name_part: \"complex_opt2\" "
                "        is_extension: true "
                "      } "
                "      name { "
                "        name_part: \"barney\" "
                "        is_extension: false "
                "      } "
                "      aggregate_value: \"waldo: 100\" "
                "    } "
                "  } "
                "}",
                &file_proto));

            const FileDescriptor* file = pool.BuildFile(file_proto);
            ASSERT_TRUE(file != nullptr);
            ASSERT_EQ(1, file->message_type_count());

            const MessageOptions& options = file->message_type(0)->options();
            EXPECT_EQ(3,
                options.GetExtension(sharkbuf_unittest::complex_opt2).barney_size());
            EXPECT_EQ(
                1, options.GetExtension(sharkbuf_unittest::complex_opt2).barney(0).waldo());
            EXPECT_EQ(
                10,
                options.GetExtension(sharkbuf_unittest::complex_opt2).barney(1).waldo());
            EXPECT_EQ(
                100,
                options.GetExtension(sharkbuf_unittest::complex_opt2).barney(2).waldo());
        }

        // Check that aggregate options were parsed and saved correctly in
        // the appropriate descriptors.
        TEST(CustomOptions, AggregateOptions) {
            const Descriptor* msg = sharkbuf_unittest::AggregateMessage::descriptor();
            const FileDescriptor* file = msg->file();
            const FieldDescriptor* field = msg->FindFieldByName("fieldname");
            const EnumDescriptor* enumd = file->FindEnumTypeByName("AggregateEnum");
            const EnumValueDescriptor* enumv = enumd->FindValueByName("VALUE");
            const ServiceDescriptor* service = file->FindServiceByName("AggregateService");
            const MethodDescriptor* method = service->FindMethodByName("Method");

            // Tests for the different types of data embedded in fileopt
            const sharkbuf_unittest::Aggregate& file_options = file->options().GetExtension(sharkbuf_unittest::fileopt);
            EXPECT_EQ(100, file_options.i());
            EXPECT_EQ("FileAnnotation", file_options.s());
            EXPECT_EQ("NestedFileAnnotation", file_options.sub().s());
            EXPECT_EQ("FileExtensionAnnotation",
                file_options.file().GetExtension(sharkbuf_unittest::fileopt).s());
            EXPECT_EQ("EmbeddedMessageSetElement",
                file_options.mset()
                    .GetExtension(sharkbuf_unittest::AggregateMessageSetElement ::
                            message_set_extension)
                    .s());

            sharkbuf_unittest::AggregateMessageSetElement any_payload;
            ASSERT_TRUE(file_options.any().UnpackTo(&any_payload));
            EXPECT_EQ("EmbeddedMessageSetElement", any_payload.s());

            // Simple tests for all the other types of annotations
            EXPECT_EQ("MessageAnnotation",
                msg->options().GetExtension(sharkbuf_unittest::msgopt).s());
            EXPECT_EQ("FieldAnnotation",
                field->options().GetExtension(sharkbuf_unittest::fieldopt).s());
            EXPECT_EQ("EnumAnnotation",
                enumd->options().GetExtension(sharkbuf_unittest::enumopt).s());
            EXPECT_EQ("EnumValueAnnotation",
                enumv->options().GetExtension(sharkbuf_unittest::enumvalopt).s());
            EXPECT_EQ("ServiceAnnotation",
                service->options().GetExtension(sharkbuf_unittest::serviceopt).s());
            EXPECT_EQ("MethodAnnotation",
                method->options().GetExtension(sharkbuf_unittest::methodopt).s());
        }

        TEST(CustomOptions, UnusedImportError) {
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);

            FileDescriptorProto file_proto;
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            FileDescriptorProto any_proto;
            shark::Any::descriptor()->file()->CopyTo(&any_proto);
            ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);

            sharkbuf_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
                &file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            pool.AddUnusedImportTrackFile("custom_options_import.shark", true);
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"custom_options_import.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"tests/sk/lib/unittest_custom_options.shark\" ",
                &file_proto));

            MockErrorCollector error_collector;
            EXPECT_FALSE(pool.BuildFileCollectingErrors(file_proto, &error_collector));
            EXPECT_EQ(
                "custom_options_import.shark: "
                "tests/sk/lib/unittest_custom_options.shark: IMPORT: Import "
                "tests/sk/lib/unittest_custom_options.shark is unused.\n",
                error_collector.text_);
        }

        // Verifies that proto files can correctly be parsed, even if the
        // custom options defined in the file are incompatible with those
        // compiled in the binary. See http://b/19276250.
        TEST(CustomOptions, OptionsWithIncompatibleDescriptors) {
            DescriptorPool pool;
            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);
            FileDescriptorProto file_proto;
            MessageOptions::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // Create a new file descriptor proto containing a subset of the
            // messages defined in tests/sk/lib/unittest_custom_options.shark.
            file_proto.Clear();
            file_proto.set_name("sk/unittest_custom_options.shark");
            file_proto.set_package("sharkbuf_unittest");
            file_proto.add_dependency("shark/descriptor.shark");

            // Add the "required_enum_opt" extension.
            FieldDescriptorProto* extension = file_proto.add_extension();
            sharkbuf_unittest::OldOptionType::descriptor()
                ->file()
                ->FindExtensionByName("required_enum_opt")
                ->CopyTo(extension);

            // Add a test message that uses the "required_enum_opt" option.
            DescriptorProto* test_message_type = file_proto.add_message_type();
            sharkbuf_unittest::TestMessageWithRequiredEnumOption::descriptor()->CopyTo(
                test_message_type);

            // Instruct the extension to use NewOptionType instead of
            // OldOptionType, and add the descriptor of NewOptionType.
            extension->set_type_name(".sharkbuf_unittest.NewOptionType");
            DescriptorProto* new_option_type = file_proto.add_message_type();
            sharkbuf_unittest::NewOptionType::descriptor()->CopyTo(new_option_type);

            // Replace the value of the "required_enum_opt" option used in the
            // test message with an enum value that only exists in NewOptionType.
            ASSERT_TRUE(
                TextFormat::ParseFromString("uninterpreted_option { "
                                            "  name { "
                                            "    name_part: 'required_enum_opt' "
                                            "    is_extension: true "
                                            "  } "
                                            "  aggregate_value: 'value: NEW_VALUE'"
                                            "}",
                    test_message_type->mutable_options()));

            // NEW_VALUE fits the underlying type, so the file builds.
            EXPECT_TRUE(pool.BuildFile(file_proto) != nullptr);
        }

        // Test that FileDescriptor::DebugString() formats custom options correctly.
        TEST(CustomOptions, DebugString) {
            DescriptorPool pool;

            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);
            FileDescriptorProto file_proto;
            MessageOptions::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // Add "foo.shark":
            //   import "shark/descriptor.shark";
            //   package "sharkbuf_unittest";
            //   option (sharkbuf_unittest.cc_option1) = 1;
            //   option (sharkbuf_unittest.cc_option2) = 2;
            //   extend shark.FieldOptions {
            //     optional int32 cc_option1 = 7736974;
            //     optional int32 cc_option2 = 7736975;
            //   }
            ASSERT_TRUE(TextFormat::ParseFromString(
                "name: \"foo.shark\" "
                "package: \"sharkbuf_unittest\" "
                "dependency: \"shark/descriptor.shark\" "
                "options { "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"sharkbuf_unittest.cc_option1\" "
                "      is_extension: true "
                "    } "
                "    positive_int_value: 1 "
                "  } "
                "  uninterpreted_option { "
                "    name { "
                "      name_part: \"sharkbuf_unittest.cc_option2\" "
                "      is_extension: true "
                "    } "
                "    positive_int_value: 2 "
                "  } "
                "} "
                "extension { "
                "  name: \"cc_option1\" "
                "  extendee: \".shark.FileOptions\" "
                // This field number is intentionally chosen to be the same as
                // (.fileopt1) defined in sk/unittest_custom_options.shark (linked
                // in this test binary). This is to test whether we are messing
                // generated pool with custom descriptor pools when dealing with
                // custom options.
                "  number: 7736974 "
                "  label: OPTIONAL "
                "  type:INT32 "
                "}"
                "extension { "
                "  name: \"cc_option2\" "
                "  extendee: \".shark.FileOptions\" "
                "  number: 7736975 "
                "  label: OPTIONAL "
                "  type:INT32 "
                "}",
                &file_proto));
            const FileDescriptor* descriptor = pool.BuildFile(file_proto);
            ASSERT_TRUE(descriptor != nullptr);

            EXPECT_EQ(2, descriptor->extension_count());

            ASSERT_EQ(
                "syntax = \"proto2\";\n"
                "\n"
                "import \"shark/descriptor.shark\";\n"
                "package sharkbuf_unittest;\n"
                "\n"
                "option (.sharkbuf_unittest.cc_option1) = 1;\n"
                "option (.sharkbuf_unittest.cc_option2) = 2;\n"
                "\n"
                "extend .shark.FileOptions {\n"
                "  optional int32 cc_option1 = 7736974;\n"
                "  optional int32 cc_option2 = 7736975;\n"
                "}\n"
                "\n",
                descriptor->DebugString());
        }

        // ===================================================================

        class ValidationErrorTest : public testing::Test {
        protected:
            // Parse file_text as a FileDescriptorProto in text format and add it
            // to the DescriptorPool.  Expect no errors.
            const FileDescriptor* BuildFile(const std::string& file_text) {
                FileDescriptorProto file_proto;
                EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
                return GOOGLE_CHECK_NOTNULL(pool_.BuildFile(file_proto));
            }

            // Parse file_text as a FileDescriptorProto in text format and add it
            // to the DescriptorPool.  Expect errors to be produced which match the
            // given error text.
            void BuildFileWithErrors(const std::string& file_text,
                const std::string& expected_errors) {
                FileDescriptorProto file_proto;
                ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));

                MockErrorCollector error_collector;
                EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector) == nullptr);
                EXPECT_EQ(expected_errors, error_collector.text_);
            }

            // Parse file_text as a FileDescriptorProto in text format and add it
            // to the DescriptorPool.  Expect errors to be produced which match the
            // given warning text.
            void BuildFileWithWarnings(const std::string& file_text,
                const std::string& expected_warnings) {
                FileDescriptorProto file_proto;
                ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));

                MockErrorCollector error_collector;
                EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector));
                EXPECT_EQ(expected_warnings, error_collector.warning_text_);
            }

            // Builds some already-parsed file in our test pool.
            void BuildFileInTestPool(const FileDescriptor* file) {
                const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
                ASSERT_NE(meta_fd, nullptr);
                FileDescriptorProto meta_proto;
                meta_fd->CopyTo(&meta_proto);
                ASSERT_NE(pool_.BuildFile(meta_proto), nullptr);
                FileDescriptorProto file_proto;
                file->CopyTo(&file_proto);
                ASSERT_TRUE(pool_.BuildFile(file_proto) != nullptr);
            }

            // Build descriptor.shark in our test pool. This allows us to extend it in
            // the test pool, so we can test custom options.
            void BuildDescriptorMessagesInTestPool() {
                BuildFileInTestPool(DescriptorProto::descriptor()->file());
            }

            DescriptorPool pool_;
        };

        TEST_F(ValidationErrorTest, AlreadyDefined) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" }"
                "message_type { name: \"Foo\" }",

                "foo.shark: Foo: NAME: \"Foo\" is already defined.\n");
        }

        TEST_F(ValidationErrorTest, AlreadyDefinedInPackage) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "package: \"foo.bar\" "
                "message_type { name: \"Foo\" }"
                "message_type { name: \"Foo\" }",

                "foo.shark: foo.bar.Foo: NAME: \"Foo\" is already defined in "
                "\"foo.bar\".\n");
        }

        TEST_F(ValidationErrorTest, AlreadyDefinedInOtherFile) {
            BuildFile(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" }");

            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "message_type { name: \"Foo\" }",

                "bar.shark: Foo: NAME: \"Foo\" is already defined in file "
                "\"foo.shark\".\n");
        }

        TEST_F(ValidationErrorTest, PackageAlreadyDefined) {
            BuildFile(
                "name: \"foo.shark\" "
                "message_type { name: \"foo\" }");
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "package: \"foo.bar\"",

                "bar.shark: foo: NAME: \"foo\" is already defined (as something other "
                "than a package) in file \"foo.shark\".\n");
        }

        TEST_F(ValidationErrorTest, MissingName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { }",

                "foo.shark: : NAME: Missing name.\n");
        }

        TEST_F(ValidationErrorTest, InvalidName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"$\" }",

                "foo.shark: $: NAME: \"$\" is not a valid identifier.\n");
        }

        TEST_F(ValidationErrorTest, InvalidPackageName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "package: \"foo.$\"",

                "foo.shark: foo.$: NAME: \"$\" is not a valid identifier.\n");
        }

// 'str' is a static C-style string that may contain '\0'
#define STATIC_STR(str) std::string((str), sizeof(str) - 1)

        TEST_F(ValidationErrorTest, NullCharSymbolName) {
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "package: \"foo\""
                "message_type { "
                "  name: '\\000\\001\\013.Bar' "
                "  field { name: \"foo\" number:  9 label:OPTIONAL type:INT32 "
                "} "
                "}",
                STATIC_STR("bar.shark: foo.\0\x1\v.Bar: NAME: \"\0\x1\v.Bar\" is not a "
                           "valid identifier.\nbar.shark: foo.\0\x1\v.Bar.foo: NAME: "
                           "\"foo.\0\x1\v.Bar.foo\" contains null character.\nbar.shark: "
                           "foo.\0\x1\v.Bar: NAME: \"foo.\0\x1\v.Bar\" contains null "
                           "character.\n"));
        }

        TEST_F(ValidationErrorTest, NullCharFileName) {
            BuildFileWithErrors(
                "name: \"bar\\000\\001\\013.shark\" "
                "package: \"outer.foo\"",
                STATIC_STR("bar\0\x1\v.shark: bar\0\x1\v.shark: NAME: "
                           "\"bar\0\x1\v.shark\" contains null character.\n"));
        }

        TEST_F(ValidationErrorTest, NullCharPackageName) {
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "package: \"\\000\\001\\013.\"",
                STATIC_STR("bar.shark: \0\x1\v.: NAME: \"\0\x1\v.\" contains null "
                           "character.\n"));
        }

        TEST_F(ValidationErrorTest, MissingFileName) {
            BuildFileWithErrors("",

                ": : OTHER: Missing field: FileDescriptorProto.name.\n");
        }

        TEST_F(ValidationErrorTest, DupeDependency) {
            BuildFile("name: \"foo.shark\"");
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "dependency: \"foo.shark\" "
                "dependency: \"foo.shark\" ",

                "bar.shark: foo.shark: IMPORT: Import \"foo.shark\" was listed twice.\n");
        }

        TEST_F(ValidationErrorTest, UnknownDependency) {
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "dependency: \"foo.shark\" ",

                "bar.shark: foo.shark: IMPORT: Import \"foo.shark\" has not been "
                "loaded.\n");
        }

        TEST_F(ValidationErrorTest, InvalidPublicDependencyIndex) {
            BuildFile("name: \"foo.shark\"");
            BuildFileWithErrors(
                "name: \"bar.shark\" "
                "dependency: \"foo.shark\" "
                "public_dependency: 1",
                "bar.shark: bar.shark: OTHER: Invalid public dependency index.\n");
        }

        TEST_F(ValidationErrorTest, ForeignUnimportedPackageNoCrash) {
            // Used to crash:  If we depend on a non-existent file and then refer to a
            // package defined in a file that we didn't import, and that package is
            // nested within a parent package which this file is also in, and we don't
            // include that parent package in the name (i.e. we do a relative lookup)...
            // Yes, really.
            BuildFile(
                "name: 'foo.shark' "
                "package: 'outer.foo' ");
            BuildFileWithErrors(
                "name: 'bar.shark' "
                "dependency: 'baz.shark' "
                "package: 'outer.bar' "
                "message_type { "
                "  name: 'Bar' "
                "  field { name:'bar' number:1 label:OPTIONAL type_name:'foo.Foo' }"
                "}",

                "bar.shark: baz.shark: IMPORT: Import \"baz.shark\" has not been "
                "loaded.\n"
                "bar.shark: outer.bar.Bar.bar: TYPE: \"outer.foo\" seems to be defined "
                "in "
                "\"foo.shark\", which is not imported by \"bar.shark\".  To use it here, "
                "please add the necessary import.\n");
        }

        TEST_F(ValidationErrorTest, DupeFile) {
            BuildFile(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" }");
            // Note:  We should *not* get redundant errors about "Foo" already being
            //   defined.
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" } "
                // Add another type so that the files aren't identical (in which case
                // there would be no error).
                "enum_type { name: \"Bar\" }",

                "foo.shark: foo.shark: OTHER: A file with this name is already in the "
                "pool.\n");
        }

        TEST_F(ValidationErrorTest, FieldInExtensionRange) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number:  9 label:OPTIONAL type:INT32 "
                "}"
                "  field { name: \"bar\" number: 10 label:OPTIONAL type:INT32 "
                "}"
                "  field { name: \"baz\" number: 19 label:OPTIONAL type:INT32 "
                "}"
                "  field { name: \"moo\" number: 20 label:OPTIONAL type:INT32 "
                "}"
                "  extension_range { start: 10 end: 20 }"
                "}",

                "foo.shark: Foo.bar: NUMBER: Extension range 10 to 19 includes field "
                "\"bar\" (10).\n"
                "foo.shark: Foo.baz: NUMBER: Extension range 10 to 19 includes field "
                "\"baz\" (19).\n"
                "foo.shark: Foo: NUMBER: Suggested field numbers for Foo: 1, 2\n");
        }

        TEST_F(ValidationErrorTest, OverlappingExtensionRanges) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 10 end: 20 }"
                "  extension_range { start: 20 end: 30 }"
                "  extension_range { start: 19 end: 21 }"
                "}",

                "foo.shark: Foo: NUMBER: Extension range 19 to 20 overlaps with "
                "already-defined range 10 to 19.\n"
                "foo.shark: Foo: NUMBER: Extension range 19 to 20 overlaps with "
                "already-defined range 20 to 29.\n");
        }

        TEST_F(ValidationErrorTest, ReservedFieldError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: 15 label:OPTIONAL type:INT32 "
                "}"
                "  reserved_range { start: 10 end: 20 }"
                "}",

                "foo.shark: Foo.foo: NUMBER: Field \"foo\" uses reserved number 15.\n"
                "foo.shark: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
        }

        TEST_F(ValidationErrorTest, ReservedExtensionRangeError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 10 end: 20 }"
                "  reserved_range { start: 5 end: 15 }"
                "}",

                "foo.shark: Foo: NUMBER: Extension range 10 to 19"
                " overlaps with reserved range 5 to 14.\n");
        }

        TEST_F(ValidationErrorTest, ReservedExtensionRangeAdjacent) {
            BuildFile(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 10 end: 20 }"
                "  reserved_range { start: 5 end: 10 }"
                "}");
        }

        TEST_F(ValidationErrorTest, ReservedRangeOverlap) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  reserved_range { start: 10 end: 20 }"
                "  reserved_range { start: 5 end: 15 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range 5 to 14"
                " overlaps with already-defined range 10 to 19.\n");
        }

        TEST_F(ValidationErrorTest, ReservedNameError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: 15 label:OPTIONAL type:INT32 "
                "}"
                "  field { name: \"bar\" number: 16 label:OPTIONAL type:INT32 "
                "}"
                "  field { name: \"baz\" number: 17 label:OPTIONAL type:INT32 "
                "}"
                "  reserved_name: \"foo\""
                "  reserved_name: \"bar\""
                "}",

                "foo.shark: Foo.foo: NAME: Field name \"foo\" is reserved.\n"
                "foo.shark: Foo.bar: NAME: Field name \"bar\" is reserved.\n");
        }

        TEST_F(ValidationErrorTest, ReservedNameRedundant) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  reserved_name: \"foo\""
                "  reserved_name: \"foo\""
                "}",

                "foo.shark: foo: NAME: Field name \"foo\" is reserved multiple times.\n");
        }

        TEST_F(ValidationErrorTest, ReservedFieldsDebugString) {
            const FileDescriptor* file = BuildFile(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  reserved_name: \"foo\""
                "  reserved_name: \"bar\""
                "  reserved_range { start: 5 end: 6 }"
                "  reserved_range { start: 10 end: 20 }"
                "}");

            ASSERT_EQ(
                "syntax = \"proto2\";\n\n"
                "message Foo {\n"
                "  reserved 5, 10 to 19;\n"
                "  reserved \"foo\", \"bar\";\n"
                "}\n\n",
                file->DebugString());
        }

        TEST_F(ValidationErrorTest, DebugStringReservedRangeMax) {
            const FileDescriptor* file = BuildFile(strings::Substitute(
                "name: \"foo.shark\" "
                "enum_type { "
                "  name: \"Bar\""
                "  value { name:\"BAR\" number:1 }"
                "  reserved_range { start: 5 end: $0 }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  reserved_range { start: 5 end: $1 }"
                "}",
                std::numeric_limits<int>::max(), FieldDescriptor::kMaxNumber + 1));

            ASSERT_EQ(
                "syntax = \"proto2\";\n\n"
                "enum Bar {\n"
                "  BAR = 1;\n"
                "  reserved 5 to max;\n"
                "}\n\n"
                "message Foo {\n"
                "  reserved 5 to max;\n"
                "}\n\n",
                file->DebugString());
        }

        TEST_F(ValidationErrorTest, EnumReservedFieldError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:15 }"
                "  reserved_range { start: 10 end: 20 }"
                "}",

                "foo.shark: Foo.BAR: NUMBER: Enum value \"BAR\" uses reserved number 15.\n");
        }

        TEST_F(ValidationErrorTest, EnumNegativeReservedFieldError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:-15 }"
                "  reserved_range { start: -20 end: -10 }"
                "}",

                "foo.shark: Foo.BAR: NUMBER: Enum value \"BAR\" uses reserved number -15.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedRangeOverlap) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:0 }"
                "  reserved_range { start: 10 end: 20 }"
                "  reserved_range { start: 5 end: 15 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range 5 to 15"
                " overlaps with already-defined range 10 to 20.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedRangeOverlapByOne) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:0 }"
                "  reserved_range { start: 10 end: 20 }"
                "  reserved_range { start: 5 end: 10 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range 5 to 10"
                " overlaps with already-defined range 10 to 20.\n");
        }

        TEST_F(ValidationErrorTest, EnumNegativeReservedRangeOverlap) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:0 }"
                "  reserved_range { start: -20 end: -10 }"
                "  reserved_range { start: -15 end: -5 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range -15 to -5"
                " overlaps with already-defined range -20 to -10.\n");
        }

        TEST_F(ValidationErrorTest, EnumMixedReservedRangeOverlap) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:20 }"
                "  reserved_range { start: -20 end: 10 }"
                "  reserved_range { start: -15 end: 5 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range -15 to 5"
                " overlaps with already-defined range -20 to 10.\n");
        }

        TEST_F(ValidationErrorTest, EnumMixedReservedRangeOverlap2) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:20 }"
                "  reserved_range { start: -20 end: 10 }"
                "  reserved_range { start: 10 end: 10 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range 10 to 10"
                " overlaps with already-defined range -20 to 10.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedRangeStartGreaterThanEnd) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"BAR\" number:20 }"
                "  reserved_range { start: 11 end: 10 }"
                "}",

                "foo.shark: Foo: NUMBER: Reserved range end number must be greater"
                " than start number.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedNameError) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"FOO\" number:15 }"
                "  value { name:\"BAR\" number:15 }"
                "  reserved_name: \"FOO\""
                "  reserved_name: \"BAR\""
                "}",

                "foo.shark: Foo.FOO: NAME: Enum value \"FOO\" is reserved.\nfoo.shark: Foo.BAR: NAME: Enum value \"BAR\" is reserved.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedNameRedundant) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"FOO\" number:15 }"
                "  reserved_name: \"foo\""
                "  reserved_name: \"foo\""
                "}",

                "foo.shark: foo: NAME: Enum value \"foo\" is reserved multiple times.\n");
        }

        TEST_F(ValidationErrorTest, EnumReservedFieldsDebugString) {
            const FileDescriptor* file = BuildFile(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Foo\""
                "  value { name:\"FOO\" number:3 }"
                "  reserved_name: \"foo\""
                "  reserved_name: \"bar\""
                "  reserved_range { start: -6 end: -6 }"
                "  reserved_range { start: -5 end: -4 }"
                "  reserved_range { start: -1 end: 1 }"
                "  reserved_range { start: 5 end: 5 }"
                "  reserved_range { start: 10 end: 19 }"
                "}");

            ASSERT_EQ(
                "syntax = \"proto2\";\n\n"
                "enum Foo {\n"
                "  FOO = 3;\n"
                "  reserved -6, -5 to -4, -1 to 1, 5, 10 to 19;\n"
                "  reserved \"foo\", \"bar\";\n"
                "}\n\n",
                file->DebugString());
        }

        TEST_F(ValidationErrorTest, InvalidDefaults) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""

                // Invalid number.
                "  field { name: \"foo\" number: 1 label: OPTIONAL type:INT32"
                "          default_value: \"abc\" }"

                // Empty default value.
                "  field { name: \"bar\" number: 2 label: OPTIONAL type:INT32"
                "          default_value: \"\" }"

                // Invalid boolean.
                "  field { name: \"baz\" number: 3 label: OPTIONAL type:BOOL"
                "          default_value: \"abc\" }"

                // Messages can't have defaults.
                "  field { name: \"moo\" number: 4 label: OPTIONAL type: "
                "MESSAGE"
                "          default_value: \"abc\" type_name: \"Foo\" }"

                // Same thing, but we don't know that this field has message type until
                // we look up the type name.
                "  field { name: \"mooo\" number: 5 label: OPTIONAL"
                "          default_value: \"abc\" type_name: \"Foo\" }"

                // Repeateds can't have defaults.
                "  field { name: \"corge\" number: 6 label: REPEATED type: "
                "INT32"
                "          default_value: \"1\" }"
                "}",

                "foo.shark: Foo.foo: DEFAULT_VALUE: Couldn't parse default value "
                "\"abc\".\n"
                "foo.shark: Foo.bar: DEFAULT_VALUE: Couldn't parse default value \"\".\n"
                "foo.shark: Foo.baz: DEFAULT_VALUE: Boolean default must be true or "
                "false.\n"
                "foo.shark: Foo.moo: DEFAULT_VALUE: Messages can't have default values.\n"
                "foo.shark: Foo.corge: DEFAULT_VALUE: Repeated fields can't have default "
                "values.\n"
                // This ends up being reported later because the error is detected at
                // cross-linking time.
                "foo.shark: Foo.mooo: DEFAULT_VALUE: Messages can't have default "
                "values.\n");
        }

        TEST_F(ValidationErrorTest, NegativeFieldNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: -1 label:OPTIONAL type:INT32 "
                "}"
                "}",

                "foo.shark: Foo.foo: NUMBER: Field numbers must be positive integers.\n"
                "foo.shark: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
        }

        TEST_F(ValidationErrorTest, HugeFieldNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: 0x70000000 "
                "          label:OPTIONAL type:INT32 }"
                "}",

                "foo.shark: Foo.foo: NUMBER: Field numbers cannot be greater than "
                "536870911.\n"
                "foo.shark: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
        }

        TEST_F(ValidationErrorTest, ReservedFieldNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field {name:\"foo\" number: 18999 label:OPTIONAL "
                "type:INT32 }"
                "  field {name:\"bar\" number: 19000 label:OPTIONAL "
                "type:INT32 }"
                "  field {name:\"baz\" number: 19999 label:OPTIONAL "
                "type:INT32 }"
                "  field {name:\"moo\" number: 20000 label:OPTIONAL "
                "type:INT32 }"
                "}",

                "foo.shark: Foo.bar: NUMBER: Field numbers 19000 through 19999 are "
                "reserved for the protocol buffer library implementation.\n"
                "foo.shark: Foo.baz: NUMBER: Field numbers 19000 through 19999 are "
                "reserved for the protocol buffer library implementation.\n"
                "foo.shark: Foo: NUMBER: Suggested field numbers for Foo: 1, 2\n");
        }

        TEST_F(ValidationErrorTest, ExtensionMissingExtendee) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension { name: \"foo\" number: 1 label: OPTIONAL"
                "              type_name: \"Foo\" }"
                "}",

                "foo.shark: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee not set for "
                "extension field.\n");
        }

        TEST_F(ValidationErrorTest, NonExtensionWithExtendee) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "  extension_range { start: 1 end: 2 }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: 1 label: OPTIONAL"
                "          type_name: \"Foo\" extendee: \"Bar\" }"
                "}",

                "foo.shark: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee set for "
                "non-extension field.\n");
        }

        TEST_F(ValidationErrorTest, FieldOneofIndexTooLarge) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:INT32 "
                "          oneof_index: 1 }"
                "  field { name:\"dummy\" number:2 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  oneof_decl { name:\"bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: FieldDescriptorProto.oneof_index 1 is out of "
                "range for type \"Foo\".\n");
        }

        TEST_F(ValidationErrorTest, FieldOneofIndexNegative) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:INT32 "
                "          oneof_index: -1 }"
                "  field { name:\"dummy\" number:2 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  oneof_decl { name:\"bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: FieldDescriptorProto.oneof_index -1 is out "
                "of "
                "range for type \"Foo\".\n");
        }

        TEST_F(ValidationErrorTest, OneofFieldsConsecutiveDefinition) {
            // Fields belonging to the same oneof must be defined consecutively.
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo1\" number: 1 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  field { name:\"bar\" number: 2 label:OPTIONAL type:INT32 }"
                "  field { name:\"foo2\" number: 3 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  oneof_decl { name:\"foos\" }"
                "}",

                "foo.shark: Foo.bar: TYPE: Fields in the same oneof must be defined "
                "consecutively. \"bar\" cannot be defined before the completion of the "
                "\"foos\" oneof definition.\n");

            // Prevent interleaved fields, which belong to different oneofs.
            BuildFileWithErrors(
                "name: \"foo2.shark\" "
                "message_type {"
                "  name: \"Foo2\""
                "  field { name:\"foo1\" number: 1 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  field { name:\"bar1\" number: 2 label:OPTIONAL type:INT32 "
                "          oneof_index: 1 }"
                "  field { name:\"foo2\" number: 3 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  field { name:\"bar2\" number: 4 label:OPTIONAL type:INT32 "
                "          oneof_index: 1 }"
                "  oneof_decl { name:\"foos\" }"
                "  oneof_decl { name:\"bars\" }"
                "}",
                "foo2.shark: Foo2.bar1: TYPE: Fields in the same oneof must be defined "
                "consecutively. \"bar1\" cannot be defined before the completion of the "
                "\"foos\" oneof definition.\n"
                "foo2.shark: Foo2.foo2: TYPE: Fields in the same oneof must be defined "
                "consecutively. \"foo2\" cannot be defined before the completion of the "
                "\"bars\" oneof definition.\n");

            // Another case for normal fields and different oneof fields interleave.
            BuildFileWithErrors(
                "name: \"foo3.shark\" "
                "message_type {"
                "  name: \"Foo3\""
                "  field { name:\"foo1\" number: 1 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  field { name:\"bar1\" number: 2 label:OPTIONAL type:INT32 "
                "          oneof_index: 1 }"
                "  field { name:\"baz\" number: 3 label:OPTIONAL type:INT32 }"
                "  field { name:\"foo2\" number: 4 label:OPTIONAL type:INT32 "
                "          oneof_index: 0 }"
                "  oneof_decl { name:\"foos\" }"
                "  oneof_decl { name:\"bars\" }"
                "}",
                "foo3.shark: Foo3.baz: TYPE: Fields in the same oneof must be defined "
                "consecutively. \"baz\" cannot be defined before the completion of the "
                "\"foos\" oneof definition.\n");
        }

        TEST_F(ValidationErrorTest, FieldNumberConflict) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"foo\" number: 1 label:OPTIONAL type:INT32 }"
                "  field { name: \"bar\" number: 1 label:OPTIONAL type:INT32 }"
                "}",

                "foo.shark: Foo.bar: NUMBER: Field number 1 has already been used in "
                "\"Foo\" by field \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, BadMessageSetExtensionType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"MessageSet\""
                "  options { message_set_wire_format: true }"
                "  extension_range { start: 4 end: 5 }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  extension { name:\"foo\" number:4 label:OPTIONAL type:INT32"
                "              extendee: \"MessageSet\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: Extensions of MessageSets must be optional "
                "messages.\n");
        }

        TEST_F(ValidationErrorTest, BadMessageSetExtensionLabel) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"MessageSet\""
                "  options { message_set_wire_format: true }"
                "  extension_range { start: 4 end: 5 }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  extension { name:\"foo\" number:4 label:REPEATED "
                "type:MESSAGE"
                "              type_name: \"Foo\" extendee: \"MessageSet\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: Extensions of MessageSets must be optional "
                "messages.\n");
        }

        TEST_F(ValidationErrorTest, FieldInMessageSet) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  options { message_set_wire_format: true }"
                "  field { name: \"foo\" number: 1 label:OPTIONAL type:INT32 }"
                "}",

                "foo.shark: Foo.foo: NAME: MessageSets cannot have fields, only "
                "extensions.\n");
        }

        TEST_F(ValidationErrorTest, NegativeExtensionRangeNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: -10 end: -1 }"
                "}",

                "foo.shark: Foo: NUMBER: Extension numbers must be positive integers.\n");
        }

        TEST_F(ValidationErrorTest, HugeExtensionRangeNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 1 end: 0x70000000 }"
                "}",

                "foo.shark: Foo: NUMBER: Extension numbers cannot be greater than "
                "536870911.\n");
        }

        TEST_F(ValidationErrorTest, ExtensionRangeEndBeforeStart) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 10 end: 10 }"
                "  extension_range { start: 10 end: 5 }"
                "}",

                "foo.shark: Foo: NUMBER: Extension range end number must be greater than "
                "start number.\n"
                "foo.shark: Foo: NUMBER: Extension range end number must be greater than "
                "start number.\n");
        }

        TEST_F(ValidationErrorTest, EmptyEnum) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type { name: \"Foo\" }"
                // Also use the empty enum in a message to make sure there are no crashes
                // during validation (possible if the code attempts to derive a default
                // value for the field).
                "message_type {"
                "  name: \"Bar\""
                "  field { name: \"foo\" number: 1 label:OPTIONAL "
                "type_name:\"Foo\" }"
                "  field { name: \"bar\" number: 2 label:OPTIONAL "
                "type_name:\"Foo\" "
                "          default_value: \"NO_SUCH_VALUE\" }"
                "}",

                "foo.shark: Foo: NAME: Enums must contain at least one value.\n"
                "foo.shark: Bar.bar: DEFAULT_VALUE: Enum type \"Foo\" has no value named "
                "\"NO_SUCH_VALUE\".\n");
        }

        TEST_F(ValidationErrorTest, UndefinedExtendee) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension { name:\"foo\" number:1 label:OPTIONAL type:INT32"
                "              extendee: \"Bar\" }"
                "}",

                "foo.shark: Foo.foo: EXTENDEE: \"Bar\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, NonMessageExtendee) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } }"
                "message_type {"
                "  name: \"Foo\""
                "  extension { name:\"foo\" number:1 label:OPTIONAL type:INT32"
                "              extendee: \"Bar\" }"
                "}",

                "foo.shark: Foo.foo: EXTENDEE: \"Bar\" is not a message type.\n");
        }

        TEST_F(ValidationErrorTest, NotAnExtensionNumber) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  extension { name:\"foo\" number:1 label:OPTIONAL type:INT32"
                "              extendee: \"Bar\" }"
                "}",

                "foo.shark: Foo.foo: NUMBER: \"Bar\" does not declare 1 as an extension "
                "number.\n");
        }

        TEST_F(ValidationErrorTest, RequiredExtension) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "  extension_range { start: 1000 end: 10000 }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  extension {"
                "    name:\"foo\""
                "    number:1000"
                "    label:REQUIRED"
                "    type:INT32"
                "    extendee: \"Bar\""
                "  }"
                "}",

                "foo.shark: Foo.foo: TYPE: The extension Foo.foo cannot be required.\n");
        }

        TEST_F(ValidationErrorTest, UndefinedFieldType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: \"Bar\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, UndefinedFieldTypeWithDefault) {
            // See b/12533582. Previously this failed because the default value was not
            // accepted by the parser, which assumed an enum type, leading to an unclear
            // error message. We want this input to yield a validation error instead,
            // since the unknown type is the primary problem.
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"int\" "
                "          default_value:\"1\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: \"int\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, UndefinedNestedFieldType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  nested_type { name:\"Baz\" }"
                "  field { name:\"foo\" number:1"
                "          label:OPTIONAL"
                "          type_name:\"Foo.Baz.Bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: \"Foo.Baz.Bar\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, FieldTypeDefinedInUndeclaredDependency) {
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" } ");

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}",
                "foo.shark: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.shark\", "
                "which is not imported by \"foo.shark\".  To use it here, please add the "
                "necessary import.\n");
        }

        TEST_F(ValidationErrorTest, FieldTypeDefinedInIndirectDependency) {
            // Test for hidden dependencies.
            //
            // // bar.shark
            // message Bar{}
            //
            // // forward.shark
            // import "bar.shark"
            //
            // // foo.shark
            // import "forward.shark"
            // message Foo {
            //   optional Bar foo = 1;  // Error, needs to import bar.shark explicitly.
            // }
            //
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" }");

            BuildFile(
                "name: \"forward.shark\""
                "dependency: \"bar.shark\"");

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"forward.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}",
                "foo.shark: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.shark\", "
                "which is not imported by \"foo.shark\".  To use it here, please add the "
                "necessary import.\n");
        }

        TEST_F(ValidationErrorTest, FieldTypeDefinedInPublicDependency) {
            // Test for public dependencies.
            //
            // // bar.shark
            // message Bar{}
            //
            // // forward.shark
            // import public "bar.shark"
            //
            // // foo.shark
            // import "forward.shark"
            // message Foo {
            //   optional Bar foo = 1;  // Correct. "bar.shark" is public imported into
            //                          // forward.shark, so when "foo.shark" imports
            //                          // "forward.shark", it imports "bar.shark" too.
            // }
            //
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" }");

            BuildFile(
                "name: \"forward.shark\""
                "dependency: \"bar.shark\" "
                "public_dependency: 0");

            BuildFile(
                "name: \"foo.shark\" "
                "dependency: \"forward.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}");
        }

        TEST_F(ValidationErrorTest, FieldTypeDefinedInTransitivePublicDependency) {
            // Test for public dependencies.
            //
            // // bar.shark
            // message Bar{}
            //
            // // forward.shark
            // import public "bar.shark"
            //
            // // forward2.shark
            // import public "forward.shark"
            //
            // // foo.shark
            // import "forward2.shark"
            // message Foo {
            //   optional Bar foo = 1;  // Correct, public imports are transitive.
            // }
            //
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" }");

            BuildFile(
                "name: \"forward.shark\""
                "dependency: \"bar.shark\" "
                "public_dependency: 0");

            BuildFile(
                "name: \"forward2.shark\""
                "dependency: \"forward.shark\" "
                "public_dependency: 0");

            BuildFile(
                "name: \"foo.shark\" "
                "dependency: \"forward2.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}");
        }

        TEST_F(ValidationErrorTest,
            FieldTypeDefinedInPrivateDependencyOfPublicDependency) {
            // Test for public dependencies.
            //
            // // bar.shark
            // message Bar{}
            //
            // // forward.shark
            // import "bar.shark"
            //
            // // forward2.shark
            // import public "forward.shark"
            //
            // // foo.shark
            // import "forward2.shark"
            // message Foo {
            //   optional Bar foo = 1;  // Error, the "bar.shark" is not public imported
            //                          // into "forward.shark", so will not be imported
            //                          // into either "forward2.shark" or "foo.shark".
            // }
            //
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" }");

            BuildFile(
                "name: \"forward.shark\""
                "dependency: \"bar.shark\"");

            BuildFile(
                "name: \"forward2.shark\""
                "dependency: \"forward.shark\" "
                "public_dependency: 0");

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"forward2.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}",
                "foo.shark: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.shark\", "
                "which is not imported by \"foo.shark\".  To use it here, please add the "
                "necessary import.\n");
        }

        TEST_F(ValidationErrorTest, SearchMostLocalFirst) {
            // The following should produce an error that Bar.Baz is resolved but
            // not defined:
            //   message Bar { message Baz {} }
            //   message Foo {
            //     message Bar {
            //       // Placing "message Baz{}" here, or removing Foo.Bar altogether,
            //       // would fix the error.
            //     }
            //     optional Bar.Baz baz = 1;
            //   }
            // An one point the lookup code incorrectly did not produce an error in this
            // case, because when looking for Bar.Baz, it would try "Foo.Bar.Baz" first,
            // fail, and ten try "Bar.Baz" and succeed, even though "Bar" should actually
            // refer to the inner Bar, not the outer one.
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "  nested_type { name: \"Baz\" }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  nested_type { name: \"Bar\" }"
                "  field { name:\"baz\" number:1 label:OPTIONAL"
                "          type_name:\"Bar.Baz\" }"
                "}",

                "foo.shark: Foo.baz: TYPE: \"Bar.Baz\" is resolved to \"Foo.Bar.Baz\","
                " which is not defined. The innermost scope is searched first in name "
                "resolution. Consider using a leading '.'(i.e., \".Bar.Baz\") to start "
                "from the outermost scope.\n");
        }

        TEST_F(ValidationErrorTest, SearchMostLocalFirst2) {
            // This test would find the most local "Bar" first, and does, but
            // proceeds to find the outer one because the inner one's not an
            // aggregate.
            BuildFile(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "  nested_type { name: \"Baz\" }"
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  field { name: \"Bar\" number:1 type:BYTES } "
                "  field { name:\"baz\" number:2 label:OPTIONAL"
                "          type_name:\"Bar.Baz\" }"
                "}");
        }

        TEST_F(ValidationErrorTest, PackageOriginallyDeclaredInTransitiveDependent) {
            // Imagine we have the following:
            //
            // foo.shark:
            //   package foo.bar;
            // bar.shark:
            //   package foo.bar;
            //   import "foo.shark";
            //   message Bar {}
            // baz.shark:
            //   package foo;
            //   import "bar.shark"
            //   message Baz { optional bar.Bar moo = 1; }
            //
            // When validating baz.shark, we will look up "bar.Bar".  As part of this
            // lookup, we first lookup "bar" then try to find "Bar" within it.  "bar"
            // should resolve to "foo.bar".  Note, though, that "foo.bar" was originally
            // defined in foo.shark, which is not a direct dependency of baz.shark.  The
            // implementation of FindSymbol() normally only returns symbols in direct
            // dependencies, not indirect ones.  This test insures that this does not
            // prevent it from finding "foo.bar".

            BuildFile(
                "name: \"foo.shark\" "
                "package: \"foo.bar\" ");
            BuildFile(
                "name: \"bar.shark\" "
                "package: \"foo.bar\" "
                "dependency: \"foo.shark\" "
                "message_type { name: \"Bar\" }");
            BuildFile(
                "name: \"baz.shark\" "
                "package: \"foo\" "
                "dependency: \"bar.shark\" "
                "message_type { "
                "  name: \"Baz\" "
                "  field { name:\"moo\" number:1 label:OPTIONAL "
                "          type_name:\"bar.Bar\" }"
                "}");
        }

        TEST_F(ValidationErrorTest, FieldTypeNotAType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL "
                "          type_name:\".Foo.bar\" }"
                "  field { name:\"bar\" number:2 label:OPTIONAL type:INT32 }"
                "}",

                "foo.shark: Foo.foo: TYPE: \".Foo.bar\" is not a type.\n");
        }

        TEST_F(ValidationErrorTest, RelativeFieldTypeNotAType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  nested_type {"
                "    name: \"Bar\""
                "    field { name:\"Baz\" number:2 label:OPTIONAL type:INT32 }"
                "  }"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL "
                "          type_name:\"Bar.Baz\" }"
                "}",
                "foo.shark: Foo.foo: TYPE: \"Bar.Baz\" is not a type.\n");
        }

        TEST_F(ValidationErrorTest, FieldTypeMayBeItsName) {
            BuildFile(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Bar\""
                "}"
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"Bar\" number:1 label:OPTIONAL type_name:\"Bar\" }"
                "}");
        }

        TEST_F(ValidationErrorTest, EnumFieldTypeIsMessage) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Bar\" } "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:ENUM"
                "          type_name:\"Bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: \"Bar\" is not an enum type.\n");
        }

        TEST_F(ValidationErrorTest, MessageFieldTypeIsEnum) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:MESSAGE"
                "          type_name:\"Bar\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: \"Bar\" is not a message type.\n");
        }

        TEST_F(ValidationErrorTest, BadEnumDefaultValue) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\""
                "          default_value:\"NO_SUCH_VALUE\" }"
                "}",

                "foo.shark: Foo.foo: DEFAULT_VALUE: Enum type \"Bar\" has no value named "
                "\"NO_SUCH_VALUE\".\n");
        }

        TEST_F(ValidationErrorTest, EnumDefaultValueIsInteger) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type_name:\"Bar\""
                "          default_value:\"0\" }"
                "}",

                "foo.shark: Foo.foo: DEFAULT_VALUE: Default value for an enum field must "
                "be an identifier.\n");
        }

        TEST_F(ValidationErrorTest, PrimitiveWithTypeName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:INT32"
                "          type_name:\"Foo\" }"
                "}",

                "foo.shark: Foo.foo: TYPE: Field with primitive type has type_name.\n");
        }

        TEST_F(ValidationErrorTest, NonPrimitiveWithoutTypeName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:OPTIONAL type:MESSAGE }"
                "}",

                "foo.shark: Foo.foo: TYPE: Field with message or enum type missing "
                "type_name.\n");
        }

        TEST_F(ValidationErrorTest, OneofWithNoFields) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  oneof_decl { name:\"bar\" }"
                "}",

                "foo.shark: Foo.bar: NAME: Oneof must have at least one field.\n");
        }

        TEST_F(ValidationErrorTest, OneofLabelMismatch) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"Foo\""
                "  field { name:\"foo\" number:1 label:REPEATED type:INT32 "
                "          oneof_index:0 }"
                "  oneof_decl { name:\"bar\" }"
                "}",

                "foo.shark: Foo.foo: NAME: Fields of oneofs must themselves have label "
                "OPTIONAL.\n");
        }

        TEST_F(ValidationErrorTest, InputTypeNotDefined) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" } "
                "service {"
                "  name: \"TestService\""
                "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
                "}",

                "foo.shark: TestService.A: INPUT_TYPE: \"Bar\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, InputTypeNotAMessage) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" } "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
                "service {"
                "  name: \"TestService\""
                "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
                "}",

                "foo.shark: TestService.A: INPUT_TYPE: \"Bar\" is not a message type.\n");
        }

        TEST_F(ValidationErrorTest, OutputTypeNotDefined) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" } "
                "service {"
                "  name: \"TestService\""
                "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
                "}",

                "foo.shark: TestService.A: OUTPUT_TYPE: \"Bar\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, OutputTypeNotAMessage) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { name: \"Foo\" } "
                "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
                "service {"
                "  name: \"TestService\""
                "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
                "}",

                "foo.shark: TestService.A: OUTPUT_TYPE: \"Bar\" is not a message type.\n");
        }

        TEST_F(ValidationErrorTest, IllegalPackedField) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {\n"
                "  name: \"Foo\""
                "  field { name:\"packed_string\" number:1 label:REPEATED "
                "          type:STRING "
                "          options { uninterpreted_option {"
                "            name { name_part: \"packed\" is_extension: false }"
                "            identifier_value: \"true\" }}}\n"
                "  field { name:\"packed_message\" number:3 label:REPEATED "
                "          type_name: \"Foo\""
                "          options { uninterpreted_option {"
                "            name { name_part: \"packed\" is_extension: false }"
                "            identifier_value: \"true\" }}}\n"
                "  field { name:\"optional_int32\" number: 4 label: OPTIONAL "
                "          type:INT32 "
                "          options { uninterpreted_option {"
                "            name { name_part: \"packed\" is_extension: false }"
                "            identifier_value: \"true\" }}}\n"
                "}",

                "foo.shark: Foo.packed_string: TYPE: [packed = true] can only be "
                "specified for repeated primitive fields.\n"
                "foo.shark: Foo.packed_message: TYPE: [packed = true] can only be "
                "specified for repeated primitive fields.\n"
                "foo.shark: Foo.optional_int32: TYPE: [packed = true] can only be "
                "specified for repeated primitive fields.\n");
        }

        TEST_F(ValidationErrorTest, OptionWrongType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { "
                "  name: \"TestMessage\" "
                "  field { name:\"foo\" number:1 label:OPTIONAL type:STRING "
                "          options { uninterpreted_option { name { name_part: \"ctype\" "
                "                                                  is_extension: false }"
                "                                           positive_int_value: 1 }"
                "          }"
                "  }"
                "}\n",

                "foo.shark: TestMessage.foo: OPTION_VALUE: Value must be identifier for "
                "enum-valued option \"shark.FieldOptions.ctype\".\n");
        }

        TEST_F(ValidationErrorTest, OptionExtendsAtomicType) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { "
                "  name: \"TestMessage\" "
                "  field { name:\"foo\" number:1 label:OPTIONAL type:STRING "
                "          options { uninterpreted_option { name { name_part: \"ctype\" "
                "                                                  is_extension: false }"
                "                                           name { name_part: \"foo\" "
                "                                                  is_extension: true }"
                "                                           positive_int_value: 1 }"
                "          }"
                "  }"
                "}\n",

                "foo.shark: TestMessage.foo: OPTION_NAME: Option \"ctype\" is an "
                "atomic type, not a message.\n");
        }

        TEST_F(ValidationErrorTest, DupOption) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { "
                "  name: \"TestMessage\" "
                "  field { name:\"foo\" number:1 label:OPTIONAL type:UINT32 "
                "          options { uninterpreted_option { name { name_part: \"ctype\" "
                "                                                  is_extension: false }"
                "                                           identifier_value: \"CORD\" }"
                "                    uninterpreted_option { name { name_part: \"ctype\" "
                "                                                  is_extension: false }"
                "                                           identifier_value: \"CORD\" }"
                "          }"
                "  }"
                "}\n",

                "foo.shark: TestMessage.foo: OPTION_NAME: Option \"ctype\" was "
                "already set.\n");
        }

        TEST_F(ValidationErrorTest, InvalidOptionName) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type { "
                "  name: \"TestMessage\" "
                "  field { name:\"foo\" number:1 label:OPTIONAL type:BOOL "
                "          options { uninterpreted_option { "
                "                      name { name_part: \"uninterpreted_option\" "
                "                             is_extension: false }"
                "                      positive_int_value: 1 "
                "                    }"
                "          }"
                "  }"
                "}\n",

                "foo.shark: TestMessage.foo: OPTION_NAME: Option must not use "
                "reserved name \"uninterpreted_option\".\n");
        }

        TEST_F(ValidationErrorTest, RepeatedMessageOption) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "message_type: { name: \"Bar\" field: { "
                "  name: \"foo\" number: 1 label: OPTIONAL type:INT32 } "
                "} "
                "extension { name: \"bar\" number: 7672757 label: REPEATED "
                "            type:MESSAGE type_name: \"Bar\" "
                "            extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"bar\" "
                "                                        is_extension: true } "
                "                                 name { name_part: \"foo\" "
                "                                        is_extension: false } "
                "                                 positive_int_value: 1 } }",

                "foo.shark: foo.shark: OPTION_NAME: Option field \"(bar)\" is a "
                "repeated message. Repeated message options must be initialized "
                "using an aggregate value.\n");
        }

        TEST_F(ValidationErrorTest, ResolveUndefinedOption) {
            // The following should produce an error that baz.bar is resolved but not
            // defined.
            // foo.shark:
            //   package baz
            //   import shark/descriptor.shark
            //   message Bar { optional int32 foo = 1; }
            //   extend FileOptions { optional Bar bar = 7672757; }
            //
            // moo.shark:
            //   package moo.baz
            //   option (baz.bar).foo = 1;
            //
            // Although "baz.bar" is already defined, the lookup code will try
            // "moo.baz.bar", since it's the match from the innermost scope, which will
            // cause a symbol not defined error.
            BuildDescriptorMessagesInTestPool();

            BuildFile(
                "name: \"foo.shark\" "
                "package: \"baz\" "
                "dependency: \"shark/descriptor.shark\" "
                "message_type: { name: \"Bar\" field: { "
                "  name: \"foo\" number: 1 label: OPTIONAL type:INT32 } "
                "} "
                "extension { name: \"bar\" number: 7672757 label: OPTIONAL "
                "            type:MESSAGE type_name: \"Bar\" "
                "            extendee: \"shark.FileOptions\" }");

            BuildFileWithErrors(
                "name: \"moo.shark\" "
                "package: \"moo.baz\" "
                "options { uninterpreted_option { name { name_part: \"baz.bar\" "
                "                                        is_extension: true } "
                "                                 name { name_part: \"foo\" "
                "                                        is_extension: false } "
                "                                 positive_int_value: 1 } }",

                "moo.shark: moo.shark: OPTION_NAME: Option \"(baz.bar)\" is resolved to "
                "\"(moo.baz.bar)\","
                " which is not defined. The innermost scope is searched first in name "
                "resolution. Consider using a leading '.'(i.e., \"(.baz.bar)\") to start "
                "from the outermost scope.\n");
        }

        TEST_F(ValidationErrorTest, UnknownOption) {
            BuildFileWithErrors(
                "name: \"moo.shark\" "
                "package: \"moo.baz\" "
                "options { uninterpreted_option { name { name_part: \"baaz.bar\" "
                "                                        is_extension: true } "
                "                                 name { name_part: \"foo\" "
                "                                        is_extension: false } "
                "                                 positive_int_value: 1 } }",

                "moo.shark: moo.shark: OPTION_NAME: Option \"(baaz.bar)\" unknown. "
                "Ensure "
                "that your proto definition file imports the proto which defines the "
                "option.\n");
        }

        TEST_F(ValidationErrorTest, CustomOptionConflictingFieldNumber) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo1\" number: 7672757 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FieldOptions\" }"
                "extension { name: \"foo2\" number: 7672757 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FieldOptions\" }",

                "foo.shark: foo2: NUMBER: Extension number 7672757 has already been used "
                "in \"shark.FieldOptions\" by extension \"foo1\".\n");
        }

        TEST_F(ValidationErrorTest, Int32OptionValueOutOfPositiveRange) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 positive_int_value: 0x80000000 } "
                "}",

                "foo.shark: foo.shark: OPTION_VALUE: Value out of range "
                "for int32 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, Int32OptionValueOutOfNegativeRange) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 negative_int_value: -0x80000001 } "
                "}",

                "foo.shark: foo.shark: OPTION_VALUE: Value out of range "
                "for int32 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, Int32OptionValueIsNotPositiveInt) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 string_value: \"5\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be integer "
                "for int32 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, Int64OptionValueOutOfRange) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:INT64 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 positive_int_value: 0x8000000000000000 "
                "} "
                "}",

                "foo.shark: foo.shark: OPTION_VALUE: Value out of range "
                "for int64 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, Int64OptionValueIsNotPositiveInt) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:INT64 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 identifier_value: \"5\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be integer "
                "for int64 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, UInt32OptionValueOutOfRange) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:UINT32 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 positive_int_value: 0x100000000 } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value out of range "
                "for uint32 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, UInt32OptionValueIsNotPositiveInt) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:UINT32 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 double_value: -5.6 } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be non-negative integer "
                "for uint32 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, UInt64OptionValueIsNotPositiveInt) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:UINT64 extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 negative_int_value: -5 } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be non-negative integer "
                "for uint64 option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, FloatOptionValueIsNotNumber) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:FLOAT extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 string_value: \"bar\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be number "
                "for float option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, DoubleOptionValueIsNotNumber) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:DOUBLE extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 string_value: \"bar\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be number "
                "for double option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, BoolOptionValueIsNotTrueOrFalse) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:BOOL extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 identifier_value: \"bar\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be \"true\" or \"false\" "
                "for boolean option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, EnumOptionValueIsNotIdentifier) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
                "                              value { name: \"BAZ\" number: 2 } }"
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:ENUM type_name: \"FooEnum\" "
                "            extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 string_value: \"MOOO\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be identifier for "
                "enum-valued option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, EnumOptionValueIsNotEnumValueName) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
                "                              value { name: \"BAZ\" number: 2 } }"
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:ENUM type_name: \"FooEnum\" "
                "            extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 identifier_value: \"MOOO\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Enum type \"FooEnum\" has no value "
                "named \"MOOO\" for option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, EnumOptionValueIsSiblingEnumValueName) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "enum_type { name: \"FooEnum1\" value { name: \"BAR\" number: 1 } "
                "                               value { name: \"BAZ\" number: 2 } }"
                "enum_type { name: \"FooEnum2\" value { name: \"MOO\" number: 1 } "
                "                               value { name: \"MOOO\" number: 2 } }"
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:ENUM type_name: \"FooEnum1\" "
                "            extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 identifier_value: \"MOOO\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Enum type \"FooEnum1\" has no value named \"MOOO\" for option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, StringOptionValueIsNotString) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:STRING extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 identifier_value: \"MOOO\" } }",

                "foo.shark: foo.shark: OPTION_VALUE: Value must be quoted string "
                "for "
                "string option \"foo\".\n");
        }

        TEST_F(ValidationErrorTest, JsonNameOptionOnExtensions) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "package: \"foo\" "
                "message_type {"
                "  name: \"Foo\""
                "  extension_range { start: 10 end: 20 }"
                "}"
                "extension {"
                "  name: \"value\""
                "  number: 10"
                "  label: OPTIONAL"
                "  type:INT32"
                "  extendee: \"foo.Foo\""
                "  json_name: \"myName\""
                "}",
                "foo.shark: foo.value: OPTION_NAME: option json_name is not allowed on "
                "extension fields.\n");
        }

        TEST_F(ValidationErrorTest, DuplicateExtensionFieldNumber) {
            BuildDescriptorMessagesInTestPool();

            BuildFile(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"option1\" number: 1000 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FileOptions\" }");

            BuildFileWithWarnings(
                "name: \"bar.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "extension { name: \"option2\" number: 1000 label: OPTIONAL "
                "            type:INT32 extendee: \"shark.FileOptions\" }",
                "bar.shark: option2: NUMBER: Extension number 1000 has already been used "
                "in \"shark.FileOptions\" by extension \"option1\" defined in "
                "foo.shark.\n");
        }

        // Helper function for tests that check for aggregate value parsing
        // errors.  The "value" argument is embedded inside the
        // "uninterpreted_option" portion of the result.
        static std::string EmbedAggregateValue(const char* value) {
            return strings::Substitute(
                "name: \"foo.shark\" "
                "dependency: \"shark/descriptor.shark\" "
                "message_type { name: \"Foo\" } "
                "extension { name: \"foo\" number: 7672757 label: OPTIONAL "
                "            type:MESSAGE type_name: \"Foo\" "
                "            extendee: \"shark.FileOptions\" }"
                "options { uninterpreted_option { name { name_part: \"foo\" "
                "                                        is_extension: true } "
                "                                 $0 } }",
                value);
        }

        TEST_F(ValidationErrorTest, AggregateValueNotFound) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                EmbedAggregateValue("string_value: \"\""),
                "foo.shark: foo.shark: OPTION_VALUE: Option \"foo\" is a message. "
                "To set the entire message, use syntax like "
                "\"foo = { <proto text format> }\". To set fields within it, use "
                "syntax like \"foo.foo = value\".\n");
        }

        TEST_F(ValidationErrorTest, AggregateValueParseError) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                EmbedAggregateValue("aggregate_value: \"1+2\""),
                "foo.shark: foo.shark: OPTION_VALUE: Error while parsing option "
                "value for \"foo\": Expected identifier, got: 1\n");
        }

        TEST_F(ValidationErrorTest, AggregateValueUnknownFields) {
            BuildDescriptorMessagesInTestPool();

            BuildFileWithErrors(
                EmbedAggregateValue("aggregate_value: \"x:100\""),
                "foo.shark: foo.shark: OPTION_VALUE: Error while parsing option "
                "value for \"foo\": Message type \"Foo\" has no field named \"x\".\n");
        }

        TEST_F(ValidationErrorTest, NotLiteImportsLite) {
            BuildFile(
                "name: \"bar.shark\" "
                "options { optimize_for: LITE_RUNTIME } ");

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"bar.shark\" ",

                "foo.shark: bar.shark: IMPORT: Files that do not use optimize_for = "
                "LITE_RUNTIME cannot import files which do use this option.  This file "
                "is not lite, but it imports \"bar.shark\" which is.\n");
        }

        TEST_F(ValidationErrorTest, LiteExtendsNotLite) {
            BuildFile(
                "name: \"bar.shark\" "
                "message_type: {"
                "  name: \"Bar\""
                "  extension_range { start: 1 end: 1000 }"
                "}");

            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "dependency: \"bar.shark\" "
                "options { optimize_for: LITE_RUNTIME } "
                "extension { name: \"ext\" number: 123 label: OPTIONAL "
                "            type:INT32 extendee: \"Bar\" }",

                "foo.shark: ext: EXTENDEE: Extensions to non-lite types can only be "
                "declared in non-lite files.  Note that you cannot extend a non-lite "
                "type to contain a lite type, but the reverse is allowed.\n");
        }

        TEST_F(ValidationErrorTest, NoLiteServices) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "options {"
                "  optimize_for: LITE_RUNTIME"
                "  cc_generic_services: true"
                "} "
                "service { name: \"Foo\" }",

                "foo.shark: Foo: NAME: Files with optimize_for = LITE_RUNTIME cannot "
                "define services unless you set cc_generic_services to false.\n");

            BuildFile(
                "name: \"bar.shark\" "
                "options {"
                "  optimize_for: LITE_RUNTIME"
                "  cc_generic_services: false"
                "} "
                "service { name: \"Bar\" }");
        }

        TEST_F(ValidationErrorTest, RollbackAfterError) {
            // Build a file which contains every kind of construct but references an
            // undefined type.  All these constructs will be added to the symbol table
            // before the undefined type error is noticed.  The DescriptorPool will then
            // have to roll everything back.
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"TestMessage\""
                "  field { name:\"foo\" label:OPTIONAL type:INT32 number:1 }"
                "} "
                "enum_type {"
                "  name: \"TestEnum\""
                "  value { name:\"BAR\" number:1 }"
                "} "
                "service {"
                "  name: \"TestService\""
                "  method {"
                "    name: \"Baz\""
                "    input_type: \"NoSuchType\"" // error
                "    output_type: \"TestMessage\""
                "  }"
                "}",

                "foo.shark: TestService.Baz: INPUT_TYPE: \"NoSuchType\" is not defined.\n");

            // Make sure that if we build the same file again with the error fixed,
            // it works.  If the above rollback was incomplete, then some symbols will
            // be left defined, and this second attempt will fail since it tries to
            // re-define the same symbols.
            BuildFile(
                "name: \"foo.shark\" "
                "message_type {"
                "  name: \"TestMessage\""
                "  field { name:\"foo\" label:OPTIONAL type:INT32 number:1 }"
                "} "
                "enum_type {"
                "  name: \"TestEnum\""
                "  value { name:\"BAR\" number:1 }"
                "} "
                "service {"
                "  name: \"TestService\""
                "  method { name:\"Baz\""
                "           input_type:\"TestMessage\""
                "           output_type:\"TestMessage\" }"
                "}");
        }

        TEST_F(ValidationErrorTest, ErrorsReportedToLogError) {
            // Test that errors are reported to GOOGLE_LOG(ERROR) if no error collector is
            // provided.

            FileDescriptorProto file_proto;
            ASSERT_TRUE(
                TextFormat::ParseFromString("name: \"foo.shark\" "
                                            "message_type { name: \"Foo\" } "
                                            "message_type { name: \"Foo\" } ",
                    &file_proto));

            std::vector<std::string> errors;

            {
                ScopedMemoryLog log;
                EXPECT_TRUE(pool_.BuildFile(file_proto) == nullptr);
                errors = log.GetMessages(ERROR);
            }

            ASSERT_EQ(2, errors.size());

            EXPECT_EQ("Invalid proto descriptor for file \"foo.shark\":", errors[0]);
            EXPECT_EQ("  Foo: \"Foo\" is already defined.", errors[1]);
        }

        TEST_F(ValidationErrorTest, DisallowEnumAlias) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Bar\""
                "  value { name:\"ENUM_A\" number:0 }"
                "  value { name:\"ENUM_B\" number:0 }"
                "}",
                "foo.shark: Bar: NUMBER: \"Bar.ENUM_B\" uses the same enum value as \"Bar.ENUM_A\". If this is intended, set 'option allow_alias = true;' to the enum definition.\n");
        }

        TEST_F(ValidationErrorTest, AllowEnumAlias) {
            BuildFile(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Bar\""
                "  value { name:\"ENUM_A\" number:0 }"
                "  value { name:\"ENUM_B\" number:0 }"
                "  options { allow_alias: true }"
                "}");
        }

        TEST_F(ValidationErrorTest, UnusedImportWarning) {
            pool_.AddUnusedImportTrackFile("bar.shark");
            BuildFile(
                "name: \"bar.shark\" "
                "message_type { name: \"Bar\" }");

            pool_.AddUnusedImportTrackFile("base.shark");
            BuildFile(
                "name: \"base.shark\" "
                "message_type { name: \"Base\" }");

            pool_.AddUnusedImportTrackFile("baz.shark");
            BuildFile(
                "name: \"baz.shark\" "
                "message_type { name: \"Baz\" }");

            pool_.AddUnusedImportTrackFile("public.shark");
            BuildFile(
                "name: \"public.shark\" "
                "dependency: \"bar.shark\""
                "public_dependency: 0");

            // // forward.shark
            // import "base.shark"       // No warning: Base message is used.
            // import "bar.shark"        // Will log a warning.
            // import public "baz.shark" // No warning: Do not track import public.
            // import "public.shark"     // No warning: public.shark has import public.
            // message Forward {
            //   optional Base base = 1;
            // }
            //
            pool_.AddUnusedImportTrackFile("forward.shark");
            BuildFileWithWarnings(
                "name: \"forward.shark\""
                "dependency: \"base.shark\""
                "dependency: \"bar.shark\""
                "dependency: \"baz.shark\""
                "dependency: \"public.shark\""
                "public_dependency: 2 "
                "message_type {"
                "  name: \"Forward\""
                "  field { name:\"base\" number:1 label:OPTIONAL "
                "type_name:\"Base\" }"
                "}",
                "forward.shark: bar.shark: IMPORT: Import bar.shark is unused.\n");
        }

        namespace {
            void FillValidMapEntry(FileDescriptorProto* file_proto) {
                ASSERT_TRUE(TextFormat::ParseFromString(
                    "name: 'foo.shark' "
                    "message_type { "
                    "  name: 'Foo' "
                    "  field { "
                    "    name: 'foo_map' number: 1 label:REPEATED "
                    "    type_name: 'FooMapEntry' "
                    "  } "
                    "  nested_type { "
                    "    name: 'FooMapEntry' "
                    "    options {  map_entry: true } "
                    "    field { "
                    "      name: 'key' number: 1 type:INT32 label:OPTIONAL "
                    "    } "
                    "    field { "
                    "      name: 'value' number: 2 type:INT32 label:OPTIONAL "
                    "    } "
                    "  } "
                    "} "
                    "message_type { "
                    "  name: 'Bar' "
                    "  extension_range { start: 1 end: 10 }"
                    "} ",
                    file_proto));
            }
            static const char* kMapEntryErrorMessage = "foo.shark: Foo.foo_map: TYPE: map_entry should not be set explicitly. "
                                                       "Use map<KeyType, ValueType> instead.\n";
            static const char* kMapEntryKeyTypeErrorMessage = "foo.shark: Foo.foo_map: TYPE: Key in map fields cannot be float/double, "
                                                              "bytes or message types.\n";

        } // namespace

        TEST_F(ValidationErrorTest, MapEntryBase) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            BuildFile(file_proto.DebugString());
        }

        TEST_F(ValidationErrorTest, MapEntryExtensionRange) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "extension_range { "
                "  start: 10 end: 20 "
                "} ",
                file_proto.mutable_message_type(0)->mutable_nested_type(0));
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryExtension) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "extension { "
                "  name: 'foo_ext' extendee: '.Bar' number: 5"
                "} ",
                file_proto.mutable_message_type(0)->mutable_nested_type(0));
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryNestedType) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "nested_type { "
                "  name: 'Bar' "
                "} ",
                file_proto.mutable_message_type(0)->mutable_nested_type(0));
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryEnumTypes) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "enum_type { "
                "  name: 'BarEnum' "
                "  value { name: 'BAR_BAR' number:0 } "
                "} ",
                file_proto.mutable_message_type(0)->mutable_nested_type(0));
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryExtraField) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "field { "
                "  name: 'other_field' "
                "  label: OPTIONAL "
                "  type:INT32 "
                "  number: 3 "
                "} ",
                file_proto.mutable_message_type(0)->mutable_nested_type(0));
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryMessageName) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            file_proto.mutable_message_type(0)->mutable_nested_type(0)->set_name(
                "OtherMapEntry");
            file_proto.mutable_message_type(0)->mutable_field(0)->set_type_name(
                "OtherMapEntry");
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryNoneRepeatedMapEntry) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            file_proto.mutable_message_type(0)->mutable_field(0)->set_label(
                FieldCardinality::OPTIONAL);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryDifferentContainingType) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            // Move the nested MapEntry message into the top level, which should not pass
            // the validation.
            file_proto.mutable_message_type()->AddAllocated(
                file_proto.mutable_message_type(0)->mutable_nested_type()->ReleaseLast());
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyName) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_name("Key");
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyLabel) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_label(FieldCardinality::REQUIRED);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyNumber) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_number(3);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryValueName) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* value = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                1);
            value->set_name("Value");
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryValueLabel) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* value = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                1);
            value->set_label(FieldCardinality::REQUIRED);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryValueNumber) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* value = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                1);
            value->set_number(3);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyTypeFloat) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_type(SharkType::FLOAT);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryKeyTypeErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyTypeDouble) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_type(SharkType::DOUBLE);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryKeyTypeErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyTypeBytes) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->set_type(SharkType::BYTES);
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryKeyTypeErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryKeyTypeEnum) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->clear_type();
            key->set_type_name("BarEnum");
            EnumDescriptorProto* enum_proto = file_proto.add_enum_type();
            enum_proto->set_name("BarEnum");
            EnumValueDescriptorProto* enum_value_proto = enum_proto->add_value();
            enum_value_proto->set_name("BAR_VALUE0");
            enum_value_proto->set_number(0);
            BuildFileWithErrors(file_proto.DebugString(),
                "foo.shark: Foo.foo_map: TYPE: Key in map fields cannot "
                "be enum types.\n");
            // Enum keys are not allowed in proto3 as well.
            // Get rid of extensions for proto3 to make it proto3 compatible.
            file_proto.mutable_message_type()->RemoveLast();
            file_proto.set_syntax("proto3");
            BuildFileWithErrors(file_proto.DebugString(),
                "foo.shark: foo.shark: OTHER: syntax \"proto3\" is not "
                "supported.\n");
        }

        TEST_F(ValidationErrorTest, MapEntryKeyTypeMessage) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            FieldDescriptorProto* key = file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
                0);
            key->clear_type();
            key->set_type_name(".Bar");
            BuildFileWithErrors(file_proto.DebugString(), kMapEntryKeyTypeErrorMessage);
        }

        TEST_F(ValidationErrorTest, MapEntryConflictsWithField) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "field { "
                "  name: 'FooMapEntry' "
                "  type:INT32 "
                "  label: OPTIONAL "
                "  number: 100 "
                "}",
                file_proto.mutable_message_type(0));
            BuildFileWithErrors(
                file_proto.DebugString(),
                "foo.shark: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
                "\"Foo\".\n"
                "foo.shark: Foo.foo_map: TYPE: \"FooMapEntry\" is not defined.\n"
                "foo.shark: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
                "with an existing field.\n");
        }

        TEST_F(ValidationErrorTest, MapEntryConflictsWithMessage) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "nested_type { "
                "  name: 'FooMapEntry' "
                "}",
                file_proto.mutable_message_type(0));
            BuildFileWithErrors(
                file_proto.DebugString(),
                "foo.shark: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
                "\"Foo\".\n"
                "foo.shark: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
                "with an existing nested message type.\n");
        }

        TEST_F(ValidationErrorTest, MapEntryConflictsWithEnum) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "enum_type { "
                "  name: 'FooMapEntry' "
                "  value { name: 'ENTRY_FOO' number: 0 }"
                "}",
                file_proto.mutable_message_type(0));
            BuildFileWithErrors(
                file_proto.DebugString(),
                "foo.shark: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
                "\"Foo\".\n"
                "foo.shark: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
                "with an existing enum type.\n");
        }

        TEST_F(ValidationErrorTest, Proto3SyntaxRejected) {
            BuildFileWithErrors(
                "name: 'foo.shark' "
                "syntax: 'proto3' "
                "message_type { name: 'Foo' }",
                "foo.shark: foo.shark: OTHER: syntax \"proto3\" is not supported.\n");
        }

        TEST_F(ValidationErrorTest, Proto3OptionalRejected) {
            BuildFileWithErrors(
                "name: 'foo.shark' "
                "message_type { "
                "  name: 'Foo' "
                "  field { name:'foo' number:1 label:OPTIONAL type:INT32 "
                "          proto3_optional: true } "
                "}",
                "foo.shark: Foo.foo: OTHER: proto3_optional is not supported.\n");
        }

        TEST_F(ValidationErrorTest, EnumValuesConflictWithDifferentCasing) {
            BuildFileWithWarnings(
                "syntax: 'proto2'"
                "name: 'casing1.shark' "
                "enum_type {"
                "  name: 'FooEnum' "
                "  value { name: 'BAR' number: 0 }"
                "  value { name: 'bar' number: 1 }"
                "}",
                "casing1.shark: FooEnum.bar: NAME: Enum name bar has the same name as BAR if you ignore case and strip out the enum name prefix (if any). This is error-prone and can lead to undefined behavior. Please avoid doing this. If you are using allow_alias, please assign the same numeric value to both enums.\n");

            // Not an error because both enums are mapped to the same value.
            BuildFile(
                "syntax: 'proto2'"
                "name: 'casing2.shark' "
                "enum_type {"
                "  name: 'FooEnum2' "
                "  options { allow_alias: true }"
                "  value { name: 'UNKNOWN2' number: 0 }"
                "  value { name: 'BAZ2' number: 1 }"
                "  value { name: 'baz2' number: 1 }"
                "}");
        }

        TEST_F(ValidationErrorTest, EnumValuesConflictWhenPrefixesStripped) {
            BuildFileWithWarnings(
                "syntax: 'proto2'"
                "name: 'prefix1.shark' "
                "enum_type {"
                "  name: 'FooEnum' "
                "  value { name: 'FOO_ENUM_BAZ' number: 0 }"
                "  value { name: 'BAZ' number: 1 }"
                "}",
                "prefix1.shark: FooEnum.BAZ: NAME: Enum name BAZ has the same name as FOO_ENUM_BAZ "
                "if you ignore case and strip out the enum name prefix (if any). "
                "This is error-prone and can lead to undefined behavior. "
                "Please avoid doing this. If you are using allow_alias, please assign "
                "the same numeric value to both enums.\n");

            BuildFileWithWarnings(
                "syntax: 'proto2'"
                "name: 'prefix2.shark' "
                "enum_type {"
                "  name: 'FooEnumB' "
                "  value { name: 'FOOENUMB_QUX' number: 0 }"
                "  value { name: 'QUX' number: 1 }"
                "}",
                "prefix2.shark: FooEnumB.QUX: NAME: Enum name QUX has the same name as FOOENUMB_QUX "
                "if you ignore case and strip out the enum name prefix (if any). "
                "This is error-prone and can lead to undefined behavior. "
                "Please avoid doing this. If you are using allow_alias, please assign "
                "the same numeric value to both enums.\n");

            BuildFileWithWarnings(
                "syntax: 'proto2'"
                "name: 'prefix3.shark' "
                "enum_type {"
                "  name: 'CorEnum' "
                "  value { name: 'COR_ENUM_BAR_ZED' number: 0 }"
                "  value { name: 'BAR__ZED' number: 1 }"
                "}",
                "prefix3.shark: CorEnum.BAR__ZED: NAME: Enum name BAR__ZED has the same name as "
                "COR_ENUM_BAR_ZED if you ignore case and strip out the enum name prefix "
                "(if any). This is error-prone and can lead to undefined behavior. "
                "Please avoid doing this. If you are using allow_alias, please assign "
                "the same numeric value to both enums.\n");

            BuildFileWithWarnings(
                "syntax: 'proto2'"
                "name: 'prefix4.shark' "
                "enum_type {"
                "  name: 'DogEnum' "
                "  value { name: 'DOG_ENUM__BAR_WAX' number: 0 }"
                "  value { name: 'BAR_WAX' number: 1 }"
                "}",
                "prefix4.shark: DogEnum.BAR_WAX: NAME: Enum name BAR_WAX has the same name as "
                "DOG_ENUM__BAR_WAX if you ignore case and strip out the enum name prefix "
                "(if any). This is error-prone and can lead to undefined behavior. "
                "Please avoid doing this. If you are using allow_alias, please assign "
                "the same numeric value to both enums.\n");

            // This isn't an error because the underscore will cause the PascalCase to
            // differ by case (BarBaz vs. Barbaz).
            BuildFile(
                "syntax: 'proto2'"
                "name: 'prefix5.shark' "
                "enum_type {"
                "  name: 'KeepEnum' "
                "  value { name: 'KEEP_BAR_NAP' number: 0 }"
                "  value { name: 'KEEPBARNAP' number: 1 }"
                "}");
        }

        TEST_F(ValidationErrorTest, MapEntryConflictsWithOneof) {
            FileDescriptorProto file_proto;
            FillValidMapEntry(&file_proto);
            TextFormat::MergeFromString(
                "oneof_decl { "
                "  name: 'FooMapEntry' "
                "}"
                "field { "
                "  name: 'int_field' "
                "  type:INT32 "
                "  label: OPTIONAL "
                "  oneof_index: 0 "
                "  number: 100 "
                "} ",
                file_proto.mutable_message_type(0));
            BuildFileWithErrors(
                file_proto.DebugString(),
                "foo.shark: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
                "\"Foo\".\n"
                "foo.shark: Foo.foo_map: TYPE: \"FooMapEntry\" is not defined.\n"
                "foo.shark: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
                "with an existing oneof type.\n");
        }

        TEST_F(ValidationErrorTest, MapEntryUsesNoneZeroEnumDefaultValue) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "enum_type {"
                "  name: \"Bar\""
                "  value { name:\"ENUM_A\" number:1 }"
                "  value { name:\"ENUM_B\" number:2 }"
                "}"
                "message_type {"
                "  name: 'Foo' "
                "  field { "
                "    name: 'foo_map' number: 1 label:REPEATED "
                "    type_name: 'FooMapEntry' "
                "  } "
                "  nested_type { "
                "    name: 'FooMapEntry' "
                "    options {  map_entry: true } "
                "    field { "
                "      name: 'key' number: 1 type:INT32 label:OPTIONAL "
                "    } "
                "    field { "
                "      name: 'value' number: 2 type_name:\"Bar\" label:OPTIONAL "
                "    } "
                "  } "
                "}",
                "foo.shark: Foo.foo_map: "
                "TYPE: Enum value in map must define 0 as the first value.\n");
        }

        TEST_F(ValidationErrorTest, Proto3RequiredFields) {
            BuildFile(
                "name: 'foo.shark' "
                "package: 'p3req' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  field { name:'foo' number:1 label:REQUIRED type:INT32 } "
                "}");

            // applied to nested types as well.
            BuildFile(
                "name: 'bar.shark' "
                "package: 'p3reqn' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  nested_type { "
                "    name : 'Bar' "
                "    field { name:'bar' number:1 label:REQUIRED type:INT32 } "
                "  } "
                "}");

            // optional and repeated fields are OK.
            BuildFile(
                "name: 'baz.shark' "
                "package: 'p3reqok' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  field { name:'foo' number:1 label:OPTIONAL type:INT32 } "
                "  field { name:'bar' number:2 label:REPEATED type:INT32 } "
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3DefaultValue) {
            BuildFile(
                "name: 'foo.shark' "
                "package: 'p3def' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  field { name:'foo' number:1 label:OPTIONAL type:INT32 "
                "          default_value: '1' }"
                "}");

            BuildFile(
                "name: 'bar.shark' "
                "package: 'p3defn' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  nested_type { "
                "    name : 'Bar' "
                "    field { name:'bar' number:1 label:OPTIONAL type:INT32 "
                "            default_value: '1' }"
                "  } "
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3ExtensionRange) {
            BuildFile(
                "name: 'foo.shark' "
                "package: 'p3ext' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  field { name:'foo' number:1 label:OPTIONAL type:INT32 } "
                "  extension_range { start:10 end:100 } "
                "}");

            BuildFile(
                "name: 'bar.shark' "
                "package: 'p3extn' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  nested_type { "
                "    name : 'Bar' "
                "    field { name:'bar' number:1 label:OPTIONAL type:INT32 } "
                "    extension_range { start:10 end:100 } "
                "  } "
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3MessageSetWireFormat) {
            BuildFile(
                "name: 'foo.shark' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  options { message_set_wire_format: true } "
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3Enum) {
            BuildFile(
                "name: 'foo.shark' "
                "package: 'p3en' "
                "syntax: 'proto2' "
                "enum_type { "
                "  name: 'FooEnum' "
                "  value { name: 'FOO_ONE' number:1 } "
                "}");

            BuildFile(
                "name: 'bar.shark' "
                "package: 'p3enn' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "  enum_type { "
                "    name: 'FooEnum' "
                "    value { name: 'FOO_NESTED' number:1 } "
                "  } "
                "}");

            // valid case.
            BuildFile(
                "name: 'baz.shark' "
                "package: 'p3enz' "
                "syntax: 'proto2' "
                "enum_type { "
                "  name: 'FooEnum' "
                "  value { name: 'FOO_ZERO' number:0 } "
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3EnumFromProto2) {
            // Define an enum in a proto2 file.
            BuildFile(
                "name: 'foo.shark' "
                "package: 'foo' "
                "syntax: 'proto2' "
                "enum_type { "
                "  name: 'FooEnum' "
                "  value { name: 'DEFAULT_OPTION' number:0 } "
                "}");

            // Now try to refer to it. (All tests in the fixture use the same pool, so we
            // can refer to the enum above in this definition.)
            BuildFile(
                "name: 'bar.shark' "
                "dependency: 'foo.shark' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Foo' "
                "    field { name:'bar' number:1 label:OPTIONAL type:ENUM "
                "            type_name: 'foo.FooEnum' }"
                "}");
        }

        TEST_F(ValidationErrorTest, ValidateProto3Extension) {
            // Valid for options.
            DescriptorPool pool;
            const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
            ASSERT_NE(meta_fd, nullptr);
            FileDescriptorProto meta_proto;
            meta_fd->CopyTo(&meta_proto);
            ASSERT_NE(pool.BuildFile(meta_proto), nullptr);
            FileDescriptorProto file_proto;
            // Add "shark/descriptor.shark".
            FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
            // Add "foo.shark":
            //   import "shark/descriptor.shark";
            //   extend shark.FileOptions {
            //     optional string test_file_opt = 1001;
            //   }
            //   extend shark.MessageOptions {
            //     optional string test_msg_opt = 1002;
            //   }
            //   extend shark.FieldOptions {
            //     optional string test_field_opt = 1003;
            //   }
            //   extend shark.EnumOptions {
            //     repeated int32 test_enum_opt = 1004;
            //   }
            //   extend shark.EnumValueOptions {
            //     optional int32 test_enumval_opt = 1005;
            //   }
            //   extend shark.ServiceOptions {
            //     repeated int32 test_svc_opt = 1006;
            //   }
            //   extend shark.MethodOptions {
            //     optional string test_method_opt = 1007;
            //   }
            //   extend shark.OneofOptions {
            //     optional string test_oneof_opt = 1008;
            //   }
            //   extend shark.ExtensionRangeOptions {
            //     optional string test_ext_opt = 1009;
            //   }
            file_proto.Clear();
            file_proto.set_name("foo.shark");
            file_proto.set_syntax("proto2");
            file_proto.add_dependency("shark/descriptor.shark");
            AddExtension(&file_proto, "shark.FileOptions", "test_file_opt", 1001,
                FieldCardinality::OPTIONAL,
                SharkType::STRING);
            AddExtension(&file_proto, "shark.MessageOptions", "test_msg_opt", 1001,
                FieldCardinality::OPTIONAL,
                SharkType::STRING);
            AddExtension(&file_proto, "shark.FieldOptions", "test_field_opt", 1003,
                FieldCardinality::OPTIONAL,
                SharkType::STRING);
            AddExtension(&file_proto, "shark.EnumOptions", "test_enum_opt", 1004,
                FieldCardinality::REPEATED,
                SharkType::INT32);
            AddExtension(&file_proto, "shark.EnumValueOptions", "test_enumval_opt", 1005,
                FieldCardinality::OPTIONAL,
                SharkType::INT32);
            AddExtension(&file_proto, "shark.ServiceOptions", "test_svc_opt", 1006,
                FieldCardinality::REPEATED,
                SharkType::INT32);
            AddExtension(&file_proto, "shark.MethodOptions", "test_method_opt", 1007,
                FieldCardinality::OPTIONAL,
                SharkType::STRING);
            AddExtension(&file_proto, "shark.OneofOptions", "test_oneof_opt", 1008,
                FieldCardinality::OPTIONAL,
                SharkType::STRING);
            AddExtension(&file_proto, "shark.ExtensionRangeOptions", "test_ext_opt",
                1009, FieldCardinality::OPTIONAL,
                SharkType::STRING);
            ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

            // Copy and change the package of the descriptor.shark
            BuildFile(
                "name: 'shark.shark' "
                "syntax: 'proto2' "
                "message_type { "
                "  name: 'Container' extension_range { start: 1 end: 1000 } "
                "}");
            BuildFile(
                "name: 'bar.shark' "
                "syntax: 'proto2' "
                "dependency: 'shark.shark' "
                "extension { "
                "  name: 'bar' number: 1 label: OPTIONAL type:INT32 "
                "  extendee: 'Container' "
                "}");
        }

        // Test that field names that may conflict in JSON is not allowed by protoc.
        TEST_F(ValidationErrorTest, ValidateProto3JsonName) {
            BuildFile(
                "name: 'foo.shark' "
                "package: 'p3js' "
                "syntax: 'proto2' "
                "message_type {"
                "  name: 'Foo'"
                "  field { name:'name' number:1 label:OPTIONAL type:INT32 }"
                "  field { name:'Name' number:2 label:OPTIONAL type:INT32 }"
                "}");
            BuildFile(
                "name: 'bar.shark' "
                "package: 'p3js2' "
                "syntax: 'proto2' "
                "message_type {"
                "  name: 'Foo'"
                "  field { name:'ab' number:1 label:OPTIONAL type:INT32 }"
                "  field { name:'_a__b_' number:2 label:OPTIONAL type:INT32 }"
                "}");
        }

        TEST_F(ValidationErrorTest, UnusedImportWithOtherError) {
            BuildFile(
                "name: 'bar.shark' "
                "message_type {"
                "  name: 'Bar'"
                "}");

            pool_.AddUnusedImportTrackFile("foo.shark", true);
            BuildFileWithErrors(
                "name: 'foo.shark' "
                "dependency: 'bar.shark' "
                "message_type {"
                "  name: 'Foo'"
                "  extension { name:'foo' number:1 label:OPTIONAL type:INT32"
                "              extendee: 'Baz' }"
                "}",

                // Should not also contain unused import error.
                "foo.shark: Foo.foo: EXTENDEE: \"Baz\" is not defined.\n");
        }

        TEST_F(ValidationErrorTest, PackageTooLong) {
            BuildFileWithErrors(
                "name: \"foo.shark\" "
                "syntax: \"proto2\" "
                "package: "
                "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaa\"",
                "foo.shark: "
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaa: NAME: Package name is too long\n");
        }

        // ===================================================================
        // DescriptorDatabase

        static void AddToDatabase(SimpleDescriptorDatabase* database,
            const char* file_text) {
            FileDescriptorProto file_proto;
            EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
            database->Add(file_proto);
        }

        class DatabaseBackedPoolTest : public testing::Test {
        protected:
            DatabaseBackedPoolTest() { }

            SimpleDescriptorDatabase database_;

            void SetUp() override {
                AddToDatabase(
                    &database_,
                    "name: 'foo.shark' "
                    "message_type { name:'Foo' extension_range { start: 1 end: 100 } } "
                    "enum_type { name:'TestEnum' value { name:'DUMMY' number:0 } } "
                    "service { name:'TestService' } ");
                AddToDatabase(&database_,
                    "name: 'bar.shark' "
                    "dependency: 'foo.shark' "
                    "message_type { name:'Bar' } "
                    "extension { name:'foo_ext' extendee: '.Foo' number:5 "
                    "            label:OPTIONAL type:INT32 } ");
                // Baz has an undeclared dependency on Foo.
                AddToDatabase(
                    &database_,
                    "name: 'baz.shark' "
                    "message_type { "
                    "  name:'Baz' "
                    "  field { name:'foo' number:1 label:OPTIONAL type_name:'Foo' } "
                    "}");
            }

            // We can't inject a file containing errors into a DescriptorPool, so we
            // need an actual mock DescriptorDatabase to test errors.
            class ErrorDescriptorDatabase : public DescriptorDatabase {
            public:
                ErrorDescriptorDatabase() { }
                ~ErrorDescriptorDatabase() override { }

                // implements DescriptorDatabase ---------------------------------
                bool FindFileByName(const std::string& filename,
                    FileDescriptorProto* output) override {
                    // error.shark and error2.shark cyclically import each other.
                    if (filename == "error.shark") {
                        output->Clear();
                        output->set_name("error.shark");
                        output->add_dependency("error2.shark");
                        return true;
                    } else if (filename == "error2.shark") {
                        output->Clear();
                        output->set_name("error2.shark");
                        output->add_dependency("error.shark");
                        return true;
                    } else {
                        return false;
                    }
                }
                bool FindFileContainingSymbol(const std::string& symbol_name,
                    FileDescriptorProto* output) override {
                    return false;
                }
                bool FindFileContainingExtension(const std::string& containing_type,
                    int field_number,
                    FileDescriptorProto* output) override {
                    return false;
                }
            };

            // A DescriptorDatabase that counts how many times each method has been
            // called and forwards to some other DescriptorDatabase.
            class CallCountingDatabase : public DescriptorDatabase {
            public:
                CallCountingDatabase(DescriptorDatabase* wrapped_db)
                    : wrapped_db_(wrapped_db) {
                    Clear();
                }
                ~CallCountingDatabase() override { }

                DescriptorDatabase* wrapped_db_;

                int call_count_;

                void Clear() { call_count_ = 0; }

                // implements DescriptorDatabase ---------------------------------
                bool FindFileByName(const std::string& filename,
                    FileDescriptorProto* output) override {
                    ++call_count_;
                    return wrapped_db_->FindFileByName(filename, output);
                }
                bool FindFileContainingSymbol(const std::string& symbol_name,
                    FileDescriptorProto* output) override {
                    ++call_count_;
                    return wrapped_db_->FindFileContainingSymbol(symbol_name, output);
                }
                bool FindFileContainingExtension(const std::string& containing_type,
                    int field_number,
                    FileDescriptorProto* output) override {
                    ++call_count_;
                    return wrapped_db_->FindFileContainingExtension(containing_type,
                        field_number, output);
                }
            };

            // A DescriptorDatabase which falsely always returns foo.shark when searching
            // for any symbol or extension number.  This shouldn't cause the
            // DescriptorPool to reload foo.shark if it is already loaded.
            class FalsePositiveDatabase : public DescriptorDatabase {
            public:
                FalsePositiveDatabase(DescriptorDatabase* wrapped_db)
                    : wrapped_db_(wrapped_db) { }
                ~FalsePositiveDatabase() override { }

                DescriptorDatabase* wrapped_db_;

                // implements DescriptorDatabase ---------------------------------
                bool FindFileByName(const std::string& filename,
                    FileDescriptorProto* output) override {
                    return wrapped_db_->FindFileByName(filename, output);
                }
                bool FindFileContainingSymbol(const std::string& symbol_name,
                    FileDescriptorProto* output) override {
                    return FindFileByName("foo.shark", output);
                }
                bool FindFileContainingExtension(const std::string& containing_type,
                    int field_number,
                    FileDescriptorProto* output) override {
                    return FindFileByName("foo.shark", output);
                }
            };
        };

        TEST_F(DatabaseBackedPoolTest, FindFileByName) {
            DescriptorPool pool(&database_);

            const FileDescriptor* foo = pool.FindFileByName("foo.shark");
            ASSERT_TRUE(foo != nullptr);
            EXPECT_EQ("foo.shark", foo->name());
            ASSERT_EQ(1, foo->message_type_count());
            EXPECT_EQ("Foo", foo->message_type(0)->name());

            EXPECT_EQ(foo, pool.FindFileByName("foo.shark"));

            EXPECT_TRUE(pool.FindFileByName("no_such_file.shark") == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, FindDependencyBeforeDependent) {
            DescriptorPool pool(&database_);

            const FileDescriptor* foo = pool.FindFileByName("foo.shark");
            ASSERT_TRUE(foo != nullptr);
            EXPECT_EQ("foo.shark", foo->name());
            ASSERT_EQ(1, foo->message_type_count());
            EXPECT_EQ("Foo", foo->message_type(0)->name());

            const FileDescriptor* bar = pool.FindFileByName("bar.shark");
            ASSERT_TRUE(bar != nullptr);
            EXPECT_EQ("bar.shark", bar->name());
            ASSERT_EQ(1, bar->message_type_count());
            EXPECT_EQ("Bar", bar->message_type(0)->name());

            ASSERT_EQ(1, bar->dependency_count());
            EXPECT_EQ(foo, bar->dependency(0));
        }

        TEST_F(DatabaseBackedPoolTest, FindDependentBeforeDependency) {
            DescriptorPool pool(&database_);

            const FileDescriptor* bar = pool.FindFileByName("bar.shark");
            ASSERT_TRUE(bar != nullptr);
            EXPECT_EQ("bar.shark", bar->name());
            ASSERT_EQ(1, bar->message_type_count());
            ASSERT_EQ("Bar", bar->message_type(0)->name());

            const FileDescriptor* foo = pool.FindFileByName("foo.shark");
            ASSERT_TRUE(foo != nullptr);
            EXPECT_EQ("foo.shark", foo->name());
            ASSERT_EQ(1, foo->message_type_count());
            ASSERT_EQ("Foo", foo->message_type(0)->name());

            ASSERT_EQ(1, bar->dependency_count());
            EXPECT_EQ(foo, bar->dependency(0));
        }

        TEST_F(DatabaseBackedPoolTest, FindFileContainingSymbol) {
            DescriptorPool pool(&database_);

            const FileDescriptor* file = pool.FindFileContainingSymbol("Foo");
            ASSERT_TRUE(file != nullptr);
            EXPECT_EQ("foo.shark", file->name());
            EXPECT_EQ(file, pool.FindFileByName("foo.shark"));

            EXPECT_TRUE(pool.FindFileContainingSymbol("NoSuchSymbol") == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, FindMessageTypeByName) {
            DescriptorPool pool(&database_);

            const Descriptor* type = pool.FindMessageTypeByName("Foo");
            ASSERT_TRUE(type != nullptr);
            EXPECT_EQ("Foo", type->name());
            EXPECT_EQ(type->file(), pool.FindFileByName("foo.shark"));

            EXPECT_TRUE(pool.FindMessageTypeByName("NoSuchType") == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, FindExtensionByNumber) {
            DescriptorPool pool(&database_);

            const Descriptor* foo = pool.FindMessageTypeByName("Foo");
            ASSERT_TRUE(foo != nullptr);

            const FieldDescriptor* extension = pool.FindExtensionByNumber(foo, 5);
            ASSERT_TRUE(extension != nullptr);
            EXPECT_EQ("foo_ext", extension->name());
            EXPECT_EQ(extension->file(), pool.FindFileByName("bar.shark"));

            EXPECT_TRUE(pool.FindExtensionByNumber(foo, 12) == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, FindAllExtensions) {
            DescriptorPool pool(&database_);

            const Descriptor* foo = pool.FindMessageTypeByName("Foo");

            for (int i = 0; i < 2; ++i) {
                // Repeat the lookup twice, to check that we get consistent
                // results despite the fallback database lookup mutating the pool.
                std::vector<const FieldDescriptor*> extensions;
                pool.FindAllExtensions(foo, &extensions);
                ASSERT_EQ(1, extensions.size());
                EXPECT_EQ(5, extensions[0]->number());
            }
        }

        TEST_F(DatabaseBackedPoolTest, ErrorWithoutErrorCollector) {
            ErrorDescriptorDatabase error_database;
            DescriptorPool pool(&error_database);

            std::vector<std::string> errors;

            {
                ScopedMemoryLog log;
                EXPECT_TRUE(pool.FindFileByName("error.shark") == nullptr);
                errors = log.GetMessages(ERROR);
            }

            EXPECT_FALSE(errors.empty());
        }

        TEST_F(DatabaseBackedPoolTest, ErrorWithErrorCollector) {
            ErrorDescriptorDatabase error_database;
            MockErrorCollector error_collector;
            DescriptorPool pool(&error_database, &error_collector);

            EXPECT_TRUE(pool.FindFileByName("error.shark") == nullptr);
            EXPECT_EQ(
                "error.shark: error2.shark: IMPORT: File recursively imports itself: "
                "error.shark -> error2.shark -> error.shark\n"
                "error2.shark: error.shark: IMPORT: Import \"error.shark\" was not "
                "found or had errors.\n"
                "error.shark: error2.shark: IMPORT: Import \"error2.shark\" was not "
                "found or had errors.\n",
                error_collector.text_);
        }

        TEST_F(DatabaseBackedPoolTest, UndeclaredDependencyOnUnbuiltType) {
            // Check that we find and report undeclared dependencies on types that exist
            // in the descriptor database but that have not not been built yet.
            MockErrorCollector error_collector;
            DescriptorPool pool(&database_, &error_collector);
            EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
            EXPECT_EQ(
                "baz.shark: Baz.foo: TYPE: \"Foo\" seems to be defined in \"foo.shark\", "
                "which is not imported by \"baz.shark\".  To use it here, please add "
                "the necessary import.\n",
                error_collector.text_);
        }

        TEST_F(DatabaseBackedPoolTest, RollbackAfterError) {
            // Make sure that all traces of bad types are removed from the pool. This used
            // to be b/4529436, due to the fact that a symbol resolution failure could
            // potentially cause another file to be recursively built, which would trigger
            // a checkpoint _past_ possibly invalid symbols.
            // Baz is defined in the database, but the file is invalid because it is
            // missing a necessary import.
            DescriptorPool pool(&database_);
            EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
            // Make sure that searching again for the file or the type fails.
            EXPECT_TRUE(pool.FindFileByName("baz.shark") == nullptr);
            EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, UnittestProto) {
            // Try to load all of unittest.shark from a DescriptorDatabase.  This should
            // thoroughly test all paths through DescriptorBuilder to insure that there
            // are no deadlocking problems when pool_->mutex_ is non-null.
            const FileDescriptor* original_file = sharkbuf_unittest::TestAllTypes::descriptor()->file();

            DescriptorPoolDatabase database(*DescriptorPool::generated_pool());
            DescriptorPool pool(&database);
            const FileDescriptor* file_from_database = pool.FindFileByName(original_file->name());

            ASSERT_TRUE(file_from_database != nullptr);

            FileDescriptorProto original_file_proto;
            original_file->CopyTo(&original_file_proto);

            FileDescriptorProto file_from_database_proto;
            file_from_database->CopyTo(&file_from_database_proto);

            EXPECT_EQ(original_file_proto.DebugString(),
                file_from_database_proto.DebugString());

            // Also verify that CopyTo() did not omit any information.
            EXPECT_EQ(original_file->DebugString(), file_from_database->DebugString());
        }

        TEST_F(DatabaseBackedPoolTest, DoesntRetryDbUnnecessarily) {
            // Searching for a child of an existing descriptor should never fall back
            // to the DescriptorDatabase even if it isn't found, because we know all
            // children are already loaded.
            CallCountingDatabase call_counter(&database_);
            DescriptorPool pool(&call_counter);

            const FileDescriptor* file = pool.FindFileByName("foo.shark");
            ASSERT_TRUE(file != nullptr);
            const Descriptor* foo = pool.FindMessageTypeByName("Foo");
            ASSERT_TRUE(foo != nullptr);
            const EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
            ASSERT_TRUE(test_enum != nullptr);
            const ServiceDescriptor* test_service = pool.FindServiceByName("TestService");
            ASSERT_TRUE(test_service != nullptr);

            EXPECT_NE(0, call_counter.call_count_);
            call_counter.Clear();

            EXPECT_TRUE(foo->FindFieldByName("no_such_field") == nullptr);
            EXPECT_TRUE(foo->FindExtensionByName("no_such_extension") == nullptr);
            EXPECT_TRUE(foo->FindNestedTypeByName("NoSuchMessageType") == nullptr);
            EXPECT_TRUE(foo->FindEnumTypeByName("NoSuchEnumType") == nullptr);
            EXPECT_TRUE(foo->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(test_enum->FindValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(test_service->FindMethodByName("NoSuchMethod") == nullptr);

            EXPECT_TRUE(file->FindMessageTypeByName("NoSuchMessageType") == nullptr);
            EXPECT_TRUE(file->FindEnumTypeByName("NoSuchEnumType") == nullptr);
            EXPECT_TRUE(file->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(file->FindServiceByName("NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(file->FindExtensionByName("no_such_extension") == nullptr);

            EXPECT_TRUE(pool.FindFileContainingSymbol("Foo.no.such.field") == nullptr);
            EXPECT_TRUE(pool.FindFileContainingSymbol("Foo.no_such_field") == nullptr);
            EXPECT_TRUE(pool.FindMessageTypeByName("Foo.NoSuchMessageType") == nullptr);
            EXPECT_TRUE(pool.FindFieldByName("Foo.no_such_field") == nullptr);
            EXPECT_TRUE(pool.FindExtensionByName("Foo.no_such_extension") == nullptr);
            EXPECT_TRUE(pool.FindEnumTypeByName("Foo.NoSuchEnumType") == nullptr);
            EXPECT_TRUE(pool.FindEnumValueByName("Foo.NO_SUCH_VALUE") == nullptr);
            EXPECT_TRUE(pool.FindMethodByName("TestService.NoSuchMethod") == nullptr);

            EXPECT_EQ(0, call_counter.call_count_);
        }

        TEST_F(DatabaseBackedPoolTest, DoesntReloadFilesUncesessarily) {
            // If FindFileContainingSymbol() or FindFileContainingExtension() return a
            // file that is already in the DescriptorPool, it should not attempt to
            // reload the file.
            FalsePositiveDatabase false_positive_database(&database_);
            MockErrorCollector error_collector;
            DescriptorPool pool(&false_positive_database, &error_collector);

            // First make sure foo.shark is loaded.
            const Descriptor* foo = pool.FindMessageTypeByName("Foo");
            ASSERT_TRUE(foo != nullptr);

            // Try inducing false positives.
            EXPECT_TRUE(pool.FindMessageTypeByName("NoSuchSymbol") == nullptr);
            EXPECT_TRUE(pool.FindExtensionByNumber(foo, 22) == nullptr);

            // No errors should have been reported.  (If foo.shark was incorrectly
            // loaded multiple times, errors would have been reported.)
            EXPECT_EQ("", error_collector.text_);
        }

        // DescriptorDatabase that attempts to induce exponentially-bad performance
        // in DescriptorPool. For every positive N, the database contains a file
        // fileN.shark, which defines a message MessageN, which contains fields of
        // type MessageK for all K in [0,N). Message0 is not defined anywhere
        // (file0.shark exists, but is empty), so every other file and message type
        // will fail to build.
        //
        // If the DescriptorPool is not careful to memoize errors, an attempt to
        // build a descriptor for MessageN can require O(2^N) time.
        class ExponentialErrorDatabase : public DescriptorDatabase {
        public:
            ExponentialErrorDatabase() { }
            ~ExponentialErrorDatabase() override { }

            // implements DescriptorDatabase ---------------------------------
            bool FindFileByName(const std::string& filename,
                FileDescriptorProto* output) override {
                int file_num = -1;
                FullMatch(filename, "file", ".shark", &file_num);
                if (file_num > -1) {
                    return PopulateFile(file_num, output);
                } else {
                    return false;
                }
            }
            bool FindFileContainingSymbol(const std::string& symbol_name,
                FileDescriptorProto* output) override {
                int file_num = -1;
                FullMatch(symbol_name, "Message", "", &file_num);
                if (file_num > 0) {
                    return PopulateFile(file_num, output);
                } else {
                    return false;
                }
            }
            bool FindFileContainingExtension(const std::string& containing_type,
                int field_number,
                FileDescriptorProto* output) override {
                return false;
            }

        private:
            void FullMatch(const std::string& name, const std::string& begin_with,
                const std::string& end_with, int* file_num) {
                int begin_size = begin_with.size();
                int end_size = end_with.size();
                if (name.substr(0, begin_size) != begin_with || name.substr(name.size() - end_size, end_size) != end_with) {
                    return;
                }
                safe_strto32(
                    name.substr(begin_size, name.size() - end_size - begin_size), file_num);
            }

            bool PopulateFile(int file_num, FileDescriptorProto* output) {
                GOOGLE_CHECK_GE(file_num, 0);
                output->Clear();
                output->set_name(strings::Substitute("file$0.shark", file_num));
                // file0.shark doesn't define Message0
                if (file_num > 0) {
                    DescriptorProto* message = output->add_message_type();
                    message->set_name(strings::Substitute("Message$0", file_num));
                    for (int i = 0; i < file_num; ++i) {
                        output->add_dependency(strings::Substitute("file$0.shark", i));
                        FieldDescriptorProto* field = message->add_field();
                        field->set_name(strings::Substitute("field$0", i));
                        field->set_number(i);
                        field->set_label(FieldCardinality::OPTIONAL);
                        field->set_type(SharkType::MESSAGE);
                        field->set_type_name(strings::Substitute("Message$0", i));
                    }
                }
                return true;
            }
        };

        TEST_F(DatabaseBackedPoolTest, DoesntReloadKnownBadFiles) {
            ExponentialErrorDatabase error_database;
            DescriptorPool pool(&error_database);

            GOOGLE_LOG(INFO) << "A timeout in this test probably indicates a real bug.";

            EXPECT_TRUE(pool.FindFileByName("file40.shark") == nullptr);
            EXPECT_TRUE(pool.FindMessageTypeByName("Message40") == nullptr);
        }

        TEST_F(DatabaseBackedPoolTest, DoesntFallbackOnWrongType) {
            // If a lookup finds a symbol of the wrong type (e.g. we pass a type name
            // to FindFieldByName()), we should fail fast, without checking the fallback
            // database.
            CallCountingDatabase call_counter(&database_);
            DescriptorPool pool(&call_counter);

            const FileDescriptor* file = pool.FindFileByName("foo.shark");
            ASSERT_TRUE(file != nullptr);
            const Descriptor* foo = pool.FindMessageTypeByName("Foo");
            ASSERT_TRUE(foo != nullptr);
            const EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
            ASSERT_TRUE(test_enum != nullptr);

            EXPECT_NE(0, call_counter.call_count_);
            call_counter.Clear();

            EXPECT_TRUE(pool.FindMessageTypeByName("TestEnum") == nullptr);
            EXPECT_TRUE(pool.FindFieldByName("Foo") == nullptr);
            EXPECT_TRUE(pool.FindExtensionByName("Foo") == nullptr);
            EXPECT_TRUE(pool.FindEnumTypeByName("Foo") == nullptr);
            EXPECT_TRUE(pool.FindEnumValueByName("Foo") == nullptr);
            EXPECT_TRUE(pool.FindServiceByName("Foo") == nullptr);
            EXPECT_TRUE(pool.FindMethodByName("Foo") == nullptr);

            EXPECT_EQ(0, call_counter.call_count_);
        }

        // ===================================================================

        class AbortingErrorCollector : public DescriptorPool::ErrorCollector {
        public:
            AbortingErrorCollector() { }

            void AddError(const std::string& filename, const std::string& element_name,
                const Message* message, ErrorLocation location,
                const std::string& error_message) override {
                GOOGLE_LOG(FATAL) << "AddError() called unexpectedly: " << filename << " ["
                                  << element_name << "]: " << error_message;
            }

        private:
            GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(AbortingErrorCollector);
        };

        // A source tree containing only one file.
        class SingletonSourceTree : public compiler::SourceTree {
        public:
            SingletonSourceTree(const std::string& filename, const std::string& contents)
                : filename_(filename)
                , contents_(contents) { }

            io::ZeroCopyInputStream* Open(const std::string& filename) override {
                return filename == filename_
                    ? new io::ArrayInputStream(contents_.data(), contents_.size())
                    : nullptr;
            }

        private:
            const std::string filename_;
            const std::string contents_;

            GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SingletonSourceTree);
        };

        const char* const kSourceLocationTestInput = "syntax = \"proto2\";\n"
                                                     "option go_package = \"com.foo.bar\";\n"
                                                     "option (test_file_opt) = \"foobar\";\n"
                                                     "message A {\n"
                                                     "  option (test_msg_opt) = \"foobar\";\n"
                                                     "  optional int32 a = 1 [deprecated = true];\n"
                                                     "  message B {\n"
                                                     "    required double b = 1 [(test_field_opt) = \"foobar\"];\n"
                                                     "  }\n"
                                                     "  oneof c {\n"
                                                     "    option (test_oneof_opt) = \"foobar\";\n"
                                                     "    string d = 2;\n"
                                                     "    string e = 3;\n"
                                                     "    string f = 4;\n"
                                                     "  }\n"
                                                     "}\n"
                                                     "enum Indecision {\n"
                                                     "  option (test_enum_opt) = 21;\n"
                                                     "  option (test_enum_opt) = 42;\n"
                                                     "  option (test_enum_opt) = 63;\n"
                                                     "  YES   = 1 [(test_enumval_opt).a = 100];\n"
                                                     "  NO    = 2 [(test_enumval_opt) = {a:200}];\n"
                                                     "  MAYBE = 3;\n"
                                                     "}\n"
                                                     "service S {\n"
                                                     "  option (test_svc_opt) = {a:100};\n"
                                                     "  option (test_svc_opt) = {a:200};\n"
                                                     "  option (test_svc_opt) = {a:300};\n"
                                                     "  rpc Method(A) returns (A.B);\n"
                                                     // Put an empty line here to make the source location range match.
                                                     "\n"
                                                     "  rpc OtherMethod(A) returns (A) {\n"
                                                     "    option deprecated = true;\n"
                                                     "    option (test_method_opt) = \"foobar\";\n"
                                                     "  }\n"
                                                     "}\n"
                                                     "message MessageWithExtensions {\n"
                                                     "  extensions 1000 to 2000, 2001 to max [(test_ext_opt) = \"foobar\"];\n"
                                                     "}\n"
                                                     "extend MessageWithExtensions {\n"
                                                     "  list<int32> int32_extension = 1001 [packed=true];\n"
                                                     "}\n"
                                                     "message C {\n"
                                                     "  extend MessageWithExtensions {\n"
                                                     "    optional C message_extension = 1002;\n"
                                                     "  }\n"
                                                     "}\n"
                                                     "import \"shark/descriptor.shark\";\n"
                                                     "extend shark.FileOptions {\n"
                                                     "  optional string test_file_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.MessageOptions {\n"
                                                     "  optional string test_msg_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.FieldOptions {\n"
                                                     "  optional string test_field_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.EnumOptions {\n"
                                                     "  list<int32> test_enum_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.EnumValueOptions {\n"
                                                     "  optional A test_enumval_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.ServiceOptions {\n"
                                                     "  list<A> test_svc_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.MethodOptions {\n"
                                                     "  optional string test_method_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.OneofOptions {\n"
                                                     "  optional string test_oneof_opt = 10101;\n"
                                                     "}\n"
                                                     "extend shark.ExtensionRangeOptions {\n"
                                                     "  optional string test_ext_opt = 10101;\n"
                                                     "}\n";

        class SourceLocationTest : public testing::Test {
        public:
            SourceLocationTest()
                : source_tree_("/test/test.shark", kSourceLocationTestInput)
                , simple_db_()
                , source_tree_db_(&source_tree_)
                , merged_db_(&simple_db_, &source_tree_db_)
                , pool_(&merged_db_, &collector_) {

                const FileDescriptor* meta_fd = DescriptorPool::generated_pool()->FindFileByName("shark/meta.shark");
                meta_fd->CopyTo(&meta_proto_);
                simple_db_.Add(meta_proto_);

                // we need descriptor.shark to be accessible by the pool
                // since our test file imports it
                FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto_);
                simple_db_.Add(file_proto_);
            }

            static std::string PrintSourceLocation(const SourceLocation& loc) {
                return strings::Substitute("$0:$1-$2:$3", 1 + loc.start_line,
                    1 + loc.start_column, 1 + loc.end_line,
                    1 + loc.end_column);
            }

        private:
            FileDescriptorProto file_proto_;
            FileDescriptorProto meta_proto_;
            AbortingErrorCollector collector_;
            SingletonSourceTree source_tree_;
            SimpleDescriptorDatabase simple_db_; // contains descriptor.shark
            compiler::SourceTreeDescriptorDatabase source_tree_db_; // loads test.shark
            MergedDescriptorDatabase merged_db_; // combines above two dbs

        protected:
            DescriptorPool pool_;

            // tag number of all custom options in above test file
            static constexpr int kCustomOptionFieldNumber = 10101;
            // tag number of field "a" in message type "A" in above test file
            static constexpr int kAFieldNumber = 1;
        };

        // TODO(adonovan): implement support for option fields and for
        // subparts of declarations.

        TEST_F(SourceLocationTest, GetSourceLocation) {
            SourceLocation loc;

            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));

            const Descriptor* a_desc = file_desc->FindMessageTypeByName("A");
            EXPECT_TRUE(a_desc->GetSourceLocation(&loc));
            EXPECT_EQ("4:1-16:2", PrintSourceLocation(loc));

            const Descriptor* a_b_desc = a_desc->FindNestedTypeByName("B");
            EXPECT_TRUE(a_b_desc->GetSourceLocation(&loc));
            EXPECT_EQ("7:3-9:4", PrintSourceLocation(loc));

            const EnumDescriptor* e_desc = file_desc->FindEnumTypeByName("Indecision");
            EXPECT_TRUE(e_desc->GetSourceLocation(&loc));
            EXPECT_EQ("17:1-24:2", PrintSourceLocation(loc));

            const EnumValueDescriptor* yes_desc = e_desc->FindValueByName("YES");
            EXPECT_TRUE(yes_desc->GetSourceLocation(&loc));
            EXPECT_EQ("21:3-21:42", PrintSourceLocation(loc));

            const ServiceDescriptor* s_desc = file_desc->FindServiceByName("S");
            EXPECT_TRUE(s_desc->GetSourceLocation(&loc));
            EXPECT_EQ("25:1-35:2", PrintSourceLocation(loc));

            const MethodDescriptor* m_desc = s_desc->FindMethodByName("Method");
            EXPECT_TRUE(m_desc->GetSourceLocation(&loc));
            EXPECT_EQ("29:3-29:31", PrintSourceLocation(loc));
        }

        TEST_F(SourceLocationTest, ExtensionSourceLocation) {
            SourceLocation loc;

            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));

            const FieldDescriptor* int32_extension_desc = file_desc->FindExtensionByName("int32_extension");
            EXPECT_TRUE(int32_extension_desc->GetSourceLocation(&loc));
            EXPECT_EQ("40:3-40:52", PrintSourceLocation(loc));

            const Descriptor* c_desc = file_desc->FindMessageTypeByName("C");
            EXPECT_TRUE(c_desc->GetSourceLocation(&loc));
            EXPECT_EQ("42:1-46:2", PrintSourceLocation(loc));

            const FieldDescriptor* message_extension_desc = c_desc->FindExtensionByName("message_extension");
            EXPECT_TRUE(message_extension_desc->GetSourceLocation(&loc));
            EXPECT_EQ("44:5-44:41", PrintSourceLocation(loc));
        }
        TEST_F(SourceLocationTest, InterpretedOptionSourceLocation) {
            // This one's a doozy. It checks every kind of option, including
            // extension range options.

            // We are verifying that the file's source info contains correct
            // info for interpreted options and that it does *not* contain
            // any info for corresponding uninterpreted option path.

            SourceLocation loc;

            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));

            // File options
            {
                int path[] = { FileDescriptorProto::kOptionsFieldNumber,
                    FileOptions::kGoPackageFieldNumber };
                int unint[] = { FileDescriptorProto::kOptionsFieldNumber,
                    FileOptions::kUninterpretedOptionFieldNumber, 0 };

                std::vector<int> vpath(path, path + 2);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("2:1-2:35", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 3);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber };
                int unint[] = { FileDescriptorProto::kOptionsFieldNumber,
                    FileOptions::kUninterpretedOptionFieldNumber, 1 };
                std::vector<int> vpath(path, path + 2);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("3:1-3:35", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 3);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Message option
            {
                int path[] = { FileDescriptorProto::kMessageTypeFieldNumber, 0,
                    DescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber, 0,
                    DescriptorProto::kOptionsFieldNumber,
                    MessageOptions::kUninterpretedOptionFieldNumber, 0 };
                std::vector<int> vpath(path, path + 4);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("5:3-5:36", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Field option
            {
                int path[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    0,
                    DescriptorProto::kFieldFieldNumber,
                    0,
                    FieldDescriptorProto::kOptionsFieldNumber,
                    FieldOptions::kDeprecatedFieldNumber };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    0,
                    DescriptorProto::kFieldFieldNumber,
                    0,
                    FieldDescriptorProto::kOptionsFieldNumber,
                    FieldOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("6:25-6:42", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Nested message option
            {
                int path[] = {
                    FileDescriptorProto::kMessageTypeFieldNumber, 0,
                    DescriptorProto::kNestedTypeFieldNumber, 0,
                    DescriptorProto::kFieldFieldNumber, 0,
                    FieldDescriptorProto::kOptionsFieldNumber, kCustomOptionFieldNumber
                };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    0,
                    DescriptorProto::kNestedTypeFieldNumber,
                    0,
                    DescriptorProto::kFieldFieldNumber,
                    0,
                    FieldDescriptorProto::kOptionsFieldNumber,
                    FieldOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 8);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("8:28-8:55", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 9);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // One-of option
            {
                int path[] = {
                    FileDescriptorProto::kMessageTypeFieldNumber, 0,
                    DescriptorProto::kOneofDeclFieldNumber, 0,
                    OneofDescriptorProto::kOptionsFieldNumber, kCustomOptionFieldNumber
                };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    0,
                    DescriptorProto::kOneofDeclFieldNumber,
                    0,
                    OneofDescriptorProto::kOptionsFieldNumber,
                    OneofOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("11:5-11:40", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Enum option, repeated options
            {
                int path[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 0 };
                int unint[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    EnumOptions::kUninterpretedOptionFieldNumber, 0 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("18:3-18:31", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 1 };
                int unint[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    EnumOptions::kUninterpretedOptionFieldNumber, 1 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("19:3-19:31", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 2 };
                int unint[] = { FileDescriptorProto::kEnumTypeFieldNumber, 0,
                    EnumDescriptorProto::kOptionsFieldNumber,
                    OneofOptions::kUninterpretedOptionFieldNumber, 2 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("20:3-20:31", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Enum value options
            {
                // option w/ message type that directly sets field
                int path[] = { FileDescriptorProto::kEnumTypeFieldNumber,
                    0,
                    EnumDescriptorProto::kValueFieldNumber,
                    0,
                    EnumValueDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber,
                    kAFieldNumber };
                int unint[] = { FileDescriptorProto::kEnumTypeFieldNumber,
                    0,
                    EnumDescriptorProto::kValueFieldNumber,
                    0,
                    EnumValueDescriptorProto::kOptionsFieldNumber,
                    EnumValueOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 7);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("21:14-21:40", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kEnumTypeFieldNumber,
                    0,
                    EnumDescriptorProto::kValueFieldNumber,
                    1,
                    EnumValueDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber };
                int unint[] = { FileDescriptorProto::kEnumTypeFieldNumber,
                    0,
                    EnumDescriptorProto::kValueFieldNumber,
                    1,
                    EnumValueDescriptorProto::kOptionsFieldNumber,
                    EnumValueOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("22:14-22:42", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Service option, repeated options
            {
                int path[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 0 };
                int unint[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    ServiceOptions::kUninterpretedOptionFieldNumber, 0 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("26:3-26:35", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 1 };
                int unint[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    ServiceOptions::kUninterpretedOptionFieldNumber, 1 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("27:3-27:35", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    kCustomOptionFieldNumber, 2 };
                int unint[] = { FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kOptionsFieldNumber,
                    ServiceOptions::kUninterpretedOptionFieldNumber, 2 };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("28:3-28:35", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Method options
            {
                int path[] = { FileDescriptorProto::kServiceFieldNumber,
                    0,
                    ServiceDescriptorProto::kMethodFieldNumber,
                    1,
                    MethodDescriptorProto::kOptionsFieldNumber,
                    MethodOptions::kDeprecatedFieldNumber };
                int unint[] = { FileDescriptorProto::kServiceFieldNumber,
                    0,
                    ServiceDescriptorProto::kMethodFieldNumber,
                    1,
                    MethodDescriptorProto::kOptionsFieldNumber,
                    MethodOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("32:5-32:30", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = {
                    FileDescriptorProto::kServiceFieldNumber, 0,
                    ServiceDescriptorProto::kMethodFieldNumber, 1,
                    MethodDescriptorProto::kOptionsFieldNumber, kCustomOptionFieldNumber
                };
                int unint[] = { FileDescriptorProto::kServiceFieldNumber,
                    0,
                    ServiceDescriptorProto::kMethodFieldNumber,
                    1,
                    MethodDescriptorProto::kOptionsFieldNumber,
                    MethodOptions::kUninterpretedOptionFieldNumber,
                    1 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("33:5-33:41", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Extension range options
            {
                int path[] = { FileDescriptorProto::kMessageTypeFieldNumber, 1,
                    DescriptorProto::kExtensionRangeFieldNumber, 0,
                    DescriptorProto_ExtensionRange::kOptionsFieldNumber };
                std::vector<int> vpath(path, path + 5);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("37:40-37:67", PrintSourceLocation(loc));
            }
            {
                int path[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    1,
                    DescriptorProto::kExtensionRangeFieldNumber,
                    0,
                    DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                    kCustomOptionFieldNumber };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    1,
                    DescriptorProto::kExtensionRangeFieldNumber,
                    0,
                    DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                    ExtensionRangeOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("37:41-37:66", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
            {
                int path[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    1,
                    DescriptorProto::kExtensionRangeFieldNumber,
                    1,
                    DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                    kCustomOptionFieldNumber };
                int unint[] = { FileDescriptorProto::kMessageTypeFieldNumber,
                    1,
                    DescriptorProto::kExtensionRangeFieldNumber,
                    1,
                    DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                    ExtensionRangeOptions::kUninterpretedOptionFieldNumber,
                    0 };
                std::vector<int> vpath(path, path + 6);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("37:41-37:66", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 7);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }

            // Field option on extension
            {
                int path[] = { FileDescriptorProto::kExtensionFieldNumber, 0,
                    FieldDescriptorProto::kOptionsFieldNumber,
                    FieldOptions::kPackedFieldNumber };
                int unint[] = { FileDescriptorProto::kExtensionFieldNumber, 0,
                    FieldDescriptorProto::kOptionsFieldNumber,
                    FieldOptions::kUninterpretedOptionFieldNumber, 0 };
                std::vector<int> vpath(path, path + 4);
                EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
                EXPECT_EQ("40:39-40:50", PrintSourceLocation(loc));

                std::vector<int> vunint(unint, unint + 5);
                EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
            }
        }

        // Missing SourceCodeInfo doesn't cause crash:
        TEST_F(SourceLocationTest, GetSourceLocation_MissingSourceCodeInfo) {
            SourceLocation loc;

            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));

            FileDescriptorProto proto;
            file_desc->CopyTo(&proto); // Note, this discards the SourceCodeInfo.
            EXPECT_FALSE(proto.has_source_code_info());

            DescriptorPool bad1_pool(&pool_);
            const FileDescriptor* bad1_file_desc = GOOGLE_CHECK_NOTNULL(bad1_pool.BuildFile(proto));
            const Descriptor* bad1_a_desc = bad1_file_desc->FindMessageTypeByName("A");
            EXPECT_FALSE(bad1_a_desc->GetSourceLocation(&loc));
        }

        // Corrupt SourceCodeInfo doesn't cause crash:
        TEST_F(SourceLocationTest, GetSourceLocation_BogusSourceCodeInfo) {
            SourceLocation loc;

            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));

            FileDescriptorProto proto;
            file_desc->CopyTo(&proto); // Note, this discards the SourceCodeInfo.
            EXPECT_FALSE(proto.has_source_code_info());
            SourceCodeInfo_Location* loc_msg = proto.mutable_source_code_info()->add_location();
            loc_msg->add_path(1);
            loc_msg->add_path(2);
            loc_msg->add_path(3);
            loc_msg->add_span(4);
            loc_msg->add_span(5);
            loc_msg->add_span(6);

            DescriptorPool bad2_pool(&pool_);
            const FileDescriptor* bad2_file_desc = GOOGLE_CHECK_NOTNULL(bad2_pool.BuildFile(proto));
            const Descriptor* bad2_a_desc = bad2_file_desc->FindMessageTypeByName("A");
            EXPECT_FALSE(bad2_a_desc->GetSourceLocation(&loc));
        }

        // ===================================================================

        const char* const kCopySourceCodeInfoToTestInput = "syntax = \"proto2\";\n"
                                                           "message Foo {}\n";

        // Required since source code information is not preserved by
        // FileDescriptorTest.
        class CopySourceCodeInfoToTest : public testing::Test {
        public:
            CopySourceCodeInfoToTest()
                : source_tree_("/test/test.shark", kCopySourceCodeInfoToTestInput)
                , db_(&source_tree_)
                , pool_(&db_, &collector_) { }

        private:
            AbortingErrorCollector collector_;
            SingletonSourceTree source_tree_;
            compiler::SourceTreeDescriptorDatabase db_;

        protected:
            DescriptorPool pool_;
        };

        TEST_F(CopySourceCodeInfoToTest, CopyTo_DoesNotCopySourceCodeInfo) {
            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));
            FileDescriptorProto file_desc_proto;
            ASSERT_FALSE(file_desc_proto.has_source_code_info());

            file_desc->CopyTo(&file_desc_proto);
            EXPECT_FALSE(file_desc_proto.has_source_code_info());
        }

        TEST_F(CopySourceCodeInfoToTest, CopySourceCodeInfoTo) {
            const FileDescriptor* file_desc = GOOGLE_CHECK_NOTNULL(pool_.FindFileByName("/test/test.shark"));
            FileDescriptorProto file_desc_proto;
            ASSERT_FALSE(file_desc_proto.has_source_code_info());

            file_desc->CopySourceCodeInfoTo(&file_desc_proto);
            const SourceCodeInfo& info = file_desc_proto.source_code_info();
            ASSERT_EQ(4, info.location_size());
            // Get the Foo message location
            const SourceCodeInfo_Location& foo_location = info.location(2);
            ASSERT_EQ(2, foo_location.path_size());
            EXPECT_EQ(FileDescriptorProto::kMessageTypeFieldNumber, foo_location.path(0));
            EXPECT_EQ(0, foo_location.path(1)); // Foo is the first message defined
            ASSERT_EQ(3, foo_location.span_size()); // Foo spans one line
            EXPECT_EQ(1, foo_location.span(0)); // Foo is declared on line 1
            EXPECT_EQ(0, foo_location.span(1)); // Foo starts at column 0
            EXPECT_EQ(14, foo_location.span(2)); // Foo ends on column 14
        }

        // ===================================================================

        class LazilyBuildDependenciesTest : public testing::Test {
        public:
            LazilyBuildDependenciesTest()
                : pool_(&db_, nullptr) {
                pool_.InternalSetLazilyBuildDependencies();
            }

            void ParseProtoAndAddToDb(const char* proto) {
                FileDescriptorProto tmp;
                ASSERT_TRUE(TextFormat::ParseFromString(proto, &tmp));
                db_.Add(tmp);
            }

            void ParseProtoAndAddToDb(const std::string& proto) {
                FileDescriptorProto tmp;
                ASSERT_TRUE(TextFormat::ParseFromString(proto, &tmp));
                db_.Add(tmp);
            }

            void AddSimpleMessageProtoFileToDb(const char* file_name,
                const char* message_name) {
                ParseProtoAndAddToDb("name: '" + std::string(file_name) + ".shark' "
                                                                          "package: \"sharkbuf_unittest\" "
                                                                          "message_type { "
                                                                          "  name:'"
                    + std::string(message_name) + "' "
                                                  "  field { name:'a' number:1 "
                                                  "  label:OPTIONAL "
                                                  "  type_name:'int32' } "
                                                  "}");
            }

            void AddSimpleEnumProtoFileToDb(const char* file_name, const char* enum_name,
                const char* enum_value_name) {
                ParseProtoAndAddToDb("name: '" + std::string(file_name) + ".shark' "
                                                                          "package: 'sharkbuf_unittest' "
                                                                          "enum_type { "
                                                                          "  name:'"
                    + std::string(enum_name) + "' "
                                               "  value { name:'"
                    + std::string(enum_value_name) + "' number:1 } "
                                                     "}");
            }

        protected:
            SimpleDescriptorDatabase db_;
            DescriptorPool pool_;
        };

        TEST_F(LazilyBuildDependenciesTest, Message) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'bar.shark' "
                "message_type { "
                "  name:'Foo' "
                "  field { name:'bar' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Bar' } "
                "}");
            AddSimpleMessageProtoFileToDb("bar", "Bar");

            // Verify neither has been built yet.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("foo.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.shark"));

            const FileDescriptor* file = pool_.FindFileByName("foo.shark");

            // Verify only foo gets built when asking for foo.shark
            EXPECT_TRUE(file != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.shark"));

            // Verify calling FindFieldBy* works when the type of the field was
            // not built at cross link time. Verify this doesn't build the file
            // the field's type is defined in, as well.
            const Descriptor* desc = file->FindMessageTypeByName("Foo");
            const FieldDescriptor* field = desc->FindFieldByName("bar");
            EXPECT_TRUE(field != nullptr);
            EXPECT_EQ(field, desc->FindFieldByNumber(1));
            EXPECT_EQ(field, desc->FindFieldByLowercaseName("bar"));
            EXPECT_EQ(field, desc->FindFieldByCamelcaseName("bar"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.shark"));

            // Finally, verify that if we call message_type() on the field, we will
            // build the file where the message is defined, and get a valid descriptor
            EXPECT_TRUE(field->message_type() != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.shark"));
        }

        TEST_F(LazilyBuildDependenciesTest, Enum) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'enum1.shark' "
                "dependency: 'enum2.shark' "
                "message_type { "
                "  name:'Lazy' "
                "  field { name:'enum1' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Enum1' } "
                "  field { name:'enum2' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Enum2' } "
                "}");
            AddSimpleEnumProtoFileToDb("enum1", "Enum1", "ENUM1");
            AddSimpleEnumProtoFileToDb("enum2", "Enum2", "ENUM2");

            const FileDescriptor* file = pool_.FindFileByName("foo.shark");

            // Verify calling enum_type() on a field whose definition is not
            // yet built will build the file and return a descriptor.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("enum1.shark"));
            const Descriptor* desc = file->FindMessageTypeByName("Lazy");
            EXPECT_TRUE(desc != nullptr);
            const FieldDescriptor* field = desc->FindFieldByName("enum1");
            EXPECT_TRUE(field != nullptr);
            EXPECT_TRUE(field->enum_type() != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("enum1.shark"));

            // Verify calling default_value_enum() on a field whose definition is not
            // yet built will build the file and return a descriptor to the value.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("enum2.shark"));
            field = desc->FindFieldByName("enum2");
            EXPECT_TRUE(field != nullptr);
            EXPECT_TRUE(field->default_value_enum() != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("enum2.shark"));
        }

        TEST_F(LazilyBuildDependenciesTest, Type) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'message1.shark' "
                "dependency: 'message2.shark' "
                "dependency: 'enum1.shark' "
                "dependency: 'enum2.shark' "
                "message_type { "
                "  name:'Lazy' "
                "  field { name:'message1' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Message1' } "
                "  field { name:'message2' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Message2' } "
                "  field { name:'enum1' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Enum1' } "
                "  field { name:'enum2' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Enum2' } "
                "}");
            AddSimpleMessageProtoFileToDb("message1", "Message1");
            AddSimpleMessageProtoFileToDb("message2", "Message2");
            AddSimpleEnumProtoFileToDb("enum1", "Enum1", "ENUM1");
            AddSimpleEnumProtoFileToDb("enum2", "Enum2", "ENUM2");

            const FileDescriptor* file = pool_.FindFileByName("foo.shark");

            // Verify calling type() on a field that is a message type will
            // build the type defined in another file.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("message1.shark"));
            const Descriptor* desc = file->FindMessageTypeByName("Lazy");
            EXPECT_TRUE(desc != nullptr);
            const FieldDescriptor* field = desc->FindFieldByName("message1");
            EXPECT_TRUE(field != nullptr);
            EXPECT_EQ(field->type(), SharkFieldType::MESSAGE);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("message1.shark"));

            // Verify calling cpp_type() on a field that is a message type will
            // build the type defined in another file.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.shark"));
            field = desc->FindFieldByName("message2");
            EXPECT_TRUE(field != nullptr);
            EXPECT_EQ(field->cpp_type(), SharkFieldCppType::CPPTYPE_MESSAGE);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("message2.shark"));

            // Verify calling type() on a field that is an enum type will
            // build the type defined in another file.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("enum1.shark"));
            field = desc->FindFieldByName("enum1");
            EXPECT_TRUE(field != nullptr);
            EXPECT_EQ(field->type(), SharkFieldType::ENUM);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("enum1.shark"));

            // Verify calling cpp_type() on a field that is an enum type will
            // build the type defined in another file.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("enum2.shark"));
            field = desc->FindFieldByName("enum2");
            EXPECT_TRUE(field != nullptr);
            EXPECT_EQ(field->cpp_type(), SharkFieldCppType::CPPTYPE_ENUM);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("enum2.shark"));
        }

        TEST_F(LazilyBuildDependenciesTest, Extension) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'bar.shark' "
                "dependency: 'baz.shark' "
                "extension { extendee: '.sharkbuf_unittest.Bar' name:'bar' number:11"
                "            label:OPTIONAL type_name:'.sharkbuf_unittest.Baz' }");
            ParseProtoAndAddToDb(
                "name: 'bar.shark' "
                "package: 'sharkbuf_unittest' "
                "message_type { "
                "  name:'Bar' "
                "  extension_range { start: 10 end: 20 }"
                "}");
            AddSimpleMessageProtoFileToDb("baz", "Baz");

            // Verify none have been built yet.
            EXPECT_FALSE(pool_.InternalIsFileLoaded("foo.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.shark"));

            const FileDescriptor* file = pool_.FindFileByName("foo.shark");

            // Verify foo.bar gets loaded, and bar.shark gets loaded
            // to register the extension. baz.shark should not get loaded.
            EXPECT_TRUE(file != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.shark"));
            EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.shark"));
        }

        TEST_F(LazilyBuildDependenciesTest, Service) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'message1.shark' "
                "dependency: 'message2.shark' "
                "dependency: 'message3.shark' "
                "dependency: 'message4.shark' "
                "service {"
                "  name: 'LazyService'"
                "  method { name: 'A' input_type:  '.sharkbuf_unittest.Message1' "
                "                     output_type: '.sharkbuf_unittest.Message2' }"
                "}");
            AddSimpleMessageProtoFileToDb("message1", "Message1");
            AddSimpleMessageProtoFileToDb("message2", "Message2");
            AddSimpleMessageProtoFileToDb("message3", "Message3");
            AddSimpleMessageProtoFileToDb("message4", "Message4");

            const FileDescriptor* file = pool_.FindFileByName("foo.shark");

            // Verify calling FindServiceByName or FindMethodByName doesn't build the
            // files defining the input and output type, and input_type() and
            // output_type() does indeed build the appropriate files.
            const ServiceDescriptor* service = file->FindServiceByName("LazyService");
            EXPECT_TRUE(service != nullptr);
            const MethodDescriptor* method = service->FindMethodByName("A");
            EXPECT_FALSE(pool_.InternalIsFileLoaded("message1.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.shark"));
            EXPECT_TRUE(method != nullptr);
            EXPECT_TRUE(method->input_type() != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("message1.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.shark"));
            EXPECT_TRUE(method->output_type() != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("message2.shark"));
        }

        TEST_F(LazilyBuildDependenciesTest, GeneratedFile) {
            // Most testing is done with custom pools with lazy dependencies forced on,
            // do some sanity checking that lazy imports is on by default for the
            // generated pool, and do custom options testing with generated to
            // be able to use the GetExtension ids for the custom options.

            // Verify none of the files are loaded yet.
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_custom_option.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_enum.shark"));

            // Verify calling autogenerated function to get a descriptor in the base
            // file will build that file but none of it's imports. This verifies that
            // lazily_build_dependencies_ is set on the generated pool, and also that
            // the generated function "descriptor()" doesn't somehow subvert the laziness
            // by manually loading the dependencies or something.
            EXPECT_TRUE(sharkbuf_unittest::lazy_imports::ImportedMessage::descriptor() != nullptr);
            EXPECT_TRUE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_custom_option.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_enum.shark"));

            // Verify custom options work when defined in an import that isn't loaded,
            // and that a non-default value of a custom option doesn't load the file
            // where that enum is defined.
            const MessageOptions& options = sharkbuf_unittest::lazy_imports::MessageCustomOption::descriptor()
                                                ->options();
            sharkbuf_unittest::lazy_imports::LazyEnum custom_option_value = options.GetExtension(sharkbuf_unittest::lazy_imports::lazy_enum_option);

            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_custom_option.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_enum.shark"));
            EXPECT_EQ(custom_option_value, sharkbuf_unittest::lazy_imports::LazyEnum::LAZY_ENUM_1);

            const MessageOptions& options2 = sharkbuf_unittest::lazy_imports::MessageCustomOption2::descriptor()
                                                 ->options();
            custom_option_value = options2.GetExtension(sharkbuf_unittest::lazy_imports::lazy_enum_option);

            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_custom_option.shark"));
            EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
                "tests/sk/lib/unittest_lazy_dependencies_enum.shark"));
            EXPECT_EQ(custom_option_value, sharkbuf_unittest::lazy_imports::LazyEnum::LAZY_ENUM_0);
        }

        TEST_F(LazilyBuildDependenciesTest, Dependency) {
            ParseProtoAndAddToDb(
                "name: 'foo.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'bar.shark' "
                "message_type { "
                "  name:'Foo' "
                "  field { name:'bar' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Bar' } "
                "}");
            ParseProtoAndAddToDb(
                "name: 'bar.shark' "
                "package: 'sharkbuf_unittest' "
                "dependency: 'baz.shark' "
                "message_type { "
                "  name:'Bar' "
                "  field { name:'baz' number:1 label:OPTIONAL "
                "type_name:'.sharkbuf_unittest.Baz' } "
                "}");
            AddSimpleMessageProtoFileToDb("baz", "Baz");

            const FileDescriptor* foo_file = pool_.FindFileByName("foo.shark");
            EXPECT_TRUE(foo_file != nullptr);
            // As expected, requesting foo.shark shouldn't build it's dependencies
            EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.shark"));

            // Verify calling dependency(N) will build the dependency, but
            // not that file's dependencies.
            const FileDescriptor* bar_file = foo_file->dependency(0);
            EXPECT_TRUE(bar_file != nullptr);
            EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.shark"));
            EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.shark"));
        }

        // ===================================================================

    } // namespace descriptor_unittest
} // namespace shark

#include <shark/port_undef.inc>
