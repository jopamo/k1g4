#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Update the kernel's zstd import from upstream.
#
# By default this script:
#   1) fetches facebook/zstd dev into a temporary checkout
#   2) runs contrib/linux-kernel "make test"
#   3) imports the generated files into this kernel tree
#   4) reapplies kernel RVV/SVE2 integration required by the imported code
#   5) normalizes imported text to kernel whitespace rules
#   6) records the exact imported upstream commit
#
# Usage example:
#   ./scripts/update-zstd.sh
#
# Or with an explicit ref:
#   ./scripts/update-zstd.sh --ref v1.5.7-kernel

set -eu

die() {
	echo "error: $*" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: $0 [options]

Options:
  --linux <path>       Linux tree to update (default: current git toplevel)
  --repo <url>         zstd git URL (default: https://github.com/facebook/zstd)
  --ref <ref>          zstd ref to import (default: dev)
  --zstd-path <path>   Existing zstd checkout to reuse
  --skip-test          Skip 'make test' in contrib/linux-kernel
  --allow-dirty        Allow importing into a dirty Linux tree
  --keep-workdir       Keep temporary zstd checkout
  -h, --help           Show this help
EOF
}

linux_path=""
repo="https://github.com/facebook/zstd"
ref="dev"
zstd_path=""
run_test=1
allow_dirty=0
keep_workdir=0
tmp_zstd=""

while [ "$#" -gt 0 ]; do
	case "$1" in
	--linux)
		[ "$#" -ge 2 ] || die "--linux needs a path"
		linux_path=$2
		shift 2
		;;
	--repo)
		[ "$#" -ge 2 ] || die "--repo needs a URL"
		repo=$2
		shift 2
		;;
	--ref)
		[ "$#" -ge 2 ] || die "--ref needs a git ref"
		ref=$2
		shift 2
		;;
	--zstd-path)
		[ "$#" -ge 2 ] || die "--zstd-path needs a path"
		zstd_path=$2
		shift 2
		;;
	--skip-test)
		run_test=0
		shift
		;;
	--allow-dirty)
		allow_dirty=1
		shift
		;;
	--keep-workdir)
		keep_workdir=1
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		die "unknown option: $1"
		;;
	esac
done

if [ -z "$linux_path" ]; then
	linux_path=$(git rev-parse --show-toplevel 2>/dev/null) ||
		die "run inside a Linux git tree or pass --linux <path>"
fi

linux_path=$(cd "$linux_path" && pwd)
git -C "$linux_path" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
	die "not a git tree: $linux_path"
[ -d "$linux_path/lib/zstd" ] || die "missing $linux_path/lib/zstd"
[ -d "$linux_path/include/linux" ] || die "missing $linux_path/include/linux"

if [ "$allow_dirty" -eq 0 ] &&
   [ -n "$(git -C "$linux_path" status --porcelain --untracked-files=normal)" ]; then
	die "Linux tree is dirty; commit/stash first or pass --allow-dirty"
fi

cleanup() {
	if [ -n "$tmp_zstd" ] && [ "$keep_workdir" -eq 0 ] && [ -d "$tmp_zstd" ]; then
		rm -rf "$tmp_zstd"
	fi
}
trap cleanup EXIT INT TERM HUP

if [ -z "$zstd_path" ]; then
	tmp_zstd=$(mktemp -d "${TMPDIR:-/tmp}/zstd-import.XXXXXX")
	zstd_path=$tmp_zstd
	git clone "$repo" "$zstd_path"
fi

zstd_path=$(cd "$zstd_path" && pwd)
git -C "$zstd_path" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
	die "not a git tree: $zstd_path"

git -C "$zstd_path" fetch --tags "$repo" "$ref"
git -C "$zstd_path" checkout --detach FETCH_HEAD
zstd_commit=$(git -C "$zstd_path" rev-parse HEAD)

contrib_path=$zstd_path/contrib/linux-kernel
[ -f "$contrib_path/Makefile" ] || die "missing $contrib_path/Makefile"

echo "zstd repo:  $repo"
echo "zstd ref:   $ref"
echo "zstd tree:  $zstd_path"
echo "linux tree: $linux_path"

if [ "$run_test" -eq 1 ]; then
	make -C "$contrib_path" test
fi

make -C "$contrib_path" import LINUX="$linux_path"

python3 - "$linux_path" <<'PY'
import sys
from pathlib import Path

linux = Path(sys.argv[1])

def insert_after(relative_path, marker, addition):
    path = linux / relative_path
    text = path.read_text()
    if addition in text:
        return
    if marker not in text:
        raise SystemExit(f"error: missing kernel integration marker in {path}")
    path.write_text(text.replace(marker, marker + addition, 1))

insert_after(
    "lib/zstd/common/zstd_internal.h",
    "#define ZSTD_TRACE 0\n",
    "\n#if defined(ZSTD_ARCH_RISCV_RVV)\n"
    "#include <riscv_vector.h>\n"
    "#endif\n",
)
insert_after(
    "lib/zstd/compress/hist.c",
    '#include "hist.h"\n',
    "\n#if defined(ZSTD_ARCH_ARM_SVE2)\n"
    "#include <arm_sve.h>\n"
    "#endif\n",
)
insert_after(
    "lib/zstd/compress/zstd_compress.c",
    '#include  "../common/bits.h"      /* ZSTD_highbit32, ZSTD_rotateRight_U64 */\n',
    "\n#if defined(ZSTD_ARCH_ARM_SVE2)\n"
    "#include <arm_sve.h>\n"
    "#endif\n",
)
insert_after(
    "lib/zstd/compress/zstd_lazy.c",
    '#include "../common/bits.h" /* ZSTD_countTrailingZeros64 */\n',
    "\n#if defined(ZSTD_ARCH_RISCV_RVV)\n"
    "#include <riscv_vector.h>\n"
    "#endif\n",
)

hist_h = linux / "lib/zstd/compress/hist.h"
text = hist_h.read_text()
old = "#if defined(__ARM_FEATURE_SVE2)\n#define HIST_WKSP_SIZE_U32 0"
new = "#if defined(ZSTD_ARCH_ARM_SVE2)\n#define HIST_WKSP_SIZE_U32 0"
if old in text:
    text = text.replace(old, new, 1)
elif new not in text:
    raise SystemExit(f"error: unrecognized SVE2 workspace guard in {hist_h}")
hist_h.write_text(text)
PY

python3 - "$linux_path" <<'PY'
import sys
from pathlib import Path

linux = Path(sys.argv[1])
paths = [
    linux / "include/linux/zstd.h",
    linux / "include/linux/zstd_errors.h",
    linux / "include/linux/zstd_lib.h",
]
paths.extend(path for path in (linux / "lib/zstd").rglob("*") if path.is_file())

for path in paths:
    lines = [line.rstrip(" \t") for line in path.read_text().splitlines()]
    while lines and not lines[-1]:
        lines.pop()
    path.write_text("\n".join(lines) + "\n")
PY

cat > "$linux_path/lib/zstd/README.upstream" <<EOF_UPSTREAM
SPDX-License-Identifier: GPL-2.0

The imported include/linux/zstd*.h and lib/zstd sources are from:

Repository: $repo
Commit: $zstd_commit
EOF_UPSTREAM

zstd_desc=$(git -C "$zstd_path" describe --tags --always 2>/dev/null || true)
zstd_ver=$(awk '
	/^#define ZSTD_VERSION_(MAJOR|MINOR|RELEASE)/ {
		if (out != "")
			out = out "."
		out = out $3
	}
	END {
		print out
	}
' "$linux_path/include/linux/zstd_lib.h")

echo
echo "Import complete."
echo "upstream: $zstd_desc"
echo "upstream commit: $zstd_commit"
echo "kernel zstd version: $zstd_ver"
echo "modified files:"
git -C "$linux_path" status --short -- \
	include/linux/zstd.h \
	include/linux/zstd_errors.h \
	include/linux/zstd_lib.h \
	lib/zstd \
	scripts/update-zstd.sh

if [ -n "$tmp_zstd" ] && [ "$keep_workdir" -eq 1 ]; then
	echo "temporary checkout kept at: $tmp_zstd"
fi
