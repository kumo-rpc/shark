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

# Default exported target (install(EXPORT) requires at least one).
# Teaching demos (foo / xxd / binaries) live in CMakeLists.examples.txt
# and are installed only when generating with -DKMCMAKE_GEN_EXAMPLES=ON.

kmcmake_cc_interface(
        NAMESPACE ${PROJECT_NAME}
        NAME skills
        HEADERS
        skills.h
        version.h
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PUBLIC
)


kmcmake_cc_library(
        NAME sharkc
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        compiler/thriftl.cc
        compiler/thrifty.cc
        compiler/audit/t_audit.cc
        compiler/common.cc
        compiler/generate/go_validator_generator.cc
        compiler/generate/t_c_glib_generator.cc
        compiler/generate/t_cl_generator.cc
        compiler/generate/t_cpp_generator.cc
        compiler/generate/t_d_generator.cc
        compiler/generate/t_dart_generator.cc
        compiler/generate/t_delphi_generator.cc
        compiler/generate/t_erl_generator.cc
        compiler/generate/t_generator.cc
        compiler/generate/t_go_generator.cc
        compiler/generate/t_gv_generator.cc
        compiler/generate/t_haxe_generator.cc
        compiler/generate/t_html_generator.cc
        compiler/generate/t_java_generator.cc
        compiler/generate/t_javame_generator.cc
        compiler/generate/t_js_generator.cc
        compiler/generate/t_json_generator.cc
        compiler/generate/t_kotlin_generator.cc
        compiler/generate/t_lua_generator.cc
        compiler/generate/t_markdown_generator.cc
        compiler/generate/t_mmd_generator.cc
        compiler/generate/t_netstd_generator.cc
        compiler/generate/t_ocaml_generator.cc
        compiler/generate/t_perl_generator.cc
        compiler/generate/t_php_generator.cc
        compiler/generate/t_py_generator.cc
        compiler/generate/t_rb_generator.cc
        compiler/generate/t_rs_generator.cc
        compiler/generate/t_st_generator.cc
        compiler/generate/t_xml_generator.cc
        compiler/generate/t_xsd_generator.cc
        compiler/generate/validator_parser.cc
        compiler/parse/parse.cc
        compiler/parse/t_typedef.cc
        CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
        LINKS
            ${KMCMAKE_DEPS_LINK}
        PUBLIC
)


kmcmake_cc_binary(
        NAME sharkc
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        compiler/main.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::sharkc_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)
