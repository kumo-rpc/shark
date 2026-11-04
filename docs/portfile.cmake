vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO xxx/hadar
        REF "v${VERSION}"
        SHA512 d9038ad947da11df1f30a3e336ebee9e3cfaf25f00e19718f907d4da7f7aded7c327afa0632790f1d43eebe0b747aa18a8d2cce7815b4364aa80d8a95c9c3069
        HEAD_REF main
)
# VERSION comes from sibling vcpkg.json. Replace REPO / SHA512 after
#   vcpkg hash-file  or  vcpkg hash  on the downloaded tarball.
# Vanilla vcpkg: vcpkg_from_github (same arguments).

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DKMCMAKE_BUILD_TEST=OFF
        -DKMCMAKE_BUILD_BENCHMARK=OFF
        -DKMCMAKE_BUILD_EXAMPLES=OFF
)
# Port builds the library only. Do not enable GEN_EXAMPLES / CPM here.
# Vanilla vcpkg: vcpkg_cmake_configure.

vcpkg_cmake_install()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(PACKAGE_NAME hadar CONFIG_PATH lib/cmake/hadar)
# PACKAGE_NAME / CONFIG_PATH must match kmcmake install:
#   ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
vcpkg_fixup_pkgconfig()

# Only if the project installs executables. Drop these lines for a lib-only port.
vcpkg_copy_tools(TOOL_NAMES hadar AUTO_CLEAN)
vcpkg_copy_tools(TOOL_NAMES hadar_dict AUTO_CLEAN)
vcpkg_copy_tools(TOOL_NAMES hadar_phrase_extract AUTO_CLEAN)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
