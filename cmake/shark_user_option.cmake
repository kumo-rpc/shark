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
# ------------------------------------------------------------------------------
# User option entrypoint
# ------------------------------------------------------------------------------
# This file is loaded before:
#   - @CHANGEME@_deps.cmake
#   - @CHANGEME@_cxx_config.cmake
#
# Put project option() and KMCMAKE_* CACHE overrides here so deps/cxx can branch on them.
# See docs/AI_MIGRATE.md.
#
# set(KMCMAKE_RUNTIME_SIMD_LEVEL AVX2 CACHE STRING "" FORCE)
# set(KMCMAKE_BUILD_TEST ON CACHE BOOL "" FORCE)
# set(KMCMAKE_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
# set(KMCMAKE_USE_CPM ON CACHE BOOL "" FORCE)   # load cmake/*_cpm.cmake
# set(CMAKE_CXX_STANDARD 20)                    # before cxx_config default 17
#
# list(APPEND KMCMAKE_CXX_OPTIONS "-fopenmp")
# ------------------------------------------------------------------------------

