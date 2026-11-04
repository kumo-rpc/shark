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
# AI: Compiler flags definitions and compiler detection.
# AI: Exports the following variables for users to consume:
# AI:   KMCMAKE_BASE_CXX_FLAGS   - base compiler flags for current compiler
# AI:   KMCMAKE_BASE_LINK_FLAGS  - linker flags for current compiler
# AI:
# AI: Debug/release mode flags (CMAKE_CXX_FLAGS_*).
# AI: CMAKE_CXX_STANDARD / CMAKE_BUILD_TYPE defaults live in cmake/*_cxx_config.cmake.

# ============================================================
# Per-compiler flag definitions
# ============================================================
set(_KMCMAKE_CLANG_CL_FLAGS
    "/W3"
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
    "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
)

set(_KMCMAKE_GCC_FLAGS
    "-Wall"
    "-Wextra"
    "-Wno-cast-qual"
    "-Wconversion-null"
    "-Wformat-security"
    "-Woverlength-strings"
    "-Wpointer-arith"
    "-Wno-undef"
    "-Wunused-local-typedefs"
    "-Wunused-result"
    "-Wvarargs"
    "-Wno-attributes"
    "-Wno-implicit-fallthrough"
    "-Wno-unused-parameter"
    "-Wno-unused-function"
    "-Wwrite-strings"
    "-Wclass-memaccess"
    "-Wno-sign-compare"
    "-DNOMINMAX"
)

set(_KMCMAKE_LLVM_FLAGS
    "-Wall"
    "-Wextra"
    "-Wno-cast-qual"
    "-Wno-conversion"
    "-Wno-sign-compare"
    "-Wfloat-overflow-conversion"
    "-Wfloat-zero-conversion"
    "-Wfor-loop-analysis"
    "-Wformat-security"
    "-Wgnu-redeclared-enum"
    "-Winfinite-recursion"
    "-Wliteral-conversion"
    "-Wmissing-declarations"
    "-Woverlength-strings"
    "-Wpointer-arith"
    "-Wself-assign"
    "-Wno-shadow"
    "-Wstring-conversion"
    "-Wtautological-overlap-compare"
    "-Wno-undef"
    "-Wuninitialized"
    "-Wunreachable-code"
    "-Wunused-comparison"
    "-Wunused-local-typedefs"
    "-Wunused-result"
    "-Wno-vla"
    "-Wwrite-strings"
    "-Wno-float-conversion"
    "-Wno-implicit-float-conversion"
    "-Wno-implicit-int-float-conversion"
    "-Wno-implicit-int-conversion"
    "-Wno-shorten-64-to-32"
    "-Wno-sign-conversion"
    "-Wno-unused-parameter"
    "-Wno-unused-function"
    "-DNOMINMAX"
)

set(_KMCMAKE_MSVC_FLAGS
    "/W3"
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
    "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
    "/bigobj"
    "/wd4005"
    "/wd4068"
    "/wd4180"
    "/wd4244"
    "/wd4267"
    "/wd4503"
    "/wd4800"
)

set(_KMCMAKE_MSVC_LINKOPTS
    "-ignore:4221"
)

# ============================================================
# Select base flags by compiler ID
# ============================================================
set(KMCMAKE_BASE_LINK_FLAGS "")

if (BUILD_SHARED_LIBS AND MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(KMCMAKE_BASE_CXX_FLAGS "${_KMCMAKE_GCC_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if (MSVC)
        set(KMCMAKE_BASE_CXX_FLAGS "${_KMCMAKE_CLANG_CL_FLAGS}")
        set(KMCMAKE_BASE_LINK_FLAGS "${_KMCMAKE_MSVC_LINKOPTS}")
    else ()
        set(KMCMAKE_BASE_CXX_FLAGS "${_KMCMAKE_LLVM_FLAGS}")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
            set(KMCMAKE_BASE_LINK_FLAGS "-fsanitize=leak")
        endif ()
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(KMCMAKE_BASE_CXX_FLAGS "${_KMCMAKE_MSVC_FLAGS}")
    set(KMCMAKE_BASE_LINK_FLAGS "${_KMCMAKE_MSVC_LINKOPTS}")
else ()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. Building with no default flags")
    set(KMCMAKE_BASE_CXX_FLAGS "")
endif ()

# ============================================================
# C++ standard and build type defaults
# ============================================================
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /DDEBUG" CACHE STRING "Debug mode flags for MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG" CACHE STRING "Release mode flags for MSVC")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /DNDEBUG" CACHE STRING "RelWithDebInfo mode flags for MSVC")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "Debug mode flags for GCC/Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Release mode flags for GCC/Clang")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING "RelWithDebInfo mode flags for GCC/Clang")
endif()

if (DEFINED ENV{KMCMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{KMCMAKE_CXX_FLAGS}")
endif ()

kmcmake_print_list_label("BASE_CXX_FLAGS:" KMCMAKE_BASE_CXX_FLAGS)
