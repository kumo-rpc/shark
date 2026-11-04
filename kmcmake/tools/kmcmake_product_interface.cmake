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
# Product export helpers
#
# Product export: kmcmake_export_product_properties → set_property INTERFACE_*
# Dependencies:   target_link_libraries (elsewhere)
# Install:        kmcmake_install_library_target
###################################################################################################

include(GNUInstallDirs)

# Default public include paths for a product (BUILD + INSTALL genex).
# Extra paths: ARGN
function(kmcmake_default_public_includes out_var)
    set(_incs
            ${ARGN}
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    set(${out_var} "${_incs}" PARENT_SCOPE)
endfunction()

# Set product export properties (INTERFACE_*) on an existing target.
#
# kmcmake_export_product_properties(<target>
#   INCLUDES ...
#   DEFINES ...
#   OBJECTS ...   # APPEND INTERFACE_* from these targets (no link / no .o)
# )
function(kmcmake_export_product_properties target)
    set(options)
    set(args)
    set(list_args INCLUDES DEFINES OBJECTS)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}" "${args}" "${list_args}")

    if (NOT TARGET ${target})
        kmcmake_error("kmcmake_export_product_properties: target `${target}` does not exist")
    endif ()

    if (ARG_INCLUDES)
        set_property(TARGET ${target} APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES ${ARG_INCLUDES})
    endif ()

    if (ARG_DEFINES)
        set_property(TARGET ${target} APPEND PROPERTY
                INTERFACE_COMPILE_DEFINITIONS ${ARG_DEFINES})
    endif ()

    foreach (obj IN LISTS ARG_OBJECTS)
        if (NOT TARGET ${obj})
            kmcmake_error("kmcmake_export_product_properties: OBJECT `${obj}` is not a target")
        endif ()
        # Copy property values at configure time (do not use $<TARGET_PROPERTY:...>
        # genex — that would make install(EXPORT) require the OBJECT target).
        get_target_property(_obj_incs ${obj} INTERFACE_INCLUDE_DIRECTORIES)
        if (_obj_incs)
            set_property(TARGET ${target} APPEND PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES ${_obj_incs})
        endif ()
        get_target_property(_obj_defs ${obj} INTERFACE_COMPILE_DEFINITIONS)
        if (_obj_defs)
            set_property(TARGET ${target} APPEND PROPERTY
                    INTERFACE_COMPILE_DEFINITIONS ${_obj_defs})
        endif ()
        get_target_property(_obj_libs ${obj} INTERFACE_LINK_LIBRARIES)
        if (_obj_libs)
            set_property(TARGET ${target} APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ${_obj_libs})
        endif ()
    endforeach ()

    set_target_properties(${target} PROPERTIES
            INTERFACE_KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            INTERFACE_KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            INTERFACE_KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            EXPORT_PROPERTIES "KMCMAKE_RUNTIME_SIMD_LEVEL;KMCMAKE_ARCH_FLAGS;KMCMAKE_CXX_OPTIONS"
    )
endfunction()

# Back-compat alias name.
function(kmcmake_set_product_interface target)
    kmcmake_export_product_properties(${target} ${ARGN})
endfunction()

# install(TARGETS) into ${PROJECT_NAME}Targets.
# Kind: STATIC | SHARED | INTERFACE | OBJECT
function(kmcmake_install_library_target target kind)
    if (NOT TARGET ${target})
        kmcmake_error("kmcmake_install_library_target: target `${target}` does not exist")
    endif ()

    if (kind STREQUAL "OBJECT")
        install(TARGETS ${target}
                EXPORT ${PROJECT_NAME}Targets
                OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}/objects/${target}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    elseif (kind STREQUAL "INTERFACE")
        install(TARGETS ${target}
                EXPORT ${PROJECT_NAME}Targets
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    else ()
        # STATIC / SHARED
        install(TARGETS ${target}
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif ()
endfunction()
