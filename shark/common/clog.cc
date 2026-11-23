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

#include <shark/common/clog.h>
#include <mutex>
#include <turbo/log/logging.h>

namespace shark::common {

    static std::once_flag clog_init;
    static bool clog_debug { false };

    MessageOnlySink clog_sink;
    static void init_clog_once() {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kFatal);
        turbo::initialize_log();
        turbo::add_log_sink(&clog_sink);
        if (clog_debug) {
            turbo::set_min_log_level(turbo::LogSeverityAtLeast::kDebug);
        }
        turbo::flush_log_sinks();
    }

    static void init_clog() {
        std::call_once(clog_init, init_clog_once);
    }

    void report_debug(std::string_view msg) {
        init_clog();
        KLOG(DEBUG)<<msg;
    }

    void report_info(std::string_view msg) {
        init_clog();
        KLOG(INFO)<<msg;
    }

    void report_warn(std::string_view msg) {
        init_clog();
        KLOG(WARNING)<<msg;
    }

    void report_error(std::string_view msg) {
        init_clog();
        KLOG(ERROR)<<msg;
    }

    void clog_enable_debug() {
        clog_debug = true;
        init_clog();
        turbo::set_min_log_level(turbo::LogSeverityAtLeast::kDebug);
    }
}  // namespace shark::common

