
#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPILER_DIR="${ROOT_DIR}/shark/compiler"

if ! command -v bison >/dev/null 2>&1; then
    echo "Error: bison not found" >&2
    exit 1
fi

if ! command -v flex >/dev/null 2>&1; then
    echo "Error: flex not found" >&2
    exit 1
fi

echo "Using Bison: $(bison --version | head -n 1)"
echo "Using Flex: $(flex --version)"

cd "${COMPILER_DIR}"

echo "Generating Bison parser..."
bison -d -o thrifty.cc thrifty.yy

echo "Generating Flex scanner..."
flex -o thriftl.cc thriftl.ll

echo "Parser generation completed."
