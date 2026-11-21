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
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/kmcmake/package)

include(kmcmake_option)
include(default_setting)


################################################################################################
# platform info
################################################################################################

set(KMCMAKE_PRETTY_NAME)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    cmake_host_system_information(RESULT KMCMAKE_PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
    kmcmake_print("${KMCMAKE_PRETTY_NAME}")

    cmake_host_system_information(RESULT KMCMAKE_DISTRO QUERY DISTRIB_INFO)
    kmcmake_print_list_label("KMCMAKE_DISTRO:" KMCMAKE_DISTRO)
    foreach (dis IN LISTS KMCMAKE_DISTRO)
        kmcmake_print("${dis} = `${${dis}}`")
    endforeach ()
    set(KMCMAKE_DISTRO_VERSION_ID "${DISTRIB_VERSION_ID}")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(KMCMAKE_PRETTY_NAME "darwin")
elseif (CMAKE_SYSTEM_NAME MATCHES "iOS")
    set(KMCMAKE_PRETTY_NAME "ios")
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(KMCMAKE_PRETTY_NAME "windows")
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
    set(KMCMAKE_PRETTY_NAME "android")
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(KMCMAKE_PRETTY_NAME "freebsd")
else ()
    message(FATAL_ERROR "unknown system: ${CMAKE_SYSTEM_NAME}")
endif ()

string(TOLOWER ${KMCMAKE_PRETTY_NAME} LC_KMCMAKE_PRETTY_NAME)
string(TOUPPER ${KMCMAKE_PRETTY_NAME} UP_KMCMAKE_PRETTY_NAME)

include(kmcmake_product_interface)
include(kmcmake_cc_object)
include(kmcmake_ar_static)
include(kmcmake_ar_shared)
include(kmcmake_cc_interface)
include(kmcmake_cc_library)
include(kmcmake_cc_test)
include(kmcmake_cc_binary)
include(kmcmake_cc_benchmark)
include(kmcmake_cc_proto_object)
################################################################################################
# out of source build
################################################################################################
macro(KMCMAKE_ENSURE_OUT_OF_SOURCE_BUILD errorMessage)

    string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" is_insource)
    if (is_insource)
        kmcmake_error(${errorMessage} "In-source builds are not allowed.
    CMake would overwrite the makefiles distributed with Compiler-RT.
    Please create a directory and run cmake from there, passing the path
    to this source directory as the last argument.
    This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
    Please delete them.")

    endif (is_insource)

endmacro(KMCMAKE_ENSURE_OUT_OF_SOURCE_BUILD)

option(KMCMAKE_USE_SYSTEM_INCLUDES "" OFF)
if (VERBOSE_CMAKE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

if (KMCMAKE_USE_CXX11_ABI)
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
else ()
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif ()

if (CONDA_ENV_ENABLE)
    list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
    include_directories($ENV{CONDA_PREFIX}/include)
    link_directories($ENV{CONDA_PREFIX}/${CMAKE_INSTALL_LIBDIR})
endif ()

if (KMCMAKE_INSTALL_LIB)
    set(CMAKE_INSTALL_LIBDIR lib)
endif ()

if (KMCMAKE_USE_SYSTEM_INCLUDES)
    set(KMCMAKE_INTERNAL_INCLUDE_WARNING_GUARD SYSTEM)
else ()
    set(KMCMAKE_INTERNAL_INCLUDE_WARNING_GUARD "")
endif ()

KMCMAKE_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

include(kmcmake_cc_proto)
include(kmcmake_cc_xxd)
include(git_commit)