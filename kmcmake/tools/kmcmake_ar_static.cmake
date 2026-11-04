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
# kmcmake_ar_static
#
# Pack OBJECT targets into a STATIC archive (.a). Does not compile sources.
# Does NOT set product INTERFACE_* and does NOT install — caller does those steps.
#
# Dependencies: LINKS / PLINKS / WLINKS → target_link_libraries
###################################################################################################

function(kmcmake_ar_static)
    set(options)
    set(args NAME
            NAMESPACE
            OUTPUT_NAME
    )
    set(list_args
            OBJECTS
            DEPS
            LINKS
            PLINKS
            WLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            ARG
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${ARG_NAME}" STREQUAL "")
        kmcmake_error("kmcmake_ar_static: NAME is required")
    endif ()
    if (NOT ARG_NAMESPACE OR "${ARG_NAMESPACE}" STREQUAL "")
        set(ARG_NAMESPACE ${PROJECT_NAME})
    endif ()
    if (NOT ARG_OBJECTS)
        kmcmake_error("kmcmake_ar_static(${ARG_NAME}): OBJECTS is required")
    endif ()
    if ("${ARG_OUTPUT_NAME}" STREQUAL "")
        set(ARG_OUTPUT_NAME ${ARG_NAME})
    endif ()

    set(_objs_flatten)
    foreach (obj IN LISTS ARG_OBJECTS)
        if (NOT TARGET ${obj})
            kmcmake_error("kmcmake_ar_static(${ARG_NAME}): OBJECT `${obj}` is not a target")
        endif ()
        list(APPEND _objs_flatten $<TARGET_OBJECTS:${obj}>)
    endforeach ()

    set(_target ${ARG_NAME}_static)
    add_library(${_target} STATIC ${_objs_flatten})
    set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${ARG_OUTPUT_NAME})
    add_library(${ARG_NAMESPACE}::${ARG_NAME}_static ALIAS ${_target})

    foreach (obj IN LISTS ARG_OBJECTS)
        add_dependencies(${_target} ${obj})
    endforeach ()
    if (ARG_DEPS)
        add_dependencies(${_target} ${ARG_DEPS})
    endif ()

    # Dependencies only (product export is a separate step).
    if (ARG_PLINKS)
        target_link_libraries(${_target} PRIVATE ${ARG_PLINKS})
    endif ()
    if (ARG_LINKS)
        target_link_libraries(${_target} PUBLIC ${ARG_LINKS})
    endif ()
    if (ARG_WLINKS)
        target_link_libraries(${_target} PRIVATE ${ARG_WLINKS})
    endif ()

    foreach (arg IN LISTS ARG_UNPARSED_ARGUMENTS)
        message(WARNING "kmcmake_ar_static: unparsed argument: ${arg}")
    endforeach ()
endfunction()
