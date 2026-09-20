#!/bin/sh
# Link launcher used by bdk_add_extra_outputs.
# Runs the real linker, then prints that program's memory table, .bin/.hex,
# and size as one block so parallel -j builds do not interleave reports.
#
# Env: BDK_TARGET_NAME, BDK_OBJCOPY, BDK_SIZE
# Args: the compiler/linker command line CMake would have run.

set -u

target="${BDK_TARGET_NAME:?BDK_TARGET_NAME is not set}"
objcopy="${BDK_OBJCOPY:?BDK_OBJCOPY is not set}"
size="${BDK_SIZE:?BDK_SIZE is not set}"

log=$(mktemp) || exit 1
report=$(mktemp) || exit 1
trap 'rm -f "$log" "$report"' EXIT

set +e
"$@" >"$log" 2>&1
status=$?
set -e

if [ "$status" -ne 0 ]; then
    cat "$log"
    exit "$status"
fi

elf=""
prev=""
for arg in "$@"; do
    if [ "$prev" = "-o" ]; then
        elf=$arg
        break
    fi
    case "$arg" in
        -o?*)
            elf=${arg#-o}
            break
            ;;
    esac
    prev=$arg
done

if [ -z "$elf" ] || [ ! -f "$elf" ]; then
    cat "$log"
    echo "BDK: link wrapper could not find the .elf (-o)" >&2
    exit 1
fi

dir=$(dirname "$elf")
bin="${dir}/${target}.bin"
hex="${dir}/${target}.hex"

"$objcopy" -O binary "$elf" "$bin"
"$objcopy" -O ihex "$elf" "$hex"

{
    echo "======== ${target} ========"
    cat "$log"
    echo "BDK: generating ${target}.bin / ${target}.hex"
    "$size" "$elf"
    echo ""
} >"$report"

# One write of the finished report so -j jobs do not mix lines.
cat "$report"
