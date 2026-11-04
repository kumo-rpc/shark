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
# AI: x86/x86_64 SIMD level enablement.
# AI: Reads KMCMAKE_RUNTIME_SIMD_LEVEL and KMCMAKE_X86_HAS_* from detect phase,
# AI: then enables features level-by-level.
# AI:
# AI: Input:
# AI:   KMCMAKE_RUNTIME_SIMD_LEVEL  - target level string
# AI:   KMCMAKE_X86_HAS_*           - BOOL detection results
# AI:   SSE1_FLAG, SSE2_FLAG, ...   - per-feature flag variables
# AI:
# AI: Output variables:
# AI:   KMCMAKE_ARCH_OPTION          - list of compiler flags for chosen level
# AI:   KMCMAKE_ARCH_DEFS            - list of compile definitions
# AI:   KMCMAKE_ARCH_ENABLE_SSE      - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_SSE2     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_SSE3     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_SSSE3    - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_SSE4_1   - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_SSE4_2   - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_AVX      - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_AVX2     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_AVX512F  - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_BMI1     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_BMI2     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_FMA      - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_F16C     - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_POPCNT   - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_LZCNT    - BOOL
# AI:   KMCMAKE_ARCH_ENABLE_MOVBE    - BOOL
# AI:
# AI: Users may override any KMCMAKE_ARCH_ENABLE_* variable before including
# AI: this file to force-disable a feature even when the hardware supports it.

# AI: --- Predefine output enable flags ---
set(KMCMAKE_ARCH_ENABLE_SSE FALSE)
set(KMCMAKE_ARCH_ENABLE_SSE2 FALSE)
set(KMCMAKE_ARCH_ENABLE_SSE3 FALSE)
set(KMCMAKE_ARCH_ENABLE_SSSE3 FALSE)
set(KMCMAKE_ARCH_ENABLE_SSE4_1 FALSE)
set(KMCMAKE_ARCH_ENABLE_SSE4_2 FALSE)
set(KMCMAKE_ARCH_ENABLE_AVX FALSE)
set(KMCMAKE_ARCH_ENABLE_AVX2 FALSE)
set(KMCMAKE_ARCH_ENABLE_AVX512F FALSE)
set(KMCMAKE_ARCH_ENABLE_BMI1 FALSE)
set(KMCMAKE_ARCH_ENABLE_BMI2 FALSE)
set(KMCMAKE_ARCH_ENABLE_FMA FALSE)
set(KMCMAKE_ARCH_ENABLE_F16C FALSE)
set(KMCMAKE_ARCH_ENABLE_POPCNT FALSE)
set(KMCMAKE_ARCH_ENABLE_LZCNT FALSE)
set(KMCMAKE_ARCH_ENABLE_MOVBE FALSE)

# AI: Define ordered level list
set(_X86_SIMD_LEVELS NONE SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2 AVX AVX2 AVX512)

string(TOUPPER "${KMCMAKE_RUNTIME_SIMD_LEVEL}" _LEVEL)
if(NOT _LEVEL)
    set(_LEVEL "AVX2")
endif()

list(FIND _X86_SIMD_LEVELS "${_LEVEL}" _LEVEL_IDX)
if(_LEVEL_IDX EQUAL -1)
    kmcmake_error("Invalid KMCMAKE_RUNTIME_SIMD_LEVEL='${KMCMAKE_RUNTIME_SIMD_LEVEL}'. Valid: ${_X86_SIMD_LEVELS}")
endif()

if(_LEVEL STREQUAL "NONE")
    kmcmake_print("x86 SIMD: level=NONE, no flags enabled")
    return()
endif()

# AI: Helper macro: enable a feature if hardware supports it.
# Empty flags (e.g. MSVC x64 SSE/SSE2 baseline) still mark ENABLE_* but do not
# append to KMCMAKE_ARCH_OPTION.
macro(_x86_enable_feature feature flag_var)
    if(KMCMAKE_X86_HAS_${feature} AND NOT KMCMAKE_ARCH_ENABLE_${feature})
        if(NOT "${${flag_var}}" STREQUAL "")
            list(APPEND KMCMAKE_ARCH_OPTION ${${flag_var}})
        endif()
        set(KMCMAKE_ARCH_ENABLE_${feature} TRUE)
        kmcmake_print("x86 SIMD: enabled ${feature}")
    endif()
endmacro()

# AI: Walk level-by-level
if(_LEVEL_IDX GREATER_EQUAL 1)
    _x86_enable_feature(SSE SSE1_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 2)
    _x86_enable_feature(SSE2 SSE2_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 3)
    _x86_enable_feature(SSE3 SSE3_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 4)
    _x86_enable_feature(SSSE3 SSSE3_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 5)
    _x86_enable_feature(SSE4_1 SSE4_1_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 6)
    _x86_enable_feature(SSE4_2 SSE4_2_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 7)
    _x86_enable_feature(AVX AVX_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 8)
    _x86_enable_feature(AVX2 AVX2_FLAG)
    # AI: Sub-features bundled with AVX2 level
    _x86_enable_feature(BMI1 BMI1_FLAG)
    _x86_enable_feature(BMI2 BMI2_FLAG)
    _x86_enable_feature(FMA FMA_FLAG)
    _x86_enable_feature(F16C F16C_FLAG)
    _x86_enable_feature(POPCNT POPCNT_FLAG)
    _x86_enable_feature(LZCNT LZCNT_FLAG)
    _x86_enable_feature(MOVBE MOVBE_FLAG)
endif()
if(_LEVEL_IDX GREATER_EQUAL 9)
    if(MSVC)
        _x86_enable_feature(AVX512F AVX512_FLAG)
    else()
        _x86_enable_feature(AVX512F AVX512F_FLAG)
    endif()
endif()

list(REMOVE_DUPLICATES KMCMAKE_ARCH_OPTION)

if(NOT KMCMAKE_ARCH_OPTION)
    kmcmake_warn("KMCMAKE_RUNTIME_SIMD_LEVEL=${_LEVEL} requested but no matching feature detected")
endif()

kmcmake_print_label("x86 ARCH_OPTION" "${KMCMAKE_ARCH_OPTION}")