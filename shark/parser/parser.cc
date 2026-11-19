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
// Recursive descent FTW.

#include <float.h>
#include <shark/parser/parser.h>
#include <shark/stubs/ascii.h>

#include <cstdint>
#include <limits>
#include <shark/descriptor.h>
#include <shark/descriptor.skp.h>
#include <shark/persist/wired/wire_format.h>
#include <shark/parser/tokenizer.h>
#include <shark/stubs/casts.h>
#include <shark/stubs/common.h>
#include <shark/stubs/hash.h>
#include <shark/stubs/logging.h>
#include <shark/stubs/map_util.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/escaping.h>
#include <unordered_map>

namespace shark {
    namespace compiler {

        using internal::WireFormat;

        namespace {

            typedef std::unordered_map<std::string, SharkType> TypeNameMap;

            const TypeNameMap& GetTypeNameTable() {
                static auto* table = new auto([]() {
                    TypeNameMap result;

                    result["double"] = SharkType::DOUBLE;
                    result["float"] = SharkType::FLOAT;
                    result["uint64"] = SharkType::UINT64;
                    result["fixed64"] = SharkType::FIXED64;
                    result["fixed32"] = SharkType::FIXED32;
                    result["bool"] = SharkType::BOOL;
                    result["string"] = SharkType::STRING;
                    result["bytes"] = SharkType::BYTES;
                    result["uint32"] = SharkType::UINT32;
                    result["sfixed32"] = SharkType::SFIXED32;
                    result["sfixed64"] = SharkType::SFIXED64;
                    result["int32"] = SharkType::INT32;
                    result["int64"] = SharkType::INT64;
                    result["sint32"] = SharkType::SINT32;
                    result["sint64"] = SharkType::SINT64;
                    result["int8"] = SharkType::INT8;
                    result["uint8"] = SharkType::UINT8;
                    result["int16"] = SharkType::INT16;
                    result["uint16"] = SharkType::UINT16;

                    return result;
                }());
                return *table;
            }

            // Camel-case the field name and append "Entry" for generated map entry name.
            // e.g. map<KeyType, ValueType> foo_map => FooMapEntry
            std::string MapEntryName(const std::string& field_name) {
                std::string result;
                static const char kSuffix[] = "Entry";
                result.reserve(field_name.size() + sizeof(kSuffix));
                bool cap_next = true;
                for (const char field_name_char : field_name) {
                    if (field_name_char == '_') {
                        cap_next = true;
                    } else if (cap_next) {
                        // Note: Do not use ctype.h due to locales.
                        if ('a' <= field_name_char && field_name_char <= 'z') {
                            result.push_back(field_name_char - 'a' + 'A');
                        } else {
                            result.push_back(field_name_char);
                        }
                        cap_next = false;
                    } else {
                        result.push_back(field_name_char);
                    }
                }
                result.append(kSuffix);
                return result;
            }

        } // anonymous namespace

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) \
    if (STATEMENT) {  \
    } else            \
        return false

        // ===================================================================

        Parser::Parser()
            : input_(nullptr)
            , error_collector_(nullptr)
            , source_location_table_(nullptr)
            , had_errors_(false)
            , require_syntax_identifier_(false)
            , stop_after_syntax_identifier_(false) {
        }

        Parser::~Parser() { }

        // ===================================================================

        inline bool Parser::looking_at(const char* text) {
            return input_->current().text == text;
        }

        inline bool Parser::looking_at_type(io::Tokenizer::TokenType token_type) {
            return input_->current().type == token_type;
        }

        inline bool Parser::at_end() {
            return looking_at_type(io::Tokenizer::TYPE_END);
        }

        bool Parser::try_consume(const char* text) {
            if (looking_at(text)) {
                input_->Next();
                return true;
            } else {
                return false;
            }
        }

        bool Parser::consume(const char* text, const char* error) {
            if (try_consume(text)) {
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::consume(const char* text) {
            std::string error = "Expected \"" + std::string(text) + "\".";
            return consume(text, error.c_str());
        }

        bool Parser::consume_identifier(std::string* output, const char* error) {
            if (looking_at_type(io::Tokenizer::TYPE_IDENTIFIER)) {
                *output = input_->current().text;
                input_->Next();
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::consume_integer(int* output, const char* error) {
            if (looking_at_type(io::Tokenizer::TYPE_INTEGER)) {
                uint64_t value = 0;
                if (!io::Tokenizer::parse_integer(input_->current().text,
                        std::numeric_limits<int32_t>::max(),
                        &value)) {
                    add_error("Integer out of range.");
                    // We still return true because we did, in fact, parse an integer.
                }
                *output = value;
                input_->Next();
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::consume_signed_integer(int* output, const char* error) {
            bool is_negative = false;
            uint64_t max_value = std::numeric_limits<int32_t>::max();
            if (try_consume("-")) {
                is_negative = true;
                max_value += 1;
            }
            uint64_t value = 0;
            DO(consume_integer64(max_value, &value, error));
            if (is_negative)
                value *= -1;
            *output = value;
            return true;
        }

        bool Parser::consume_integer64(uint64_t max_value, uint64_t* output,
            const char* error) {
            if (looking_at_type(io::Tokenizer::TYPE_INTEGER)) {
                if (!io::Tokenizer::parse_integer(input_->current().text, max_value,
                        output)) {
                    add_error("Integer out of range.");
                    // We still return true because we did, in fact, parse an integer.
                    *output = 0;
                }
                input_->Next();
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::consume_number(double* output, const char* error) {
            if (looking_at_type(io::Tokenizer::TYPE_FLOAT)) {
                *output = io::Tokenizer::parse_float(input_->current().text);
                input_->Next();
                return true;
            } else if (looking_at_type(io::Tokenizer::TYPE_INTEGER)) {
                // Also accept integers.
                uint64_t value = 0;
                if (!io::Tokenizer::parse_integer(input_->current().text,
                        std::numeric_limits<uint64_t>::max(),
                        &value)) {
                    add_error("Integer out of range.");
                    // We still return true because we did, in fact, parse a number.
                }
                *output = value;
                input_->Next();
                return true;
            } else if (looking_at("inf")) {
                *output = std::numeric_limits<double>::infinity();
                input_->Next();
                return true;
            } else if (looking_at("nan")) {
                *output = std::numeric_limits<double>::quiet_NaN();
                input_->Next();
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::consume_string(std::string* output, const char* error) {
            if (looking_at_type(io::Tokenizer::TYPE_STRING)) {
                io::Tokenizer::parse_string(input_->current().text, output);
                input_->Next();
                // Allow C++ like concatenation of adjacent string tokens.
                while (looking_at_type(io::Tokenizer::TYPE_STRING)) {
                    io::Tokenizer::parse_string_append(input_->current().text, output);
                    input_->Next();
                }
                return true;
            } else {
                add_error(error);
                return false;
            }
        }

        bool Parser::try_consume_end_of_declaration(const char* text,
            const LocationRecorder* location) {
            if (looking_at(text)) {
                std::string leading, trailing;
                std::vector<std::string> detached;
                input_->next_with_comments(&trailing, &detached, &leading);

                // Save the leading comments for next time, and recall the leading comments
                // from last time.
                leading.swap(upcoming_doc_comments_);

                if (location != nullptr) {
                    upcoming_detached_comments_.swap(detached);
                    location->AttachComments(&leading, &trailing, &detached);
                } else if (strcmp(text, "}") == 0) {
                    // If the current location is null and we are finishing the current scope,
                    // drop pending upcoming detached comments.
                    upcoming_detached_comments_.swap(detached);
                } else {
                    // Otherwise, append the new detached comments to the existing upcoming
                    // detached comments.
                    upcoming_detached_comments_.insert(upcoming_detached_comments_.end(),
                        detached.begin(), detached.end());
                }

                return true;
            } else {
                return false;
            }
        }

        bool Parser::consume_end_of_declaration(const char* text,
            const LocationRecorder* location) {
            if (try_consume_end_of_declaration(text, location)) {
                return true;
            } else {
                add_error("Expected \"" + std::string(text) + "\".");
                return false;
            }
        }

        // -------------------------------------------------------------------

        void Parser::add_error(int line, int column, const std::string& error) {
            if (error_collector_ != nullptr) {
                error_collector_->add_error(line, column, error);
            }
            had_errors_ = true;
        }

        void Parser::add_error(const std::string& error) {
            add_error(input_->current().line, input_->current().column, error);
        }

        void Parser::add_warning(const std::string& warning) {
            if (error_collector_ != nullptr) {
                error_collector_->add_warning(input_->current().line,
                    input_->current().column, warning);
            }
        }

        // -------------------------------------------------------------------

        Parser::LocationRecorder::LocationRecorder(Parser* parser)
            : parser_(parser)
            , source_code_info_(parser->source_code_info_)
            , location_(parser_->source_code_info_->add_location()) {
            location_->add_span(parser_->input_->current().line);
            location_->add_span(parser_->input_->current().column);
        }

        Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent) {
            Init(parent, parent.source_code_info_);
        }

        Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
            int path1,
            SourceCodeInfo* source_code_info) {
            Init(parent, source_code_info);
            AddPath(path1);
        }

        Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
            int path1) {
            Init(parent, parent.source_code_info_);
            AddPath(path1);
        }

        Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
            int path1, int path2) {
            Init(parent, parent.source_code_info_);
            AddPath(path1);
            AddPath(path2);
        }

        void Parser::LocationRecorder::Init(const LocationRecorder& parent,
            SourceCodeInfo* source_code_info) {
            parser_ = parent.parser_;
            source_code_info_ = source_code_info;

            location_ = source_code_info_->add_location();
            location_->mutable_path()->CopyFrom(parent.location_->path());

            location_->add_span(parser_->input_->current().line);
            location_->add_span(parser_->input_->current().column);
        }

        Parser::LocationRecorder::~LocationRecorder() {
            if (location_->span_size() <= 2) {
                EndAt(parser_->input_->previous());
            }
        }

        void Parser::LocationRecorder::AddPath(int path_component) {
            location_->add_path(path_component);
        }

        void Parser::LocationRecorder::StartAt(const io::Tokenizer::Token& token) {
            location_->set_span(0, token.line);
            location_->set_span(1, token.column);
        }

        void Parser::LocationRecorder::StartAt(const LocationRecorder& other) {
            location_->set_span(0, other.location_->span(0));
            location_->set_span(1, other.location_->span(1));
        }

        void Parser::LocationRecorder::EndAt(const io::Tokenizer::Token& token) {
            if (token.line != location_->span(0)) {
                location_->add_span(token.line);
            }
            location_->add_span(token.end_column);
        }

        void Parser::LocationRecorder::RecordLegacyLocation(
            const Message* descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location) {
            if (parser_->source_location_table_ != nullptr) {
                parser_->source_location_table_->Add(
                    descriptor, location, location_->span(0), location_->span(1));
            }
        }

        void Parser::LocationRecorder::RecordLegacyImportLocation(
            const Message* descriptor, const std::string& name) {
            if (parser_->source_location_table_ != nullptr) {
                parser_->source_location_table_->AddImport(
                    descriptor, name, location_->span(0), location_->span(1));
            }
        }

        int Parser::LocationRecorder::CurrentPathSize() const {
            return location_->path_size();
        }

        void Parser::LocationRecorder::AttachComments(
            std::string* leading, std::string* trailing,
            std::vector<std::string>* detached_comments) const {
            SHARK_CHECK(!location_->has_leading_comments());
            SHARK_CHECK(!location_->has_trailing_comments());

            if (!leading->empty()) {
                location_->mutable_leading_comments()->swap(*leading);
            }
            if (!trailing->empty()) {
                location_->mutable_trailing_comments()->swap(*trailing);
            }
            for (int i = 0; i < detached_comments->size(); ++i) {
                location_->add_leading_detached_comments()->swap((*detached_comments)[i]);
            }
            detached_comments->clear();
        }

        // -------------------------------------------------------------------

        void Parser::SkipStatement() {
            while (true) {
                if (at_end()) {
                    return;
                } else if (looking_at_type(io::Tokenizer::TYPE_SYMBOL)) {
                    if (try_consume_end_of_declaration(";", nullptr)) {
                        return;
                    } else if (try_consume("{")) {
                        SkipRestOfBlock();
                        return;
                    } else if (looking_at("}")) {
                        return;
                    }
                }
                input_->Next();
            }
        }

        void Parser::SkipRestOfBlock() {
            while (true) {
                if (at_end()) {
                    return;
                } else if (looking_at_type(io::Tokenizer::TYPE_SYMBOL)) {
                    if (try_consume_end_of_declaration("}", nullptr)) {
                        return;
                    } else if (try_consume("{")) {
                        SkipRestOfBlock();
                    }
                }
                input_->Next();
            }
        }

        // ===================================================================

        bool EnumValueFits(EnumUnderlyingType type, int32_t number);

        bool Parser::validate_enum(const EnumDescriptorProto* proto) {
            bool has_allow_alias = false;
            bool allow_alias = false;

            for (int i = 0; i < proto->options().uninterpreted_option_size(); i++) {
                const UninterpretedOption option = proto->options().uninterpreted_option(i);
                if (option.name_size() != 1 || option.name(0).is_extension()) {
                    continue;
                }
                const std::string& name = option.name(0).name_part();
                if (name == "allow_alias") {
                    has_allow_alias = true;
                    allow_alias = option.identifier_value() == "true";
                }
            }

            if (has_allow_alias && !allow_alias) {
                std::string error = "\"" + proto->name() + "\" declares 'option allow_alias = false;' which has no effect. "
                                                           "Please remove the declaration.";
                // This needlessly clutters declarations with nops.
                add_error(error);
                return false;
            }

            std::set<int> used_values;
            bool has_duplicates = false;
            for (int i = 0; i < proto->value_size(); ++i) {
                const EnumValueDescriptorProto& enum_value = proto->value(i);
                if (used_values.find(enum_value.number()) != used_values.end()) {
                    has_duplicates = true;
                    break;
                } else {
                    used_values.insert(enum_value.number());
                }
            }
            if (allow_alias && !has_duplicates) {
                std::string error = "\"" + proto->name() + "\" declares support for enum aliases but no enum values share field "
                                                           "numbers. Please remove the unnecessary 'option allow_alias = true;' "
                                                           "declaration.";
                // Generate an error if an enum declares support for duplicate enum values
                // and does not use it protect future authors.
                add_error(error);
                return false;
            }

            // Enforce that enum constants must be UPPER_CASE except in case of
            // enum_alias.
            if (!allow_alias) {
                for (const auto& enum_value : proto->value()) {
                    if (!is_upper_underscore(enum_value.name())) {
                        add_warning(
                            "Enum constant should be in UPPER_CASE. Found: " + enum_value.name() + ". See https://developers.google.com/protocol-buffers/docs/style");
                    }
                }
            }

            if (proto->options().has_underlying_type()) {
                for (int i = 0; i < proto->value_size(); ++i) {
                    if (!EnumValueFits(proto->options().underlying_type(),
                            proto->value(i).number())) {
                        add_error("Enum value \"" + proto->value(i).name() + "\" does not fit the underlying type.");
                        return false;
                    }
                }
            }

            return true;
        }

        bool Parser::Parse(io::Tokenizer* input, FileDescriptorProto* file) {
            input_ = input;
            had_errors_ = false;
            syntax_identifier_.clear();

            // Note that |file| could be NULL at this point if
            // stop_after_syntax_identifier_ is true.  So, we conservatively allocate
            // SourceCodeInfo on the stack, then swap it into the FileDescriptorProto
            // later on.
            SourceCodeInfo source_code_info;
            source_code_info_ = &source_code_info;

            if (looking_at_type(io::Tokenizer::TYPE_START)) {
                // Advance to first token.
                input_->next_with_comments(nullptr, &upcoming_detached_comments_,
                    &upcoming_doc_comments_);
            }

            {
                LocationRecorder root_location(this);
                root_location.RecordLegacyLocation(file,
                    DescriptorPool::ErrorCollector::OTHER);

                if (!looking_at("syntax")) {
                    add_error(
                        "File must begin with a syntax statement, e.g. 'syntax = "
                        "\"proto2\";'.");
                    return false;
                }
                if (!ParseSyntaxIdentifier(root_location)) {
                    // Don't attempt to parse the file if we didn't recognize the syntax
                    // identifier.
                    return false;
                }
                if (file != nullptr)
                    file->set_syntax(syntax_identifier_);

                if (stop_after_syntax_identifier_)
                    return !had_errors_;

                // Repeatedly parse statements until we reach the end of the file.
                while (!at_end()) {
                    if (!ParseTopLevelStatement(file, root_location)) {
                        // This statement failed to parse.  Skip it, but keep looping to parse
                        // other statements.
                        SkipStatement();

                        if (looking_at("}")) {
                            add_error("Unmatched \"}\".");
                            input_->next_with_comments(nullptr, &upcoming_detached_comments_,
                                &upcoming_doc_comments_);
                        }
                    }
                }
            }

            input_ = nullptr;
            source_code_info_ = nullptr;
            assert(file != nullptr);
            source_code_info.Swap(file->mutable_source_code_info());
            return !had_errors_;
        }

        bool Parser::ParseSyntaxIdentifier(const LocationRecorder& parent) {
            LocationRecorder syntax_location(parent,
                FileDescriptorProto::kSyntaxFieldNumber);
            DO(consume(
                "syntax",
                "File must begin with a syntax statement, e.g. 'syntax = \"proto2\";'."));
            DO(consume("="));
            io::Tokenizer::Token syntax_token = input_->current();
            std::string syntax;
            DO(consume_string(&syntax, "Expected syntax identifier."));
            DO(consume_end_of_declaration(";", &syntax_location));

            syntax_identifier_ = syntax;

            if (syntax != "proto2") {
                add_error(syntax_token.line, syntax_token.column,
                    "Unrecognized syntax identifier \"" + syntax + "\".  This parser "
                                                                   "only recognizes \"proto2\".");
                return false;
            }

            return true;
        }

        bool Parser::ParseTopLevelStatement(FileDescriptorProto* file,
            const LocationRecorder& root_location) {
            if (try_consume_end_of_declaration(";", nullptr)) {
                // empty statement; ignore
                return true;
            } else if (looking_at("struct")) {
                LocationRecorder location(root_location,
                    FileDescriptorProto::kMessageTypeFieldNumber,
                    file->message_type_size());
                return ParseMessageDefinition(file->add_message_type(), location, file);
            } else if (looking_at("enum")) {
                LocationRecorder location(root_location,
                    FileDescriptorProto::kEnumTypeFieldNumber,
                    file->enum_type_size());
                return ParseEnumDefinition(file->add_enum_type(), location, file);
            } else if (looking_at("servlet")) {
                LocationRecorder location(root_location,
                    FileDescriptorProto::kServletFieldNumber,
                    file->servlet_size());
                return parse_servlet_definition(file->add_servlet(), location, file);
            } else if (looking_at("extend")) {
                LocationRecorder location(root_location,
                    FileDescriptorProto::kAnonymousExtensionFieldNumber);
                return parse_extend(
                    file->mutable_anonymous_extension(), file->mutable_message_type(), root_location,
                    FileDescriptorProto::kMessageTypeFieldNumber, location, file);
            } else if (looking_at("import")) {
                return ParseImport(file->mutable_dependency(),
                    file->mutable_public_dependency(),
                    file->mutable_weak_dependency(), root_location, file);
            } else if (looking_at("package")) {
                return ParsePackage(file, root_location, file);
            } else if (looking_at("option")) {
                LocationRecorder location(root_location,
                    FileDescriptorProto::kOptionsFieldNumber);
                return prse_option(file->mutable_options(), location, file,
                    OPTION_STATEMENT);
            } else {
                add_error("Expected top-level statement (e.g. \"struct\").");
                return false;
            }
        }

        // -------------------------------------------------------------------
        // Messages

        bool Parser::ParseMessageDefinition(
            DescriptorProto* message, const LocationRecorder& message_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("struct"));
            {
                LocationRecorder location(message_location,
                    DescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(message,
                    DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(message->mutable_name(), "Expected message name."));
                if (!is_upper_camel_case(message->name())) {
                    add_warning(
                        "Message name should be in UpperCamelCase. Found: " + message->name() + ". See https://developers.google.com/protocol-buffers/docs/style");
                }
            }
            DO(ParseMessageBlock(message, message_location, containing_file));
            return true;
        }

        namespace {

            const int kMaxRangeSentinel = -1;

            bool IsMessageSetWireFormatMessage(const DescriptorProto& message) {
                const MessageOptions& options = message.options();
                for (int i = 0; i < options.uninterpreted_option_size(); ++i) {
                    const UninterpretedOption& uninterpreted = options.uninterpreted_option(i);
                    if (uninterpreted.name_size() == 1 && uninterpreted.name(0).name_part() == "message_set_wire_format" && uninterpreted.identifier_value() == "true") {
                        return true;
                    }
                }
                return false;
            }

            // Modifies any extension ranges that specified 'max' as the end of the
            // extension range, and sets them to the type-specific maximum. The actual max
            // tag number can only be determined after all options have been parsed.
            void AdjustExtensionRangesWithMaxEndNumber(DescriptorProto* message) {
                const bool is_message_set = IsMessageSetWireFormatMessage(*message);
                const int max_extension_number = is_message_set
                    ? std::numeric_limits<int32_t>::max()
                    : FieldDescriptor::kMaxNumber + 1;
                for (int i = 0; i < message->extension_range_size(); ++i) {
                    if (message->extension_range(i).end() == kMaxRangeSentinel) {
                        message->mutable_extension_range(i)->set_end(max_extension_number);
                    }
                }
            }

            // Modifies any reserved ranges that specified 'max' as the end of the
            // reserved range, and sets them to the type-specific maximum. The actual max
            // tag number can only be determined after all options have been parsed.
            void AdjustReservedRangesWithMaxEndNumber(DescriptorProto* message) {
                const bool is_message_set = IsMessageSetWireFormatMessage(*message);
                const int max_field_number = is_message_set
                    ? std::numeric_limits<int32_t>::max()
                    : FieldDescriptor::kMaxNumber + 1;
                for (int i = 0; i < message->reserved_range_size(); ++i) {
                    if (message->reserved_range(i).end() == kMaxRangeSentinel) {
                        message->mutable_reserved_range(i)->set_end(max_field_number);
                    }
                }
            }

        } // namespace

        bool Parser::ParseMessageBlock(DescriptorProto* message,
            const LocationRecorder& message_location,
            const FileDescriptorProto* containing_file) {
            DO(consume_end_of_declaration("{", &message_location));

            while (!try_consume_end_of_declaration("}", nullptr)) {
                if (at_end()) {
                    add_error("Reached end of input in message definition (missing '}').");
                    return false;
                }

                if (!ParseMessageStatement(message, message_location, containing_file)) {
                    // This statement failed to parse.  Skip it, but keep looping to parse
                    // other statements.
                    SkipStatement();
                }
            }

            if (message->extension_range_size() > 0) {
                AdjustExtensionRangesWithMaxEndNumber(message);
            }
            if (message->reserved_range_size() > 0) {
                AdjustReservedRangesWithMaxEndNumber(message);
            }
            return true;
        }

        bool Parser::ParseMessageStatement(DescriptorProto* message,
            const LocationRecorder& message_location,
            const FileDescriptorProto* containing_file) {
            if (try_consume_end_of_declaration(";", nullptr)) {
                // empty statement; ignore
                return true;
            } else if (looking_at("struct")) {
                LocationRecorder location(message_location,
                    DescriptorProto::kNestedTypeFieldNumber,
                    message->nested_type_size());
                return ParseMessageDefinition(message->add_nested_type(), location,
                    containing_file);
            } else if (looking_at("enum")) {
                LocationRecorder location(message_location,
                    DescriptorProto::kEnumTypeFieldNumber,
                    message->enum_type_size());
                return ParseEnumDefinition(message->add_enum_type(), location,
                    containing_file);
            } else if (looking_at("extensions")) {
                LocationRecorder location(message_location,
                    DescriptorProto::kExtensionRangeFieldNumber);
                return ParseExtensions(message, location, containing_file);
            } else if (looking_at("reserved")) {
                return ParseReserved(message, message_location);
            } else if (looking_at("extend")) {
                LocationRecorder location(message_location,
                    DescriptorProto::kAnonymousExtensionFieldNumber);
                return parse_extend(message->mutable_anonymous_extension(),
                    message->mutable_nested_type(), message_location,
                    DescriptorProto::kNestedTypeFieldNumber, location,
                    containing_file);
            } else if (looking_at("option")) {
                LocationRecorder location(message_location,
                    DescriptorProto::kOptionsFieldNumber);
                return prse_option(message->mutable_options(), location, containing_file,
                    OPTION_STATEMENT);
            } else if (looking_at("union")) {
                int oneof_index = message->oneof_decl_size();
                LocationRecorder oneof_location(
                    message_location, DescriptorProto::kOneofDeclFieldNumber, oneof_index);

                return parse_oneof(message->add_oneof_decl(), message, oneof_index,
                    oneof_location, message_location, containing_file);
            } else {
                LocationRecorder location(message_location,
                    DescriptorProto::kFieldFieldNumber,
                    message->field_size());
                return ParseMessageField(
                    message->add_field(), message->mutable_nested_type(), message_location,
                    DescriptorProto::kNestedTypeFieldNumber, location, containing_file);
            }
        }

        bool Parser::ParseMessageField(FieldDescriptorProto* field,
            RepeatedPtrField<DescriptorProto>* messages,
            const LocationRecorder& parent_location,
            int location_field_number_for_nested_type,
            const LocationRecorder& field_location,
            const FileDescriptorProto* containing_file) {
            {
                FieldCardinality label;
                if (parse_label(&label, field_location)) {
                    field->set_label(label);
                }
            }

            return ParseMessageFieldNoLabel(field, messages, parent_location,
                location_field_number_for_nested_type,
                field_location, containing_file);
        }

        bool Parser::ParseMessageFieldNoLabel(
            FieldDescriptorProto* field, RepeatedPtrField<DescriptorProto>* messages,
            const LocationRecorder& parent_location,
            int location_field_number_for_nested_type,
            const LocationRecorder& field_location,
            const FileDescriptorProto* containing_file) {
            MapField map_field;
            // Parse type.
            {
                LocationRecorder location(field_location); // add path later
                location.RecordLegacyLocation(field, DescriptorPool::ErrorCollector::TYPE);

                bool type_parsed = false;
                auto type = SharkType::INT32;
                std::string type_name;

                // list<T> is a repeated field of type T. The field type stays T.
                // optional and required before list<> are accepted and ignored.
                bool list_field = false;
                if (try_consume("list")) {
                    if (looking_at("<")) {
                        if (field->has_oneof_index()) {
                            add_error("List fields are not allowed in unions.");
                            return false;
                        }
                        io::Tokenizer::Token list_token = input_->previous();
                        DO(consume("<"));
                        io::Tokenizer::Token type_start = input_->current();
                        DO(parse_type(&type, &type_name));
                        io::Tokenizer::Token type_end = input_->previous();
                        DO(consume(">"));
                        location.StartAt(type_start);
                        location.EndAt(type_end);
                        field->set_label(FieldCardinality::REPEATED);
                        {
                            LocationRecorder label_location(
                                field_location, FieldDescriptorProto::kLabelFieldNumber);
                            label_location.StartAt(list_token);
                            label_location.EndAt(list_token);
                        }
                        list_field = true;
                        type_parsed = true;
                    } else {
                        type_parsed = true;
                        type_name = "list";
                    }
                }

                // Special case map field. We only treat the field as a map field if the
                // field type name starts with the word "map" with a following "<".
                if (!list_field && try_consume("map")) {
                    if (looking_at("<")) {
                        map_field.is_map_field = true;
                        DO(ParseMapType(&map_field, field, location));
                    } else {
                        // False positive
                        type_parsed = true;
                        type_name = "map";
                    }
                }
                if (!map_field.is_map_field) {
                    // Handle the case where no explicit label is given for a non-map field.
                    if (!list_field && !field->has_label() && default_to_optional_fields()) {
                        field->set_label(FieldCardinality::OPTIONAL);
                    }
                    if (!field->has_label()) {
                        add_error("Expected \"required\" or \"optional\".");
                        // We can actually reasonably recover here by just assuming the user
                        // forgot the label altogether.
                        field->set_label(FieldCardinality::OPTIONAL);
                    }

                    // Handle the case where the actual type is a message or enum named
                    // "map", which we already consumed in the code above.
                    if (!type_parsed) {
                        DO(parse_type(&type, &type_name));
                    }
                    if (type_name.empty()) {
                        location.AddPath(FieldDescriptorProto::kTypeFieldNumber);
                        field->set_type(type);
                    } else {
                        location.AddPath(FieldDescriptorProto::kTypeNameFieldNumber);
                        field->set_type_name(type_name);
                    }
                }
            }

            // Parse name and '='.
            io::Tokenizer::Token name_token = input_->current();
            {
                LocationRecorder location(field_location,
                    FieldDescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(field, DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(field->mutable_name(), "Expected field name."));

                if (!is_lower_underscore(field->name())) {
                    add_warning(
                        "Field name should be lowercase. Found: " + field->name() + ". See: https://developers.google.com/protocol-buffers/docs/style");
                }
                if (is_number_follow_underscore(field->name())) {
                    add_warning(
                        "Number should not come right after an underscore. Found: " + field->name() + ". See: https://developers.google.com/protocol-buffers/docs/style");
                }
            }
            DO(consume("=", "Missing field number."));

            // Parse field number.
            {
                LocationRecorder location(field_location,
                    FieldDescriptorProto::kNumberFieldNumber);
                location.RecordLegacyLocation(field,
                    DescriptorPool::ErrorCollector::NUMBER);
                int number;
                DO(consume_integer(&number, "Expected field number."));
                field->set_number(number);
            }

            // Parse options.
            DO(parse_field_options(field, field_location, containing_file));

            (void)name_token;
            (void)messages;
            (void)parent_location;
            (void)location_field_number_for_nested_type;
            DO(consume_end_of_declaration(";", &field_location));

            // Create a map entry type if this is a map field.
            if (map_field.is_map_field) {
                generate_map_entry(map_field, field, messages);
            }

            return true;
        }

        bool Parser::ParseMapType(MapField* map_field, FieldDescriptorProto* field,
            LocationRecorder& type_name_location) {
            if (field->has_oneof_index()) {
                add_error("Map fields are not allowed in unions.");
                return false;
            }
            if (field->has_label()) {
                add_error(
                    "Field labels (required/optional) are not allowed on "
                    "map fields.");
                return false;
            }
            if (field->has_extendee()) {
                add_error("Map fields are not allowed to be extensions.");
                return false;
            }
            field->set_label(FieldCardinality::REPEATED);
            DO(consume("<"));
            DO(parse_type(&map_field->key_type, &map_field->key_type_name));
            DO(consume(","));
            DO(parse_type(&map_field->value_type, &map_field->value_type_name));
            DO(consume(">"));
            // Defer setting of the type name of the map field until the
            // field name is parsed. Add the source location though.
            type_name_location.AddPath(FieldDescriptorProto::kTypeNameFieldNumber);
            return true;
        }

        void Parser::generate_map_entry(const MapField& map_field,
            FieldDescriptorProto* field,
            RepeatedPtrField<DescriptorProto>* messages) {
            DescriptorProto* entry = messages->Add();
            std::string entry_name = MapEntryName(field->name());
            field->set_type_name(entry_name);
            entry->set_name(entry_name);
            entry->mutable_options()->set_map_entry(true);
            FieldDescriptorProto* key_field = entry->add_field();
            key_field->set_name("key");
            key_field->set_label(FieldCardinality::OPTIONAL);
            key_field->set_number(1);
            if (map_field.key_type_name.empty()) {
                key_field->set_type(map_field.key_type);
            } else {
                key_field->set_type_name(map_field.key_type_name);
            }
            FieldDescriptorProto* value_field = entry->add_field();
            value_field->set_name("value");
            value_field->set_label(FieldCardinality::OPTIONAL);
            value_field->set_number(2);
            if (map_field.value_type_name.empty()) {
                value_field->set_type(map_field.value_type);
            } else {
                value_field->set_type_name(map_field.value_type_name);
            }
            // Propagate the "enforce_utf8" option to key and value fields if they
            // are strings. This helps simplify the implementation of code generators
            // and also reflection-based parsing code.
            //
            // The following definition:
            //   message Foo {
            //     map<string, string> value = 1 [enforce_utf8 = false];
            //   }
            // will be interpreted as:
            //   message Foo {
            //     message ValueEntry {
            //       option map_entry = true;
            //       string key = 1 [enforce_utf8 = false];
            //       string value = 2 [enforce_utf8 = false];
            //     }
            //     repeated ValueEntry value = 1 [enforce_utf8 = false];
            //  }
            //
            // TODO(xiaofeng): Remove this when the "enforce_utf8" option is removed
            // from protocol compiler.
            for (int i = 0; i < field->options().uninterpreted_option_size(); ++i) {
                const UninterpretedOption& option = field->options().uninterpreted_option(i);
                if (option.name_size() == 1 && option.name(0).name_part() == "enforce_utf8" && !option.name(0).is_extension()) {
                    if (key_field->type() == SharkType::STRING) {
                        key_field->mutable_options()->add_uninterpreted_option()->CopyFrom(
                            option);
                    }
                    if (value_field->type() == SharkType::STRING) {
                        value_field->mutable_options()->add_uninterpreted_option()->CopyFrom(
                            option);
                    }
                }
            }
        }

        bool Parser::parse_field_options(FieldDescriptorProto* field,
            const LocationRecorder& field_location,
            const FileDescriptorProto* containing_file) {
            if (!looking_at("["))
                return true;

            LocationRecorder location(field_location,
                FieldDescriptorProto::kOptionsFieldNumber);

            DO(consume("["));

            // Parse field options.
            do {
                if (looking_at("default")) {
                    // We intentionally pass field_location rather than location here, since
                    // the default value is not actually an option.
                    DO(parse_default_assignment(field, field_location, containing_file));
                } else if (looking_at("json_name")) {
                    // Like default value, this "json_name" is not an actual option.
                    DO(parse_json_name(field, field_location, containing_file));
                } else {
                    DO(prse_option(field->mutable_options(), location, containing_file,
                        OPTION_ASSIGNMENT));
                }
            } while (try_consume(","));

            DO(consume("]"));
            return true;
        }

        bool Parser::parse_default_assignment(
            FieldDescriptorProto* field, const LocationRecorder& field_location,
            const FileDescriptorProto* containing_file) {
            if (field->has_default_value()) {
                add_error("Already set option \"default\".");
                field->clear_default_value();
            }

            DO(consume("default"));
            DO(consume("="));

            LocationRecorder location(field_location,
                FieldDescriptorProto::kDefaultValueFieldNumber);
            location.RecordLegacyLocation(field,
                DescriptorPool::ErrorCollector::DEFAULT_VALUE);
            std::string* default_value = field->mutable_default_value();

            if (!field->has_type()) {
                // The field has a type name, but we don't know if it is a message or an
                // enum yet. (If it were a primitive type, |field| would have a type set
                // already.) In this case, simply take the current string as the default
                // value; we will catch the error later if it is not a valid enum value.
                // (N.B. that we do not check whether the current token is an identifier:
                // doing so throws strange errors when the user mistypes a primitive
                // typename and we assume it's an enum. E.g.: "optional int foo = 1 [default
                // = 42]". In such a case the fundamental error is really that "int" is not
                // a type, not that "42" is not an identifier. See b/12533582.)
                *default_value = input_->current().text;
                input_->Next();
                return true;
            }

            switch (field->type()) {
            case SharkType::INT32:
            case SharkType::INT64:
            case SharkType::SINT32:
            case SharkType::SINT64:
            case SharkType::SFIXED32:
            case SharkType::SFIXED64:
            case SharkType::INT8:
            case SharkType::INT16: {
                uint64_t max_value = std::numeric_limits<int64_t>::max();
                if (field->type() == SharkType::INT32 || field->type() == SharkType::SINT32 || field->type() == SharkType::SFIXED32) {
                    max_value = std::numeric_limits<int32_t>::max();
                } else if (field->type() == SharkType::INT8) {
                    max_value = std::numeric_limits<int8_t>::max();
                } else if (field->type() == SharkType::INT16) {
                    max_value = std::numeric_limits<int16_t>::max();
                }

                // These types can be negative.
                if (try_consume("-")) {
                    default_value->append("-");
                    // Two's complement always has one more negative value than positive.
                    ++max_value;
                }
                // Parse the integer to verify that it is not out-of-range.
                uint64_t value;
                DO(consume_integer64(max_value, &value,
                    "Expected integer for field default value."));
                // And stringify it again.
                default_value->append(turbo::str_cat(value));
                break;
            }

            case SharkType::UINT32:
            case SharkType::UINT64:
            case SharkType::FIXED32:
            case SharkType::FIXED64:
            case SharkType::UINT8:
            case SharkType::UINT16: {
                uint64_t max_value = std::numeric_limits<uint64_t>::max();
                if (field->type() == SharkType::UINT32 || field->type() == SharkType::FIXED32) {
                    max_value = std::numeric_limits<uint32_t>::max();
                } else if (field->type() == SharkType::UINT8) {
                    max_value = std::numeric_limits<uint8_t>::max();
                } else if (field->type() == SharkType::UINT16) {
                    max_value = std::numeric_limits<uint16_t>::max();
                }

                // Numeric, not negative.
                if (try_consume("-")) {
                    add_error("Unsigned field can't have negative default value.");
                }
                // Parse the integer to verify that it is not out-of-range.
                uint64_t value;
                DO(consume_integer64(max_value, &value,
                    "Expected integer for field default value."));
                // And stringify it again.
                default_value->append(turbo::str_cat(value));
                break;
            }

            case SharkType::FLOAT:
            case SharkType::DOUBLE:
                // These types can be negative.
                if (try_consume("-")) {
                    default_value->append("-");
                }
                // Parse the integer because we have to convert hex integers to decimal
                // floats.
                double value;
                DO(consume_number(&value, "Expected number."));
                // And stringify it again.
                default_value->append(turbo::str_cat(turbo::HighPrecision(static_cast<double>(value))));
                break;

            case SharkType::BOOL:
                if (try_consume("true")) {
                    default_value->assign("true");
                } else if (try_consume("false")) {
                    default_value->assign("false");
                } else {
                    add_error("Expected \"true\" or \"false\".");
                    return false;
                }
                break;

            case SharkType::STRING:
                // Note: When file option java_string_check_utf8 is true, if a
                // non-string representation (eg byte[]) is later supported, it must
                // be checked for UTF-8-ness.
                DO(consume_string(default_value,
                    "Expected string for field default "
                    "value."));
                break;

            case SharkType::BYTES:
                DO(consume_string(default_value, "Expected string."));
                *default_value = turbo::c_escape(*default_value);
                break;

            case SharkType::ENUM:
                DO(consume_identifier(default_value,
                    "Expected enum identifier for field "
                    "default value."));
                break;

            case SharkType::MESSAGE:
                add_error("Messages can't have default values.");
                return false;
            }

            return true;
        }

        bool Parser::parse_json_name(FieldDescriptorProto* field,
            const LocationRecorder& field_location,
            const FileDescriptorProto* containing_file) {
            if (field->has_json_name()) {
                add_error("Already set option \"json_name\".");
                field->clear_json_name();
            }

            LocationRecorder location(field_location,
                FieldDescriptorProto::kJsonNameFieldNumber);
            location.RecordLegacyLocation(field,
                DescriptorPool::ErrorCollector::OPTION_NAME);

            DO(consume("json_name"));
            DO(consume("="));

            LocationRecorder value_location(location);
            value_location.RecordLegacyLocation(
                field, DescriptorPool::ErrorCollector::OPTION_VALUE);

            DO(consume_string(field->mutable_json_name(),
                "Expected string for JSON name."));
            return true;
        }

        bool Parser::parse_option_name_part(UninterpretedOption* uninterpreted_option,
            const LocationRecorder& part_location,
            const FileDescriptorProto* containing_file) {
            UninterpretedOption::NamePart* name = uninterpreted_option->add_name();
            std::string identifier; // We parse identifiers into this string.
            if (looking_at("(")) { // This is an extension.
                DO(consume("("));

                {
                    LocationRecorder location(
                        part_location, UninterpretedOption::NamePart::kNamePartFieldNumber);
                    // An extension name consists of dot-separated identifiers, and may begin
                    // with a dot.
                    if (looking_at_type(io::Tokenizer::TYPE_IDENTIFIER)) {
                        DO(consume_identifier(&identifier, "Expected identifier."));
                        name->mutable_name_part()->append(identifier);
                    }
                    while (looking_at(".")) {
                        DO(consume("."));
                        name->mutable_name_part()->append(".");
                        DO(consume_identifier(&identifier, "Expected identifier."));
                        name->mutable_name_part()->append(identifier);
                    }
                }

                DO(consume(")"));
                name->set_is_extension(true);
            } else { // This is a regular field.
                LocationRecorder location(
                    part_location, UninterpretedOption::NamePart::kNamePartFieldNumber);
                DO(consume_identifier(&identifier, "Expected identifier."));
                name->mutable_name_part()->append(identifier);
                name->set_is_extension(false);
            }
            return true;
        }

        bool Parser::parse_uninterpreted_block(std::string* value) {
            // Note that enclosing braces are not added to *value.
            // We do NOT use ConsumeEndOfStatement for this brace because it's delimiting
            // an expression, not a block of statements.
            DO(consume("{"));
            int brace_depth = 1;
            while (!at_end()) {
                if (looking_at("{")) {
                    brace_depth++;
                } else if (looking_at("}")) {
                    brace_depth--;
                    if (brace_depth == 0) {
                        input_->Next();
                        return true;
                    }
                }
                // TODO(sanjay): Interpret line/column numbers to preserve formatting
                if (!value->empty())
                    value->push_back(' ');
                value->append(input_->current().text);
                input_->Next();
            }
            add_error("Unexpected end of stream while parsing aggregate value.");
            return false;
        }

        // We don't interpret the option here. Instead we store it in an
        // UninterpretedOption, to be interpreted later.
        bool Parser::prse_option(Message* options,
            const LocationRecorder& options_location,
            const FileDescriptorProto* containing_file,
            OptionStyle style) {
            // Create an entry in the uninterpreted_option field.
            const FieldDescriptor* uninterpreted_option_field = options->GetDescriptor()->FindFieldByName("uninterpreted_option");
            SHARK_CHECK(uninterpreted_option_field != nullptr)
                << "No field named \"uninterpreted_option\" in the Options proto.";

            const Reflection* reflection = options->GetReflection();

            LocationRecorder location(
                options_location, uninterpreted_option_field->number(),
                reflection->FieldSize(*options, uninterpreted_option_field));

            if (style == OPTION_STATEMENT) {
                DO(consume("option"));
            }

            UninterpretedOption* uninterpreted_option = down_cast<UninterpretedOption*>(options->GetReflection()->AddMessage(
                options, uninterpreted_option_field));

            // Parse dot-separated name.
            {
                LocationRecorder name_location(location,
                    UninterpretedOption::kNameFieldNumber);
                name_location.RecordLegacyLocation(
                    uninterpreted_option, DescriptorPool::ErrorCollector::OPTION_NAME);

                {
                    LocationRecorder part_location(name_location,
                        uninterpreted_option->name_size());
                    DO(parse_option_name_part(uninterpreted_option, part_location,
                        containing_file));
                }

                while (looking_at(".")) {
                    DO(consume("."));
                    LocationRecorder part_location(name_location,
                        uninterpreted_option->name_size());
                    DO(parse_option_name_part(uninterpreted_option, part_location,
                        containing_file));
                }
            }

            DO(consume("="));

            {
                LocationRecorder value_location(location);
                value_location.RecordLegacyLocation(
                    uninterpreted_option, DescriptorPool::ErrorCollector::OPTION_VALUE);

                // All values are a single token, except for negative numbers, which consist
                // of a single '-' symbol, followed by a positive number.
                bool is_negative = try_consume("-");

                switch (input_->current().type) {
                case io::Tokenizer::TYPE_START:
                    SHARK_LOG(FATAL) << "Trying to read value before any tokens have been read.";
                    return false;

                case io::Tokenizer::TYPE_END:
                    add_error("Unexpected end of stream while parsing option value.");
                    return false;

                case io::Tokenizer::TYPE_WHITESPACE:
                case io::Tokenizer::TYPE_NEWLINE:
                    SHARK_CHECK(!input_->report_whitespace() && !input_->report_newlines())
                        << "Whitespace tokens were not requested.";
                    SHARK_LOG(FATAL) << "Tokenizer reported whitespace.";
                    return false;

                case io::Tokenizer::TYPE_IDENTIFIER: {
                    value_location.AddPath(
                        UninterpretedOption::kIdentifierValueFieldNumber);
                    if (is_negative) {
                        add_error("Invalid '-' symbol before identifier.");
                        return false;
                    }
                    std::string value;
                    DO(consume_identifier(&value, "Expected identifier."));
                    uninterpreted_option->set_identifier_value(value);
                    break;
                }

                case io::Tokenizer::TYPE_INTEGER: {
                    uint64_t value;
                    uint64_t max_value = is_negative
                        ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
                        : std::numeric_limits<uint64_t>::max();
                    DO(consume_integer64(max_value, &value, "Expected integer."));
                    if (is_negative) {
                        value_location.AddPath(
                            UninterpretedOption::kNegativeIntValueFieldNumber);
                        uninterpreted_option->set_negative_int_value(
                            static_cast<int64_t>(0 - value));
                    } else {
                        value_location.AddPath(
                            UninterpretedOption::kPositiveIntValueFieldNumber);
                        uninterpreted_option->set_positive_int_value(value);
                    }
                    break;
                }

                case io::Tokenizer::TYPE_FLOAT: {
                    value_location.AddPath(UninterpretedOption::kDoubleValueFieldNumber);
                    double value;
                    DO(consume_number(&value, "Expected number."));
                    uninterpreted_option->set_double_value(is_negative ? -value : value);
                    break;
                }

                case io::Tokenizer::TYPE_STRING: {
                    value_location.AddPath(UninterpretedOption::kStringValueFieldNumber);
                    if (is_negative) {
                        add_error("Invalid '-' symbol before string.");
                        return false;
                    }
                    std::string value;
                    DO(consume_string(&value, "Expected string."));
                    uninterpreted_option->set_string_value(value);
                    break;
                }

                case io::Tokenizer::TYPE_SYMBOL:
                    if (looking_at("{")) {
                        value_location.AddPath(
                            UninterpretedOption::kAggregateValueFieldNumber);
                        DO(parse_uninterpreted_block(
                            uninterpreted_option->mutable_aggregate_value()));
                    } else {
                        add_error("Expected option value.");
                        return false;
                    }
                    break;
                }
            }

            if (style == OPTION_STATEMENT) {
                DO(consume_end_of_declaration(";", &location));
            }

            return true;
        }

        bool Parser::ParseExtensions(DescriptorProto* message,
            const LocationRecorder& extensions_location,
            const FileDescriptorProto* containing_file) {
            // Parse the declaration.
            DO(consume("extensions"));

            int old_range_size = message->extension_range_size();

            do {
                // Note that kExtensionRangeFieldNumber was already pushed by the parent.
                LocationRecorder location(extensions_location,
                    message->extension_range_size());

                DescriptorProto::ExtensionRange* range = message->add_extension_range();
                location.RecordLegacyLocation(range,
                    DescriptorPool::ErrorCollector::NUMBER);

                int start, end;
                io::Tokenizer::Token start_token;

                {
                    LocationRecorder start_location(
                        location, DescriptorProto::ExtensionRange::kStartFieldNumber);
                    start_token = input_->current();
                    DO(consume_integer(&start, "Expected field number range."));
                }

                if (try_consume("to")) {
                    LocationRecorder end_location(
                        location, DescriptorProto::ExtensionRange::kEndFieldNumber);
                    if (try_consume("max")) {
                        // Set to the sentinel value - 1 since we increment the value below.
                        // The actual value of the end of the range should be set with
                        // AdjustExtensionRangesWithMaxEndNumber.
                        end = kMaxRangeSentinel - 1;
                    } else {
                        DO(consume_integer(&end, "Expected integer."));
                    }
                } else {
                    LocationRecorder end_location(
                        location, DescriptorProto::ExtensionRange::kEndFieldNumber);
                    end_location.StartAt(start_token);
                    end_location.EndAt(start_token);
                    end = start;
                }

                // Users like to specify inclusive ranges, but in code we like the end
                // number to be exclusive.
                ++end;

                range->set_start(start);
                range->set_end(end);
            } while (try_consume(","));

            if (looking_at("[")) {
                int range_number_index = extensions_location.CurrentPathSize();
                SourceCodeInfo info;

                // Parse extension range options in the first range.
                ExtensionRangeOptions* options = message->mutable_extension_range(old_range_size)->mutable_options();

                {
                    LocationRecorder index_location(
                        extensions_location, 0 /* we fill this in w/ actual index below */,
                        &info);
                    LocationRecorder location(
                        index_location, DescriptorProto::ExtensionRange::kOptionsFieldNumber);
                    DO(consume("["));

                    do {
                        DO(prse_option(options, location, containing_file, OPTION_ASSIGNMENT));
                    } while (try_consume(","));

                    DO(consume("]"));
                }

                // Then copy the extension range options to all of the other ranges we've
                // parsed.
                for (int i = old_range_size + 1; i < message->extension_range_size(); i++) {
                    message->mutable_extension_range(i)->mutable_options()->CopyFrom(
                        *options);
                }
                // and copy source locations to the other ranges, too
                for (int i = old_range_size; i < message->extension_range_size(); i++) {
                    for (int j = 0; j < info.location_size(); j++) {
                        if (info.location(j).path_size() == range_number_index + 1) {
                            // this location's path is up to the extension range index, but
                            // doesn't include options; so it's redundant with location above
                            continue;
                        }
                        SourceCodeInfo_Location* dest = source_code_info_->add_location();
                        *dest = info.location(j);
                        dest->set_path(range_number_index, i);
                    }
                }
            }

            DO(consume_end_of_declaration(";", &extensions_location));
            return true;
        }

        // This is similar to extension range parsing, except that it accepts field
        // name literals.
        bool Parser::ParseReserved(DescriptorProto* message,
            const LocationRecorder& message_location) {
            io::Tokenizer::Token start_token = input_->current();
            // Parse the declaration.
            DO(consume("reserved"));
            if (looking_at_type(io::Tokenizer::TYPE_STRING)) {
                LocationRecorder location(message_location,
                    DescriptorProto::kReservedNameFieldNumber);
                location.StartAt(start_token);
                return ParseReservedNames(message, location);
            } else {
                LocationRecorder location(message_location,
                    DescriptorProto::kReservedRangeFieldNumber);
                location.StartAt(start_token);
                return ParseReservedNumbers(message, location);
            }
        }

        bool Parser::ParseReservedNames(DescriptorProto* message,
            const LocationRecorder& parent_location) {
            do {
                LocationRecorder location(parent_location, message->reserved_name_size());
                DO(consume_string(message->add_reserved_name(), "Expected field name."));
            } while (try_consume(","));
            DO(consume_end_of_declaration(";", &parent_location));
            return true;
        }

        bool Parser::ParseReservedNumbers(DescriptorProto* message,
            const LocationRecorder& parent_location) {
            bool first = true;
            do {
                LocationRecorder location(parent_location, message->reserved_range_size());

                DescriptorProto::ReservedRange* range = message->add_reserved_range();
                int start, end;
                io::Tokenizer::Token start_token;
                {
                    LocationRecorder start_location(
                        location, DescriptorProto::ReservedRange::kStartFieldNumber);
                    start_token = input_->current();
                    DO(consume_integer(&start, (first ? "Expected field name or number range." : "Expected field number range.")));
                }

                if (try_consume("to")) {
                    LocationRecorder end_location(
                        location, DescriptorProto::ReservedRange::kEndFieldNumber);
                    if (try_consume("max")) {
                        // Set to the sentinel value - 1 since we increment the value below.
                        // The actual value of the end of the range should be set with
                        // AdjustExtensionRangesWithMaxEndNumber.
                        end = kMaxRangeSentinel - 1;
                    } else {
                        DO(consume_integer(&end, "Expected integer."));
                    }
                } else {
                    LocationRecorder end_location(
                        location, DescriptorProto::ReservedRange::kEndFieldNumber);
                    end_location.StartAt(start_token);
                    end_location.EndAt(start_token);
                    end = start;
                }

                // Users like to specify inclusive ranges, but in code we like the end
                // number to be exclusive.
                ++end;

                range->set_start(start);
                range->set_end(end);
                first = false;
            } while (try_consume(","));

            DO(consume_end_of_declaration(";", &parent_location));
            return true;
        }

        bool Parser::ParseReserved(EnumDescriptorProto* message,
            const LocationRecorder& message_location) {
            io::Tokenizer::Token start_token = input_->current();
            // Parse the declaration.
            DO(consume("reserved"));
            if (looking_at_type(io::Tokenizer::TYPE_STRING)) {
                LocationRecorder location(message_location,
                    EnumDescriptorProto::kReservedNameFieldNumber);
                location.StartAt(start_token);
                return ParseReservedNames(message, location);
            } else {
                LocationRecorder location(message_location,
                    EnumDescriptorProto::kReservedRangeFieldNumber);
                location.StartAt(start_token);
                return ParseReservedNumbers(message, location);
            }
        }

        bool Parser::ParseReservedNames(EnumDescriptorProto* message,
            const LocationRecorder& parent_location) {
            do {
                LocationRecorder location(parent_location, message->reserved_name_size());
                DO(consume_string(message->add_reserved_name(), "Expected enum value."));
            } while (try_consume(","));
            DO(consume_end_of_declaration(";", &parent_location));
            return true;
        }

        bool Parser::ParseReservedNumbers(EnumDescriptorProto* message,
            const LocationRecorder& parent_location) {
            bool first = true;
            do {
                LocationRecorder location(parent_location, message->reserved_range_size());

                EnumDescriptorProto::EnumReservedRange* range = message->add_reserved_range();
                int start, end;
                io::Tokenizer::Token start_token;
                {
                    LocationRecorder start_location(
                        location, EnumDescriptorProto::EnumReservedRange::kStartFieldNumber);
                    start_token = input_->current();
                    DO(consume_signed_integer(&start,
                        (first ? "Expected enum value or number range."
                               : "Expected enum number range.")));
                }

                if (try_consume("to")) {
                    LocationRecorder end_location(
                        location, EnumDescriptorProto::EnumReservedRange::kEndFieldNumber);
                    if (try_consume("max")) {
                        // This is in the enum descriptor path, which doesn't have the message
                        // set duality to fix up, so it doesn't integrate with the sentinel.
                        end = INT_MAX;
                    } else {
                        DO(consume_signed_integer(&end, "Expected integer."));
                    }
                } else {
                    LocationRecorder end_location(
                        location, EnumDescriptorProto::EnumReservedRange::kEndFieldNumber);
                    end_location.StartAt(start_token);
                    end_location.EndAt(start_token);
                    end = start;
                }

                range->set_start(start);
                range->set_end(end);
                first = false;
            } while (try_consume(","));

            DO(consume_end_of_declaration(";", &parent_location));
            return true;
        }

        bool Parser::parse_extend(RepeatedPtrField<FieldDescriptorProto>* extensions,
            RepeatedPtrField<DescriptorProto>* messages,
            const LocationRecorder& parent_location,
            int location_field_number_for_nested_type,
            const LocationRecorder& extend_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("extend"));

            // Parse the extendee type.
            io::Tokenizer::Token extendee_start = input_->current();
            std::string extendee;
            DO(parse_user_defined_type(&extendee));
            io::Tokenizer::Token extendee_end = input_->previous();

            // Parse the block.
            DO(consume_end_of_declaration("{", &extend_location));

            bool is_first = true;

            do {
                if (at_end()) {
                    add_error("Reached end of input in extend definition (missing '}').");
                    return false;
                }

                // Note that kExtensionFieldNumber was already pushed by the parent.
                LocationRecorder location(extend_location, extensions->size());

                FieldDescriptorProto* field = extensions->Add();

                {
                    LocationRecorder extendee_location(
                        location, FieldDescriptorProto::kExtendeeFieldNumber);
                    extendee_location.StartAt(extendee_start);
                    extendee_location.EndAt(extendee_end);

                    if (is_first) {
                        extendee_location.RecordLegacyLocation(
                            field, DescriptorPool::ErrorCollector::EXTENDEE);
                        is_first = false;
                    }
                }

                field->set_extendee(extendee);

                if (!ParseMessageField(field, messages, parent_location,
                        location_field_number_for_nested_type, location,
                        containing_file)) {
                    // This statement failed to parse.  Skip it, but keep looping to parse
                    // other statements.
                    SkipStatement();
                }
            } while (!try_consume_end_of_declaration("}", nullptr));

            return true;
        }

        bool Parser::parse_oneof(OneofDescriptorProto* oneof_decl,
            DescriptorProto* containing_type, int oneof_index,
            const LocationRecorder& oneof_location,
            const LocationRecorder& containing_type_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("union"));

            {
                LocationRecorder name_location(oneof_location,
                    OneofDescriptorProto::kNameFieldNumber);
                DO(consume_identifier(oneof_decl->mutable_name(), "Expected union name."));
            }

            DO(consume_end_of_declaration("{", &oneof_location));

            do {
                if (at_end()) {
                    add_error("Reached end of input in union definition (missing '}').");
                    return false;
                }

                if (looking_at("option")) {
                    LocationRecorder option_location(
                        oneof_location, OneofDescriptorProto::kOptionsFieldNumber);
                    if (!prse_option(oneof_decl->mutable_options(), option_location,
                            containing_file, OPTION_STATEMENT)) {
                        return false;
                    }
                    continue;
                }

                // Print a nice error if the user accidentally tries to place a label
                // on an individual member of a oneof.
                if (looking_at("required") || looking_at("optional")) {
                    add_error(
                        "Fields in unions must not have labels (required / optional).");
                    // We can continue parsing here because we understand what the user
                    // meant.  The error report will still make parsing fail overall.
                    input_->Next();
                }

                LocationRecorder field_location(containing_type_location,
                    DescriptorProto::kFieldFieldNumber,
                    containing_type->field_size());

                FieldDescriptorProto* field = containing_type->add_field();
                field->set_label(FieldCardinality::OPTIONAL);
                field->set_oneof_index(oneof_index);

                if (!ParseMessageFieldNoLabel(field, containing_type->mutable_nested_type(),
                        containing_type_location,
                        DescriptorProto::kNestedTypeFieldNumber,
                        field_location, containing_file)) {
                    // This statement failed to parse.  Skip it, but keep looping to parse
                    // other statements.
                    SkipStatement();
                }
            } while (!try_consume_end_of_declaration("}", nullptr));

            return true;
        }

        // -------------------------------------------------------------------
        // Enums

        bool EnumValueFits(EnumUnderlyingType type, int32_t number) {
            switch (type) {
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT8:
                return number >= -128 && number <= 127;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8:
                return number >= 0 && number <= 255;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT16:
                return number >= -32768 && number <= 32767;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16:
                return number >= 0 && number <= 65535;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32:
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32:
                return true;
            default:
                return true;
            }
        }

        bool ParseEnumUnderlyingType(const std::string& name, EnumUnderlyingType* type) {
            if (name == "int8") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT8;
            } else if (name == "uint8") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8;
            } else if (name == "int16") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT16;
            } else if (name == "uint16") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16;
            } else if (name == "int32") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32;
            } else if (name == "uint32") {
                *type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32;
            } else {
                return false;
            }
            return true;
        }

        bool Parser::ParseEnumDefinition(EnumDescriptorProto* enum_type,
            const LocationRecorder& enum_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("enum"));
            // "enum class" is sugar: plain enum and enum class are equivalent and
            // always generate a C++ enum class.
            try_consume("class");

            {
                LocationRecorder location(enum_location,
                    EnumDescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(enum_type,
                    DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(enum_type->mutable_name(), "Expected enum name."));
            }

            // "enum" and "enum class" are equivalent and always record an
            // underlying_type (defaulting to int32) so both spellings produce
            // byte-identical descriptors.
            EnumUnderlyingType underlying_type = EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32;
            if (try_consume(":")) {
                std::string type_name;
                DO(consume_identifier(&type_name, "Expected enum underlying type."));
                if (!ParseEnumUnderlyingType(type_name, &underlying_type)) {
                    add_error("Unknown enum underlying type \"" + type_name + "\".");
                    return false;
                }
            }

            DO(ParseEnumBlock(enum_type, enum_location, containing_file, underlying_type));

            enum_type->mutable_options()->set_underlying_type(underlying_type);

            DO(validate_enum(enum_type));

            return true;
        }

        bool Parser::ParseEnumBlock(EnumDescriptorProto* enum_type,
            const LocationRecorder& enum_location,
            const FileDescriptorProto* containing_file,
            EnumUnderlyingType underlying_type) {
            DO(consume_end_of_declaration("{", &enum_location));

            while (!try_consume_end_of_declaration("}", nullptr)) {
                if (at_end()) {
                    add_error("Reached end of input in enum definition (missing '}').");
                    return false;
                }

                if (!ParseEnumStatement(enum_type, enum_location, containing_file,
                        underlying_type)) {
                    // This statement failed to parse.  Skip it, but keep looping to parse
                    // other statements.
                    SkipStatement();
                }
            }

            return true;
        }

        bool Parser::ParseEnumStatement(EnumDescriptorProto* enum_type,
            const LocationRecorder& enum_location,
            const FileDescriptorProto* containing_file,
            EnumUnderlyingType underlying_type) {
            if (try_consume_end_of_declaration(";", nullptr)) {
                // empty statement; ignore
                return true;
            } else if (looking_at("option")) {
                LocationRecorder location(enum_location,
                    EnumDescriptorProto::kOptionsFieldNumber);
                return prse_option(enum_type->mutable_options(), location, containing_file,
                    OPTION_STATEMENT);
            } else if (looking_at("reserved")) {
                return ParseReserved(enum_type, enum_location);
            } else {
                LocationRecorder location(enum_location,
                    EnumDescriptorProto::kValueFieldNumber,
                    enum_type->value_size());
                return parse_enum_constant(enum_type->add_value(), location, containing_file,
                    underlying_type);
            }
        }

        bool Parser::consume_enum_number(EnumUnderlyingType underlying_type,
            int32_t* output, const char* error) {
            if (underlying_type == EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UNSPECIFIED || underlying_type == EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32) {
                return consume_signed_integer(output, error);
            }

            const bool is_unsigned = underlying_type == EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8 || underlying_type == EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16 || underlying_type == EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32;
            const bool is_negative = try_consume("-");
            if (is_negative && is_unsigned) {
                add_error("Unsigned enum underlying type cannot be negative.");
                return false;
            }

            uint64_t max_value = 0;
            switch (underlying_type) {
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT8:
                max_value = is_negative ? 128 : 127;
                break;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8:
                max_value = 255;
                break;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT16:
                max_value = is_negative ? 32768 : 32767;
                break;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16:
                max_value = 65535;
                break;
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32:
                max_value = std::numeric_limits<uint32_t>::max();
                break;
            default:
                max_value = std::numeric_limits<int32_t>::max();
                break;
            }

            uint64_t value = 0;
            DO(consume_integer64(max_value, &value, error));
            if (is_negative) {
                *output = -static_cast<int32_t>(value);
            } else if (value > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
                *output = static_cast<int32_t>(static_cast<uint32_t>(value));
            } else {
                *output = static_cast<int32_t>(value);
            }
            return true;
        }

        bool Parser::parse_enum_constant(EnumValueDescriptorProto* enum_value,
            const LocationRecorder& enum_value_location,
            const FileDescriptorProto* containing_file,
            EnumUnderlyingType underlying_type) {
            // Parse name.
            {
                LocationRecorder location(enum_value_location,
                    EnumValueDescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(enum_value,
                    DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(enum_value->mutable_name(),
                    "Expected enum constant name."));
            }

            DO(consume("=", "Missing numeric value for enum constant."));

            // Parse value.
            {
                LocationRecorder location(enum_value_location,
                    EnumValueDescriptorProto::kNumberFieldNumber);
                location.RecordLegacyLocation(enum_value,
                    DescriptorPool::ErrorCollector::NUMBER);

                int32_t number;
                DO(consume_enum_number(underlying_type, &number, "Expected integer."));
                enum_value->set_number(number);
            }

            DO(ParseEnumConstantOptions(enum_value, enum_value_location,
                containing_file));

            DO(consume_end_of_declaration(";", &enum_value_location));

            return true;
        }

        bool Parser::ParseEnumConstantOptions(
            EnumValueDescriptorProto* value,
            const LocationRecorder& enum_value_location,
            const FileDescriptorProto* containing_file) {
            if (!looking_at("["))
                return true;

            LocationRecorder location(enum_value_location,
                EnumValueDescriptorProto::kOptionsFieldNumber);

            DO(consume("["));

            do {
                DO(prse_option(value->mutable_options(), location, containing_file,
                    OPTION_ASSIGNMENT));
            } while (try_consume(","));

            DO(consume("]"));
            return true;
        }

        // -------------------------------------------------------------------
        // Services

        bool Parser::parse_servlet_definition(
            ServletDescriptorProto* servlet, const LocationRecorder& servlet_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("servlet"));

            {
                LocationRecorder location(servlet_location,
                    ServletDescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(servlet,
                    DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(servlet->mutable_name(), "Expected servlet name."));
            }

            DO(ParseServletBlock(servlet, servlet_location, containing_file));
            return true;
        }

        bool Parser::ParseServletBlock(ServletDescriptorProto* servlet,
            const LocationRecorder& servlet_location,
            const FileDescriptorProto* containing_file) {
            DO(consume_end_of_declaration("{", &servlet_location));

            while (!try_consume_end_of_declaration("}", nullptr)) {
                if (at_end()) {
                    add_error("Reached end of input in servlet definition (missing '}').");
                    return false;
                }

                if (!ParseServletStatement(servlet, servlet_location, containing_file)) {
                    // This statement failed to parse.  Skip it, but keep looping to parse
                    // other statements.
                    SkipStatement();
                }
            }

            return true;
        }

        bool Parser::ParseServletStatement(ServletDescriptorProto* servlet,
            const LocationRecorder& servlet_location,
            const FileDescriptorProto* containing_file) {
            if (try_consume_end_of_declaration(";", nullptr)) {
                // empty statement; ignore
                return true;
            } else if (looking_at("option")) {
                LocationRecorder location(servlet_location,
                    ServletDescriptorProto::kOptionsFieldNumber);
                return prse_option(servlet->mutable_options(), location, containing_file,
                    OPTION_STATEMENT);
            } else {
                LocationRecorder location(servlet_location,
                    ServletDescriptorProto::kMethodFieldNumber,
                    servlet->method_size());
                return parse_servlet_method(servlet->add_method(), location, containing_file);
            }
        }

        bool Parser::parse_servlet_method(MethodDescriptorProto* method,
            const LocationRecorder& method_location,
            const FileDescriptorProto* containing_file) {
            DO(consume("rpc"));

            {
                LocationRecorder location(method_location,
                    MethodDescriptorProto::kNameFieldNumber);
                location.RecordLegacyLocation(method, DescriptorPool::ErrorCollector::NAME);
                DO(consume_identifier(method->mutable_name(), "Expected method name."));
            }

            // Parse input type.
            DO(consume("("));
            {
                if (looking_at("stream")) {
                    LocationRecorder location(
                        method_location, MethodDescriptorProto::kClientStreamingFieldNumber);
                    location.RecordLegacyLocation(method,
                        DescriptorPool::ErrorCollector::OTHER);
                    method->set_client_streaming(true);
                    DO(consume("stream"));
                }
                LocationRecorder location(method_location,
                    MethodDescriptorProto::kInputTypeFieldNumber);
                location.RecordLegacyLocation(method,
                    DescriptorPool::ErrorCollector::INPUT_TYPE);
                DO(parse_user_defined_type(method->mutable_input_type()));
            }
            DO(consume(")"));

            // Parse output type.
            DO(consume("returns"));
            DO(consume("("));
            {
                if (looking_at("stream")) {
                    LocationRecorder location(
                        method_location, MethodDescriptorProto::kServerStreamingFieldNumber);
                    location.RecordLegacyLocation(method,
                        DescriptorPool::ErrorCollector::OTHER);
                    DO(consume("stream"));
                    method->set_server_streaming(true);
                }
                LocationRecorder location(method_location,
                    MethodDescriptorProto::kOutputTypeFieldNumber);
                location.RecordLegacyLocation(method,
                    DescriptorPool::ErrorCollector::OUTPUT_TYPE);
                DO(parse_user_defined_type(method->mutable_output_type()));
            }
            DO(consume(")"));

            if (looking_at("{")) {
                // Options!
                DO(ParseMethodOptions(method_location, containing_file,
                    MethodDescriptorProto::kOptionsFieldNumber,
                    method->mutable_options()));
            } else {
                DO(consume_end_of_declaration(";", &method_location));
            }

            return true;
        }

        bool Parser::ParseMethodOptions(const LocationRecorder& parent_location,
            const FileDescriptorProto* containing_file,
            const int optionsFieldNumber,
            Message* mutable_options) {
            // Options!
            consume_end_of_declaration("{", &parent_location);
            while (!try_consume_end_of_declaration("}", nullptr)) {
                if (at_end()) {
                    add_error("Reached end of input in method options (missing '}').");
                    return false;
                }

                if (try_consume_end_of_declaration(";", nullptr)) {
                    // empty statement; ignore
                } else {
                    LocationRecorder location(parent_location, optionsFieldNumber);
                    if (!prse_option(mutable_options, location, containing_file,
                            OPTION_STATEMENT)) {
                        // This statement failed to parse.  Skip it, but keep looping to
                        // parse other statements.
                        SkipStatement();
                    }
                }
            }

            return true;
        }

        // -------------------------------------------------------------------

        bool Parser::parse_label(FieldCardinality* label,
            const LocationRecorder& field_location) {
            if (!looking_at("optional") && !looking_at("required")) {
                return false;
            }
            LocationRecorder location(field_location,
                FieldDescriptorProto::kLabelFieldNumber);
            if (try_consume("optional")) {
                *label = FieldCardinality::OPTIONAL;
            } else {
                consume("required");
                *label = FieldCardinality::REQUIRED;
            }
            return true;
        }

        bool Parser::parse_type(SharkType* type,
            std::string* type_name) {
            const std::string& text = input_->current().text;
            if (text == "float16" || text == "float8") {
                add_error("\"" + text + "\" is not supported.");
                return false;
            }
            const auto& type_names_table = GetTypeNameTable();
            auto iter = type_names_table.find(text);
            if (iter != type_names_table.end()) {
                *type = iter->second;
                input_->Next();
            } else {
                DO(parse_user_defined_type(type_name));
            }
            return true;
        }

        bool Parser::parse_user_defined_type(std::string* type_name) {
            type_name->clear();

            const auto& type_names_table = GetTypeNameTable();
            auto iter = type_names_table.find(input_->current().text);
            if (iter != type_names_table.end()) {
                // Note:  The only place enum types are allowed is for field types, but
                //   if we are parsing a field type then we would not get here because
                //   primitives are allowed there as well.  So this error message doesn't
                //   need to account for enums.
                add_error("Expected message type.");

                // Pretend to accept this type so that we can go on parsing.
                *type_name = input_->current().text;
                input_->Next();
                return true;
            }

            // A leading "." means the name is fully-qualified.
            if (try_consume("."))
                type_name->append(".");

            // consume the first part of the name.
            std::string identifier;
            DO(consume_identifier(&identifier, "Expected type name."));
            type_name->append(identifier);

            // consume more parts.
            while (try_consume(".")) {
                type_name->append(".");
                DO(consume_identifier(&identifier, "Expected identifier."));
                type_name->append(identifier);
            }

            return true;
        }

        // ===================================================================

        bool Parser::ParsePackage(FileDescriptorProto* file,
            const LocationRecorder& root_location,
            const FileDescriptorProto* containing_file) {
            if (file->has_package()) {
                add_error("Multiple package definitions.");
                // Don't append the new package to the old one.  Just replace it.  Not
                // that it really matters since this is an error anyway.
                file->clear_package();
            }

            LocationRecorder location(root_location,
                FileDescriptorProto::kPackageFieldNumber);
            location.RecordLegacyLocation(file, DescriptorPool::ErrorCollector::NAME);

            DO(consume("package"));

            while (true) {
                std::string identifier;
                DO(consume_identifier(&identifier, "Expected identifier."));
                file->mutable_package()->append(identifier);
                if (!try_consume("."))
                    break;
                file->mutable_package()->append(".");
            }

            DO(consume_end_of_declaration(";", &location));

            return true;
        }

        bool Parser::ParseImport(RepeatedPtrField<std::string>* dependency,
            RepeatedField<int32_t>* public_dependency,
            RepeatedField<int32_t>* weak_dependency,
            const LocationRecorder& root_location,
            const FileDescriptorProto* containing_file) {
            LocationRecorder location(root_location,
                FileDescriptorProto::kDependencyFieldNumber,
                dependency->size());

            DO(consume("import"));

            if (looking_at("public")) {
                LocationRecorder public_location(
                    root_location, FileDescriptorProto::kPublicDependencyFieldNumber,
                    public_dependency->size());
                DO(consume("public"));
                *public_dependency->Add() = dependency->size();
            } else if (looking_at("weak")) {
                LocationRecorder weak_location(
                    root_location, FileDescriptorProto::kWeakDependencyFieldNumber,
                    weak_dependency->size());
                weak_location.RecordLegacyImportLocation(containing_file, "weak");
                DO(consume("weak"));
                *weak_dependency->Add() = dependency->size();
            }

            std::string import_file;
            DO(consume_string(&import_file,
                "Expected a string naming the file to import."));
            *dependency->Add() = import_file;
            location.RecordLegacyImportLocation(containing_file, import_file);

            DO(consume_end_of_declaration(";", &location));

            return true;
        }

        // ===================================================================

        SourceLocationTable::SourceLocationTable() { }
        SourceLocationTable::~SourceLocationTable() { }

        bool SourceLocationTable::Find(
            const Message* descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location, int* line,
            int* column) const {
            const std::pair<int, int>* result = FindOrNull(location_map_, std::make_pair(descriptor, location));
            if (result == nullptr) {
                *line = -1;
                *column = 0;
                return false;
            } else {
                *line = result->first;
                *column = result->second;
                return true;
            }
        }

        bool SourceLocationTable::FindImport(const Message* descriptor,
            const std::string& name, int* line,
            int* column) const {
            const std::pair<int, int>* result = FindOrNull(import_location_map_, std::make_pair(descriptor, name));
            if (result == nullptr) {
                *line = -1;
                *column = 0;
                return false;
            } else {
                *line = result->first;
                *column = result->second;
                return true;
            }
        }

        void SourceLocationTable::Add(
            const Message* descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location, int line,
            int column) {
            location_map_[std::make_pair(descriptor, location)] = std::make_pair(line, column);
        }

        void SourceLocationTable::AddImport(const Message* descriptor,
            const std::string& name, int line,
            int column) {
            import_location_map_[std::make_pair(descriptor, name)] = std::make_pair(line, column);
        }

        void SourceLocationTable::Clear() {
            location_map_.clear();
        }

    } // namespace compiler
} // namespace shark
