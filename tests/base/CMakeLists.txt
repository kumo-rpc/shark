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


kmcmake_cc_library(
        NAME testing
        NAMESPACE shark
        SOURCES
        test.cc
        test-helpers.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)

set(BASE_CASE
        arena-test
        array-test
        common-test
        debug-test
        encoding-test
        filesystem-disk-test
        filesystem-test
        function-test
        io-test
        list-test
        map-test
        memory-test
        mutex-test
        one-of-test
        refcount-test
        string-test
        test-test
        tuple-test
        units-test
)

foreach (case IN LISTS BASE_CASE)
kmcmake_cc_test(
        NAME ${case}
        MODULE base
        SOURCES
        ${case}.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}

)

endforeach ()

kmcmake_cc_test(
        NAME thread_test
        MODULE base
        SOURCES
        async-coroutine-test.cc
        async-io-test.cc
        async-queue-test.cc
        async-test.cc
        async-unix-test.cc
        async-unix-xthread-test.cc
        async-win32-test.cc
        async-win32-xthread-test.cc
        async-xthread-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}

)


kmcmake_cc_test(
        NAME exception_test
        MODULE base
        SOURCES
        exception-test.cc
        exception-override-symbolizer-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}

)


kmcmake_cc_test(
        NAME iostream_test
        MODULE base
        SOURCES
        std/iostream-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}

)

kmcmake_cc_test(
        NAME parse_test
        MODULE base
        SOURCES
        parse/char-test.cc
        parse/common-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}

)

kmcmake_cc_test(
        NAME compat_test
        MODULE base
        SOURCES
        compat/http-test.cc
        compat/readiness-io-test.cc
        compat/url-test.cc
        CXXOPTS
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)