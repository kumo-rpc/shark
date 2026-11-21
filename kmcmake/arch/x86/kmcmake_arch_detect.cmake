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
# AI: x86/x86_64 SIMD feature detection.
# AI: Exports one variable per feature. All start with KMCMAKE_X86_HAS_.
# AI:
# AI: Output variables (BOOL):
# AI:   KMCMAKE_X86_HAS_SSE          KMCMAKE_X86_HAS_SSE2
# AI:   KMCMAKE_X86_HAS_SSE3         KMCMAKE_X86_HAS_SSSE3
# AI:   KMCMAKE_X86_HAS_SSE4_1       KMCMAKE_X86_HAS_SSE4_2
# AI:   KMCMAKE_X86_HAS_AVX          KMCMAKE_X86_HAS_AVX2
# AI:   KMCMAKE_X86_HAS_AVX512F      KMCMAKE_X86_HAS_BMI1
# AI:   KMCMAKE_X86_HAS_BMI2         KMCMAKE_X86_HAS_POPCNT
# AI:   KMCMAKE_X86_HAS_FMA          KMCMAKE_X86_HAS_F16C
# AI:   KMCMAKE_X86_HAS_LZCNT        KMCMAKE_X86_HAS_MOVBE
# AI:
# AI: Also exports flag variables for each feature:
# AI:   SSE1_FLAG, SSE2_FLAG, ..., AVX512F_FLAG, BMI1_FLAG, etc.
# AI:
# AI: Prerequisites:
# AI:   include(CheckCXXSourceRuns)
# AI:   kmcmake_detect_simd function from default_setting.cmake or equivalent

# AI: --- Predefine all detection result variables ---
set(KMCMAKE_X86_HAS_SSE FALSE)
set(KMCMAKE_X86_HAS_SSE2 FALSE)
set(KMCMAKE_X86_HAS_SSE3 FALSE)
set(KMCMAKE_X86_HAS_SSSE3 FALSE)
set(KMCMAKE_X86_HAS_SSE4_1 FALSE)
set(KMCMAKE_X86_HAS_SSE4_2 FALSE)
set(KMCMAKE_X86_HAS_AVX FALSE)
set(KMCMAKE_X86_HAS_AVX2 FALSE)
set(KMCMAKE_X86_HAS_AVX512F FALSE)
set(KMCMAKE_X86_HAS_BMI1 FALSE)
set(KMCMAKE_X86_HAS_BMI2 FALSE)
set(KMCMAKE_X86_HAS_POPCNT FALSE)
set(KMCMAKE_X86_HAS_FMA FALSE)
set(KMCMAKE_X86_HAS_F16C FALSE)
set(KMCMAKE_X86_HAS_LZCNT FALSE)
set(KMCMAKE_X86_HAS_MOVBE FALSE)
# AI: --- Compiler flag definitions per feature ---
# MSVC x64: SSE/SSE2 are the baseline ISA; /arch:SSE is invalid and /arch:SSE2
# is unnecessary (D9002 on older toolchains). Only emit /arch:AVX and above.
# Use MSVC (not WIN32) so MinGW-w64 keeps GCC-style -m* flags.
if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SSE1_FLAG "")
        set(SSE2_FLAG "")
    else()
        set(SSE1_FLAG "/arch:SSE")
        set(SSE2_FLAG "/arch:SSE2")
    endif()
    set(SSE3_FLAG "")
    set(SSSE3_FLAG "")
    set(SSE4_1_FLAG "")
    set(SSE4_2_FLAG "")
    set(AVX_FLAG "/arch:AVX")
    set(AVX2_FLAG "/arch:AVX2")
    # No dedicated MSVC /arch for these; they ride with AVX/AVX2 codegen.
    set(BMI1_FLAG "")
    set(BMI2_FLAG "")
    set(POPCNT_FLAG "")
    set(FMA_FLAG "")
    set(LZCNT_FLAG "")
    set(F16C_FLAG "")
    set(MOVBE_FLAG "")
    set(AVX512_FLAG "/arch:AVX512")
else()
    set(SSE1_FLAG "-msse")
    set(SSE2_FLAG "-msse2")
    set(SSE3_FLAG "-msse3")
    set(SSSE3_FLAG "-mssse3")
    set(SSE4_1_FLAG "-msse4.1")
    set(SSE4_2_FLAG "-msse4.2")
    set(AVX_FLAG "-mavx")
    set(AVX2_FLAG "-mavx2")
    set(BMI1_FLAG "-mbmi")
    set(BMI2_FLAG "-mbmi2")
    set(POPCNT_FLAG "-mpopcnt")
    set(FMA_FLAG "-mfma")
    set(F16C_FLAG "-mf16c")
    set(LZCNT_FLAG "-mlzcnt")
    set(MOVBE_FLAG "-mmovbe")
    set(AVX512F_FLAG "-mavx512f")
endif()

# AI: --- Detection code snippets ---
SET(SSE1_CODE "
  #include <xmmintrin.h>
  int main() {
    __m128 a;
    float vals[4] = {0,0,0,0};
    a = _mm_loadu_ps(vals);
    return 0;
  }")

SET(SSE2_CODE "
  #include <emmintrin.h>
  int main() {
    __m128d a;
    double vals[2] = {0,0};
    a = _mm_loadu_pd(vals);
    return 0;
  }")

SET(SSE3_CODE "
#include <pmmintrin.h>
int main() {
    __m128 u, v;
    u = _mm_set1_ps(0.0f);
    v = _mm_moveldup_ps(u);
    return 0;
}")

SET(SSSE3_CODE "
  #include <tmmintrin.h>
  const double v = 0;
  int main() {
    __m128i a = _mm_setzero_si128();
    __m128i b = _mm_abs_epi32(a);
    return 0;
  }")

SET(SSE4_1_CODE "
  #include <smmintrin.h>
  int main () {
     __m128i a = _mm_setzero_si128();
     __m128i b = _mm_setzero_si128();
    __m128i res = _mm_max_epi8(a, b);
    return 0;
  }")

SET(SSE4_2_CODE "
  #include <nmmintrin.h>
  int main() {
      __m128i a = _mm_setzero_si128();
      __m128i b = _mm_setzero_si128();
      __m128i c = _mm_cmpgt_epi64(a, b);
    return 0;
  }")

SET(AVX_CODE "
#if !defined __AVX__
#error \"__AVX__ define is missing\"
#endif
#include <immintrin.h>
void test() {
    __m256 a = _mm256_set1_ps(0.0f);
}
int main() { return 0; }")

SET(AVX2_CODE "
#if !defined __AVX2__
#error \"__AVX2__ define is missing\"
#endif
#include <immintrin.h>
void test() {
    int data[8] = {0,0,0,0, 0,0,0,0};
    __m256i a = _mm256_loadu_si256((const __m256i *)data);
    __m256i b = _mm256_bslli_epi128(a, 1);
}
int main() { return 0; }")

SET(AVX512_CODE "
#if defined __AVX512__ || defined __AVX512F__
#include <immintrin.h>
void test() {
    __m512i zmm = _mm512_setzero_si512();
#if defined __GNUC__ && defined __x86_64__
    asm volatile (\"\" : : : \"zmm16\", \"zmm17\", \"zmm18\", \"zmm19\");
#endif
}
#else
#error \"AVX512 is not supported\"
#endif
int main() { return 0; }")

SET(POPCNT_CODE "
#include <immintrin.h>
int main() {
    unsigned int x = 0x12345678;
    #if defined(_MSC_VER)
    unsigned int cnt = __popcnt(x);
    #else
    unsigned int cnt = _mm_popcnt_u32(x);
    #endif
    (void)cnt;
    return 0;
}")

SET(BMI1_CODE "
#if defined(_MSC_VER) && !defined(_M_BMI)
#error \"BMI1 not supported by MSVC\"
#elif defined(__GNUC__) && !defined(__BMI__)
#error \"BMI1 not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    unsigned int a = 0xFFFF0000;
    unsigned int b = 0x0000FFFF;
    #if defined(_MSC_VER)
    unsigned int res = _andn_u32(a, b);
    #else
    unsigned int res = __builtin_ia32_andn_u32(a, b);
    #endif
    (void)res;
    return 0;
}")

SET(BMI2_CODE "
#if defined(_MSC_VER) && !defined(_M_BMI2)
#error \"BMI2 not supported by MSVC\"
#elif defined(__GNUC__) && !defined(__BMI2__)
#error \"BMI2 not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    unsigned int x = 0x12345678;
    #if defined(_MSC_VER)
    unsigned int res = _bzhi_u32(x, 16);
    #else
    unsigned int res = __builtin_ia32_bzhi_u32(x, 16);
    #endif
    (void)res;
    return 0;
}")

SET(FMA_CODE "
#if defined(_MSC_VER) && !defined(__AVX2__)
#error \"FMA requires AVX2 support in MSVC\"
#elif defined(__GNUC__) && !defined(__FMA__)
#error \"FMA not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    __m256 a = _mm256_set1_ps(1.5f);
    __m256 b = _mm256_set1_ps(2.5f);
    __m256 c = _mm256_set1_ps(3.5f);
    __m256 res = _mm256_fmadd_ps(a, b, c);
    float out[8];
    _mm256_storeu_ps(out, res);
    return (out[0] > 0.0f) ? 0 : 1;
}")

SET(F16C_CODE "
#if defined(_MSC_VER) && !defined(__AVX__)
#error \"F16C requires AVX support in MSVC\"
#elif defined(__GNUC__) && !defined(__F16C__)
#error \"F16C not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    __m128i fp16_data = _mm_set_epi16(0x3C00, 0x4000, 0x4200, 0x4400,
                                       0x4600, 0x4800, 0x4A00, 0x4C00);
    __m256 fp32_data = _mm256_cvtph_ps(fp16_data);
    __m128i fp16_result = _mm256_cvtps_ph(fp32_data, _MM_FROUND_TO_NEAREST_INT);
    return _mm_extract_epi16(fp16_result, 0) == 0x3C00 ? 0 : 1;
}")

SET(LZCNT_CODE "
#if defined(_MSC_VER) && !defined(_M_AMD64) && !defined(_M_IX86)
#error \"LZCNT requires x86/x86_64 architecture in MSVC\"
#elif defined(__GNUC__) && !defined(__LZCNT__)
#error \"LZCNT not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    unsigned int x = 0x80000000;
    unsigned int y = 0x00000001;
#if defined(_MSC_VER)
    unsigned int cnt_x = _lzcnt_u32(x);
    unsigned int cnt_y = _lzcnt_u32(y);
#else
    unsigned int cnt_x = __builtin_clz(x);
    unsigned int cnt_y = __builtin_clz(y);
#endif
    return (cnt_x == 0 && cnt_y == 31) ? 0 : 1;
}")

SET(MOVBE_CODE "
#if defined(_MSC_VER) && !defined(__SSE2__)
#error \"MOVBE requires SSE2 support in MSVC\"
#elif defined(__GNUC__) && !defined(__MOVBE__)
#error \"MOVBE not supported by GCC/Clang\"
#endif
#include <immintrin.h>
int main() {
    unsigned int le_data = 0x12345678;
    unsigned int be_data = _mm_movemask_epi8(_mm_loadu_si128((__m128i*)&le_data));
    unsigned int converted = _mm_extract_epi32(_mm_movbe_epi32(_mm_set_epi32(0,0,0,le_data)), 0);
    return converted == 0x78563412 ? 0 : 1;
}")

# AI: --- Run detection for each feature ---
kmcmake_detect_simd("${SSE1_CODE}"    "${SSE1_FLAG}"    KMCMAKE_X86_HAS_SSE)
kmcmake_detect_simd("${SSE2_CODE}"    "${SSE2_FLAG}"    KMCMAKE_X86_HAS_SSE2)
kmcmake_detect_simd("${SSE3_CODE}"    "${SSE3_FLAG}"    KMCMAKE_X86_HAS_SSE3)
kmcmake_detect_simd("${SSSE3_CODE}"   "${SSSE3_FLAG}"   KMCMAKE_X86_HAS_SSSE3)
kmcmake_detect_simd("${SSE4_1_CODE}"  "${SSE4_1_FLAG}"  KMCMAKE_X86_HAS_SSE4_1)
kmcmake_detect_simd("${SSE4_2_CODE}"  "${SSE4_2_FLAG}"  KMCMAKE_X86_HAS_SSE4_2)
kmcmake_detect_simd("${AVX_CODE}"     "${AVX_FLAG}"     KMCMAKE_X86_HAS_AVX)
kmcmake_detect_simd("${AVX2_CODE}"    "${AVX2_FLAG}"    KMCMAKE_X86_HAS_AVX2)
kmcmake_detect_simd("${POPCNT_CODE}"  "${POPCNT_FLAG}"  KMCMAKE_X86_HAS_POPCNT)
kmcmake_detect_simd("${BMI1_CODE}"   "${BMI1_FLAG}"    KMCMAKE_X86_HAS_BMI1)
kmcmake_detect_simd("${BMI2_CODE}"    "${BMI2_FLAG}"    KMCMAKE_X86_HAS_BMI2)
kmcmake_detect_simd("${FMA_CODE}"     "${FMA_FLAG}"     KMCMAKE_X86_HAS_FMA)
kmcmake_detect_simd("${F16C_CODE}"    "${F16C_FLAG}"    KMCMAKE_X86_HAS_F16C)
kmcmake_detect_simd("${LZCNT_CODE}"   "${LZCNT_FLAG}"   KMCMAKE_X86_HAS_LZCNT)
kmcmake_detect_simd("${MOVBE_CODE}"   "${MOVBE_FLAG}"   KMCMAKE_X86_HAS_MOVBE)
if (MSVC)
    kmcmake_detect_simd("${AVX512_CODE}" "${AVX512_FLAG}" KMCMAKE_X86_HAS_AVX512F)
else()
    kmcmake_detect_simd("${AVX512_CODE}" "${AVX512F_FLAG}" KMCMAKE_X86_HAS_AVX512F)
endif()

kmcmake_print("x86 SIMD detection complete")
kmcmake_print_list_label("x86 HAS_SSE"    KMCMAKE_X86_HAS_SSE)
kmcmake_print_list_label("x86 HAS_SSE2"   KMCMAKE_X86_HAS_SSE2)
kmcmake_print_list_label("x86 HAS_SSE3"   KMCMAKE_X86_HAS_SSE3)
kmcmake_print_list_label("x86 HAS_SSSE3"  KMCMAKE_X86_HAS_SSSE3)
kmcmake_print_list_label("x86 HAS_SSE4_1" KMCMAKE_X86_HAS_SSE4_1)
kmcmake_print_list_label("x86 HAS_SSE4_2" KMCMAKE_X86_HAS_SSE4_2)
kmcmake_print_list_label("x86 HAS_AVX"    KMCMAKE_X86_HAS_AVX)
kmcmake_print_list_label("x86 HAS_AVX2"   KMCMAKE_X86_HAS_AVX2)
kmcmake_print_list_label("x86 HAS_AVX512F" KMCMAKE_X86_HAS_AVX512F)
kmcmake_print_list_label("x86 HAS_BMI1"   KMCMAKE_X86_HAS_BMI1)
kmcmake_print_list_label("x86 HAS_BMI2"   KMCMAKE_X86_HAS_BMI2)
kmcmake_print_list_label("x86 HAS_FMA"   KMCMAKE_X86_HAS_FMA)