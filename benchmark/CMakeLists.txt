#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

configure_file(${PROJECT_SOURCE_DIR}/benchmark/config.h.in ${PROJECT_SOURCE_DIR}/benchmark/config.h @ONLY)

find_package(benchmark REQUIRED)

file(GLOB_RECURSE BENCH_PROTOS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.shark)
set(bench_proto_files)
foreach (proto_file ${BENCH_PROTOS})
    string(REPLACE .shark .skp.cc pb_file ${proto_file})
    string(REPLACE .shark .skp.h pb_hdr ${proto_file})
    add_custom_command(
            OUTPUT ${pb_file} ${pb_hdr}
            DEPENDS sharkc ${proto_file}
            COMMAND sharkc ${proto_file}
            --proto_path=${PROJECT_SOURCE_DIR}
            --cpp_out=${PROJECT_SOURCE_DIR}
            --experimental_allow_proto3_optional
    )
    list(APPEND bench_proto_files ${pb_file})
endforeach ()

kmcmake_cc_benchmark(
        NAME suite_bm
        MODULE ${PROJECT_NAME}
        SOURCES
        cpp_benchmark.cc
        ${bench_proto_files}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::shark_static benchmark::benchmark
)