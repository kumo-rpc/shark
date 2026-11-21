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
# kmcmake_cc_xxd
#
# Batch-convert static asset files into C++ sources (*.xxd.h / *.xxd.cc) wrapped as
# std::string_view, plus an aggregate xxd_gen.h / xxd_gen.cc listing all assets.
#
# Parameters:
#   OPTIONS:
#     NO_GITIGNORE_HINT  Skip the STATUS hint about adding generated files to .gitignore
#
#   ARGS:
#     NAME         Required. Prefix for exported ${NAME}_HDRS / ${NAME}_SRCS
#     NAMESPACE    Required. C++ namespace for generated symbols
#     OUTDIR       Optional. Output directory (default: CMAKE_CURRENT_SOURCE_DIR)
#                  Intentionally defaults to the source tree for IDE navigation
#     ASSETDIR     Optional. Base dir for FILES (default: CMAKE_CURRENT_SOURCE_DIR)
#     INCLUDEBASE  Optional. Base for #include <...> paths (default: PROJECT_SOURCE_DIR)
#
#   LIST_ARGS:
#     FILES        Required. Asset paths relative to ASSETDIR
#
# Exported Variables:
#   ${NAME}_HDRS   Generated headers (per-file *.xxd.h + xxd_gen.h)
#   ${NAME}_SRCS   Generated sources (per-file *.xxd.cc + xxd_gen.cc)
#
# Example:
#   kmcmake_cc_xxd(
#       NAME        gen_assets
#       NAMESPACE   myproj::assets
#       OUTDIR      ${CMAKE_CURRENT_SOURCE_DIR}
#       INCLUDEBASE ${PROJECT_SOURCE_DIR}
#       ASSETDIR    ${CMAKE_CURRENT_SOURCE_DIR}
#       FILES
#           assets/hello.txt
#   )
#   kmcmake_cc_library(
#       NAME assets
#       NAMESPACE ${PROJECT_NAME}
#       SOURCES ${gen_assets_SRCS}
#       PINCLUDES ${PROJECT_SOURCE_DIR}
#       CXXOPTS ${KMCMAKE_CXX_OPTIONS}
#   )
###################################################################################################

get_filename_component(KMCMAKE_XXD_SCRIPT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(KMCMAKE_XXD_SCRIPT "${KMCMAKE_XXD_SCRIPT_DIR}/kmcmake_xxd_script.cmake")

function(kmcmake_cc_xxd)
    set(options
            NO_GITIGNORE_HINT
    )
    set(args
            NAME
            NAMESPACE
            OUTDIR
            ASSETDIR
            INCLUDEBASE
    )
    set(list_args
            FILES
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_XXD
            "${options}"
            "${args}"
            "${list_args}"
    )

    if(NOT DEFINED KMCMAKE_CC_XXD_NAME OR KMCMAKE_CC_XXD_NAME STREQUAL "")
        message(FATAL_ERROR "kmcmake_cc_xxd: NAME must be set")
    endif()

    if(NOT DEFINED KMCMAKE_CC_XXD_NAMESPACE OR KMCMAKE_CC_XXD_NAMESPACE STREQUAL "")
        message(FATAL_ERROR "kmcmake_cc_xxd: NAMESPACE must be set")
    endif()

    if(NOT DEFINED KMCMAKE_CC_XXD_FILES OR KMCMAKE_CC_XXD_FILES STREQUAL "")
        message(FATAL_ERROR "kmcmake_cc_xxd: FILES must be set")
    endif()

    if(NOT DEFINED KMCMAKE_CC_XXD_ASSETDIR OR KMCMAKE_CC_XXD_ASSETDIR STREQUAL "")
        set(KMCMAKE_CC_XXD_ASSETDIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(NOT DEFINED KMCMAKE_CC_XXD_INCLUDEBASE OR KMCMAKE_CC_XXD_INCLUDEBASE STREQUAL "")
        set(KMCMAKE_CC_XXD_INCLUDEBASE ${PROJECT_SOURCE_DIR})
    endif()

    if(NOT DEFINED KMCMAKE_CC_XXD_OUTDIR OR KMCMAKE_CC_XXD_OUTDIR STREQUAL "")
        set(KMCMAKE_CC_XXD_OUTDIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    set(INCLUDE_FILES)
    set(HDRS)
    set(SRCS)
    set(ASSET_HDRS)
    set(GITIGNORE_HINTS)

    foreach(P ${KMCMAKE_CC_XXD_FILES})
        get_filename_component(ASSET_ABS ${P} ABSOLUTE BASE_DIR ${KMCMAKE_CC_XXD_ASSETDIR})
        if(NOT EXISTS "${ASSET_ABS}")
            message(FATAL_ERROR "kmcmake_cc_xxd: asset not found: ${ASSET_ABS}")
        endif()

        string(REGEX REPLACE "[./-]" "_" VAR_NAME "${P}")

        set(HDR ${KMCMAKE_CC_XXD_OUTDIR}/${P}.xxd.h)
        set(SRC ${KMCMAKE_CC_XXD_OUTDIR}/${P}.xxd.cc)

        file(RELATIVE_PATH FILE_REL "${KMCMAKE_CC_XXD_INCLUDEBASE}" "${HDR}")

        list(APPEND INCLUDE_FILES ${FILE_REL})
        list(APPEND HDRS ${HDR})
        list(APPEND SRCS ${SRC})
        list(APPEND ASSET_HDRS ${HDR})
        list(APPEND GITIGNORE_HINTS "${P}.xxd.h" "${P}.xxd.cc")

        add_custom_command(
                OUTPUT "${SRC}" "${HDR}"
                DEPENDS "${ASSET_ABS}" "${KMCMAKE_XXD_SCRIPT}"
                COMMAND "${CMAKE_COMMAND}"
                    "-DHPATH=${FILE_REL}"
                    "-DINPUT=${ASSET_ABS}"
                    "-DOUTPUT=${SRC}"
                    "-DNAMESPACE=${KMCMAKE_CC_XXD_NAMESPACE}"
                    "-DOUTPUTH=${HDR}"
                    "-DVNAME=${VAR_NAME}"
                    -P "${KMCMAKE_XXD_SCRIPT}"
                COMMENT "kmcmake_cc_xxd: generating ${P}.xxd.{h,cc}"
                VERBATIM
        )
    endforeach()

    set(XXD_GEN_H "${KMCMAKE_CC_XXD_OUTDIR}/xxd_gen.h")
    set(XXD_GEN_CC "${KMCMAKE_CC_XXD_OUTDIR}/xxd_gen.cc")
    list(APPEND GITIGNORE_HINTS "xxd_gen.h" "xxd_gen.cc")

    file(WRITE ${XXD_GEN_H} "/// do not modify it, gen by kmcmake_cc_xxd\n\n")
    file(APPEND ${XXD_GEN_H} "#pragma once\n")
    file(APPEND ${XXD_GEN_H} "#include <string_view>\n")
    file(APPEND ${XXD_GEN_H} "#include <utility>\n")
    file(APPEND ${XXD_GEN_H} "#include <vector>\n\n")
    file(APPEND ${XXD_GEN_H} "namespace ${KMCMAKE_CC_XXD_NAMESPACE} {\n\n")
    file(APPEND ${XXD_GEN_H} "    std::vector<std::pair<std::string_view, std::string_view>> xxd_gen_files();\n\n")
    file(APPEND ${XXD_GEN_H} "}  // namespace ${KMCMAKE_CC_XXD_NAMESPACE}\n")

    file(WRITE ${XXD_GEN_CC} "/// do not modify it, gen by kmcmake_cc_xxd\n\n")
    file(APPEND ${XXD_GEN_CC} "#include \"xxd_gen.h\"\n")
    foreach(FILE ${INCLUDE_FILES})
        file(APPEND ${XXD_GEN_CC} "#include <${FILE}>\n")
    endforeach()

    file(APPEND ${XXD_GEN_CC} "\nnamespace ${KMCMAKE_CC_XXD_NAMESPACE} {\n\n")
    file(APPEND ${XXD_GEN_CC} "    std::vector<std::pair<std::string_view, std::string_view>> xxd_gen_files() {\n")
    file(APPEND ${XXD_GEN_CC} "        return std::vector<std::pair<std::string_view, std::string_view>> {\n")
    foreach(VAR_ORG ${KMCMAKE_CC_XXD_FILES})
        string(REGEX REPLACE "[./-]" "_" VAR "${VAR_ORG}")
        file(APPEND ${XXD_GEN_CC} "            {\"${VAR_ORG}\", ${VAR}},\n")
    endforeach()
    file(APPEND ${XXD_GEN_CC} "        };\n")
    file(APPEND ${XXD_GEN_CC} "    }\n\n")
    file(APPEND ${XXD_GEN_CC} "}  // namespace ${KMCMAKE_CC_XXD_NAMESPACE}\n")

    set_source_files_properties(${XXD_GEN_CC} PROPERTIES OBJECT_DEPENDS "${ASSET_HDRS}")

    list(APPEND HDRS ${XXD_GEN_H})
    list(APPEND SRCS ${XXD_GEN_CC})

    set(${KMCMAKE_CC_XXD_NAME}_HDRS ${HDRS} PARENT_SCOPE)
    set(${KMCMAKE_CC_XXD_NAME}_SRCS ${SRCS} PARENT_SCOPE)

    if(NOT KMCMAKE_CC_XXD_NO_GITIGNORE_HINT)
        message(STATUS "kmcmake_cc_xxd(${KMCMAKE_CC_XXD_NAME}): generated files under ${KMCMAKE_CC_XXD_OUTDIR} are for IDE use; add them to .gitignore, e.g.:")
        foreach(HINT ${GITIGNORE_HINTS})
            message(STATUS "  ${HINT}")
        endforeach()
    endif()
endfunction()
