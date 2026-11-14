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

#include <set>
#include <string>

namespace shark::compiler {
    class AccessInfoMap;
}

namespace shark::compiler::cc {

        enum class EnforceOptimizeMode {
            kNoEnforcement, // Use the runtime specified by the file specific options.
            kSpeed, // Full runtime with a generated code implementation.
            kCodeSize, // Full runtime with a reflective implementation.
            kLiteRuntime,
        };

        struct FieldListenerOptions {
            bool inject_field_listener_events = false;
            std::set<std::string> forbidden_field_listener_events;
        };

        // Generator options (see generator.cc for a description of each):
        struct Options {
            const AccessInfoMap* access_info_map = nullptr;
            std::string dllexport_decl;
            std::string runtime_include_base;
            std::string annotation_pragma_name;
            std::string annotation_guard_name;
            FieldListenerOptions field_listener_options;
            EnforceOptimizeMode enforce_mode = EnforceOptimizeMode::kNoEnforcement;
            enum {
                kTCTableNever,
                kTCTableGuarded,
                kTCTableAlways
            } tctable_mode = kTCTableNever;
            int num_cc_files = 0;
            bool safe_boundary_check = false;
            bool proto_h = false;
            bool transitive_pb_h = true;
            bool annotate_headers = false;
            bool lite_implicit_weak_fields = false;
            bool bootstrap = false;
            bool opensource_runtime = false;
            bool annotate_accessor = false;
            bool unused_field_stripping = false;
            bool unverified_lazy_message_sets = false;
            bool unverified_lazy = false;
            bool profile_driven_inline_string = true;
            bool message_owned_arena_trial = false;
            bool force_split = false;
#ifdef SHARK_STABLE_EXPERIMENTS
            bool force_eagerly_verified_lazy = true;
            bool force_inline_string = true;
#else // SHARK_STABLE_EXPERIMENTS
            bool force_eagerly_verified_lazy = false;
            bool force_inline_string = false;
#endif // !SHARK_STABLE_EXPERIMENTS
        };

} // namespace shark::compiler::cc
