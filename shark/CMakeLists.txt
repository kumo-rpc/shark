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


include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

if (Inttypes_FOUND)
    # This allows the inttypes.h and stdint.h checks to succeed on platforms that
    # do not natively provide there.
    set (CMAKE_REQUIRED_INCLUDES ${INTTYPES_INCLUDE_DIRS})
endif ()

check_include_file(arpa/inet.h HAVE_ARPA_INET_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/un.h HAVE_SYS_UN_H)
check_include_file(poll.h HAVE_POLL_H)
check_include_file(sys/poll.h HAVE_SYS_POLL_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sched.h HAVE_SCHED_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)

# Check for afunix.h on Windows (since Windows 10 Insider Build 17063):
check_cxx_source_compiles(
        "
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <windows.h>
  #include <afunix.h>
  int main(){(void)sizeof(((struct sockaddr_un *)0)->sun_path); return 0;}
  "
        HAVE_AF_UNIX_H)


check_function_exists(gethostbyname HAVE_GETHOSTBYNAME)
check_function_exists(gethostbyname_r HAVE_GETHOSTBYNAME_R)
check_function_exists(strerror_r HAVE_STRERROR_R)
check_function_exists(sched_get_priority_max HAVE_SCHED_GET_PRIORITY_MAX)
check_function_exists(sched_get_priority_min HAVE_SCHED_GET_PRIORITY_MIN)


check_cxx_source_compiles(
        "
  #include <string.h>
  int main(){char b;char *a = strerror_r(0, &b, 0); static_cast<void>(a); return(0);}
  "
        STRERROR_R_CHAR_P)

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

##########################################################

configure_file(${PROJECT_SOURCE_DIR}/shark/config.h.in ${PROJECT_SOURCE_DIR}/shark/config.h @ONLY)

# Create the thrift C++ library
set(thriftcpp_SOURCES
       TApplicationException.cpp
       TOutput.cpp
       TUuid.cpp
       async/TAsyncChannel.cpp
       async/TAsyncProtocolProcessor.cpp
       async/TConcurrentClientSyncInfo.h
       async/TConcurrentClientSyncInfo.cpp
       concurrency/ThreadManager.cpp
       concurrency/TimerManager.cpp
       processor/PeekProcessor.cpp
       protocol/TBase64Utils.cpp
       protocol/TDebugProtocol.cpp
       protocol/TJSONProtocol.cpp
       protocol/TMultiplexedProtocol.cpp
       protocol/TProtocol.cpp
       transport/TTransportException.cpp
       transport/TFDTransport.cpp
       transport/TSimpleFileTransport.cpp
       transport/THttpTransport.cpp
       transport/THttpClient.cpp
       transport/THttpServer.cpp
       transport/TSocket.cpp
       transport/TSocketPool.cpp
       transport/TServerSocket.cpp
       transport/TTransportUtils.cpp
       transport/TBufferTransports.cpp
       transport/SocketCommon.cpp
       server/TConnectedClient.cpp
       server/TServerFramework.cpp
       server/TSimpleServer.cpp
       server/TThreadPoolServer.cpp
       server/TThreadedServer.cpp
       transport/TPipe.cpp
       transport/TPipeServer.cpp
       transport/TFileTransport.cpp
       transport/TSSLSocket.cpp
       transport/TSSLServerSocket.cpp
       transport/TWebSocketServer.h
       transport/TWebSocketServer.cpp
       concurrency/Thread.cpp
       concurrency/Monitor.cpp
       concurrency/Mutex.cpp
       server/TNonblockingServer.cpp
       transport/TNonblockingServerSocket.cpp
       async/TEvhttpServer.cpp
       async/TEvhttpClientChannel.cpp
       transport/TNonblockingSSLServerSocket.cpp
       transport/TZlibTransport.cpp
       protocol/THeaderProtocol.cpp
       transport/THeaderTransport.cpp
       protocol/THeaderProtocol.cpp
       transport/THeaderTransport.cpp
)


if (WIN32)
    list(APPEND thriftcpp_SOURCES
           windows/TWinsockSingleton.cpp
           windows/SocketPair.cpp
           windows/GetTimeOfDay.cpp
           windows/WinFcntl.cpp
           windows/OverlappedSubmissionThread.cpp
    )
else()
    # These files evaluate to nothing on Windows, so omit them from the
    # Windows build
    list(APPEND thriftcpp_SOURCES
           VirtualProfiling.cpp
           server/TServer.cpp
    )
endif()



kmcmake_cc_library(
        NAME shark
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${thriftcpp_SOURCES}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)
