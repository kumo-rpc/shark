# C/C++ Project Rules

Rules for source and build work in kmcmake-generated projects. For AI agents:
read this together with `docs/AI.md` and `<project>/skills.h`.

## Scope

- Applies to `*.h` / `*.hh` / `*.hpp` / `*.c` / `*.cc` / `*.cpp` / `*.cxx`
- Build/config: `cmake/*`, project `CMakeLists.txt` — follow `docs/AI.md`
- Do **not** edit `kmcmake/` unless the task is explicitly a framework change

## Before Changing Code

1. Read `skills.h` first (project API / conventions)
2. Prefer editing existing files over adding new ones
3. Match local style in the files you touch; do not drive-by reformat
4. Do not add dependencies without asking (`find_package`, CPM, vcpkg, system libs)

## Formatting

- Root `.clang-format` is the style source of truth (WebKit-based, same as xhash)
- Indent 4 spaces; braces Attach; `PointerAlignment: Left`; `ColumnLimit: 0`
- Run `clang-format` on files you own in a change; do not reformat unrelated trees
- Use `// clang-format off` / `on` only for generated or intentionally dense blocks

## Naming

| Kind | Style | Examples |
|------|--------|----------|
| Classes, structs | CamelCase (PascalCase) | `HashTable`, `IoBuffer` |
| Functions, methods | `snake_case` (required) | `hash_table_insert`, `io_buffer_reset` |
| Enumerators / enum constants | `UPPER_SNAKE` | `STATUS_OK`, `KIND_FILE` |
| Macros | `UPPER_SNAKE` | `XHASH_VERSION`, `KMCMAKE_FOO` |
| Variables / parameters | Prefer `snake_case`; stay consistent with neighboring code | |

- Prefer scoped enums (`enum class`) when adding new enums; enumerator names stay `UPPER_SNAKE`
- Do not invent a second naming scheme in new code next to CamelCase types
- Do **not** use CamelCase or lowerCamelCase for function / method names

## Includes

- **Always** use angle brackets: `#include <…>` — never `#include "…"` for project or system headers
- Paths are **from the project source root** (`PROJECT_SOURCE_DIR`), e.g.
  `#include <myproj/foo.h>`, `#include <myproj/util/bar.h>`
- Do **not** use relative includes such as `#include <../foo.h>` or `#include "bar.h"`
- Include order (still with `<>`): corresponding header (in `.cc`), project, third-party, standard library

## Include paths (CMake)

- In-tree project code must be found via the **project root** include already provided by
  kmcmake (`$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>`). Do **not** add extra
  `INCLUDES` / `PINCLUDES` / `target_include_directories` for the project's own
  subdirectories (no per-folder include roots)
- **External / third-party** packages may use `INCLUDES` / `PINCLUDES` (or their
  imported target usage requirements) as needed
- xxd / generated headers follow the same rule: `#include <project/…>` from root

## Language & Style

- C++ **17** default if `CMAKE_CXX_STANDARD` is unset; honor `-DCMAKE_CXX_STANDARD=`, presets, parent project, or `*_user_option.cmake`. Do not silently raise a standard the outer build already set.
- Prefer headers with `#pragma once`
- Prefer `nullptr`, `override`, `= delete`, scoped enums; avoid macros when a
  constant or inline function works
- No `using namespace` in headers
- Public API: clear names, documented in `skills.h` with `///` when exposed
- Errors: prefer explicit return codes / `status`-style types used by the project;
  do not invent a new error framework without discussion

## Files & Layout

```
<project>/          # library / binary sources
cmake/              # user build config (deps, options, CPM, …)
tests/              # kmcmake_cc_test
examples/           # optional
benchmark/          # optional
kmcmake/            # framework — replace on upgrade, do not hand-edit
docs/               # AI.md, this file, upgrade guides
```

- Generated files (`version.h`, `*.xxd.h` / `*.xxd.cc`, `xxd_gen.*`) are build
  outputs — edit sources/assets, not the generated blobs (except when debugging)
- Keep `skills.h` updated when public API or build conventions change

## CMake config ownership (do not scatter)

| Concern | Where it belongs | Must not live in |
|---------|------------------|------------------|
| **Project options** (`option()`, `KMCMAKE_*` CACHE overrides) | `cmake/*_user_option.cmake` (loaded first) | root `CMakeLists.txt` after deps, `kmcmake/`, `<project>/CMakeLists.txt` |
| **Project / library dependencies** (`find_package` gated by those options, CPM list → `KMCMAKE_DEPS_LINK`, system libs) | `cmake/*_deps.cmake` (+ `cmake/*_cpm.cmake` when `KMCMAKE_USE_CPM`) | `<project>/CMakeLists.txt`, random `cmake/*.cmake`, examples |
| **C++ compile options** (from options + deps → `KMCMAKE_CXX_OPTIONS`) | `cmake/*_cxx_config.cmake` | scattered `CXXOPTS` hard-codes, ad-hoc `add_definitions`, per-target flag lists in main sources |

- Main product targets under `<project>/` only **consume** `KMCMAKE_DEPS_LINK` / `KMCMAKE_CXX_OPTIONS`; they do not introduce new packages or global flag policy
- **Allowed exception:** `tests/CMakeLists.txt` and `benchmark/CMakeLists.txt` may `find_package` / link test-only or benchmark-only deps (e.g. gtest, benchmark) and set local `CXXOPTS` / `LINKS` for those targets
- `examples/` follows the same spirit as the main tree: prefer shared deps/flags from `cmake/`; do not grow a second dependency graph there

## CMake Targets

- Declare targets with `kmcmake_cc_*` helpers (see `docs/AI.md`)
- Libraries: static by default; add `SHARE` only when a shared build is required
- Link deps via `LINKS` / `PLINKS` / `KMCMAKE_DEPS_LINK`; do not hard-code absolute paths
- Tests: `kmcmake_cc_test` (+ `LINKS` for libraries — `DEPS` is build-order only)
- Do not add project-local include directories on targets; see **Include paths (CMake)** above

## Dependencies

Three channels (user chooses explicitly; no auto-detection). **Wire them only in deps/CPM as above.**

| Channel | How |
|---------|-----|
| System | `find_package` in `cmake/*_deps.cmake`; presets `default` / `test` / `test-bm` / `all` |
| CPM | `--preset=cpm*` (`KMCMAKE_USE_CPM=ON` + `cmake/*_cpm.cmake`). Small projects, or test-only deps (`gtest` / `doctest`) that are not part of the deployed product. |
| vcpkg | `--preset=vcpkg*` (`VCPKG_ROOT` → `vcpkg.cmake`). Large-scale dependency management. |

Unsuffixed names are Ninja (`build/`); `-make` is Unix Makefiles (`build-make/`). `default` / `default-make` set no cache variables. Unsuffixed `cpm` / `vcpkg` build libraries only.

- Prefer existing project patterns; do not mix the same library from two channels
- For CPM lists mirroring kmpkg: `vcpkg.json` + baseline + `depend-info` order
- Host-only tools (e.g. `kmpkg-cmake`) do not belong in `*_cpm.cmake`
- Test/benchmark-only packages may be resolved under `tests/` or `benchmark/` (see ownership table)
- **Do not** run `vcpkg install` / `kmpkg install` in the project directory. The selected preset’s toolchain (if any) resolves deps on **CMake configure**. Manual in-tree install duplicates work and can desync the build tree

## Testing & Verification

- Do not run `cmake --build` / `ctest` unless asked to verify
- When verifying: configure out-of-source, build, run the relevant tests only
- Intentional demo failures in the template may exist — do not “fix” them unless asked

## License Headers

- New source files: keep the same Apache-2.0 header style as neighboring files
- Do not strip or rewrite license headers on edit

## AI Collaboration

- Questions vs tasks: opinion questions get an answer only; wait for explicit
  “go ahead” / “改” / “干” before editing
- Propose the change (what + why) before large edits when the project’s `AI.md`
  requires approval
- Stay on the task: no drive-by renames, style churn, or unrelated refactors

### Git: current branch only

Work **only** on the user’s current branch and working tree.

- Do **not** scan, list, checkout, or compare other branches to “gather context” or
  “stay in sync” unless the user names that branch or asks for it
- Do **not** merge, rebase, or otherwise integrate other branches, and do **not**
  propose doing so, unless the user gives a clear instruction
- Current-branch `status` / `diff` / `log` for the task is fine; mutating git still
  needs approval per `docs/AI.md`

See `docs/AI.md` → **Git: stay on the user's branch** for the full table.
