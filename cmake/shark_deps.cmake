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
############################################################
# optional CPM (before find_package) — user must set KMCMAKE_USE_CPM=ON
############################################################
# CPM packages should list(APPEND KMCMAKE_DEPS_LINK ...); do not set() it.
set(KMCMAKE_DEPS_LINK "")
if (KMCMAKE_USE_CPM)
    include(${PROJECT_NAME}_cpm)
endif ()

############################################################
# system pthread and rt, dl
############################################################
set(KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS "")

# Record a private find_package(...) call for generated <Project>Config.cmake.
# Usage:
#   kmcmake_private_find_package(ZLIB REQUIRED)
function(kmcmake_private_find_package)
    string(JOIN " " _KMCMAKE_PRIVATE_FIND_PACKAGE_ARGS ${ARGV})
    string(APPEND KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS
            "find_dependency(${_KMCMAKE_PRIVATE_FIND_PACKAGE_ARGS})\n")
    set(KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS "${KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS}" PARENT_SCOPE)
endfunction()

# Record a private find_library(...) call for generated <Project>Config.cmake.
# Usage:
#   kmcmake_private_find_library(MYLIB NAMES mylib PATHS /opt/mylib/lib)
function(kmcmake_private_find_library)
    string(JOIN " " _KMCMAKE_PRIVATE_FIND_LIBRARY_ARGS ${ARGV})
    string(APPEND KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS
            "find_library(${_KMCMAKE_PRIVATE_FIND_LIBRARY_ARGS})\n")
    set(KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS "${KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS}" PARENT_SCOPE)
endfunction()

set(KMCMAKE_SYSTEM_DYLINK)
if (APPLE)
    find_library(CoreFoundation CoreFoundation)
    list(APPEND KMCMAKE_SYSTEM_DYLINK ${CoreFoundation})
else (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND KMCMAKE_SYSTEM_DYLINK rt dl)
endif ()

if (KMCMAKE_BUILD_TEST)
    enable_testing()
endif (KMCMAKE_BUILD_TEST)

find_package(Threads REQUIRED)
kmcmake_private_find_package(Threads REQUIRED)
list(APPEND KMCMAKE_SYSTEM_DYLINK Threads::Threads)
find_package(turbo REQUIRED)
############################################################
#
# add you libs to the KMCMAKE_DEPS_LINK variable eg as turbo
# so you can and system pthread and rt, dl already add to
# KMCMAKE_SYSTEM_DYLINK, using it for fun.
# CPM packages already appended above when KMCMAKE_USE_CPM=ON.
##########################################################
list(APPEND KMCMAKE_DEPS_LINK
        #${TURBO_LIB}
        turbo::turbo_static
        ${KMCMAKE_SYSTEM_DYLINK}
        )
list(REMOVE_DUPLICATES KMCMAKE_DEPS_LINK)
kmcmake_print_list_label("Denpendcies:" KMCMAKE_DEPS_LINK)
