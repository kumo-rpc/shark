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

#pragma once

#include <cstdio>
#include <string_view>

#include <turbo/base/log_severity.h>
#include <turbo/log/log_sink.h>
#include <turbo/log/sinks/internal/fs_helper.h>

namespace shark::common {

    /// A lightweight LogSink that outputs only the log message without
    /// timestamp, thread ID, or internal source-location prefixes.
    class MessageOnlySink final : public turbo::LogSink {
    public:
        explicit MessageOnlySink(std::FILE* file = stderr)
            : _file(file)
            , _color_active(file != nullptr && turbo::log_internal::is_color_terminal(file)) { }

        void send(const turbo::LogEntry& entry) override {
            // Called under LogSinkSet mutex — no per-sink lock needed.
            if (_file == nullptr) {
                return;
            }

            // Extracts pure message content with trailing newline, stripping log prefixes.
            const std::string_view msg = entry.text_message_with_newline();
            if (msg.empty()) {
                return;
            }

            if (!_color_active) {
                std::fwrite(msg.data(), 1, msg.size(), _file);
                return;
            }

            const std::string_view color = color_for(entry.log_severity());
            std::fwrite(color.data(), 1, color.size(), _file);
            std::fwrite(msg.data(), 1, msg.size(), _file);
            std::fwrite(kReset.data(), 1, kReset.size(), _file);
        }

        void flush() override {
            if (_file != nullptr) {
                std::fflush(_file);
            }
        }

    private:
        static constexpr std::string_view kReset = "\033[m";
        static constexpr std::string_view kGreen = "\033[32m";
        static constexpr std::string_view kYellowBold = "\033[33m\033[1m";
        static constexpr std::string_view kRedBold =    "\033[31m\033[1m";
        static constexpr std::string_view kBoldOnRed = "\033[1m\033[41m";
        static constexpr std::string_view kCyan = "\033[36m";
        static constexpr std::string_view kWhite = "\033[37m";

        [[nodiscard]] static constexpr std::string_view color_for(turbo::LogSeverity sev) noexcept {
            switch (sev) {
            case turbo::LogSeverity::kTrace:
                return kWhite;
            case turbo::LogSeverity::kDebug:
                return kCyan;
            case turbo::LogSeverity::kInfo:
                return kGreen;
            case turbo::LogSeverity::kWarning:
                return kYellowBold;
            case turbo::LogSeverity::kError:
                return kRedBold;
            case turbo::LogSeverity::kFatal:
                return kBoldOnRed;
            }
            return kReset;
        }

        std::FILE* _file { nullptr };
        bool _color_active { false };
    };

    void report_debug(std::string_view msg);
    void report_info(std::string_view msg);
    void report_warn(std::string_view msg);
    void report_error(std::string_view msg);

    void clog_enable_debug();
} // namespace shark::common
