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

################################################################################################
# options
################################################################################################
option(VERBOSE_KMCMAKE_BUILD "print kmcmake detail information" OFF)

option(VERBOSE_CMAKE_BUILD "verbose cmake make debug" OFF)

option(CONDA_ENV_ENABLE "enable conda auto env" OFF)

option(KMCMAKE_USE_CXX11_ABI "use cxx11 abi or not" ON)

option(KMCMAKE_BUILD_TEST "enable project test or not" ON)

option(KMCMAKE_BUILD_BENCHMARK "enable project benchmark or not" OFF)

option(KMCMAKE_BUILD_EXAMPLES "enable project examples or not" OFF)

option(KMCMAKE_STATUS_PRINT "kmcmake print or not, default on" ON)

option(KMCMAKE_INSTALL_LIB "avoid centos install to lib64" OFF)

# When ON, <project>_deps.cmake includes <project>_cpm.cmake which may call
# CPMAddPackage / CPMFindPackage (CPM.cmake lives under kmcmake/tools/).
# Default OFF. Enable explicitly — no auto-detection of kmpkg/vcpkg/system.
option(KMCMAKE_USE_CPM "fetch dependencies via CPM.cmake (user-managed)" OFF)

# Runtime SIMD target level used by dispatch/config logic.
# Valid values:
#   NONE, SSE, SSE2, SSE3, SSSE3, SSE4_1, SSE4_2, AVX, AVX2, AVX512
set(KMCMAKE_RUNTIME_SIMD_LEVEL "AVX2" CACHE STRING "Runtime SIMD level from NONE to AVX512")
set_property(CACHE KMCMAKE_RUNTIME_SIMD_LEVEL PROPERTY STRINGS
        NONE
        SSE
        SSE2
        SSE3
        SSSE3
        SSE4_1
        SSE4_2
        AVX
        AVX2
        AVX512
)

string(TOUPPER "${KMCMAKE_RUNTIME_SIMD_LEVEL}" KMCMAKE_RUNTIME_SIMD_LEVEL)
set(_KMCMAKE_RUNTIME_SIMD_LEVEL_VALUES
        NONE SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2 AVX AVX2 AVX512)
if (NOT KMCMAKE_RUNTIME_SIMD_LEVEL IN_LIST _KMCMAKE_RUNTIME_SIMD_LEVEL_VALUES)
    message(FATAL_ERROR
            "Invalid KMCMAKE_RUNTIME_SIMD_LEVEL='${KMCMAKE_RUNTIME_SIMD_LEVEL}'. "
            "Valid values: NONE, SSE, SSE2, SSE3, SSSE3, SSE4_1, SSE4_2, AVX, AVX2, AVX512.")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)