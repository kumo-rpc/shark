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
# AI: ARM/AArch64 SIMD level enablement.
# AI:
# AI: Input:
# AI:   KMCMAKE_RUNTIME_SIMD_LEVEL  - target level string (NONE or NEON)
# AI:   KMCMAKE_ARM_HAS_*          - BOOL detection results
# AI:   KMCMAKE_ARM_IS_64          - BOOL architecture flag
# AI:
# AI: Output:
# AI:   KMCMAKE_ARCH_OPTION         - compiler flags

set(KMCMAKE_ARCH_ENABLE_FMA FALSE)

string(TOUPPER "${KMCMAKE_RUNTIME_SIMD_LEVEL}" _LEVEL)

if(_LEVEL STREQUAL "NONE")
    kmcmake_print("ARM SIMD: level=NONE, no flags enabled")
    return()
endif()

if(KMCMAKE_ARM_IS_64)
    # ARM64: NEON is baseline, only FMA needs an explicit flag
    if(KMCMAKE_ARM_HAS_FMA)
        list(APPEND KMCMAKE_ARCH_OPTION ${FMA_FLAG})
        set(KMCMAKE_ARCH_ENABLE_FMA TRUE)
    endif()
else()
    # ARM32: add -mfpu flags for each supported feature
    if(KMCMAKE_ARM_HAS_NEON)
        list(APPEND KMCMAKE_ARCH_OPTION ${NEON_FLAG})
    endif()
    if(KMCMAKE_ARM_HAS_VFPv4)
        list(APPEND KMCMAKE_ARCH_OPTION ${VFPv4_FLAG})
    endif()
    if(KMCMAKE_ARM_HAS_FMA)
        list(APPEND KMCMAKE_ARCH_OPTION ${FMA_FLAG})
        set(KMCMAKE_ARCH_ENABLE_FMA TRUE)
    endif()
endif()

list(REMOVE_DUPLICATES KMCMAKE_ARCH_OPTION)

if(NOT KMCMAKE_ARCH_OPTION)
    kmcmake_warn("ARM SIMD: KMCMAKE_RUNTIME_SIMD_LEVEL=${_LEVEL} requested but no NEON support detected")
endif()

kmcmake_print_label("ARM ARCH_OPTION" "${KMCMAKE_ARCH_OPTION}")
