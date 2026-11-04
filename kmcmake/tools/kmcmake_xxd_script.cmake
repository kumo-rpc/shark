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
# CMake equivalent of `xxd -i` — invoked via:
#   cmake -DINPUT=... -DOUTPUT=... -DOUTPUTH=... -DVNAME=... -DNAMESPACE=... -DHPATH=... -P kmcmake_xxd_script.cmake

if(NOT INPUT OR NOT OUTPUT OR NOT OUTPUTH OR NOT VNAME OR NOT NAMESPACE OR NOT HPATH)
    message(FATAL_ERROR "kmcmake_xxd_script: INPUT, OUTPUT, OUTPUTH, VNAME, NAMESPACE, HPATH are required")
endif()

file(READ "${INPUT}" hex_data HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," hex_sequence "${hex_data}")
string(LENGTH "${hex_data}" hex_len)
math(EXPR len "${hex_len} / 2")

file(WRITE "${OUTPUTH}" "/// do not modify it, gen by kmcmake_xxd_script.cmake\n\n")
file(APPEND "${OUTPUTH}" "#pragma once\n\n")
file(APPEND "${OUTPUTH}" "#include <string_view>\n\n")
file(APPEND "${OUTPUTH}" "namespace ${NAMESPACE} {\n\n")
file(APPEND "${OUTPUTH}" "    extern std::string_view ${VNAME};\n\n")
file(APPEND "${OUTPUTH}" "}  // namespace ${NAMESPACE}\n")

file(WRITE "${OUTPUT}" "/// do not modify it, gen by kmcmake_xxd_script.cmake\n\n")
file(APPEND "${OUTPUT}" "#include <${HPATH}>\n")
file(APPEND "${OUTPUT}" "#include <string_view>\n\n")
file(APPEND "${OUTPUT}" "namespace ${NAMESPACE} {\n\n")
file(APPEND "${OUTPUT}" "    const unsigned char ${VNAME}_array[] = {${hex_sequence}};\n")
file(APPEND "${OUTPUT}" "    unsigned int ${VNAME}_len = ${len};\n")
file(APPEND "${OUTPUT}" "    std::string_view ${VNAME} = std::string_view(reinterpret_cast<const char*>(${VNAME}_array), ${VNAME}_len);\n\n")
file(APPEND "${OUTPUT}" "}  // namespace ${NAMESPACE}\n")
