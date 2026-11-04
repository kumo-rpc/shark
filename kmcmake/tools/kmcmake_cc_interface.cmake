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
# kmcmake_cc_interface
#
# Header-only INTERFACE product.
# Product export: set_property INTERFACE_* via kmcmake_export_product_properties
# Dependencies: LINKS → target_link_libraries(INTERFACE) when provided
###################################################################################################

function(kmcmake_cc_interface)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
    )
    set(args NAME
            NAMESPACE
    )

    set(list_args
            HEADERS
            INCLUDES
            COPTS
            CXXOPTS
            CUOPTS
            DEFINES
            LINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_INTERFACE
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${KMCMAKE_CC_INTERFACE_NAME}" STREQUAL "")
        get_filename_component(KMCMAKE_CC_INTERFACE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" KMCMAKE_CC_INTERFACE_NAME ${KMCMAKE_CC_INTERFACE_NAME})
        kmcmake_print(" Library, NAME argument not provided. Using folder name:  ${KMCMAKE_CC_INTERFACE_NAME}")
    endif ()

    if ("${KMCMAKE_CC_INTERFACE_NAMESPACE}" STREQUAL "")
        set(KMCMAKE_CC_INTERFACE_NAMESPACE ${PROJECT_NAME})
        message(" Library, NAMESPACE argument not provided. Using target alias:  ${KMCMAKE_CC_INTERFACE_NAMESPACE}::${KMCMAKE_CC_INTERFACE_NAME}")
    endif ()

    set(${KMCMAKE_CC_INTERFACE_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (KMCMAKE_CC_INTERFACE_EXCLUDE_SYSTEM)
        set(${KMCMAKE_CC_INTERFACE_NAME}_INCLUDE_SYSTEM "")
    endif ()

    kmcmake_raw("-----------------------------------")
    if (KMCMAKE_CC_INTERFACE_PUBLIC)
        set(KMCMAKE_CC_INTERFACE_INFO "${KMCMAKE_CC_INTERFACE_NAMESPACE}::${KMCMAKE_CC_INTERFACE_NAME}  INTERFACE PUBLIC")
    else ()
        set(KMCMAKE_CC_INTERFACE_INFO "${KMCMAKE_CC_INTERFACE_NAMESPACE}::${KMCMAKE_CC_INTERFACE_NAME}  INTERFACE INTERNAL")
    endif ()
    kmcmake_print_label("Create Library" "${KMCMAKE_CC_INTERFACE_INFO}")
    kmcmake_raw("-----------------------------------")
    if (VERBOSE_KMCMAKE_BUILD)
        kmcmake_print_list_label("Headers" KMCMAKE_CC_INTERFACE_HEADERS)
        kmcmake_print_list_label("Includes" KMCMAKE_CC_INTERFACE_INCLUDES)
        kmcmake_raw("-----------------------------------")
    endif ()

    add_library(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE)
    add_library(${KMCMAKE_CC_INTERFACE_NAMESPACE}::${KMCMAKE_CC_INTERFACE_NAME}
            ALIAS ${KMCMAKE_CC_INTERFACE_NAME})

    # INTERFACE compile options (product-facing).
    if (KMCMAKE_CC_INTERFACE_COPTS)
        target_compile_options(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE
                $<$<COMPILE_LANGUAGE:C>:${KMCMAKE_CC_INTERFACE_COPTS}>)
    endif ()
    if (KMCMAKE_CC_INTERFACE_CXXOPTS)
        target_compile_options(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE
                $<$<COMPILE_LANGUAGE:CXX>:${KMCMAKE_CC_INTERFACE_CXXOPTS}>)
    endif ()
    if (KMCMAKE_CC_INTERFACE_CUOPTS)
        target_compile_options(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE
                $<$<COMPILE_LANGUAGE:CUDA>:${KMCMAKE_CC_INTERFACE_CUOPTS}>)
    endif ()

    # Product export
    kmcmake_default_public_includes(_iface_incs ${KMCMAKE_CC_INTERFACE_INCLUDES})
    set(_iface_prod_args INCLUDES ${_iface_incs})
    if (KMCMAKE_CC_INTERFACE_DEFINES)
        list(APPEND _iface_prod_args DEFINES ${KMCMAKE_CC_INTERFACE_DEFINES})
    endif ()
    kmcmake_export_product_properties(${KMCMAKE_CC_INTERFACE_NAME} ${_iface_prod_args})

    # Dependencies
    if (KMCMAKE_CC_INTERFACE_LINKS)
        target_link_libraries(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE
                ${KMCMAKE_CC_INTERFACE_LINKS})
    endif ()

    if (KMCMAKE_CC_INTERFACE_PUBLIC)
        kmcmake_install_library_target(${KMCMAKE_CC_INTERFACE_NAME} INTERFACE)
    endif ()

    foreach (arg IN LISTS KMCMAKE_CC_INTERFACE_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()
endfunction()
