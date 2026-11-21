# Adding a vcpkg / vckgctl port

Canonical example (copy, then rename `hadar` → your project):

- [`docs/vcpkg.json`](vcpkg.json)
- [`docs/portfile.cmake`](portfile.cmake)

Helpers in the example are **`kmpkg_*`** (vckgctl). Vanilla vcpkg uses the same steps with **`vcpkg_*`** and host deps `vcpkg-cmake` / `vcpkg-cmake-config` instead of `kmpkg-cmake` / `kmpkg-cmake-config`. Do not mix prefixes in one portfile.

No in-tree `vcpkg install` / `kmpkg install` when a toolchain preset already configures. Do not copy `vcpkg.cmake` into the repo. Do not use CPM inside a port build.

## Consume a kmcmake project (already generated)

`--preset=vcpkg*` sets:

`CMAKE_TOOLCHAIN_FILE=$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`

Need `VCPKG_ROOT`. App manifest: `vcpkg.json` at the **app** root. Port files below live under `ports/<name>/`, not in the app tree.

## Provide a port

```
ports/hadar/vcpkg.json      ← copy from docs/vcpkg.json
ports/hadar/portfile.cmake  ← copy from docs/portfile.cmake
ports/hadar/usage           ← optional one-liner for `kmpkg install` help
```

Overlay: `VCPKG_OVERLAY_PORTS` / `vcpkg-configuration.json` `overlay-ports`. Registry: same two files in the registry repo.

### `vcpkg.json` (worked example)

| Field | Example | Rule |
|-------|---------|------|
| `name` | `hadar` | Directory name under `ports/`. Same as CMake `project()` / `PACKAGE_NAME` in the portfile. |
| `version` | `1.2.1` | Becomes `${VERSION}` in `kmpkg_from_github(REF "v${VERSION}")`. Tag must exist. |
| `description` / `homepage` / `license` | hadar / GitHub / Apache-2.0 | Schema: [vcpkg.json](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json) |
| `dependencies` | `darts`, `marisa-trie`, … | Ports this library `find_package`s. Same names as the registry. |
| host `kmpkg-cmake` + `kmpkg-cmake-config` | required | Pulls `kmpkg_cmake_configure` / `kmpkg_cmake_config_fixup`. Vanilla: `vcpkg-cmake` + `vcpkg-cmake-config`. |

Tests / examples / benchmark stay **off** in the port (see OPTIONS). If you must, add a `features` block (`tests` → `KMCMAKE_BUILD_TEST`) — the hadar example does not, because a port should ship the library.

Replace every dependency string with what `*_deps.cmake` actually finds. Do not list `gtest` unless a feature enables tests.

### `portfile.cmake` (worked example)

Order is the standard vcpkg pipeline. Walk the hadar file:

1. **Fetch** — `kmpkg_from_github` / `vcpkg_from_github`  
   `REPO`, `REF "v${VERSION}"`, `SHA512` of that tarball, `HEAD_REF` for `--head`.  
   After bumping version: download the archive and recompute SHA512 (`kmpkg hash-file` / `vcpkg hash`).

2. **Configure** — `kmpkg_cmake_configure`  
   Always pass kmcmake knobs as `OPTIONS` (no second option parser):

   ```
   -DKMCMAKE_BUILD_TEST=OFF
   -DKMCMAKE_BUILD_BENCHMARK=OFF
   -DKMCMAKE_BUILD_EXAMPLES=OFF
   ```

   Leave `KMCMAKE_USE_CPM` off. Do not pass `-DKMCMAKE_GEN_EXAMPLES` (that is the **template generator**, not the project).

3. **Install** — `kmpkg_cmake_install` / `vcpkg_cmake_install`

4. **PDBs** — `kmpkg_copy_pdbs` (Windows; no-op elsewhere)

5. **CMake package** — `kmpkg_cmake_config_fixup(PACKAGE_NAME hadar CONFIG_PATH lib/cmake/hadar)`  
   `CONFIG_PATH` must match kmcmake: `lib/cmake/${PROJECT_NAME}` (or `lib64/cmake/…` only if you changed GNUInstallDirs). Do not hand-write `*Config.cmake` in the port.

6. **pkg-config** — `kmpkg_fixup_pkgconfig` (kmcmake installs `*.pc`)

7. **Tools** — `kmpkg_copy_tools(TOOL_NAMES … AUTO_CLEAN)` **only** for installed executables (`hadar`, `hadar_dict`, …). Lib-only ports: delete these lines.

8. **Cleanup** — `file(REMOVE_RECURSE debug/include debug/share)`

9. **License** — `kmpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")`

### Checklist when cloning hadar → your port

- [ ] `vcpkg.json` `name` = `ports/<name>/` = `PACKAGE_NAME` = `CONFIG_PATH` last component
- [ ] `version` matches git tag used in `REF`
- [ ] `SHA512` recomputed for that tag
- [ ] `dependencies` = runtime `find_package` list from `cmake/*_deps.cmake`
- [ ] host cmake helpers present
- [ ] `KMCMAKE_BUILD_TEST/BENCHMARK/EXAMPLES=OFF`
- [ ] `copy_tools` only for real `kmcmake_cc_binary` install names
- [ ] Overlay/registry points at `ports/`

## Overlay consume

```bash
export VCPKG_ROOT=…
# VCPKG_OVERLAY_PORTS=/path/to/ports
cmake --preset=vcpkg
```
