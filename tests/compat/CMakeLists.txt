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


shark_schema_generate(
        COMP_FILES
        COMP_HDRS
        SCHEMAS
        json-test.capnp
)

kmcmake_cc_test(
        NAME all_test
        MODULE compat
        SOURCES
        ${COMP_FILES}
        byte-stream-test.cc
        http-over-capnp-old-test.cc
        http-over-capnp-perf-test.cc
        http-over-capnp-test.cc
        json-rpc-test.cc
        json-test.cc
        websocket-rpc-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark-rpc_static
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)