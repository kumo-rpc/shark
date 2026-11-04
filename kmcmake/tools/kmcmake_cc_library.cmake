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
# kmcmake_cc_library
#
# Orchestrates (separate steps):
#   1. SOURCES → kmcmake_cc_object (INTERNAL compile unit)
#   2. OBJECTS → kmcmake_ar_static  (pack .a + deps)
#   3. optional → kmcmake_ar_shared (pack .so + deps)
#   4. export props → kmcmake_export_product_properties
#   5. PUBLIC → kmcmake_install_library_target
#
# LINKS / PLINKS are forwarded to the INTERNAL object (compile usage) and to
# ar_static / ar_shared (link). WLINKS are pack-only.
#
# Public API unchanged:
#   - <namespace>::<name>_static
#   - <namespace>::<name>  (shared alias, only if SHARE)
###################################################################################################

function(kmcmake_cc_library)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
            UNITY
            SHARE
    )
    set(args NAME
            NAMESPACE
    )

    set(list_args
            DEPS
            SOURCES
            OBJECTS
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            PLINKS
            WLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_LIB
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${KMCMAKE_CC_LIB_NAME}" STREQUAL "")
        get_filename_component(KMCMAKE_CC_LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" KMCMAKE_CC_LIB_NAME ${KMCMAKE_CC_LIB_NAME})
        kmcmake_print(" Library, NAME argument not provided. Using folder name:  ${KMCMAKE_CC_LIB_NAME}")
    endif ()

    if ("${KMCMAKE_CC_LIB_NAMESPACE}" STREQUAL "")
        set(KMCMAKE_CC_LIB_NAMESPACE ${PROJECT_NAME})
        kmcmake_print(" Library, NAMESPACE argument not provided. Using target alias:  ${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}")
    endif ()

    kmcmake_raw("-----------------------------------")
    if (KMCMAKE_CC_LIB_PUBLIC)
        if (KMCMAKE_CC_LIB_SHARE)
            set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  SHARED&STATIC PUBLIC")
        else ()
            set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  STATIC PUBLIC")
        endif ()
    else ()
        if (KMCMAKE_CC_LIB_SHARE)
            set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  SHARED&STATIC INTERNAL")
        else ()
            set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  STATIC INTERNAL")
        endif ()
    endif ()

    kmcmake_print_label("Create Library" "${KMCMAKE_LIB_INFO}")
    kmcmake_raw("-----------------------------------")
    if (VERBOSE_KMCMAKE_BUILD)
        kmcmake_print_list_label("Sources" KMCMAKE_CC_LIB_SOURCES)
        kmcmake_print_list_label("Objects" KMCMAKE_CC_LIB_OBJECTS)
        kmcmake_print_list_label("Deps" KMCMAKE_CC_LIB_DEPS)
        kmcmake_print_list_label("COPTS" KMCMAKE_CC_LIB_COPTS)
        kmcmake_print_list_label("CXXOPTS" KMCMAKE_CC_LIB_CXXOPTS)
        kmcmake_print_list_label("CUOPTS" KMCMAKE_CC_LIB_CUOPTS)
        kmcmake_print_list_label("Defines" KMCMAKE_CC_LIB_DEFINES)
        kmcmake_print_list_label("Includes" KMCMAKE_CC_LIB_INCLUDES)
        kmcmake_print_list_label("Private Includes" KMCMAKE_CC_LIB_PINCLUDES)
        kmcmake_print_list_label("Links" KMCMAKE_CC_LIB_LINKS)
        kmcmake_print_list_label("Private Links" KMCMAKE_CC_LIB_PLINKS)
        kmcmake_raw("-----------------------------------")
    endif ()

    # --- 1) compile SOURCES via cc_object (INTERNAL: no product export) ---
    set(_lib_objects ${KMCMAKE_CC_LIB_OBJECTS})
    if (KMCMAKE_CC_LIB_SOURCES)
        set(_obj_name ${KMCMAKE_CC_LIB_NAME}_OBJECT)
        kmcmake_default_public_includes(_compile_incs ${KMCMAKE_CC_LIB_INCLUDES})

        set(_obj_call NAME ${_obj_name} NAMESPACE ${KMCMAKE_CC_LIB_NAMESPACE} INTERNAL)
        if (KMCMAKE_CC_LIB_EXCLUDE_SYSTEM)
            list(APPEND _obj_call EXCLUDE_SYSTEM)
        endif ()
        if (KMCMAKE_CC_LIB_UNITY)
            list(APPEND _obj_call UNITY)
        endif ()
        list(APPEND _obj_call SOURCES ${KMCMAKE_CC_LIB_SOURCES})
        if (KMCMAKE_CC_LIB_HEADERS)
            list(APPEND _obj_call HEADERS ${KMCMAKE_CC_LIB_HEADERS})
        endif ()
        list(APPEND _obj_call PINCLUDES ${_compile_incs} ${KMCMAKE_CC_LIB_PINCLUDES})
        if (KMCMAKE_CC_LIB_COPTS)
            list(APPEND _obj_call COPTS ${KMCMAKE_CC_LIB_COPTS})
        endif ()
        if (KMCMAKE_CC_LIB_CXXOPTS)
            list(APPEND _obj_call CXXOPTS ${KMCMAKE_CC_LIB_CXXOPTS})
        endif ()
        if (KMCMAKE_CC_LIB_CUOPTS)
            list(APPEND _obj_call CUOPTS ${KMCMAKE_CC_LIB_CUOPTS})
        endif ()
        if (KMCMAKE_CC_LIB_DEPS)
            list(APPEND _obj_call DEPS ${KMCMAKE_CC_LIB_DEPS})
        endif ()
        # Usage requirements needed while compiling SOURCES (includes/defines).
        if (KMCMAKE_CC_LIB_LINKS)
            list(APPEND _obj_call LINKS ${KMCMAKE_CC_LIB_LINKS})
        endif ()
        if (KMCMAKE_CC_LIB_PLINKS)
            list(APPEND _obj_call PLINKS ${KMCMAKE_CC_LIB_PLINKS})
        endif ()
        kmcmake_cc_object(${_obj_call})

        if (KMCMAKE_CC_LIB_DEFINES)
            target_compile_definitions(${_obj_name} PRIVATE ${KMCMAKE_CC_LIB_DEFINES})
        endif ()
        if (KMCMAKE_CC_LIB_HEADERS)
            target_precompile_headers(${_obj_name} PUBLIC
                    <vector>
                    <string>
                    ${KMCMAKE_CC_LIB_HEADERS}
            )
        endif ()

        list(APPEND _lib_objects ${_obj_name})
    endif ()

    if (NOT _lib_objects)
        kmcmake_error("no source or object give to the library ${KMCMAKE_CC_LIB_NAME}")
    endif ()

    # --- 2/3) pack static / shared (deps only) ---
    set(_ar_call
            NAME ${KMCMAKE_CC_LIB_NAME}
            NAMESPACE ${KMCMAKE_CC_LIB_NAMESPACE}
            OBJECTS ${_lib_objects}
    )
    if (KMCMAKE_CC_LIB_DEPS)
        list(APPEND _ar_call DEPS ${KMCMAKE_CC_LIB_DEPS})
    endif ()
    if (KMCMAKE_CC_LIB_LINKS)
        list(APPEND _ar_call LINKS ${KMCMAKE_CC_LIB_LINKS})
    endif ()
    if (KMCMAKE_CC_LIB_PLINKS)
        list(APPEND _ar_call PLINKS ${KMCMAKE_CC_LIB_PLINKS})
    endif ()
    if (KMCMAKE_CC_LIB_WLINKS)
        list(APPEND _ar_call WLINKS ${KMCMAKE_CC_LIB_WLINKS})
    endif ()

    kmcmake_ar_static(${_ar_call} OUTPUT_NAME ${KMCMAKE_CC_LIB_NAME})

    if (KMCMAKE_CC_LIB_SHARE)
        kmcmake_ar_shared(${_ar_call})
    endif ()

    # --- 4) product export properties (INTERFACE_*) ---
    # Merge usage from user OBJECTS only (not the INTERNAL <name>_OBJECT compile unit).
    kmcmake_default_public_includes(_pub_incs ${KMCMAKE_CC_LIB_INCLUDES})
    set(_export_args INCLUDES ${_pub_incs})
    if (KMCMAKE_CC_LIB_OBJECTS)
        list(APPEND _export_args OBJECTS ${KMCMAKE_CC_LIB_OBJECTS})
    endif ()
    if (KMCMAKE_CC_LIB_DEFINES)
        list(APPEND _export_args DEFINES ${KMCMAKE_CC_LIB_DEFINES})
    endif ()

    kmcmake_export_product_properties(${KMCMAKE_CC_LIB_NAME}_static ${_export_args})
    if (KMCMAKE_CC_LIB_SHARE)
        kmcmake_export_product_properties(${KMCMAKE_CC_LIB_NAME}_shared ${_export_args})
    endif ()

    # --- 5) install ---
    if (KMCMAKE_CC_LIB_PUBLIC)
        kmcmake_install_library_target(${KMCMAKE_CC_LIB_NAME}_static STATIC)
        if (KMCMAKE_CC_LIB_SHARE)
            kmcmake_install_library_target(${KMCMAKE_CC_LIB_NAME}_shared SHARED)
        endif ()
    endif ()

    foreach (arg IN LISTS KMCMAKE_CC_LIB_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()
endfunction()
