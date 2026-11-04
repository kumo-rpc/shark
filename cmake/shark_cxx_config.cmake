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
# AI: Thin user-configurable C++ options layer.
# AI: Imports framework-level flags from kmcmake/tools/ and kmcmake/arch/,
# AI: then assembles KMCMAKE_CXX_OPTIONS used by all build targets.
# AI:
# AI: Available variables (defined in framework modules):
# AI:   KMCMAKE_BASE_CXX_FLAGS   - compiler flags from kmcmake_compiler_flags.cmake
# AI:   KMCMAKE_SIMD_CXX_FLAGS   - SIMD flags from kmcmake_arch.cmake
# AI:   KMCMAKE_RANDEN_FLAGS     - AES/hardware random flags
# AI:
# AI: Append your own flags below the aggregation point.

include(kmcmake_compiler_flags)
include(kmcmake_arch)

# Default 17 only if unset. Outer -DCMAKE_CXX_STANDARD=, CMakePresets,
# parent add_subdirectory, or *_user_option.cmake can set it first.
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif ()
if (NOT CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(KMCMAKE_CXX_OPTIONS
    ${KMCMAKE_BASE_CXX_FLAGS}
    ${KMCMAKE_SIMD_CXX_FLAGS}
    ${KMCMAKE_RANDEN_FLAGS}
)
list(REMOVE_DUPLICATES KMCMAKE_CXX_OPTIONS)
kmcmake_print_list_label("CXX_OPTIONS:" KMCMAKE_CXX_OPTIONS)

###############################
# User custom flags (optional)
# =============================
# Examples:
#   list(APPEND KMCMAKE_CXX_OPTIONS "-fopenmp")
#   list(APPEND KMCMAKE_CXX_OPTIONS "-mno-avx512f")
###############################
