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

#include <shark/compiler/cc/message_layout_helper.h>

namespace shark::compiler::cc {

    // Rearranges the fields of a message to minimize padding.
    // Fields are grouped by the type and the size.
    // For example, grouping four boolean fields and one int32
    // field results in zero padding overhead. See OptimizeLayout's
    // comment for details.
    class PaddingOptimizer : public MessageLayoutHelper {
    public:
        PaddingOptimizer() { }
        ~PaddingOptimizer() override { }

        void OptimizeLayout(std::vector<const FieldDescriptor*>* fields,
            const Options& options,
            MessageSCCAnalyzer* scc_analyzer) override;
    };

} // namespace shark::compiler::cc
