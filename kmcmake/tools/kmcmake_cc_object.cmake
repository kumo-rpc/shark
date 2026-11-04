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
###################################################################################################
# kmcmake_cc_object
#
# Thin wrap around add_library(OBJECT). This is the compile-unit primitive:
# one target => one compile-option set (e.g. one SIMD tier).
#
# Consumers that only use $<TARGET_OBJECTS:...> do NOT get usage requirements.
# Prefer also target_link_libraries(consumer <PUBLIC|PRIVATE> obj) so
# INTERFACE include/defines/links propagate (CMake standard behavior).
#
# Example:
#   kmcmake_cc_object(
#       NAME foo_avx2
#       NAMESPACE myproj
#       SOURCES a.cc b.cc
#       INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include
#       PINCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
#       DEFINES FOO_AVX2=1
#       CXXOPTS ${KMCMAKE_CXX_OPTIONS}
#       SIMD_LEVEL AVX2          # optional: append arch flags for this tier
#       LINKS some::iface        # PUBLIC usage (propagates to linkers of this OBJECT)
#       PLINKS Threads::Threads  # PRIVATE (compile this OBJECT only)
#   )
#   # Alias: myproj::foo_avx2
#
# Optional PUBLIC: install + export the OBJECT target (IMPORTED_OBJECTS +
# INTERFACE includes). Default is build-tree only.
################################################################################

# Resolve compiler flags for a SIMD tier using detect outputs (*_FLAG / HAS_*).
# When level matches KMCMAKE_RUNTIME_SIMD_LEVEL, reuse KMCMAKE_ARCH_OPTION.
function(_kmcmake_cc_object_resolve_simd_flags out_var level)
    string(TOUPPER "${level}" level)
    if(level STREQUAL "" OR level STREQUAL "NONE")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(DEFINED KMCMAKE_RUNTIME_SIMD_LEVEL)
        string(TOUPPER "${KMCMAKE_RUNTIME_SIMD_LEVEL}" _global)
        if(level STREQUAL "${_global}" AND DEFINED KMCMAKE_ARCH_OPTION)
            set(${out_var} "${KMCMAKE_ARCH_OPTION}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(_levels NONE SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2 AVX AVX2 AVX512)
    list(FIND _levels "${level}" _idx)
    if(_idx EQUAL -1)
        kmcmake_warn(
            "kmcmake_cc_object: SIMD_LEVEL=${level} not in x86 ladder; "
            "falling back to KMCMAKE_ARCH_OPTION")
        set(${out_var} "${KMCMAKE_ARCH_OPTION}" PARENT_SCOPE)
        return()
    endif()

    set(_flags "")
    macro(_kmcmake_obj_try_flag feature flag_var)
        if(KMCMAKE_X86_HAS_${feature} AND ${flag_var})
            list(APPEND _flags ${${flag_var}})
        endif()
    endmacro()

    if(_idx GREATER_EQUAL 1)
        _kmcmake_obj_try_flag(SSE SSE1_FLAG)
    endif()
    if(_idx GREATER_EQUAL 2)
        _kmcmake_obj_try_flag(SSE2 SSE2_FLAG)
    endif()
    if(_idx GREATER_EQUAL 3)
        _kmcmake_obj_try_flag(SSE3 SSE3_FLAG)
    endif()
    if(_idx GREATER_EQUAL 4)
        _kmcmake_obj_try_flag(SSSE3 SSSE3_FLAG)
    endif()
    if(_idx GREATER_EQUAL 5)
        _kmcmake_obj_try_flag(SSE4_1 SSE4_1_FLAG)
    endif()
    if(_idx GREATER_EQUAL 6)
        _kmcmake_obj_try_flag(SSE4_2 SSE4_2_FLAG)
    endif()
    if(_idx GREATER_EQUAL 7)
        _kmcmake_obj_try_flag(AVX AVX_FLAG)
    endif()
    if(_idx GREATER_EQUAL 8)
        _kmcmake_obj_try_flag(AVX2 AVX2_FLAG)
        _kmcmake_obj_try_flag(BMI1 BMI1_FLAG)
        _kmcmake_obj_try_flag(BMI2 BMI2_FLAG)
        _kmcmake_obj_try_flag(FMA FMA_FLAG)
        _kmcmake_obj_try_flag(F16C F16C_FLAG)
        _kmcmake_obj_try_flag(POPCNT POPCNT_FLAG)
        _kmcmake_obj_try_flag(LZCNT LZCNT_FLAG)
        _kmcmake_obj_try_flag(MOVBE MOVBE_FLAG)
    endif()
    if(_idx GREATER_EQUAL 9)
        if(MSVC)
            _kmcmake_obj_try_flag(AVX512F AVX512_FLAG)
        else()
            _kmcmake_obj_try_flag(AVX512F AVX512F_FLAG)
        endif()
    endif()

    list(REMOVE_DUPLICATES _flags)
    set(${out_var} "${_flags}" PARENT_SCOPE)
endfunction()

function(kmcmake_cc_object)
    set(options
            EXCLUDE_SYSTEM
            UNITY
            PUBLIC
            INTERNAL
    )
    set(args NAME
            NAMESPACE
            SIMD_LEVEL
    )

    set(list_args
            DEPS
            SOURCES
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            PLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_OBJECT
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${KMCMAKE_CC_OBJECT_NAME}" STREQUAL "")
        get_filename_component(KMCMAKE_CC_OBJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" KMCMAKE_CC_OBJECT_NAME ${KMCMAKE_CC_OBJECT_NAME})
        kmcmake_print(" Object, NAME argument not provided. Using folder name:  ${KMCMAKE_CC_OBJECT_NAME}")
    endif ()

    if (NOT KMCMAKE_CC_OBJECT_NAMESPACE OR "${KMCMAKE_CC_OBJECT_NAMESPACE}" STREQUAL "")
        set(KMCMAKE_CC_OBJECT_NAMESPACE ${PROJECT_NAME})
        kmcmake_print(" Object, NAMESPACE argument not provided. Using alias:  ${KMCMAKE_CC_OBJECT_NAMESPACE}::${KMCMAKE_CC_OBJECT_NAME}")
    endif ()

    if ("${KMCMAKE_CC_OBJECT_SOURCES}" STREQUAL "")
        kmcmake_error("kmcmake_cc_object(${KMCMAKE_CC_OBJECT_NAME}): SOURCES is required")
    endif ()

    set(_obj_cxxopts ${KMCMAKE_CC_OBJECT_CXXOPTS})
    if(KMCMAKE_CC_OBJECT_SIMD_LEVEL)
        _kmcmake_cc_object_resolve_simd_flags(_simd_flags "${KMCMAKE_CC_OBJECT_SIMD_LEVEL}")
        if(_simd_flags)
            list(APPEND _obj_cxxopts ${_simd_flags})
            list(REMOVE_DUPLICATES _obj_cxxopts)
        endif()
    endif()

    kmcmake_raw("-----------------------------------")
    if (KMCMAKE_CC_OBJECT_PUBLIC)
        set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_OBJECT_NAMESPACE}::${KMCMAKE_CC_OBJECT_NAME}  OBJECT PUBLIC")
    else ()
        set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_OBJECT_NAMESPACE}::${KMCMAKE_CC_OBJECT_NAME}  OBJECT")
    endif ()

    set(${KMCMAKE_CC_OBJECT_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (KMCMAKE_CC_OBJECT_EXCLUDE_SYSTEM)
        set(${KMCMAKE_CC_OBJECT_NAME}_INCLUDE_SYSTEM "")
    endif ()

    kmcmake_print_label("Create Library" "${KMCMAKE_LIB_INFO}")
    kmcmake_raw("-----------------------------------")
    if (VERBOSE_KMCMAKE_BUILD)
        kmcmake_print_list_label("Sources" KMCMAKE_CC_OBJECT_SOURCES)
        kmcmake_print_list_label("Deps" KMCMAKE_CC_OBJECT_DEPS)
        kmcmake_print_list_label("COPTS" KMCMAKE_CC_OBJECT_COPTS)
        kmcmake_print_list_label("CXXOPTS" _obj_cxxopts)
        kmcmake_print_list_label("CUOPTS" KMCMAKE_CC_OBJECT_CUOPTS)
        kmcmake_print_list_label("Defines" KMCMAKE_CC_OBJECT_DEFINES)
        kmcmake_print_list_label("Includes" KMCMAKE_CC_OBJECT_INCLUDES)
        kmcmake_print_list_label("Private Includes" KMCMAKE_CC_OBJECT_PINCLUDES)
        kmcmake_print_list_label("Links" KMCMAKE_CC_OBJECT_LINKS)
        kmcmake_print_list_label("Private Links" KMCMAKE_CC_OBJECT_PLINKS)
        if(KMCMAKE_CC_OBJECT_SIMD_LEVEL)
            kmcmake_print_label("SIMD_LEVEL" "${KMCMAKE_CC_OBJECT_SIMD_LEVEL}")
        endif()
        kmcmake_raw("-----------------------------------")
    endif ()

    add_library(${KMCMAKE_CC_OBJECT_NAME} OBJECT
            ${KMCMAKE_CC_OBJECT_SOURCES}
            ${KMCMAKE_CC_OBJECT_HEADERS}
    )
    add_library(${KMCMAKE_CC_OBJECT_NAMESPACE}::${KMCMAKE_CC_OBJECT_NAME}
            ALIAS ${KMCMAKE_CC_OBJECT_NAME})

    if (KMCMAKE_CC_OBJECT_UNITY)
        set_target_properties(${KMCMAKE_CC_OBJECT_NAME} PROPERTIES
                UNITY_BUILD ON
                UNITY_BUILD_BATCH_SIZE 20
        )
    endif ()

    if (KMCMAKE_CC_OBJECT_DEPS)
        add_dependencies(${KMCMAKE_CC_OBJECT_NAME} ${KMCMAKE_CC_OBJECT_DEPS})
    endif ()

    # PIC so objects can be linked into shared libs later.
    set_property(TARGET ${KMCMAKE_CC_OBJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)

    # Compile flags / private includes: self-compile only (not product export).
    target_compile_options(${KMCMAKE_CC_OBJECT_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:${KMCMAKE_CC_OBJECT_COPTS}>)
    target_compile_options(${KMCMAKE_CC_OBJECT_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${_obj_cxxopts}>)
    target_compile_options(${KMCMAKE_CC_OBJECT_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:${KMCMAKE_CC_OBJECT_CUOPTS}>)

    kmcmake_default_public_includes(_obj_pub_incs ${KMCMAKE_CC_OBJECT_INCLUDES})
    # Self-compile needs public paths + PINCLUDES.
    # INTERNAL (library compile unit): caller puts compile paths in PINCLUDES only.
    if (KMCMAKE_CC_OBJECT_INTERNAL)
        target_include_directories(${KMCMAKE_CC_OBJECT_NAME}
                ${${KMCMAKE_CC_OBJECT_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${KMCMAKE_CC_OBJECT_PINCLUDES}
        )
    else ()
        target_include_directories(${KMCMAKE_CC_OBJECT_NAME}
                ${${KMCMAKE_CC_OBJECT_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${_obj_pub_incs}
                ${KMCMAKE_CC_OBJECT_PINCLUDES}
        )
    endif ()
    if (KMCMAKE_CC_OBJECT_DEFINES)
        target_compile_definitions(${KMCMAKE_CC_OBJECT_NAME} PRIVATE
                ${KMCMAKE_CC_OBJECT_DEFINES})
    endif ()

    # Product export: INTERFACE_* via set_property (skipped for INTERNAL).
    if (NOT KMCMAKE_CC_OBJECT_INTERNAL)
        set(_obj_prod_args INCLUDES ${_obj_pub_incs})
        if (KMCMAKE_CC_OBJECT_DEFINES)
            list(APPEND _obj_prod_args DEFINES ${KMCMAKE_CC_OBJECT_DEFINES})
        endif ()
        kmcmake_export_product_properties(${KMCMAKE_CC_OBJECT_NAME} ${_obj_prod_args})
    endif ()

    # Dependencies
    if (KMCMAKE_CC_OBJECT_LINKS)
        target_link_libraries(${KMCMAKE_CC_OBJECT_NAME} PUBLIC ${KMCMAKE_CC_OBJECT_LINKS})
    endif ()
    if (KMCMAKE_CC_OBJECT_PLINKS)
        target_link_libraries(${KMCMAKE_CC_OBJECT_NAME} PRIVATE ${KMCMAKE_CC_OBJECT_PLINKS})
    endif ()

    if (KMCMAKE_CC_OBJECT_PUBLIC AND NOT KMCMAKE_CC_OBJECT_INTERNAL)
        kmcmake_install_library_target(${KMCMAKE_CC_OBJECT_NAME} OBJECT)
    endif ()

    foreach (arg IN LISTS KMCMAKE_CC_OBJECT_UNPARSED_ARGUMENTS)
        message(WARNING "kmcmake_cc_object: unparsed argument: ${arg}")
    endforeach ()

endfunction()
