#!/usr/bin/env bash

# Regenerate the checked-in C++ for built-in protos.
# sharkc is compiled from these files, so CMake cannot generate them.

set -eu

root=$(cd "$(dirname "$0")" && pwd)
cd "$root"

if [[ ! -f shark/schema.capnp ]]; then
  echo "Could not find shark/schema.capnp. Run this script from the repository." >&2
  exit 1
fi

runtime_protos=(
  shark/c++.capnp
  shark/schema.capnp
  shark/stream.capnp
  shark/format/json/json.capnp
  shark/compiler/lexer.capnp
  shark/compiler/grammar.capnp
)

build_dir=build-bootstrap
sharkc="$build_dir/shark/shark"
shark_cpp="$build_dir/shark/shark-cpp"

cmake --preset=bootstrap

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

round=1
while true; do
  echo "Round $round"
  cmake --build "$build_dir"

  sources=()
  for proto in "${runtime_protos[@]}"; do
    sources+=("$root/$proto")
  done
  "$sharkc" compile -I "$root" -o "$shark_cpp:$tmp" --src-prefix "$root" "${sources[@]}"

  changed=0
  for proto in "${runtime_protos[@]}"; do
    base=${proto%.capnp}
    if ! cmp -s "$base.capnp.h" "$tmp/$base.capnp.h" || ! cmp -s "$base.capnp.cc" "$tmp/$base.capnp.cc"; then
      cp "$tmp/$base.capnp.h" "$base.capnp.h"
      cp "$tmp/$base.capnp.cc" "$base.capnp.cc"
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
