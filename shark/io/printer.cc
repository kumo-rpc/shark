// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <shark/io/printer.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/match.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/strip.h>

namespace shark::io {

    Printer::Printer(ZeroCopyOutputStream* output, char variable_delimiter)
        : variable_delimiter_(variable_delimiter)
        , output_(output)
        , buffer_(nullptr)
        , buffer_size_(0)
        , offset_(0)
        , at_start_of_line_(true)
        , failed_(false)
        , annotation_collector_(nullptr) { }

    Printer::Printer(ZeroCopyOutputStream* output, char variable_delimiter,
        AnnotationCollector* annotation_collector)
        : variable_delimiter_(variable_delimiter)
        , output_(output)
        , buffer_(nullptr)
        , buffer_size_(0)
        , offset_(0)
        , at_start_of_line_(true)
        , failed_(false)
        , annotation_collector_(annotation_collector) { }

    Printer::~Printer() {
        // Only BackUp() if we invoked Next() at least once, and we have never failed.
        // Note that we always call `Backup`, i.e. we call BackUp(0) as some output
        // streams have buffered output, and BackUp() serves as a flush event in such
        // implementations.
        if (buffer_ != nullptr && !failed_) {
            output_->back_up(buffer_size_);
        }
    }

    bool Printer::get_substitution_range(const char* varname,
        std::pair<size_t, size_t>* range) {
        std::map<std::string, std::pair<size_t, size_t>>::const_iterator iter = substitutions_.find(varname);
        if (iter == substitutions_.end()) {
            KLOG(DFATAL) << " Undefined variable in annotation: " << varname;
            return false;
        }
        if (iter->second.first > iter->second.second) {
            KLOG(DFATAL) << " Variable used for annotation used multiple times: "
                         << varname;
            return false;
        }
        *range = iter->second;
        return true;
    }

    void Printer::annotate(const char* begin_varname, const char* end_varname,
        const std::string& file_path,
        const std::vector<int>& path) {
        if (annotation_collector_ == nullptr) {
            // Can't generate signatures with this Printer.
            return;
        }
        std::pair<size_t, size_t> begin, end;
        if (!get_substitution_range(begin_varname, &begin) || !get_substitution_range(end_varname, &end)) {
            return;
        }
        if (begin.first > end.second) {
            KLOG(DFATAL) << "  Annotation has negative length from " << begin_varname
                         << " to " << end_varname;
        } else {
            annotation_collector_->add_annotation(begin.first, end.second, file_path,
                path);
        }
    }

    void Printer::print(const std::map<std::string, std::string>& variables,
        const char* text) {
        int size = strlen(text);
        int pos = 0; // The number of bytes we've written so far.
        substitutions_.clear();
        line_start_variables_.clear();

        for (int i = 0; i < size; i++) {
            if (text[i] == '\n') {
                // Saw newline.  If there is more text, we may need to insert an indent
                // here.  So, write what we have so far, including the '\n'.
                write_raw(text + pos, i - pos + 1);
                pos = i + 1;

                // Setting this true will cause the next write_raw() to insert an indent
                // first.
                at_start_of_line_ = true;
                line_start_variables_.clear();

            } else if (text[i] == variable_delimiter_) {
                // Saw the start of a variable name.

                // Write what we have so far.
                write_raw(text + pos, i - pos);
                pos = i + 1;

                // Find closing delimiter.
                const char* end = strchr(text + pos, variable_delimiter_);
                if (end == nullptr) {
                    KLOG(DFATAL) << " Unclosed variable name.";
                    end = text + pos;
                }
                int endpos = end - text;

                std::string varname(text + pos, endpos - pos);
                if (varname.empty()) {
                    // Two delimiters in a row reduce to a literal delimiter character.
                    write_raw(&variable_delimiter_, 1);
                } else {
                    // Replace with the variable's value.
                    auto iter = variables.find(varname);
                    if (iter == variables.end()) {
                        KLOG(DFATAL) << " Undefined variable: " << varname;
                    } else {
                        if (at_start_of_line_ && iter->second.empty()) {
                            line_start_variables_.push_back(varname);
                        }
                        write_raw(iter->second.data(), iter->second.size());
                        std::pair<std::map<std::string, std::pair<size_t, size_t>>::iterator,
                            bool>
                            inserted = substitutions_.insert(std::make_pair(
                                varname,
                                std::make_pair(offset_ - iter->second.size(), offset_)));
                        if (!inserted.second) {
                            // This variable was used multiple times.  Make its span have
                            // negative length so we can detect it if it gets used in an
                            // annotation.
                            inserted.first->second = std::make_pair(1, 0);
                        }
                    }
                }

                // Advance past this variable.
                i = endpos;
                pos = endpos + 1;
            }
        }

        // Write the rest.
        write_raw(text + pos, size - pos);
    }

    void Printer::indent() {
        indent_ += "    ";
    }

    void Printer::outdent() {
        if (indent_.empty()) {
            KLOG(DFATAL) << " Outdent() without matching Indent().";
            return;
        }

        indent_.resize(indent_.size() - 4);
    }

    void Printer::print_raw(const std::string& data) {
        write_raw(data.data(), data.size());
    }

    void Printer::print_raw(const char* data) {
        if (failed_)
            return;
        write_raw(data, strlen(data));
    }

    void Printer::write_raw(const char* data, int size) {
        if (failed_)
            return;
        if (size == 0)
            return;

        if (at_start_of_line_ && (size > 0) && (data[0] != '\n')) {
            // Insert an indent.
            at_start_of_line_ = false;
            copy_to_buffer(indent_.data(), indent_.size());
            if (failed_)
                return;
            // Fix up empty variables (e.g., "{") that should be annotated as
            // coming after the indent.
            for (auto i = line_start_variables_.begin();
                i != line_start_variables_.end(); ++i) {
                substitutions_[*i].first += indent_.size();
                substitutions_[*i].second += indent_.size();
            }
        }

        // If we're going to write any data, clear line_start_variables_, since
        // we've either updated them in the block above or they no longer refer to
        // the current line.
        line_start_variables_.clear();

        copy_to_buffer(data, size);
    }

    bool Printer::next() {
        do {
            void* void_buffer;
            if (!output_->next(&void_buffer, &buffer_size_)) {
                failed_ = true;
                return false;
            }
            buffer_ = reinterpret_cast<char*>(void_buffer);
        } while (buffer_size_ == 0);
        return true;
    }

    void Printer::copy_to_buffer(const char* data, int size) {
        if (failed_)
            return;
        if (size == 0)
            return;

        while (size > buffer_size_) {
            // Data exceeds space in the buffer.  Copy what we can and request a
            // new buffer.
            if (buffer_size_ > 0) {
                memcpy(buffer_, data, buffer_size_);
                offset_ += buffer_size_;
                data += buffer_size_;
                size -= buffer_size_;
            }
            void* void_buffer;
            failed_ = !output_->next(&void_buffer, &buffer_size_);
            if (failed_)
                return;
            buffer_ = reinterpret_cast<char*>(void_buffer);
        }

        // Buffer is big enough to receive the data; copy it.
        memcpy(buffer_, data, size);
        buffer_ += size;
        buffer_size_ -= size;
        offset_ += size;
    }

    void Printer::indent_if_at_start() {
        if (at_start_of_line_) {
            copy_to_buffer(indent_.data(), indent_.size());
            at_start_of_line_ = false;
        }
    }

    void Printer::format_internal(const std::vector<std::string>& args,
        const std::map<std::string, std::string>& vars,
        const char* format) {
        auto save = format;
        int arg_index = 0;
        std::vector<AnnotationCollector::Annotation> annotations;
        while (*format) {
            char c = *format++;
            switch (c) {
            case '$':
                format = write_variable(args, vars, format, &arg_index, &annotations);
                continue;
            case '\n':
                at_start_of_line_ = true;
                line_start_variables_.clear();
                break;
            default:
                indent_if_at_start();
                break;
            }
            push_back(c);
        }
        if (arg_index != static_cast<int>(args.size())) {
            KLOG(FATAL) << " Unused arguments. " << save;
        }
        if (!annotations.empty()) {
            KLOG(FATAL) << " Annotation range is not-closed, expect $}$. " << save;
        }
    }

    const char* Printer::write_variable(
        const std::vector<std::string>& args,
        const std::map<std::string, std::string>& vars, const char* format,
        int* arg_index, std::vector<AnnotationCollector::Annotation>* annotations) {
        auto start = format;
        auto end = strchr(format, '$');
        if (!end) {
            KLOG(FATAL) << " Unclosed variable name.";
        }
        format = end + 1;
        if (end == start) {
            // "$$" is an escape for just '$'
            indent_if_at_start();
            push_back('$');
            return format;
        }
        if (*start == '{') {
            KCHECK(std::isdigit(start[1]));
            KCHECK_EQ(end - start, 2);
            int idx = start[1] - '1';
            if (idx < 0 || static_cast<size_t>(idx) >= args.size()) {
                KLOG(FATAL) << "Annotation ${" << idx + 1 << "$ is out of bounds.";
            }
            if (idx > *arg_index) {
                KLOG(FATAL) << "Annotation arg must be in correct order as given. Expected"
                            << " ${" << (*arg_index) + 1 << "$ got ${" << idx + 1 << "$.";
            } else if (idx == *arg_index) {
                (*arg_index)++;
            }
            indent_if_at_start();
            annotations->push_back({ { offset_, 0 }, args[idx] });
            return format;
        } else if (*start == '}') {
            KCHECK(annotations);
            if (annotations->empty()) {
                KLOG(FATAL) << "Unexpected end of annotation found.";
            }
            auto& a = annotations->back();
            a.first.second = offset_;
            if (annotation_collector_)
                annotation_collector_->add_annotation_new(a);
            annotations->pop_back();
            return format;
        }
        auto start_var = start;
        while (start_var < end && *start_var == ' ')
            start_var++;
        if (start_var == end) {
            KLOG(FATAL) << " Empty variable.";
        }
        auto end_var = end;
        while (start_var < end_var && *(end_var - 1) == ' ')
            end_var--;
        std::string var_name {
            start_var, static_cast<std::string::size_type>(end_var - start_var)
        };
        std::string sub;
        if (std::isdigit(var_name[0])) {
            KCHECK_EQ(var_name.size(), 1U); // No need for multi-digits
            int idx = var_name[0] - '1'; // Start counting at 1
            KCHECK_GE(idx, 0);
            if (static_cast<size_t>(idx) >= args.size()) {
                KLOG(FATAL) << "Argument $" << idx + 1 << "$ is out of bounds.";
            }
            if (idx > *arg_index) {
                KLOG(FATAL) << "Arguments must be used in same order as given. Expected $"
                            << (*arg_index) + 1 << "$ got $" << idx + 1 << "$.";
            } else if (idx == *arg_index) {
                (*arg_index)++;
            }
            sub = args[idx];
        } else {
            auto it = vars.find(var_name);
            if (it == vars.end()) {
                KLOG(FATAL) << " Unknown variable: " << var_name << ".";
            }
            sub = it->second;
        }

        // By returning here in case of empty we also skip possible spaces inside
        // the $...$, i.e. "void$ dllexpor$ f();" -> "void f();" in the empty case.
        if (sub.empty())
            return format;

        // We're going to write something non-empty so we need a possible indent.
        indent_if_at_start();

        // Write the possible spaces in front.
        copy_to_buffer(start, start_var - start);
        // Write a non-empty substituted variable.
        copy_to_buffer(sub.c_str(), sub.size());
        // Finish off with writing possible trailing spaces.
        copy_to_buffer(end_var, end - end_var);
        return format;
    }

} // namespace shark::io
