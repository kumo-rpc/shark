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

# Stub. Teaching binaries are in CMakeLists.examples.txt
# (installed when generating with -DKMCMAKE_GEN_EXAMPLES=ON).

#[[
cmake_minimum_required(VERSION 3.16)
project("Cap'n Proto Samples" CXX)

find_package(CapnProto CONFIG REQUIRED)

capnp_generate_cpp(addressbookSources addressbookHeaders addressbook.shk)
add_executable(addressbook addressbook.c++ ${addressbookSources})
target_link_libraries(addressbook PRIVATE CapnProto::shark)
target_include_directories(addressbook PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Don't build the rpc sample if find_package() found an installation of Cap'n Proto lite.
if(TARGET CapnProto::shark-rpc)
    capnp_generate_cpp(calculatorSources calculatorHeaders calculator.shk)
    add_executable(calculator-client calculator-client.c++ ${calculatorSources})
    add_executable(calculator-server calculator-server.c++ ${calculatorSources})
    target_link_libraries(calculator-client PRIVATE CapnProto::shark-rpc)
    target_link_libraries(calculator-server PRIVATE CapnProto::shark-rpc)
    target_include_directories(calculator-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(calculator-server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
]]