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
# AI: Entry point for architecture-specific SIMD detection and level control.
# AI: Usage:
# AI:   include(kmcmake_arch)
# AI:   After this call, the following variables are available:
# AI:     KMCMAKE_ARCH_OPTION        - compiler flags for selected SIMD level
# AI:     KMCMAKE_ARCH_ENABLE_SSE    - BOOL, whether SSE is enabled
# AI:     KMCMAKE_ARCH_ENABLE_AVX2   - BOOL, whether AVX2 is enabled
# AI:     (and per-architecture specific enable flags)
# AI:
# AI: Prerequisites:
# AI:   KMCMAKE_RUNTIME_SIMD_LEVEL  - target SIMD level (NONE/SSE/.../AVX512)
# AI:   kmcmake_print/kmcmake_warn  - from default_setting.cmake
# AI:
# AI: This file routes to the correct architecture detection based on
# AI: CMAKE_SYSTEM_PROCESSOR.

include(CheckCXXSourceCompiles)

# AI: Utility function used by per-arch detect modules to check a SIMD feature.
# AI: Usage: kmcmake_detect_simd(code flags OUT_VAR)
function(kmcmake_detect_simd code flags OUT_VAR)
    if(NOT code OR code STREQUAL "")
        set(${OUT_VAR} FALSE PARENT_SCOPE)
        kmcmake_print("simd check: code is empty -> false")
        return()
    endif()
    if(NOT flags OR flags STREQUAL "")
        set(_OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS "")
        check_cxx_source_compiles("${code}" _TEST_FLAG)
        set(CMAKE_REQUIRED_FLAGS ${_OLD_REQUIRED_FLAGS})
        set(${OUT_VAR} ${_TEST_FLAG} PARENT_SCOPE)
        return()
    endif()
    foreach(_FLAG ${flags})
        set(_OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS ${_FLAG})
        check_cxx_source_compiles("${code}" _TEST_FLAG)
        set(CMAKE_REQUIRED_FLAGS ${_OLD_REQUIRED_FLAGS})
        if(NOT _TEST_FLAG)
            set(${OUT_VAR} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${OUT_VAR} TRUE PARENT_SCOPE)
endfunction()

# AI: Convert CMake BOOL to integer 0/1 for use in version.h.in
macro(_kmcmake_to_int _var)
    if(${_var})
        set(${_var} 1)
    else()
        set(${_var} 0)
    endif()
endmacro()

# Reset output variables
set(KMCMAKE_ARCH_OPTION "")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    include(arch/x86/kmcmake_arch_detect)
    include(arch/x86/kmcmake_arch_level)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm.*|aarch64")
    include(arch/arm/kmcmake_arch_detect)
    include(arch/arm/kmcmake_arch_level)
else()
    kmcmake_print("Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}, no SIMD detection performed")
endif()

# AI: Export SIMD flags as KMCMAKE_SIMD_CXX_FLAGS for easy consumption
set(KMCMAKE_SIMD_CXX_FLAGS ${KMCMAKE_ARCH_OPTION})

# AI: Convert ENABLE BOOLs to 0/1 for version.h.in
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSE)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSE2)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSE3)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSSE3)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSE4_1)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_SSE4_2)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_AVX)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_AVX2)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_AVX512F)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_BMI1)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_BMI2)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_FMA)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_F16C)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_POPCNT)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_LZCNT)
_kmcmake_to_int(KMCMAKE_ARCH_ENABLE_MOVBE)

# ============================================================
# Architecture-specific hardware AES/RANDEN flags
# ============================================================
set(_KMCMAKE_RANDOM_HWAES_ARM32_FLAGS
    "-mfpu=neon"
)
set(_KMCMAKE_RANDOM_HWAES_ARM64_FLAGS
    "-march=armv8-a+crypto"
)
set(_KMCMAKE_RANDOM_HWAES_MSVC_X64_FLAGS)
set(_KMCMAKE_RANDOM_HWAES_X64_FLAGS
    "-maes"
    "-msse4.1"
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    if(MSVC)
        set(KMCMAKE_RANDEN_FLAGS "${_KMCMAKE_RANDOM_HWAES_MSVC_X64_FLAGS}")
    else()
        set(KMCMAKE_RANDEN_FLAGS "${_KMCMAKE_RANDOM_HWAES_X64_FLAGS}")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm.*|aarch64|ARM64")
    if(MSVC)
        set(KMCMAKE_RANDEN_FLAGS "")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(KMCMAKE_RANDEN_FLAGS "${_KMCMAKE_RANDOM_HWAES_ARM64_FLAGS}")
    else()
        set(KMCMAKE_RANDEN_FLAGS "${_KMCMAKE_RANDOM_HWAES_ARM32_FLAGS}")
    endif()
else()
    set(KMCMAKE_RANDEN_FLAGS "")
endif()

kmcmake_print_list_label("RANDEN_FLAGS:" KMCMAKE_RANDEN_FLAGS)