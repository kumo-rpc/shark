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

#include <shark/schema/internal/utils.h>
#include <shark/schema/internal/symbol.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>
#include <shark/schema/internal/flat_allocator.h>
#include <shark/schema/internal/descriptor_pool_tables.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    // DebugString methods ===============================================

    // Formats options one per line
    bool FormatLineOptions(int depth, const Message& options,
        const DescriptorPool* pool, std::string* output) {
        std::string prefix(depth * 2, ' ');
        std::vector<std::string> all_options;
        if (RetrieveOptions(depth, options, pool, &all_options)) {
            for (const std::string& option : all_options) {
                strings::SubstituteAndAppend(output, "$0option $1;\n", prefix, option);
            }
        }
        return !all_options.empty();
    }

    // Used by each of the option formatters.
        bool RetrieveOptions(int depth, const Message& options,
            const DescriptorPool* pool,
            std::vector<std::string>* option_entries) {
            // When printing custom options for a descriptor, we must use an options
            // message built on top of the same DescriptorPool where the descriptor
            // is coming from. This is to ensure we are interpreting custom options
            // against the right pool.
            if (options.GetDescriptor()->file()->pool() == pool) {
                return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
            } else {
                const Descriptor* option_descriptor = pool->FindMessageTypeByName(options.GetDescriptor()->full_name());
                if (option_descriptor == nullptr) {
                    // descriptor.shark is not in the pool. This means no custom options are
                    // used so we are safe to proceed with the compiled options message type.
                    return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
                }
                DynamicMessageFactory factory;
                std::unique_ptr<Message> dynamic_options(
                    factory.GetPrototype(option_descriptor)->New());
                std::string serialized = options.SerializeAsString();
                io::CodedInputStream input(
                    reinterpret_cast<const uint8_t*>(serialized.c_str()),
                    serialized.size());
                input.SetExtensionRegistry(pool, &factory);
                if (dynamic_options->ParseFromCodedStream(&input)) {
                    return RetrieveOptionsAssumingRightPool(depth, *dynamic_options,
                        option_entries);
                } else {
                    GOOGLE_LOG(ERROR) << "Found invalid proto option data for: "
                                      << options.GetDescriptor()->full_name();
                    return RetrieveOptionsAssumingRightPool(depth, options, option_entries);
                }
            }
        }

    bool RetrieveOptionsAssumingRightPool(
     int depth, const Message& options,
     std::vector<std::string>* option_entries) {
        option_entries->clear();
        const Reflection* reflection = options.GetReflection();
        std::vector<const FieldDescriptor*> fields;
        reflection->ListFields(options, &fields);
        for (const FieldDescriptor* field : fields) {
            int count = 1;
            bool repeated = false;
            if (field->is_repeated()) {
                count = reflection->FieldSize(options, field);
                repeated = true;
            }
            for (int j = 0; j < count; j++) {
                std::string fieldval;
                if (field->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE) {
                    std::string tmp;
                    TextFormat::Printer printer;
                    printer.SetExpandAny(true);
                    printer.SetInitialIndentLevel(depth + 1);
                    printer.PrintFieldValueToString(options, field, repeated ? j : -1,
                        &tmp);
                    fieldval.append("{\n");
                    fieldval.append(tmp);
                    fieldval.append(depth * 2, ' ');
                    fieldval.append("}");
                } else {
                    TextFormat::PrintFieldValueToString(options, field, repeated ? j : -1,
                        &fieldval);
                }
                std::string name;
                if (field->is_extension()) {
                    name = "(." + field->full_name() + ")";
                } else {
                    name = field->name();
                }
                option_entries->push_back(name + " = " + fieldval);
            }
        }
        return !option_entries->empty();
    }

    // Formats options that all appear together in brackets. Does not include
    // brackets.
    bool FormatBracketedOptions(int depth, const Message& options,
        const DescriptorPool* pool, std::string* output) {
        std::vector<std::string> all_options;
        if (RetrieveOptions(depth, options, pool, &all_options)) {
            output->append(Join(all_options, ", "));
        }
        return !all_options.empty();
    }

    namespace internal {
        void LazyDescriptor::Set(const Descriptor* descriptor) {
            GOOGLE_CHECK(!once_);
            descriptor_ = descriptor;
        }

        void LazyDescriptor::SetLazy(StringPiece name,
            const FileDescriptor* file) {
            // verify Init() has been called and Set hasn't been called yet.
            GOOGLE_CHECK(!descriptor_);
            GOOGLE_CHECK(!once_);
            GOOGLE_CHECK(file && file->pool_);
            GOOGLE_CHECK(file->pool_->lazily_build_dependencies_);
            GOOGLE_CHECK(!file->finished_building_);
            once_ = ::new (file->pool_->tables_->AllocateBytes(static_cast<int>(
                sizeof(internal::once_flag) + name.size() + 1))) internal::once_flag { };
            char* lazy_name = reinterpret_cast<char*>(once_ + 1);
            memcpy(lazy_name, name.data(), name.size());
            lazy_name[name.size()] = 0;
        }

        void LazyDescriptor::Once(const ServiceDescriptor* service) {
            if (once_) {
                internal::call_once(*once_, [&] {
                    auto* file = service->file();
                    GOOGLE_CHECK(file->finished_building_);
                    const char* lazy_name = reinterpret_cast<const char*>(once_ + 1);
                    descriptor_ = file->pool_->CrossLinkOnDemandHelper(lazy_name, false).descriptor();
                });
            }
        }

    } // namespace internal

} // namespace shark

#include <shark/port_undef.inc>
