#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$root/test/.build"
mkdir -p "$build_dir"
refresh_arg="${1:-}"

if command -v ninja >/dev/null 2>&1; then
    cmake -S "$root/test" -B "$build_dir" -G Ninja
else
    cmake -S "$root/test" -B "$build_dir"
fi

cmake --build "$build_dir" -j"${NPROC:-$(getconf _NPROCESSORS_ONLN)}"

if [[ "$refresh_arg" == "--refresh" ]]; then
    "$build_dir/test_runner" --refresh "$root/test"
else
    "$build_dir/test_runner" "$root/test"
fi

"$build_dir/tac_tests"

"$build_dir/ir_generator_smoke"

"$build_dir/ir_printer_tests"

"$build_dir/ir_cli_smoke"
