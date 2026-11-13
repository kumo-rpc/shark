#!/usr/bin/env bash

# Regenerate the checked-in C++ for built-in protos.
# sharkc is compiled from these files, so CMake cannot generate them.

set -eu

root=$(cd "$(dirname "$0")" && pwd)
cd "$root"

if [[ ! -f shark/descriptor.shark ]]; then
  echo "Could not find shark/descriptor.shark. Run this script from the repository." >&2
  exit 1
fi

runtime_protos=(
  shark/types/any.shark
  shark/types/api.shark
  shark/types/duration.shark
  shark/types/empty.shark
  shark/types/field_mask.shark
  shark/types/source_context.shark
  shark/types/struct.shark
  shark/types/timestamp.shark
  shark/types/type.shark
  shark/types/wrappers.shark
)

build_dir=build-upgrade
sharkc="$build_dir/shark/sharkc"

cmake --preset=vcpkg-upgrade

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

round=1
while true; do
  echo "Round $round"
  cmake --build "$build_dir" --target sharkc

  "$sharkc" -I . --cpp_out="$tmp" "${runtime_protos[@]}"
  "$sharkc" -I . --cpp_out="dllexport_decl=SHARK_EXPORT:$tmp" shark/meta.shark
  "$sharkc" -I . --cpp_out="dllexport_decl=SHARK_EXPORT:$tmp" shark/descriptor.shark
  "$sharkc" -I . --cpp_out="dllexport_decl=PROTOC_EXPORT:$tmp" shark/compiler/plugin.shark

  changed=0
  for proto in "${runtime_protos[@]}" shark/meta.shark shark/descriptor.shark shark/compiler/plugin.shark; do
    base=${proto%.shark}
    if ! cmp -s "$base.skp.h" "$tmp/$base.skp.h" || ! cmp -s "$base.skp.cc" "$tmp/$base.skp.cc"; then
      cp "$tmp/$base.skp.h" "$base.skp.h"
      cp "$tmp/$base.skp.cc" "$base.skp.cc"
      changed=1
    fi
  done

  if [[ $changed -eq 0 ]]; then
    echo "Built-in protos are up to date."
    break
  fi

  round=$((round + 1))
  if [[ $round -gt 5 ]]; then
    echo "Built-in protos were still changing after 5 rounds." >&2
    exit 1
  fi
done
