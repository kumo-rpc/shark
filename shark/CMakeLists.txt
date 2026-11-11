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

# Default exported target (install(EXPORT) requires at least one).
# Teaching demos (foo / xxd / binaries) live in CMakeLists.examples.txt
# and are installed only when generating with -DKMCMAKE_GEN_EXAMPLES=ON.

kmcmake_cc_interface(
        NAMESPACE ${PROJECT_NAME}
        NAME skills
        HEADERS
        skills.h
        version.h
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PUBLIC
)

set(BASE_SRC
        base/arena.cc
        base/array.cc
        base/async-io-unix.cc
        base/async-io-win32.cc
        base/async-io.cc
        base/async-unix.cc
        base/async-win32.cc
        base/async.cc
        base/cidr.cc
        base/common.cc
        base/debug.cc
        base/encoding.cc
        base/exception.cc
        base/filesystem-disk-unix.cc
        base/filesystem-disk-win32.cc
        base/filesystem.cc
        base/hash.cc
        base/io.cc
        base/list.cc
        base/main.cc
        base/memory.cc
        base/mutex.cc
        base/refcount.cc
        base/source-location.cc
        base/string-tree.cc
        base/string.cc
        base/table.cc
        base/thread.cc
        base/time.cc
        base/timer.cc
        base/units.cc
        base/compat/brotli.cc
        base/compat/gzip.cc
        base/compat/http.cc
        base/compat/readiness-io.cc
        base/compat/tls.cc
        base/compat/url.cc
        base/parse/char.cc
)

kmcmake_cc_library(
        NAME shark_base
        NAMESPACE shark
        SOURCES
        ${BASE_SRC}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)


set(LIB_SRCS
        capability.cc
        cpp.sk.cc
        blob.cc
        arena.cc
        layout.cc
        list.cc
        any.cc
        message.cc
        schema.sk.cc
        stream.sk.cc
        serialize.cc
        serialize-packed.cc
        schema.cc
        schema_loader.cc
        dynamic.cc
        reconnect.cc
        stringify.cc
)

kmcmake_cc_library(
        NAME shark
        NAMESPACE shark
        SOURCES
        ${LIB_SRCS}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)


kmcmake_cc_library(
        NAME shark-format
        NAMESPACE shark
        SOURCES
        format/json/json.sk.cc
        format/json/json.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

set(SHARKC_SRCS
        compiler/type-id.cc
        compiler/error-reporter.cc
        compiler/lexer.sk.cc
        compiler/lexer.cc
        compiler/grammar.sk.cc
        compiler/parser.cc
        compiler/generics.cc
        compiler/node-translator.cc
        compiler/compiler.cc
        schema_parser.cc
        serialize-text.cc
)


kmcmake_cc_library(
        NAME sharkc
        NAMESPACE shark
        SOURCES
        ${SHARKC_SRCS}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark-format_static
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

kmcmake_cc_binary(
        NAME shark
        NAMESPACE shark
        SOURCES
        compiler/module-loader.cc
        compiler/shark.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::sharkc_static
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)


kmcmake_cc_binary(
        NAME shark-cpp
        NAMESPACE shark
        SOURCES
        compiler/sharkc-cpp.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

kmcmake_cc_binary(
        NAME shark-shark
        NAMESPACE shark
        SOURCES
        compiler/sharkc-shark.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

include(shark_cc_generate)

#########################################
# rpc
shark_schema_generate(
        COM_FILES
        COM_HDRS
        SCHEMAS
        rpc/persistent.shk
        rpc/rpc.shk
        rpc/rpc-twoparty.shk
        compat/byte-stream.shk
        compat/http_over_shark.shk
        compat/json-rpc.shk
)

set(RPC_SRC
        ${COM_FILES}
        rpc/serialize-async.cc
        rpc/membrane.cc
        rpc/dynamic-capability.cc
        rpc/rpc.cc
        rpc/rpc.sk.cc
        rpc/rpc-twoparty.cc
        rpc/ez-rpc.cc
        compat/byte-stream.cc
        compat/http_over_shark.cc
        compat/json-rpc.cc
        rpc/websocket-rpc.cc
)


kmcmake_cc_library(
        NAME shark-rpc
        NAMESPACE shark
        SOURCES
        ${RPC_SRC}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)
