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
# CPM dependency list (loaded only when KMCMAKE_USE_CPM=ON)
# ------------------------------------------------------------------------------
# CPM.cmake is vendored at kmcmake/tools/CPM.cmake (official release, unmodified).
# Enable with: -DKMCMAKE_USE_CPM=ON  or set(KMCMAKE_USE_CPM ON) in *_user_option.cmake
#
# Template ships a minimal fmt package as the CPM smoke example (used by CI).
# Replace/extend for real projects. Prefer kmpkg depend-info order when mirroring.
#
# See docs/AI.md (CPM section) and https://github.com/cpm-cmake/CPM.cmake
# ------------------------------------------------------------------------------

include(CPM)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        VERSION 10.2.1
        GIT_TAG 10.2.1
        OPTIONS
        "FMT_TEST OFF"
        "FMT_DOC OFF"
)
list(APPEND KMCMAKE_DEPS_LINK fmt::fmt)
