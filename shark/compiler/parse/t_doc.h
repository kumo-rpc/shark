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

#include <shark/compiler/globals.h>
#include <shark/compiler/logging.h>


    /**
     * Documentation stubs
     *
     */
    class t_doc {

    public:
        t_doc() : has_doc_(false) {}
        virtual ~t_doc() = default;

        void set_doc(const std::string& doc) {
            doc_ = doc;
            has_doc_ = true;
            if ((g_program_doctext_lineno == g_doctext_lineno)
                && (g_program_doctext_status == STILL_CANDIDATE)) {
                g_program_doctext_status = ALREADY_PROCESSED;
                pdebug("%s", "program doctext set to ALREADY_PROCESSED");
                }
        }

        const std::string& get_doc() const { return doc_; }

        bool has_doc() { return has_doc_; }

        virtual void validate() const { ; }

    private:
        std::string doc_;
        bool has_doc_;
    };


