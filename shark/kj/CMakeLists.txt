
set(kj_sources_lite
    array.cc
    list.cc
    common.cc
    debug.cc
    exception.cc
    io.cc
    memory.cc
    mutex.cc
    string.cc
    source-location.cc
    glob-filter.cc
    hash.cc
    table.cc
    thread.cc
    main.cc
    arena.cc
    test-helpers.cc
    units.cc
    encoding.cc
)
set(kj_sources_heavy
    refcount.cc
    string-tree.cc
    time.cc
    filesystem.cc
    filesystem-disk-unix.cc
    filesystem-disk-win32.cc
    parse/char.cc
)
if(NOT CAPNP_LITE)
    set(kj_sources ${kj_sources_lite} ${kj_sources_heavy})
else()
    set(kj_sources ${kj_sources_lite})
endif()

set(_kj_defines)
if(CAPNP_LITE)
    list(APPEND _kj_defines CAPNP_LITE)
endif()

kmcmake_cc_library(
    PUBLIC
    NAME kj
    NAMESPACE ${PROJECT_NAME}
    SOURCES ${kj_sources}
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    DEFINES ${_kj_defines}
    LINKS ${KMCMAKE_DEPS_LINK}
)

if(NOT CAPNP_LITE)
    set(_kj_async_defines)
    if(WITH_FIBERS)
        list(APPEND _kj_async_defines KJ_USE_FIBERS)
    else()
        list(APPEND _kj_async_defines KJ_USE_FIBERS=0)
    endif()
    set(_kj_async_links ${PROJECT_NAME}::kj_static)
    if(_WITH_LIBUCONTEXT)
        list(APPEND _kj_async_links PkgConfig::libucontext)
    endif()
    if(WIN32)
        list(APPEND _kj_async_links ws2_32)
    endif()
    kmcmake_cc_library(
        PUBLIC
        NAME kj-async
        NAMESPACE ${PROJECT_NAME}
        SOURCES
            async.cc
            sticky-flag.cc
            async-unix.cc
            async-win32.cc
            async-io-win32.cc
            async-io.cc
            async-io-unix.cc
            cidr.cc
            timer.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEFINES ${_kj_async_defines}
        LINKS ${_kj_async_links}
    )

    set(_kj_http_defines)
    set(_kj_http_links ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static)
    if(WITH_ZLIB)
        list(APPEND _kj_http_defines KJ_HAS_ZLIB)
        list(APPEND _kj_http_links ZLIB::ZLIB)
    endif()
    kmcmake_cc_library(
        PUBLIC
        NAME kj-http
        NAMESPACE ${PROJECT_NAME}
        SOURCES compat/url.cc compat/http.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEFINES ${_kj_http_defines}
        LINKS ${_kj_http_links}
    )
endif()

if(WITH_OPENSSL AND NOT CAPNP_LITE)
    kmcmake_cc_library(
        PUBLIC
        NAME kj-tls
        NAMESPACE ${PROJECT_NAME}
        SOURCES compat/readiness-io.cc compat/tls.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEFINES KJ_HAS_OPENSSL
        LINKS ${PROJECT_NAME}::kj-async_static
        PLINKS OpenSSL::SSL OpenSSL::Crypto
    )
endif()

if(WITH_ZLIB AND NOT CAPNP_LITE)
    kmcmake_cc_library(
        PUBLIC
        NAME kj-gzip
        NAMESPACE ${PROJECT_NAME}
        SOURCES compat/gzip.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEFINES KJ_HAS_ZLIB
        LINKS ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static ZLIB::ZLIB
    )
endif()
