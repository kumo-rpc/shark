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

#include <shark/parser/pool.h>
#include <turbo/cli/cli.h>

#include <string>
#include <shark/common/clog.h>
#include <turbo/log/logging.h>
#include <vector>

int main(int argc, char** argv) {
    xcli::App app{"shark compiler"};
    std::string file;
    std::vector<std::string> includes;
    app.add_option("file", file, "idl file")->required();
    app.add_option("-I,--include", includes, "include prefix")->required();
    XCLI_PARSE(app, argc, argv);

    shark::common::clog_enable_debug();
    if (!shark::parser::RegistryPool::instance().build_file(file, includes, false)) {
        std::cout<<1<<std::endl;
        return 1;
    }
    return 0;
}
