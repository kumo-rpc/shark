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
// Shared test fixtures for .shark parser tests: ParserTest (parse →
// FileDescriptorProto comparison, error and validation helpers) and
// SourceInfoTest (SourceCodeInfo span checking). Used by
// parser_unittest.cc and parser_enum_test.cc.

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <shark/parser/parser.h>
#include <shark/descriptor.h>
#include <shark/descriptor.skp.h>
#include <shark/io/tokenizer.h>
#include <shark/io/zero_copy_stream_impl.h>
#include <shark/types/repeated_field.h>
#include <shark/stubs/map_util.h>
#include <shark/stubs/strutil.h>
#include <shark/stubs/substitute.h>
#include <shark/format/text/text_format.h>

namespace shark {
namespace compiler {

class MockErrorCollector : public io::ErrorCollector {
 public:
  MockErrorCollector() = default;
  ~MockErrorCollector() override = default;

  std::string warning_;
  std::string text_;
  // Subtracted from reported lines when the fixture inserted a syntax line.
  int line_offset = 0;

  // implements ErrorCollector ---------------------------------------
  void AddWarning(int line, int column, const std::string& message) override {
    strings::SubstituteAndAppend(&warning_, "$0:$1: $2\n", line - line_offset,
                                 column, message);
  }

  void AddError(int line, int column, const std::string& message) override {
    strings::SubstituteAndAppend(&text_, "$0:$1: $2\n", line - line_offset,
                                 column, message);
  }
};

class MockValidationErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  MockValidationErrorCollector(const SourceLocationTable& source_locations,
                               io::ErrorCollector* wrapped_collector)
      : source_locations_(source_locations),
        wrapped_collector_(wrapped_collector) {}
  ~MockValidationErrorCollector() override {}

  // implements ErrorCollector ---------------------------------------
  void AddError(const std::string& filename, const std::string& element_name,
                const Message* descriptor, ErrorLocation location,
                const std::string& message) override {
    int line, column;
    if (location == DescriptorPool::ErrorCollector::IMPORT) {
      source_locations_.FindImport(descriptor, element_name, &line, &column);
    } else {
      source_locations_.Find(descriptor, location, &line, &column);
    }
    wrapped_collector_->AddError(line, column, message);
  }

 private:
  const SourceLocationTable& source_locations_;
  io::ErrorCollector* wrapped_collector_;
};

inline bool DeclaresSyntax(const char* text) {
  const char* p = text;
  while (*p != '\0') {
    if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
      ++p;
      continue;
    }
    if (*p == '/' && p[1] == '/') {
      while (*p != '\0' && *p != '\n') ++p;
      continue;
    }
    if (*p == '/' && p[1] == '*') {
      p += 2;
      while (*p != '\0' && !(*p == '*' && p[1] == '/')) ++p;
      if (*p != '\0') p += 2;
      continue;
    }
    if (static_cast<unsigned char>(*p) == 0xEF) {
      p += 3;
      continue;
    }
    if (*p == '$') {
      ++p;
      if (*p == '$') {
        ++p;
        continue;
      }
      if (*p != '\0') ++p;
      if (*p == '$') ++p;
      continue;
    }
    break;
  }
  return strncmp(p, "syntax", 6) == 0;
}

class ParserTest : public testing::Test {
 protected:
  ParserTest() : require_syntax_identifier_(false), supply_syntax_(true) {}

  // Set up the parser to parse the given text.
  void SetupParser(const char* text) {
    error_collector_.text_.clear();
    error_collector_.warning_.clear();
    error_collector_.line_offset = 0;
    const char* parse_text = text;
    if (supply_syntax_ && !DeclaresSyntax(text)) {
      supplied_text_ = std::string("syntax = \"proto2\";\n") + text;
      parse_text = supplied_text_.c_str();
      error_collector_.line_offset = 1;
    } else {
      supplied_text_.clear();
    }
    raw_input_.reset(
        new io::ArrayInputStream(parse_text, static_cast<int>(strlen(parse_text))));
    input_.reset(new io::Tokenizer(raw_input_.get(), &error_collector_));
    parser_.reset(new Parser());
    parser_->RecordErrorsTo(&error_collector_);
    parser_->SetRequireSyntaxIdentifier(require_syntax_identifier_);
  }

  // Parse the input and expect that the resulting FileDescriptorProto matches
  // the given output.  The output is a FileDescriptorProto in protocol buffer
  // text format.
  void ExpectParsesTo(const char* input, const char* output) {
    SetupParser(input);
    FileDescriptorProto actual, expected;

    parser_->Parse(input_.get(), &actual);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
    ASSERT_EQ("", error_collector_.text_);

    // We don't cover SourceCodeInfo in these tests.
    actual.clear_source_code_info();
    if (error_collector_.line_offset != 0) {
      actual.clear_syntax();
    }

    // Parse the ASCII representation in order to canonicalize it.  We could
    // just compare directly to actual.DebugString(), but that would require
    // that the caller precisely match the formatting that DebugString()
    // produces.
    ASSERT_TRUE(TextFormat::ParseFromString(output, &expected));

    // Compare by comparing debug strings.
    // TODO(kenton):  Use differencer, once it is available.
    EXPECT_EQ(expected.DebugString(), actual.DebugString());
  }

  // Parse the text and expect that the given errors are reported.
  void ExpectHasErrors(const char* text, const char* expected_errors) {
    ExpectHasEarlyExitErrors(text, expected_errors);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
  }

  // Same as above but does not expect that the parser parses the complete
  // input.
  void ExpectHasEarlyExitErrors(const char* text, const char* expected_errors) {
    SetupParser(text);
    FileDescriptorProto file;
    parser_->Parse(input_.get(), &file);
    EXPECT_EQ(expected_errors, error_collector_.text_);
  }

  // Parse the text as a file and validate it (with a DescriptorPool), and
  // expect that the validation step reports the given errors.
  void ExpectHasValidationErrors(const char* text,
                                 const char* expected_errors) {
    SetupParser(text);
    SourceLocationTable source_locations;
    parser_->RecordSourceLocationsTo(&source_locations);

    FileDescriptorProto file;
    file.set_name("foo.shark");
    parser_->Parse(input_.get(), &file);
    EXPECT_EQ(io::Tokenizer::TYPE_END, input_->current().type);
    ASSERT_EQ("", error_collector_.text_);

    MockValidationErrorCollector validation_error_collector(source_locations,
                                                            &error_collector_);
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(
                    file, &validation_error_collector) == nullptr);
    EXPECT_EQ(expected_errors, error_collector_.text_);
  }

  MockErrorCollector error_collector_;
  DescriptorPool pool_;

  std::unique_ptr<io::ZeroCopyInputStream> raw_input_;
  std::unique_ptr<io::Tokenizer> input_;
  std::unique_ptr<Parser> parser_;
  bool require_syntax_identifier_;
  bool supply_syntax_;
  std::string supplied_text_;
};

// ===================================================================
// SourceCodeInfo tests.

// Follows a path -- as defined by SourceCodeInfo.Location.path -- from a
// message to a particular sub-field.
// * If the target is itself a message, sets *output_message to point at it,
//   *output_field to NULL, and *output_index to -1.
// * Otherwise, if the target is an element of a repeated field, sets
//   *output_message to the containing message, *output_field to the descriptor
//   of the field, and *output_index to the index of the element.
// * Otherwise, the target is a field (possibly a repeated field, but not any
//   one element).  Sets *output_message to the containing message,
//   *output_field to the descriptor of the field, and *output_index to -1.
// Returns true if the path was valid, false otherwise.  A gTest failure is
// recorded before returning false.
inline bool FollowPath(const Message& root,
                RepeatedField<int>::const_iterator path_begin,
                RepeatedField<int>::const_iterator path_end,
                const Message** output_message,
                const FieldDescriptor** output_field, int* output_index) {
  if (path_begin == path_end) {
    // Path refers to this whole message.
    *output_message = &root;
    *output_field = nullptr;
    *output_index = -1;
    return true;
  }

  const Descriptor* descriptor = root.GetDescriptor();
  const Reflection* reflection = root.GetReflection();

  const FieldDescriptor* field = descriptor->FindFieldByNumber(*path_begin);

  if (field == nullptr) {
    ADD_FAILURE() << descriptor->name()
                  << " has no field number: " << *path_begin;
    return false;
  }

  ++path_begin;

  if (field->is_repeated()) {
    if (path_begin == path_end) {
      // Path refers to the whole repeated field.
      *output_message = &root;
      *output_field = field;
      *output_index = -1;
      return true;
    }

    int index = *path_begin++;
    int size = reflection->FieldSize(root, field);

    if (index >= size) {
      ADD_FAILURE() << descriptor->name() << "." << field->name()
                    << " has size " << size
                    << ", but path contained index: " << index;
      return false;
    }

    if (field->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE) {
      // Descend into child message.
      const Message& child = reflection->GetRepeatedMessage(root, field, index);
      return FollowPath(child, path_begin, path_end, output_message,
                        output_field, output_index);
    } else if (path_begin == path_end) {
      // Path refers to this element.
      *output_message = &root;
      *output_field = field;
      *output_index = index;
      return true;
    } else {
      ADD_FAILURE() << descriptor->name() << "." << field->name()
                    << " is not a message; cannot descend into it.";
      return false;
    }
  } else {
    if (field->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE) {
      const Message& child = reflection->GetMessage(root, field);
      return FollowPath(child, path_begin, path_end, output_message,
                        output_field, output_index);
    } else if (path_begin == path_end) {
      // Path refers to this field.
      *output_message = &root;
      *output_field = field;
      *output_index = -1;
      return true;
    } else {
      ADD_FAILURE() << descriptor->name() << "." << field->name()
                    << " is not a message; cannot descend into it.";
      return false;
    }
  }
}

// Check if two spans are equal.
inline bool CompareSpans(const RepeatedField<int>& span1,
                  const RepeatedField<int>& span2) {
  if (span1.size() != span2.size()) return false;
  for (int i = 0; i < span1.size(); i++) {
    if (span1.Get(i) != span2.Get(i)) return false;
  }
  return true;
}

// Test fixture for source info tests, which check that source locations are
// recorded correctly in FileDescriptorProto.source_code_info.location.
class SourceInfoTest : public ParserTest {
 protected:
  // The parsed file (initialized by Parse()).
  FileDescriptorProto file_;

  // Parse the given text as a .shark file and populate the spans_ map with
  // all the source location spans in its SourceCodeInfo table.
  bool Parse(const char* text) {
    std::string with_syntax;
    const char* marked = text;
    if (!DeclaresSyntax(text)) {
      with_syntax = std::string("syntax = \"proto2\";\n\n") + text;
      marked = with_syntax.c_str();
    }
    ExtractMarkers(marked);
    supply_syntax_ = false;
    SetupParser(text_without_markers_.c_str());
    supply_syntax_ = true;
    if (!parser_->Parse(input_.get(), &file_)) {
      return false;
    }

    const SourceCodeInfo& source_info = file_.source_code_info();
    for (int i = 0; i < source_info.location_size(); i++) {
      const SourceCodeInfo::Location& location = source_info.location(i);
      if (!with_syntax.empty() && location.path_size() == 1 &&
          location.path(0) == FileDescriptorProto::kSyntaxFieldNumber) {
        continue;
      }
      const Message* descriptor_proto = nullptr;
      const FieldDescriptor* field = nullptr;
      int index = 0;
      if (!FollowPath(file_, location.path().begin(), location.path().end(),
                      &descriptor_proto, &field, &index)) {
        return false;
      }

      spans_.insert(
          std::make_pair(SpanKey(*descriptor_proto, field, index), &location));
    }

    return true;
  }

  void TearDown() override {
    EXPECT_TRUE(spans_.empty()) << "Forgot to call HasSpan() for:\n"
                                << spans_.begin()->second->DebugString();
  }

  // -----------------------------------------------------------------
  // HasSpan() checks that the span of source code delimited by the given
  // tags (comments) correspond via the SourceCodeInfo table to the given
  // part of the FileDescriptorProto.  (If unclear, look at the actual tests;
  // it should quickly become obvious.)

  bool HasSpan(char start_marker, char end_marker,
               const Message& descriptor_proto) {
    return HasSpanWithComment(start_marker, end_marker, descriptor_proto,
                              nullptr, -1, nullptr, nullptr, nullptr);
  }

  bool HasSpanWithComment(char start_marker, char end_marker,
                          const Message& descriptor_proto,
                          const char* expected_leading_comments,
                          const char* expected_trailing_comments,
                          const char* expected_leading_detached_comments) {
    return HasSpanWithComment(start_marker, end_marker, descriptor_proto,
                              nullptr, -1, expected_leading_comments,
                              expected_trailing_comments,
                              expected_leading_detached_comments);
  }

  bool HasSpan(char start_marker, char end_marker,
               const Message& descriptor_proto, const std::string& field_name) {
    return HasSpan(start_marker, end_marker, descriptor_proto, field_name, -1);
  }

  bool HasSpan(char start_marker, char end_marker,
               const Message& descriptor_proto, const std::string& field_name,
               int index) {
    return HasSpan(start_marker, end_marker, descriptor_proto, field_name,
                   index, nullptr, nullptr, nullptr);
  }

  bool HasSpan(char start_marker, char end_marker,
               const Message& descriptor_proto, const std::string& field_name,
               int index, const char* expected_leading_comments,
               const char* expected_trailing_comments,
               const char* expected_leading_detached_comments) {
    const FieldDescriptor* field =
        descriptor_proto.GetDescriptor()->FindFieldByName(field_name);
    if (field == nullptr) {
      ADD_FAILURE() << descriptor_proto.GetDescriptor()->name()
                    << " has no such field: " << field_name;
      return false;
    }

    return HasSpanWithComment(start_marker, end_marker, descriptor_proto, field,
                              index, expected_leading_comments,
                              expected_trailing_comments,
                              expected_leading_detached_comments);
  }

  bool HasSpan(const Message& descriptor_proto) {
    return HasSpanWithComment('\0', '\0', descriptor_proto, nullptr, -1,
                              nullptr, nullptr, nullptr);
  }

  bool HasSpan(const Message& descriptor_proto, const std::string& field_name) {
    return HasSpan('\0', '\0', descriptor_proto, field_name, -1);
  }

  bool HasSpanWithComment(char start_marker, char end_marker,
                          const Message& descriptor_proto,
                          const FieldDescriptor* field, int index,
                          const char* expected_leading_comments,
                          const char* expected_trailing_comments,
                          const char* expected_leading_detached_comments) {
    std::pair<SpanMap::iterator, SpanMap::iterator> range =
        spans_.equal_range(SpanKey(descriptor_proto, field, index));

    if (start_marker == '\0') {
      if (range.first == range.second) {
        return false;
      } else {
        spans_.erase(range.first);
        return true;
      }
    } else {
      std::pair<int, int> start_pos = FindOrDie(markers_, start_marker);
      std::pair<int, int> end_pos = FindOrDie(markers_, end_marker);

      RepeatedField<int> expected_span;
      expected_span.Add(start_pos.first);
      expected_span.Add(start_pos.second);
      if (end_pos.first != start_pos.first) {
        expected_span.Add(end_pos.first);
      }
      expected_span.Add(end_pos.second);

      for (SpanMap::iterator iter = range.first; iter != range.second; ++iter) {
        if (CompareSpans(expected_span, iter->second->span())) {
          if (expected_leading_comments == nullptr) {
            EXPECT_FALSE(iter->second->has_leading_comments());
          } else {
            EXPECT_TRUE(iter->second->has_leading_comments());
            EXPECT_EQ(expected_leading_comments,
                      iter->second->leading_comments());
          }
          if (expected_trailing_comments == nullptr) {
            EXPECT_FALSE(iter->second->has_trailing_comments());
          } else {
            EXPECT_TRUE(iter->second->has_trailing_comments());
            EXPECT_EQ(expected_trailing_comments,
                      iter->second->trailing_comments());
          }
          if (expected_leading_detached_comments == nullptr) {
            EXPECT_EQ(0, iter->second->leading_detached_comments_size());
          } else {
            EXPECT_EQ(
                expected_leading_detached_comments,
                Join(iter->second->leading_detached_comments(), "\n"));
          }

          spans_.erase(iter);
          return true;
        }
      }

      return false;
    }
  }

 private:
  struct SpanKey {
    const Message* descriptor_proto;
    const FieldDescriptor* field;
    int index;

    inline SpanKey() {}
    inline SpanKey(const Message& descriptor_proto_param,
                   const FieldDescriptor* field_param, int index_param)
        : descriptor_proto(&descriptor_proto_param),
          field(field_param),
          index(index_param) {}

    inline bool operator<(const SpanKey& other) const {
      if (descriptor_proto < other.descriptor_proto) return true;
      if (descriptor_proto > other.descriptor_proto) return false;
      if (field < other.field) return true;
      if (field > other.field) return false;
      return index < other.index;
    }
  };

  typedef std::multimap<SpanKey, const SourceCodeInfo::Location*> SpanMap;
  SpanMap spans_;
  std::map<char, std::pair<int, int> > markers_;
  std::string text_without_markers_;

  void ExtractMarkers(const char* text) {
    markers_.clear();
    text_without_markers_.clear();
    int line = 0;
    int column = 0;
    while (*text != '\0') {
      if (*text == '$') {
        ++text;
        GOOGLE_CHECK_NE('\0', *text);
        if (*text == '$') {
          text_without_markers_ += '$';
          ++column;
        } else {
          markers_[*text] = std::make_pair(line, column);
          ++text;
          GOOGLE_CHECK_EQ('$', *text);
        }
      } else if (*text == '\n') {
        ++line;
        column = 0;
        text_without_markers_ += *text;
      } else {
        text_without_markers_ += *text;
        ++column;
      }
      ++text;
    }
  }
};

}  // namespace compiler
}  // namespace shark
