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

#include <shark/compiler/parse/t_container.h>


    /**
     * A map is a lightweight container type that just wraps another two data
     * types.
     *
     */
    class t_map : public t_container {
    public:
        t_map(t_type* key_type, t_type* val_type) : key_type_(key_type), val_type_(val_type) {}

        t_type* get_key_type() const { return key_type_; }

        t_type* get_val_type() const { return val_type_; }

        bool is_map() const override { return true; }

        void validate() const override {
#ifndef ALLOW_EXCEPTIONS_AS_TYPE
            if( get_key_type()->get_true_type()->is_xception()) {
                failure("exception type \"%s\" cannot be used inside a map", get_key_type()->get_name().c_str());
            }
            if( get_val_type()->get_true_type()->is_xception()) {
                failure("exception type \"%s\" cannot be used inside a map", get_val_type()->get_name().c_str());
            }
#endif
        }

    private:
        t_type* key_type_;
        t_type* val_type_;
    };
