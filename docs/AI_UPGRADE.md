# kmcmake → 1.7.0 (AI)

Upgrade an **existing kmcmake project** to this template. Do not install the template into the real tree.

**Target:** `1.7.0`  
If the repo is still a **pre-1.4 flat** tree (no `kmcmake/` vs `cmake/` split), stop and split that first (old `AI_UPGRADE.md` SIMD/layout notes). Then come back here.

## DO / DO NOT

| DO | DO NOT |
|----|--------|
| Generate a skeleton under `/tmp/<proj>_upgrade` | Patch `kmcmake/` file-by-file |
| Replace real `kmcmake/` from that skeleton | Overwrite `cmake/`, `<project>/`, `tests/`, `examples/` |
| Copy `CMakePresets.json` and `docs/` from the skeleton | `cmake --install` of the template onto the real prefix |
| Backup `kmcmake/` and `CMakePresets.json` | Delete `*.bak-*` until the user confirms |

## Procedure

```bash
PROJ=<name>
TMP=/tmp/${PROJ}_upgrade
KMCMAKE_SRC=<path-to-kmcmake-repo>
REAL=<path-to-real-project>

rm -rf "$TMP"
cmake -S "$KMCMAKE_SRC/template" -B "$TMP/build" -DCHANGEME="$PROJ"
# Teaching foo/xxd/tests are not required for upgrade:
# cmake ... -DKMCMAKE_GEN_EXAMPLES=ON
cmake --install "$TMP/build" --prefix "$TMP/skel"

cp -a "$REAL/kmcmake" "$REAL/kmcmake.bak-pre-1.7.0"
if [ -f "$REAL/CMakePresets.json" ]; then
  cp -a "$REAL/CMakePresets.json" "$REAL/CMakePresets.json.bak-pre-1.7.0"
fi
rm -rf "$REAL/kmcmake"
cp -a "$TMP/skel/kmcmake" "$REAL/kmcmake"
cp -a "$TMP/skel/CMakePresets.json" "$REAL/CMakePresets.json"
mkdir -p "$REAL/docs"
cp -a "$TMP/skel/docs/." "$REAL/docs/"
```

If the project had extra presets, re-add them on top of skeleton `gen-*` / `deps-*` / `feat-*`. `default` is Ninja + empty toolchain (not `KMPKG_CMAKE`).

## Checklist (1.4 / 1.5 / 1.6 → 1.7)

Walk `cmake/` and root `CMakeLists.txt`. Tick these; do not “fix” product sources unless a row fails.

| Check | Where | Pass |
|-------|--------|------|
| `include(<proj>_user_option OPTIONAL)` **before** deps and cxx | root `CMakeLists.txt` | |
| Project `option()` / CACHE overrides live in `*_user_option.cmake` | `cmake/` | |
| All product `find_package` / `KMCMAKE_DEPS_LINK` in `*_deps.cmake` | `cmake/` | |
| `if (KMCMAKE_USE_CPM)` includes `*_cpm.cmake` **before** other finds | `*_deps.cmake` | |
| `KMCMAKE_CXX_OPTIONS` aggregated only in `*_cxx_config.cmake` | `cmake/` | |
| Targets use `CXXOPTS ${KMCMAKE_CXX_OPTIONS}` and `LINKS`/`PLINKS ${KMCMAKE_DEPS_LINK}` | `<project>/CMakeLists.txt` | |
| Default export still has at least one PUBLIC target (1.7 skeleton: `NAME skills` + `skills.h` + `version.h`) | `<project>/CMakeLists.txt` | |
| No `CMAKE_TOOLCHAIN_FILE=$env{KMPKG_CMAKE}` on `default` | `CMakePresets.json` | |
| Ninja → `build/`; Make → `*-make` / `build-make/` | `CMakePresets.json` | |
| `tests/CMakeLists.txt` and `benchmark/CMakeLists.txt` still `configure_file(config.h.in …)` | tests / benchmark | |
| Root `.clang-format` present if the project wants the template style | repo root | |
| `docs/AI.md`, `c_cpp_project_rules.md`, `AI_MIGRATE.md`, this file | `docs/` | |

`api` → `skills`: only rename if the old INTERFACE was the unused template `api` target. Do not rename a real product library called `api`.

## Verify (only if the user asked to build)

```bash
cd "$REAL"
rm -rf build build-make
cmake --preset=default
cmake --build build --parallel
```

Use `--preset=test` / `vcpkg-test` / `cpm-test` as the project actually builds.
