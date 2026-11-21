# kmcmake AI Context

This file describes the kmcmake template structure and conventions for AI assistants.

**Also read:** `docs/c_cpp_project_rules.md`, `docs/AI_MIGRATE.md` (plain CMake → kmcmake), `docs/AI_UPGRADE.md` (existing kmcmake → 1.7), `docs/VCPKG.md` (standard port).

## AI Constraints

**Before modifying any file:**
1. Read the file's full contents first
2. Propose the exact change (what + why) in text
3. Wait for user approval — do not edit until user says "go ahead" / "do it" / "改" / "干"

**Before running any git mutating command** (commit, push, rebase, merge, reset, etc.):
1. State the exact command you intend to run
2. Wait for user approval

**Git: stay on the user's branch (hard rule)**

Default scope is **only** the current branch and working tree. Do not go looking at
other branches “for context,” “to sync,” or “to be helpful.”

Unless the user **explicitly** names a branch or commands an action:

| Forbidden without explicit ask | Examples |
|--------------------------------|----------|
| Enumerate / browse other branches | `git branch -a`, casual remote branch listing for exploration |
| Read or diff another branch | `git log other..`, `git diff main...HEAD` when not asked, `git show other:path` |
| Switch / create from another branch | `git checkout other`, `git switch -c` from unrelated tips |
| Integrate another branch | `git merge`, `git rebase` onto/from another branch, “should we merge X?” unprompted |
| Push/pull involving other branches | push/pull of non-current branches, force-align with remote side branches |

Allowed without extra permission (still no mutate without approval above):

- `git status` / `git diff` / `git log` **on the current branch** for the active task
- Staging/commit **only when the user asked** to commit

If another branch is needed, **ask first** and quote the exact commands. Never merge or
rebase other branches unless the user clearly says to (e.g. “merge feature/foo into this”).

**Other restrictions:**
- Do not create new files unless the task explicitly requires it; prefer editing existing files
- Do not list directory trees (`ls`, `tree`) or glob files unless necessary for the task at hand
- Do not run `cmake --build` or `ctest` unless explicitly asked to verify a change
- Do not read files you already have context for — reuse what you've been given. Only re-read when the content has likely changed (e.g. after an edit you just made).
- Do not suggest adding new dependencies, libraries, or external tools without asking first
- Do not run `vcpkg install` / `kmpkg install` in the project tree — a vcpkg preset toolchain resolves deps on configure
- Do not change `kmcmake/` framework files unless the task specifically targets them
- Do not reformat, restyle, or rename variables/functions as side work — stick to the task
- Batch independent operations (reads, searches, parallel commands) to reduce round-trips
- **Distinguish questions from tasks.** If the user asks "Is X appropriate?" / "合适吗" / "应该...吧", they are seeking an opinion, not requesting action. Answer concisely and stop. Do not propose changes, do not offer to implement, do not touch files — unless the user explicitly follows up with "do it" / "改" / "干".

## AI Workflow

When the user gives a task, follow this sequence:

1. **Diagnose** — explain the problem and propose a solution
2. **Wait** — do not touch files until the user approves
3. **Execute** — only after the user says "go ahead" / "do it" / "改" / "干"

## Project Overview

kmcmake is a CMake-based C/C++ build system. The `template/` directory is the
skeleton used to bootstrap new projects. The generation flow is:

1. `cmake -S ./template -B build -DCHANGEME=myproject`  (add `-DKMCMAKE_GEN_EXAMPLES=ON` for teaching demos)
2. `cmake --install build --prefix /path/to/project`

Or `vckgctl gencmake -n myproject -o /path/to/project`. Default install is `${PROJECT}::skills` only (`skills.h`, `version.h`).

During step 1, CMake processes `@VAR@` placeholders via `configure_file()`.
During step 2, the rendered files are copied to the target directory.

## Variable Reference

All placeholder variables used in `*.in` files:

| Variable | Source | Description |
|----------|--------|-------------|
| `@CHANGEME@` | `-DCHANGEME=` | Project name (e.g. `myproj`) |
| `@CHANGEME_UP@` | `string(TOUPPER)` | Uppercase project name |
| `@CHANGEME_LOW@` | `string(TOLOWER)` | Lowercase project name |
| `@TM_KMCMAKE_VERSION@` | `template/CMakeLists.txt` | kmcmake version |
| `@CURRENT_DATE@` | `template/CMakeLists.txt` | Generation date |
| `@PROJECT_NAME@` | CMake `project()` | CMake project name |
| `@PROJECT_NAME_UP@` | `string(TOUPPER)` | Upper-case project name |
| `@PROJECT_VERSION_MAJOR@` | user set | Version components |
| `@CMAKE_INSTALL_LIBDIR@` | GNUInstallDirs | Install lib directory |
| `@KMCMAKE_ARCH_OPTION@` | kmcmake_arch | SIMD compiler flags |
| `@KMCMAKE_CONFIG_PRIVATE_FIND_SNIPPETS@` | changeme_deps | Private find_package calls |
| `@UPPERCASE_BUILD_TYPE@` | `string(TOUPPER)` | Debug/Release/etc |
| `@LC_KMCMAKE_PRETTY_NAME@` | kmcmake_module | OS distro name (lower) |
| `@KMCMAKE_DISTRO_VERSION_ID@` | kmcmake_module | OS version |
| `@CMAKE_CXX_COMPILER_ID@` | CMake | Compiler ID |
| `@CMAKE_CXX_COMPILER_VERSION@` | CMake | Compiler version |
| `@CMAKE_CXX_COMPILER_FLAGS@` | CMake | Raw compiler flags |
| `@KMCMAKE_CXX_OPTIONS@` | changeme_cxx_config | Aggregated CXX flags |
| `@CMAKE_CXX_STANDARD@` | CMake | C++ standard |
| `@GIT_COMMIT_HASH@` | git_commit.cmake | Git commit info |
| `@KMCMAKE_ARCH_ENABLE_SSE@` (et al) | kmcmake_arch | SIMD enable (0/1) |
| `@KMCMAKE_RUNTIME_SIMD_LEVEL@` | kmcmake_option | Target SIMD level |
| `@PACKAGE_INIT@` | CMakePackageConfigHelpers | Package init snippet |

## Build Flow

1. `project()` sets project name and version
2. `include(kmcmake_module)` loads framework (`kmcmake_option.cmake` + tools)
3. `include(myproj_user_option OPTIONAL)` — **options** (may override `KMCMAKE_*`, add project `option()`)
4. `include(myproj_deps)` — **deps gated by those options** (`KMCMAKE_USE_CPM` → `*_cpm.cmake`, then `find_package`)
5. `include(myproj_cxx_config)` — **flags from options + deps** → `KMCMAKE_CXX_OPTIONS`
6. `configure_file(version.h.in)`
7. `add_subdirectory(myproj)` — consume `KMCMAKE_DEPS_LINK` / `KMCMAKE_CXX_OPTIONS` only
8. `tests` / `benchmark` / `examples` if the matching `KMCMAKE_BUILD_*` is ON

Vanilla CMake mapping: `docs/AI_MIGRATE.md`.

## Directory Layout

```
project/
├── CMakeLists.txt              # Entry: includes kmcmake_module + user cmake/
├── CMakePresets.json           # empty / cpm / vcpkg × Ninja + -make
├── .clang-format               # Default style (WebKit-based, same as xhash)
├── docs/
│   ├── AI.md
│   ├── AI_MIGRATE.md           # plain CMake → kmcmake
│   ├── AI_UPGRADE.md           # existing kmcmake → 1.7
│   ├── VCPKG.md                # standard vcpkg/vckgctl port
│   ├── vcpkg.json              # example port manifest (hadar)
│   ├── portfile.cmake          # example portfile (hadar)
│   └── c_cpp_project_rules.md
├── kmcmake/                    # Framework layer — DO NOT MODIFY
│   ├── kmcmake_module.cmake    # Module entry point
│   ├── kmcmake_option.cmake    # Global options (BUILD_SHARED, SIMD level, etc.)
│   ├── arch/
│   │   ├── kmcmake_arch.cmake  # SIMD: detect + level + BOOL→int conversion
│   │   ├── x86/...             # x86 SIMD detection + level
│   │   └── arm/...             # ARM SIMD detection + level
│   ├── tools/
│   │   ├── kmcmake_compiler_flags.cmake  # BASE_CXX_FLAGS, RANDEN_FLAGS
│   │   ├── default_setting.cmake         # print functions, install dirs
│   │   ├── kmcmake_cc_library.cmake      # kmcmake_cc_library()
│   │   ├── kmcmake_cc_test.cmake         # kmcmake_cc_test()
│   │   ├── kmcmake_cc_binary.cmake       # kmcmake_cc_binary()
│   │   ├── kmcmake_cc_interface.cmake    # kmcmake_cc_interface()
│   │   ├── kmcmake_cc_object.cmake       # kmcmake_cc_object()
│   │   ├── kmcmake_cc_benchmark.cmake    # kmcmake_cc_benchmark()
│   │   ├── kmcmake_cc_proto.cmake        # Protobuf support
│   │   ├── kmcmake_cc_proto_object.cmake # Protobuf object
│   │   ├── kmcmake_cc_xxd.cmake          # Embed assets as string_view
│   │   ├── kmcmake_xxd_script.cmake      # -P script used by kmcmake_cc_xxd
│   │   ├── CPM.cmake                     # Vendored CPM.cmake (official release)
│   │   └── git_commit.cmake              # Git version info
│   └── package/                          # CPack config
├── cmake/                      # User config layer — MODIFY FREELY
│   ├── myproj_user_option.cmake    # User overrides (loaded first)
│   ├── myproj_cpm.cmake           # CPMAddPackage list (if KMCMAKE_USE_CPM)
│   ├── myproj_deps.cmake          # Dependencies
│   ├── myproj_cxx_config.cmake    # C++ flags aggregation (thin)
│   ├── myproj_cpack_config.cmake  # Packaging config
│   └── myproj_config.cmake.in     # Export template
├── myproj/                     # Source code
│   ├── CMakeLists.txt          # default: kmcmake_cc_interface NAME skills
│   ├── skills.h
│   └── version.h.in
├── tests/                      # default: configure_file(config.h.in) only
├── benchmark/                  # default: configure_file(config.h.in) only
└── examples/                   # stub unless -DKMCMAKE_GEN_EXAMPLES=ON
```

## SIMD Architecture

The `arch/` subsystem is per-CPU-architecture:

- **Detection phase** (`kmcmake_arch_detect.cmake`): probes each feature at compile-time, exports `KMCMAKE_{ARCH}_HAS_{FEATURE}` (BOOL)
- **Level phase** (`kmcmake_arch_level.cmake`): reads `KMCMAKE_RUNTIME_SIMD_LEVEL`, enables features level-by-level, exports `KMCMAKE_ARCH_ENABLE_{FEATURE}` and `KMCMAKE_SIMD_CXX_FLAGS`

To add a new architecture:
1. Create `arch/<arch>/kmcmake_arch_detect.cmake` — define `KMCMAKE_<ARCH>_HAS_*` flags
2. Create `arch/<arch>/kmcmake_arch_level.cmake` — set `KMCMAKE_ARCH_ENABLE_*` + build flags
3. Add routing in `arch/kmcmake_arch.cmake`

## Key Conventions

- **DO NOT** modify files under `kmcmake/` — these are framework files that get replaced on upgrade
- **DO** modify files under `cmake/` — these are your project's user configuration
- To override SIMD features: set `KMCMAKE_ARCH_ENABLE_{FEATURE}` to `OFF` in `user_option.cmake`
- To override base flags: set `KMCMAKE_BASE_CXX_FLAGS` in `user_option.cmake`
- All public CMake functions use `kmcmake_` prefix

## Upgrading kmcmake

| Situation | Doc |
|-----------|-----|
| Plain CMake → kmcmake (`option` / deps / flags) | `docs/AI_MIGRATE.md` |
| Existing kmcmake → **1.7.0** | `docs/AI_UPGRADE.md` |
| vcpkg port | `docs/VCPKG.md` + `docs/vcpkg.json` + `docs/portfile.cmake` |
| Old 1.5 procedure URL | `docs/AI_UPGRADE_1_5.md` (stub → `AI_UPGRADE.md`) |

## skills.h Convention

Every kmcmake-based project includes a `skills.h` file in its main source directory.
This file is a human/AI-readable summary of the project's public API, design
principles, and key conventions.

**For AI agents:**
- Read `skills.h` first — it replaces scanning all source files
- For dependency libraries, look for their `skills.h` at `<dep>/include/<dep>/skills.h`
- Style: all entries use `///` (Doxygen triple-slash) comments in English
- The file contains the project summary, build API overview, configuration variables,
  and generated macro reference

**For users:**
- Add your project-specific API documentation to `skills.h` using the same `///` style
- Keep it concise — it should be readable at a glance

## File Generation Rules

- Files with `.in` extension are processed by `configure_file(@ONLY)` — only `@VAR@` substitutions
- `@CHANGEME@` is replaced by the project name passed via `-DCHANGEME=`
- `@CHANGEME_UP@` is the uppercase variant
- Non-`.in` files are copied verbatim during `cmake --install`

---

# kmcmake API Reference

Quick usage demos for all `kmcmake_cc_*` functions. Use these as templates.

## kmcmake_cc_library

Builds static library from object sources. When `SHARE ON` is set, also builds
and installs a shared variant.

```cmake
# Minimal — public install, auto-named from folder
kmcmake_cc_library(
    PUBLIC
    NAME mylib
    SOURCES mylib.cc
    HEADERS mylib.h
)

# Full example with shared variant and dependencies
kmcmake_cc_library(
    PUBLIC
    NAME core
    NAMESPACE myproj
    SOURCES core.cc core.h
    HEADERS core.h
    SHARE                                                # build + install shared lib
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include         # public includes
    PINCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src            # private includes
    LINKS Threads::Threads                               # public link deps (also applied when compiling SOURCES)
    PLINKS myproj::util                                  # private link deps (also applied when compiling SOURCES)
    WLINKS archiver                                       # whole-archive link (pack/link only)
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}                       # compiler flags
    DEFINES MYLIB_EXPORTS                                # compile definitions
    EXCLUDE_SYSTEM                                       # don't mark includes SYSTEM
    UNITY                                                # enable unity build
)
```

Creates targets:
- `<namespace>::<name>` (shared alias, only when `SHARE ON`)
- `<namespace>::<name>_static` (static alias)

## kmcmake_cc_interface

Header-only library (no compiled sources).

```cmake
kmcmake_cc_interface(
    PUBLIC
    NAME skills
    NAMESPACE myproj
    HEADERS skills.h version.h
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
```

## kmcmake_cc_binary

Creates an executable target.

```cmake
kmcmake_cc_binary(
    NAME mytool
    SOURCES main.cc tool.cc
    DEPS myproj::core
    LINKS ${KMCMAKE_DEPS_LINK}
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKOPTS -static
    PUBLIC               # install the binary
)
```

## kmcmake_cc_object

Creates an object library (no linker step), useful for large source sets compiled once.

```cmake
kmcmake_cc_object(
    NAME bigobj
    SOURCES a.cc b.cc c.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
```

Then reference it in a library/binary via `OBJECTS`:

```cmake
kmcmake_cc_library(
    NAME combined
    OBJECTS bigobj
    # no SOURCES — uses precompiled objects
)
```

## kmcmake_cc_test

Create and register a CTest test case.

```cmake
# Basic test
kmcmake_cc_test(
    NAME my_test
    MODULE module_name              # prefix for test binary + ctest name
    SOURCES my_test.cc
    DEPS myproj::core gtest gtest_main
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)

# Extended test with custom command and expected output
kmcmake_cc_test_ext(
    NAME my_test
    MODULE module_name
    ALIAS custom_name                # appended to test name
    ARGS --flag value               # extra CLI args
    PASS_EXP "All tests passed"     # regex — test passes if output matches
    FAIL_EXP "ERROR|Failed"        # regex — test fails if output matches
    SKIP_EXP "SKIP|Skipped"        # regex — test is skipped
)

# Disabled test
kmcmake_cc_test(
    NAME disabled_test
    MODULE module
    SOURCES disabled_test.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    DISABLED
)
```

Test naming pattern: `<module>_<name>` (e.g. `module_my_test`).

## kmcmake_cc_benchmark

Same API shape as test, under `KMCMAKE_BUILD_BENCHMARK`.

```cmake
kmcmake_cc_benchmark(
    NAME bm
    MODULE module
    SOURCES benchmark.cc
    DEPS benchmark::benchmark
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
```

## kmcmake_cc_xxd

Embed static asset files as `std::string_view` C++ sources. Generated `*.xxd.h` /
`*.xxd.cc` (and `xxd_gen.h` / `xxd_gen.cc`) default to the **source tree** so IDEs
can navigate them; add the paths to `.gitignore` (configure prints a STATUS hint;
it does not modify `.gitignore`).

```cmake
kmcmake_cc_xxd(
    NAME        gen_assets
    NAMESPACE   myproj
    OUTDIR      ${CMAKE_CURRENT_SOURCE_DIR}   # IDE-friendly (default)
    INCLUDEBASE ${PROJECT_SOURCE_DIR}
    ASSETDIR    ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
        assets/hello.txt
    # NO_GITIGNORE_HINT                       # optional: silence the hint
)

kmcmake_cc_library(
    NAME assets
    NAMESPACE ${PROJECT_NAME}
    SOURCES ${gen_assets_SRCS}
    PINCLUDES ${PROJECT_SOURCE_DIR}
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
```

Exports `${NAME}_HDRS` / `${NAME}_SRCS`. Aggregate API: `xxd_gen_files()` returns
`vector<pair<string_view, string_view>>` of `{relative_path, content}`.

## CPM (optional dependency fetch)

Vendored official [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) release lives at
`kmcmake/tools/CPM.cmake` (unmodified). **Default off** — enable explicitly
(`--preset=cpm*` or `-DKMCMAKE_USE_CPM=ON`). Use CPM for **small projects**, or
when the only extra packages are **test-only** (`gtest` / `doctest`) and are
**not shipped**. Use **vcpkg** (`--preset=vcpkg*`) for large-scale dependency
management.

```cmake
# cmake/<project>_user_option.cmake  OR  --preset=cpm / -DKMCMAKE_USE_CPM=ON
set(KMCMAKE_USE_CPM ON CACHE BOOL "" FORCE)
```

When on, `<project>_deps.cmake` includes `<project>_cpm.cmake` **before**
`find_package`. Put `CPMAddPackage` / `CPMFindPackage` there and
`list(APPEND KMCMAKE_DEPS_LINK ...)`. Do not `set(KMCMAKE_DEPS_LINK)` in the
CPM file (deps appends system libs afterward).

The generated template enables **fmt 10.2.1** (`GIT_TAG 10.2.1`, no `v` prefix) in
`*_cpm.cmake` as the default smoke package; CI job `cpm+xxd` runs
`base_cpm_test` + `base_xxd_test`. Replace packages as needed for real projects.

No auto-detection of vcpkg/toolchain — overlapping managers are the
user's choice. Use `--preset=vcpkg*` when `VCPKG_ROOT` is set. Mapping from a
kmpkg manifest: use `vcpkg.json` + `kmpkg-configuration.json` baseline, then
`kmpkg depend-info` topological order to fill `*_cpm.cmake`.

---

# Quick CMakeLists.txt Templates

## Module (subdirectory) CMakeLists.txt

```cmake
kmcmake_cc_library(
    PUBLIC
    NAME mysub
    NAMESPACE ${PROJECT_NAME}
    SOURCES mysub.cc
    HEADERS mysub.h
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    PLINKS ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_binary(
    NAME mysub_main
    SOURCES main.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    DEPS ${PROJECT_NAME}::mysub
    LINKS ${KMCMAKE_DEPS_LINK}
)
```

## Test CMakeLists.txt

```cmake
kmcmake_cc_test(
    NAME unit_test
    MODULE module
    SOURCES unit_test.cc
    DEPS ${PROJECT_NAME}::mysub gtest gtest_main
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
```

## User Option Override (in cmake/*_user_option.cmake)

```cmake
# Force-disable AVX2 even if hardware supports it
set(KMCMAKE_ARCH_ENABLE_AVX2 OFF)

# Change SIMD target
set(KMCMAKE_RUNTIME_SIMD_LEVEL SSE4_2 CACHE STRING "" FORCE)

# Add extra compiler flags
list(APPEND KMCMAKE_CXX_OPTIONS "-fopenmp")
```