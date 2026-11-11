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


function(shark_schema_generate OUT_FILES OUT_HEADERS)
    set(options)
    set(args
            LANG
            OUTDIR
            PREFIX
    )

    set(list_args
            SCHEMAS
            DEPS
            INCLUDES
    )

    cmake_parse_arguments(
            PARSE_ARGV 2
            SHARK_SCHEMA_GEN
            "${options}"
            "${args}"
            "${list_args}"
    )

    if (NOT SHARK_SCHEMA_GEN_LANG)
        set(SHARK_SCHEMA_GEN_LANG cpp_out)
    endif ()

    if (NOT SHARK_SCHEMA_GEN_SCHEMAS)
        message(SEND_ERROR "shark_schema_generate() called without any schema source files.")
    endif ()


    #Use cmake targets available
    if (TARGET shark::shark)
        if (NOT SHARK_EXECUTABLE)
            set(SHARK_EXECUTABLE $<TARGET_FILE:shark::shark>)
        endif ()
        if (NOT SHARK_CXX_EXECUTABLE)
            set(SHARK_CXX_EXECUTABLE $<TARGET_FILE:shark::shark-cpp>)
        endif ()
        if (NOT SHARK_INCLUDE_DIRECTORY)
            if (DEFINED shark_INCLUDE_DIRECTORY AND shark_INCLUDE_DIRECTORY)
                set(SHARK_INCLUDE_DIRECTORY "${shark_INCLUDE_DIRECTORY}")
            else ()
                set(SHARK_INCLUDE_DIRECTORY "${PROJECT_SOURCE_DIR}")
            endif ()
        endif ()
        list(APPEND SHARK_SCHEMA_GEN_DEPS shark::shark shark-cpp)
    endif ()


    if (NOT SHARK_EXECUTABLE)
        message(SEND_ERROR "Could not locate shark executable (SHARK_EXECUTABLE).")
    endif ()
    if (NOT SHARK_CXX_EXECUTABLE)
        message(SEND_ERROR "Could not locate shark-cpp executable (SHARK_CXX_EXECUTABLE).")
    endif ()
    if (NOT SHARK_INCLUDE_DIRECTORY)
        message(SEND_ERROR "Could not locate shark header files (SHARK_INCLUDE_DIRECTORY).")
    endif ()

    if (NOT SHARK_SCHEMA_GEN_PREFIX)
        set(SHARK_SCHEMA_GEN_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
    endif ()

    if(NOT SHARK_SCHEMA_GEN_OUTDIR)
        set(SHARK_SCHEMA_GEN_OUTDIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
     set(output_dir ":${SHARK_SCHEMA_GEN_OUTDIR}")

    if (${SHARK_INCLUDE_DIRECTORY} STREQUAL ${PROJECT_SOURCE_DIR})
        set(INCLUDE_FLAGS "-I${SHARK_INCLUDE_DIRECTORY}")
    else ()
        set(INCLUDE_FLAGS "-I${SHARK_INCLUDE_DIRECTORY} -I${PROJECT_SOURCE_DIR}")
    endif ()

    foreach (INC ${SHARK_SCHEMA_GEN_INCLUDES})
        if (NOT "${INC}" STREQUAL "")
            set(INCLUDE_FLAGS "${INCLUDE_FLAGS} -I${INC}")
        endif ()
    endforeach ()

    set(${OUT_FILES})
    set(${OUT_HEADERS})

    foreach (schema_file ${SHARK_SCHEMA_GEN_SCHEMAS})
        get_filename_component(file_path "${schema_file}" ABSOLUTE)
        if (NOT EXISTS "${file_path}")
            message(FATAL_ERROR "shark Proto schema file '${file_path}' does not exist!")
        endif ()

        # Output files are placed in SHARK_SCHEMA_GEN_OUTDIR, at a location as if they were
        # relative to SHARK_SCHEMA_GEN_OUTDIR.
        string(LENGTH "${SHARK_SCHEMA_GEN_PREFIX}" prefix_len)
        string(SUBSTRING "${file_path}" 0 ${prefix_len} output_prefix)
        if (NOT "${SHARK_SCHEMA_GEN_PREFIX}" STREQUAL "${output_prefix}")
            message(SEND_ERROR "Could not determine output path for '${schema_file}' ('${file_path}') with source prefix '${SHARK_SCHEMA_GEN_PREFIX}' into '${SHARK_SCHEMA_GEN_OUTDIR}'.")
        endif ()

        string(SUBSTRING "${file_path}" ${prefix_len} -1 output_path)
        set(output_base "${SHARK_SCHEMA_GEN_OUTDIR}${output_path}")
        string(REGEX REPLACE "\\.(capnp|shk)$" "" output_stem "${output_base}")
        message(STATUS "output_path: ${output_path}")
        message(STATUS "output_base: ${output_stem}")
        message(STATUS "${SHARK_CXX_EXECUTABLE}${output_dir}")
        add_custom_command(
                OUTPUT "${output_stem}.sk.cc" "${output_stem}.sk.h"
                COMMAND "${SHARK_EXECUTABLE}"
                ARGS compile
                -o ${SHARK_CXX_EXECUTABLE}${output_dir}
                --src-prefix ${SHARK_SCHEMA_GEN_PREFIX}
                ${include_path}
                ${INCLUDE_FLAGS}
                ${file_path}
                DEPENDS "${schema_file}" ${tool_depends}
                COMMENT "Compiling shark Proto schema ${schema_file}"
                VERBATIM
        )

        list(APPEND ${OUT_FILES} "${output_stem}.sk.cc")
        list(APPEND ${OUT_HEADERS} "${output_stem}.sk.h")
    endforeach ()

    set_source_files_properties(${${OUT_FILES}} ${${OUT_HEADERS}} PROPERTIES GENERATED TRUE)
    set(${OUT_FILES} ${${OUT_FILES}} PARENT_SCOPE)
    set(${OUT_HEADERS} ${${OUT_HEADERS}} PARENT_SCOPE)

endfunction()
