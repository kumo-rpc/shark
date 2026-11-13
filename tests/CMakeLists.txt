# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

configure_file(${PROJECT_SOURCE_DIR}/tests/config.h.in ${PROJECT_SOURCE_DIR}/tests/config.h @ONLY)
include(kmcmake_cpp_test_sk.cmake)

find_package(GTest REQUIRED)

add_subdirectory(sk)

add_subdirectory(common)

add_subdirectory(compiler)
add_subdirectory(format)
add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(memory)
add_subdirectory(stubs)
add_subdirectory(types)
add_subdirectory(util)
add_subdirectory(transfer)
