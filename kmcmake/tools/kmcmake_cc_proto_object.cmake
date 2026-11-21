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
# kmcmake_cc_proto_object
#
# Generate protobuf sources and immediately build them as an OBJECT target.
# This helper keeps the old `kmcmake_cc_proto` untouched while providing a
# one-shot flow for common usage.
#
# Parameters:
#   OPTIONS:
#     EXCLUDE_SYSTEM  Pass through to `kmcmake_cc_object`.
#     UNITY           Enable unity build for the generated object target.
#
#   ARGS:
#     NAME            Logical object target name. Also used as proto output var prefix.
#     NAMESPACE       Optional namespace for object alias. Defaults in `kmcmake_cc_object`.
#     OUTDIR          Required protobuf output directory for generated *.pb.cc/*.pb.h.
#
#   LIST_ARGS:
#     PROTOS          Required proto files list.
#     DEPS            Extra dependencies for proto generation and object target.
#     INCLUDES        Include paths passed to protobuf generation and object compile.
#     DEFINES         Compile definitions passed to `kmcmake_cc_object`.
#     COPTS           C compile options passed to `kmcmake_cc_object`.
#     CXXOPTS         C++ compile options passed to `kmcmake_cc_object`.
#     CUOPTS          CUDA compile options passed to `kmcmake_cc_object`.
#
# Behavior:
#   1) Call `kmcmake_cc_proto` to generate protobuf C++ sources.
#   2) Reuse generated `${NAME}_SRCS`/`${NAME}_HDRS`.
#   3) Call `kmcmake_cc_object` to build an object target in one step.
#
# Example:
#   set(PROTO_FILES
#       proto/binlog.proto
#       proto/common.proto
#   )
#
#   kmcmake_cc_proto_object(
#       NAME proto_obj
#       NAMESPACE ksearch
#       PROTOS ${PROTO_FILES}
#       OUTDIR ${PROJECT_SOURCE_DIR}
#       CXXOPTS ${KMCMAKE_CXX_OPTIONS}
#   )
###################################################################################################
function(kmcmake_cc_proto_object)
    set(options
            EXCLUDE_SYSTEM
            UNITY
    )
    set(args
            NAME
            NAMESPACE
            OUTDIR
    )
    set(list_args
            PROTOS
            DEPS
            INCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_PROTO_OBJECT
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${KMCMAKE_CC_PROTO_OBJECT_NAME}" STREQUAL "")
        kmcmake_error("kmcmake_cc_proto_object requires NAME")
    endif ()
    if ("${KMCMAKE_CC_PROTO_OBJECT_OUTDIR}" STREQUAL "")
        kmcmake_error("kmcmake_cc_proto_object requires OUTDIR")
    endif ()
    if ("${KMCMAKE_CC_PROTO_OBJECT_PROTOS}" STREQUAL "")
        kmcmake_error("kmcmake_cc_proto_object requires PROTOS")
    endif ()

    # First: generate *.pb.cc/*.pb.h with the existing stable macro.
    kmcmake_cc_proto(
            NAME ${KMCMAKE_CC_PROTO_OBJECT_NAME}
            OUTDIR ${KMCMAKE_CC_PROTO_OBJECT_OUTDIR}
            PROTOS ${KMCMAKE_CC_PROTO_OBJECT_PROTOS}
            DEPS ${KMCMAKE_CC_PROTO_OBJECT_DEPS}
            INCLUDES ${KMCMAKE_CC_PROTO_OBJECT_INCLUDES}
    )

    set(_OBJECT_OPTIONS)
    if (KMCMAKE_CC_PROTO_OBJECT_EXCLUDE_SYSTEM)
        list(APPEND _OBJECT_OPTIONS EXCLUDE_SYSTEM)
    endif ()
    if (KMCMAKE_CC_PROTO_OBJECT_UNITY)
        list(APPEND _OBJECT_OPTIONS UNITY)
    endif ()

    # Second: build generated sources as OBJECT target.
    kmcmake_cc_object(
            ${_OBJECT_OPTIONS}
            NAME ${KMCMAKE_CC_PROTO_OBJECT_NAME}
            NAMESPACE ${KMCMAKE_CC_PROTO_OBJECT_NAMESPACE}
            SOURCES ${${KMCMAKE_CC_PROTO_OBJECT_NAME}_SRCS}
            HEADERS ${${KMCMAKE_CC_PROTO_OBJECT_NAME}_HDRS}
            DEPS ${KMCMAKE_CC_PROTO_OBJECT_DEPS}
            INCLUDES ${KMCMAKE_CC_PROTO_OBJECT_INCLUDES}
            DEFINES ${KMCMAKE_CC_PROTO_OBJECT_DEFINES}
            COPTS ${KMCMAKE_CC_PROTO_OBJECT_COPTS}
            CXXOPTS ${KMCMAKE_CC_PROTO_OBJECT_CXXOPTS}
            CUOPTS ${KMCMAKE_CC_PROTO_OBJECT_CUOPTS}
    )
endfunction()
