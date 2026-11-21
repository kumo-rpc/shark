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
# AI: ARM/AArch64 SIMD feature detection.
# AI: Exports one variable per feature. All start with KMCMAKE_ARM_HAS_.
# AI:
# AI: Output variables (BOOL):
# AI:   KMCMAKE_ARM_HAS_NEON
# AI:   KMCMAKE_ARM_HAS_VFPv4
# AI:   KMCMAKE_ARM_HAS_FMA
# AI:
# AI: Also exports flag variables:
# AI:   NEON_FLAG, VFPv4_FLAG, FMA_FLAG
# AI:
# AI: Prerequisites:
# AI:   include(CheckCXXSourceRuns)
# AI:   include(CheckCXXSourceCompiles)

include(CheckCXXSourceCompiles)

set(KMCMAKE_ARM_HAS_NEON FALSE)
set(KMCMAKE_ARM_HAS_VFPv4 FALSE)
set(KMCMAKE_ARM_HAS_FMA FALSE)

# Detect ARM32 vs ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|AARCH64|ARM64")
    set(KMCMAKE_ARM_IS_64 TRUE)
else()
    set(KMCMAKE_ARM_IS_64 FALSE)
endif()

if(MSVC)
    # MSVC: no -march/-mfpu flags needed, features are implied by arch
    set(NEON_FLAG "")
    set(VFPv4_FLAG "")
    set(FMA_FLAG "")
elseif(KMCMAKE_ARM_IS_64)
    # ARM64 (AArch64): NEON and VFPv4 are baseline, no -mfpu flags
    set(NEON_FLAG "")
    set(VFPv4_FLAG "")
    set(FMA_FLAG "-march=armv8.1-a")

    # NEON is part of the AArch64 base architecture
    set(KMCMAKE_ARM_HAS_NEON TRUE)

    # VFPv4 is also part of the AArch64 base
    set(KMCMAKE_ARM_HAS_VFPv4 TRUE)

    # FMA check: requires ARMv8.1-A or later
    set(FMA_CODE "
#if !defined(__ARM_FEATURE_FMA)
#error \"FMA not supported\"
#endif
#include <arm_neon.h>
int main() {
    float32x4_t a = vdupq_n_f32(1.0f);
    float32x4_t b = vdupq_n_f32(2.0f);
    float32x4_t c = vfmaq_f32(a, b, a);
    return 0;
}")
    set(CMAKE_REQUIRED_FLAGS "${FMA_FLAG}")
    check_cxx_source_compiles("${FMA_CODE}" _KMCMAKE_ARM_FMA_OK)
    if(_KMCMAKE_ARM_FMA_OK)
        set(KMCMAKE_ARM_HAS_FMA TRUE)
    endif()
    set(CMAKE_REQUIRED_FLAGS "")

else()
    # ARM32: use -mfpu flags (MSVC excluded above, GCC/Clang only)
    set(NEON_FLAG "-mfpu=neon")
    set(VFPv4_FLAG "-mfpu=vfpv4")
    set(FMA_FLAG "-mfpu=neon -mfma")

    # NEON check
    set(NEON_CODE "
#include <arm_neon.h>
int main() { float32x4_t a = vdupq_n_f32(0.0f); return 0; }")
    set(CMAKE_REQUIRED_FLAGS "${NEON_FLAG}")
    check_cxx_source_compiles("${NEON_CODE}" _KMCMAKE_ARM_NEON_OK)
    if(_KMCMAKE_ARM_NEON_OK)
        set(KMCMAKE_ARM_HAS_NEON TRUE)
    endif()

    # VFPv4 check
    set(VFPv4_CODE "
#if !(defined(__ARM_FP) && (__ARM_FP & 0x8))
#error \"VFPv4 not supported\"
#endif
int main() { float a = 0.0f; return 0; }")
    set(CMAKE_REQUIRED_FLAGS "${VFPv4_FLAG}")
    check_cxx_source_compiles("${VFPv4_CODE}" _KMCMAKE_ARM_VFPv4_OK)
    if(_KMCMAKE_ARM_VFPv4_OK)
        set(KMCMAKE_ARM_HAS_VFPv4 TRUE)
    endif()

    # FMA check
    set(FMA_CODE "
#if !defined(__ARM_FEATURE_FMA)
#error \"FMA not supported\"
#endif
#include <arm_neon.h>
int main() {
    float32x4_t a = vdupq_n_f32(1.0f);
    float32x4_t b = vdupq_n_f32(2.0f);
    float32x4_t c = vfmaq_f32(a, b, a);
    return 0;
}")
    set(CMAKE_REQUIRED_FLAGS "${FMA_FLAG}")
    check_cxx_source_compiles("${FMA_CODE}" _KMCMAKE_ARM_FMA_OK)
    if(_KMCMAKE_ARM_FMA_OK)
        set(KMCMAKE_ARM_HAS_FMA TRUE)
    endif()

    set(CMAKE_REQUIRED_FLAGS "")
endif()

kmcmake_print("ARM SIMD detection complete")
kmcmake_print_label("ARM IS_64"      ${KMCMAKE_ARM_IS_64})
kmcmake_print_label("ARM HAS_NEON"  ${KMCMAKE_ARM_HAS_NEON})
kmcmake_print_label("ARM HAS_VFPv4" ${KMCMAKE_ARM_HAS_VFPv4})
kmcmake_print_label("ARM HAS_FMA"   ${KMCMAKE_ARM_HAS_FMA})
