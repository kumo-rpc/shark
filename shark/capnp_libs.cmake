
set(capnp_sources_lite
    c++.skp.cc
    blob.cc
    arena.cc
    layout.cc
    list.cc
    any.cc
    message.cc
    schema.skp.cc
    stream.skp.cc
    serialize.cc
    serialize-packed.cc
)
set(capnp_sources_heavy
    schema.cc
    schema-loader.cc
    dynamic.cc
    stringify.cc
)
if(NOT CAPNP_LITE)
    set(capnp_sources ${capnp_sources_lite} ${capnp_sources_heavy})
else()
    set(capnp_sources ${capnp_sources_lite})
endif()

set(_capnp_links ${PROJECT_NAME}::kj_static)
if(NOT CAPNP_LITE)
    list(APPEND _capnp_links ${PROJECT_NAME}::kj-async_static)
endif()

kmcmake_cc_library(
    PUBLIC
    NAME capnp
    NAMESPACE ${PROJECT_NAME}
    SOURCES ${capnp_sources}
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS ${_capnp_links}
)

if(NOT CAPNP_LITE)
    kmcmake_cc_library(
        PUBLIC
        NAME capnp-rpc
        NAMESPACE ${PROJECT_NAME}
        SOURCES
            serialize-async.cc
            capability.cc
            membrane.cc
            dynamic-capability.cc
            rpc.cc
            rpc.skp.cc
            rpc-twoparty.cc
            rpc-twoparty.skp.cc
            persistent.skp.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
    )

    kmcmake_cc_library(
        PUBLIC
        NAME capnp-json
        NAMESPACE ${PROJECT_NAME}
        SOURCES compat/json.cc compat/json.skp.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
    )

    kmcmake_cc_library(
        PUBLIC
        NAME capnp-http
        NAMESPACE ${PROJECT_NAME}
        SOURCES
            compat/byte-stream.cc
            compat/byte-stream.skp.cc
            compat/http-over-capnp.cc
            compat/http-over-capnp.skp.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp-rpc_static ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj-http_static ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
    )

    kmcmake_cc_library(
        PUBLIC
        NAME capnp-websocket
        NAMESPACE ${PROJECT_NAME}
        SOURCES compat/websocket-rpc.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::capnp-rpc_static ${PROJECT_NAME}::kj-http_static ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
    )

    kmcmake_cc_library(
        PUBLIC
        NAME capnpc
        NAMESPACE ${PROJECT_NAME}
        SOURCES
            compiler/type-id.cc
            compiler/error-reporter.cc
            compiler/lexer.skp.cc
            compiler/lexer.cc
            compiler/grammar.skp.cc
            compiler/parser.cc
            compiler/generics.cc
            compiler/node-translator.cc
            compiler/compiler.cc
            schema-parser.cc
            serialize-text.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
    )

    kmcmake_cc_binary(
        PUBLIC
        NAME capnp_tool
        SOURCES compiler/module-loader.cc compiler/capnp.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEPS ${PROJECT_NAME}::capnpc_static
        LINKS ${PROJECT_NAME}::capnpc_static ${PROJECT_NAME}::capnp-json_static ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
        DEFINES "CAPNP_INCLUDE_DIR=\"${CMAKE_INSTALL_FULL_INCLUDEDIR}\"" "VERSION=\"${PROJECT_VERSION}\""
    )
    kmcmake_cc_binary(
        PUBLIC
        NAME capnpc_cpp
        SOURCES compiler/capnpc-c++.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
    )
    kmcmake_cc_binary(
        PUBLIC
        NAME capnpc_capnp
        SOURCES compiler/capnpc-capnp.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
    )
endif()
