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

################################################################################################
# kmcmake_cc_test
################################################################################################

function(kmcmake_cc_test)
    set(options
            DISABLED
            SKIP
            EXT
            EXCLUDE_SYSTEM
    )
    set(args NAME
            MODULE
    )
    set(list_args
            DEPS
            SOURCES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            INCLUDES
            COMMAND
            LINKS
    )

    cmake_parse_arguments(
            KMCMAKE_CC_TEST
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )
    if (NOT KMCMAKE_CC_TEST_MODULE)
        kmcmake_error("no module name")
    endif ()
    kmcmake_raw("-----------------------------------")
    kmcmake_print_label("Building Test" "${KMCMAKE_CC_TEST_NAME}")
    kmcmake_raw("-----------------------------------")

    set(${KMCMAKE_CC_TEST_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (KMCMAKE_CC_TEST_EXCLUDE_SYSTEM)
        set(${KMCMAKE_CC_TEST_NAME}_INCLUDE_SYSTEM "")
    endif ()

    if (VERBOSE_KMCMAKE_BUILD)
        kmcmake_print_list_label("Sources" KMCMAKE_CC_TEST_SOURCES)
        kmcmake_print_list_label("Deps" KMCMAKE_CC_TEST_DEPS)
        kmcmake_print_list_label("COPTS" KMCMAKE_CC_TEST_COPTS)
        kmcmake_print_list_label("Defines" KMCMAKE_CC_TEST_DEFINES)
        kmcmake_print_list_label("Links" KMCMAKE_CC_TEST_LINKS)
        message("-----------------------------------")
    endif ()
    set(KMCMAKE_BUILD_THIS_TEST ON)
    set(KMCMAKE_RUN_THIS_TEST ON)
    if (KMCMAKE_CC_TEST_DISABLED)
        set(KMCMAKE_BUILD_THIS_TEST OFF)
        set(KMCMAKE_RUN_THIS_TEST OFF)
    endif ()
    if (KMCMAKE_CC_TEST_SKIP)
        set(KMCMAKE_RUN_THIS_TEST OFF)
    endif ()
    if (KMCMAKE_CC_TEST_EXT)
        set(KMCMAKE_RUN_THIS_TEST OFF)
    endif ()

    set(testcase ${KMCMAKE_CC_TEST_MODULE}_${KMCMAKE_CC_TEST_NAME})
    if (${KMCMAKE_CC_TEST_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_TEST)
        set(KMCMAKE_RUN_THIS_TEST OFF)
    endif ()

    if (NOT KMCMAKE_BUILD_THIS_TEST)
        return()
    endif ()

    add_executable(${testcase} ${KMCMAKE_CC_TEST_SOURCES})

    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:C>:${KMCMAKE_CC_TEST_COPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${KMCMAKE_CC_TEST_CXXOPTS}>)
    target_compile_options(${testcase} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${KMCMAKE_CC_TEST_CUOPTS}>)
    if (KMCMAKE_CC_TEST_DEPS)
        add_dependencies(${testcase} ${KMCMAKE_CC_TEST_DEPS})
    endif ()
    target_link_libraries(${testcase} PRIVATE ${KMCMAKE_CC_TEST_LINKS})

    target_compile_definitions(${testcase}
            PUBLIC
            ${KMCMAKE_CC_TEST_DEFINES}
    )

    target_include_directories(${testcase} ${${KMCMAKE_CC_TEST_NAME}_INCLUDE_SYSTEM}
            PUBLIC
            ${KMCMAKE_CC_TEST_INCLUDES}
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
    if (NOT KMCMAKE_CC_TEST_COMMAND)
        set(KMCMAKE_CC_TEST_COMMAND ${testcase})
    endif ()

    if (KMCMAKE_RUN_THIS_TEST)
        add_test(NAME ${testcase}
                COMMAND ${KMCMAKE_CC_TEST_COMMAND})
    endif ()

endfunction()

function(kmcmake_cc_test_ext)
    set(options
            DISABLE
    )
    set(args NAME
            MODULE
            ALIAS
    )
    set(list_args
            ARGS
            FAIL_EXP
            SKIP_EXP
            PASS_EXP
    )

    cmake_parse_arguments(
            KMCMAKE_CC_TEST_EXT
            "${options}"
            "${args}"
            "${list_args}"
            ${ARGN}
    )

    set(KMCMAKE_RUN_THIS_TEST ON)
    if (KMCMAKE_CC_TEST_EXT_DISABLE)
        set(KMCMAKE_RUN_THIS_TEST OFF)
    endif ()

    if (KMCMAKE_CC_TEST_EXT_MODULE)
        set(basecmd ${KMCMAKE_CC_TEST_EXT_MODULE}_${KMCMAKE_CC_TEST_EXT_NAME})
        if (${KMCMAKE_CC_TEST_EXT_MODULE} IN_LIST ${PROJECT_NAME}_SKIP_TEST)
            set(KMCMAKE_RUN_THIS_TEST OFF)
        endif ()
    else ()
        set(basecmd ${KMCMAKE_CC_TEST_EXT_NAME})
    endif ()

    if (KMCMAKE_CC_TEST_EXT_ALIAS)
        set(test_name ${KMCMAKE_CC_TEST_EXT_MODULE}_${KMCMAKE_CC_TEST_EXT_NAME}_${KMCMAKE_CC_TEST_EXT_ALIAS})
    else ()
        set(test_name ${KMCMAKE_CC_TEST_EXT_MODULE}_${KMCMAKE_CC_TEST_EXT_NAME})
    endif ()

    if (KMCMAKE_RUN_THIS_TEST)
        add_test(NAME ${test_name} COMMAND ${basecmd} ${KMCMAKE_CC_TEST_EXT_ARGS})
        if (KMCMAKE_CC_TEST_EXT_FAIL_EXP)
            set_property(TEST ${test_name} PROPERTY FAIL_REGULAR_EXPRESSION ${KMCMAKE_CC_TEST_EXT_FAIL_EXP})
        endif ()
        if (KMCMAKE_CC_TEST_EXT_PASS_EXP)
            set_property(TEST ${test_name} PROPERTY PASS_REGULAR_EXPRESSION ${KMCMAKE_CC_TEST_EXT_PASS_EXP})
        endif ()
        if (KMCMAKE_CC_TEST_EXT_SKIP_EXP)
            set_property(TEST ${test_name} PROPERTY SKIP_REGULAR_EXPRESSION ${KMCMAKE_CC_TEST_EXT_SKIP_EXP})
        endif ()
    endif ()

endfunction()

