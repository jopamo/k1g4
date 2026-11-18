#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Update the kernel's LZ4 import from upstream.
#
# This script:
#   1) fetches lz4 at a chosen ref
#   2) copies upstream lib/lz4.{c,h}
#   3) applies kernel-compat patches required by freestanding wrappers
#   4) updates the kernel's public LZ4 stream-state ABI shim
#   5) optionally runs upstream tests and kernel KUnit LZ4 tests
#   6) records the exact imported upstream commit after verification
#
# Example:
#   ./scripts/update-lz4.sh --ref dev

set -eu

die() {
	echo "error: $*" >&2
	exit 1
}

usage() {
	cat <<EOF_USAGE
Usage: $0 [options]

Options:
  --linux <path>         Linux tree to update (default: current git toplevel)
  --repo <url>           lz4 git URL (default: https://github.com/lz4/lz4)
  --ref <ref>            lz4 ref to import (default: dev)
  --lz4-path <path>      Existing lz4 checkout to reuse
  --skip-upstream-test   Skip upstream 'make test'
  --skip-kunit           Skip kernel KUnit LZ4 verification
  --kunit-build-dir <p>  KUnit build dir (default: /tmp/linux-kunit-lz4)
  --kunit-timeout <sec>  KUnit timeout (default: 300)
  --allow-dirty          Allow importing into a dirty Linux tree
  --keep-workdir         Keep temporary lz4 checkout
  -h, --help             Show this help
EOF_USAGE
}

linux_path=""
repo="https://github.com/lz4/lz4"
ref="dev"
lz4_path=""
run_upstream_test=1
run_kunit=1
kunit_build_dir="/tmp/linux-kunit-lz4"
kunit_timeout=300
allow_dirty=0
keep_workdir=0
tmp_lz4=""

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
	--lz4-path)
		[ "$#" -ge 2 ] || die "--lz4-path needs a path"
		lz4_path=$2
		shift 2
		;;
	--skip-upstream-test)
		run_upstream_test=0
		shift
		;;
	--skip-kunit)
		run_kunit=0
		shift
		;;
	--kunit-build-dir)
		[ "$#" -ge 2 ] || die "--kunit-build-dir needs a path"
		kunit_build_dir=$2
		shift 2
		;;
	--kunit-timeout)
		[ "$#" -ge 2 ] || die "--kunit-timeout needs seconds"
		kunit_timeout=$2
		shift 2
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

[ -f "$linux_path/include/linux/lz4.h" ] || die "missing include/linux/lz4.h"
[ -f "$linux_path/lib/lz4/lz4_compress.c" ] || die "missing lib/lz4/lz4_compress.c"
[ -f "$linux_path/lib/lz4/lz4_decompress.c" ] || die "missing lib/lz4/lz4_decompress.c"
[ -f "$linux_path/lib/lz4/Makefile" ] || die "missing lib/lz4/Makefile"

if [ "$allow_dirty" -eq 0 ] &&
   [ -n "$(git -C "$linux_path" status --porcelain --untracked-files=no)" ]; then
	die "Linux tree is dirty; commit/stash first or pass --allow-dirty"
fi

cleanup() {
	if [ -n "$tmp_lz4" ] && [ "$keep_workdir" -eq 0 ] && [ -d "$tmp_lz4" ]; then
		rm -rf "$tmp_lz4"
	fi
}
trap cleanup EXIT INT TERM HUP

if [ -z "$lz4_path" ]; then
	tmp_lz4=$(mktemp -d "${TMPDIR:-/tmp}/lz4-import.XXXXXX")
	lz4_path=$tmp_lz4
	git clone "$repo" "$lz4_path"
fi

lz4_path=$(cd "$lz4_path" && pwd)
git -C "$lz4_path" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
	die "not a git tree: $lz4_path"

git -C "$lz4_path" fetch --tags "$repo" "$ref"
git -C "$lz4_path" checkout --detach FETCH_HEAD
lz4_commit=$(git -C "$lz4_path" rev-parse HEAD)

[ -f "$lz4_path/lib/lz4.c" ] || die "missing upstream lib/lz4.c"
[ -f "$lz4_path/lib/lz4.h" ] || die "missing upstream lib/lz4.h"

if [ "$run_upstream_test" -eq 1 ]; then
	make -C "$lz4_path" test
fi

cp "$lz4_path/lib/lz4.c" "$linux_path/lib/lz4/lz4.c"
cp "$lz4_path/lib/lz4.h" "$linux_path/lib/lz4/lz4.h"

patch_lz4_h() {
	python3 - "$1" <<'PY'
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
s = path.read_text()

orig = '#include <stddef.h>   /* size_t */'
if orig not in s:
    raise SystemExit(f"error: expected '{orig}' in {path}")
s = s.replace(orig, '#include <linux/types.h>', 1)

pattern = re.compile(
    r'#if defined\(__cplusplus\) \|\| \(defined \(__STDC_VERSION__\) && \(__STDC_VERSION__ >= 199901L\) /\* C99 \*/\)\n'
    r'# include <stdint.h>\n'
    r'  typedef int8_t         LZ4_i8;\n'
    r'  typedef unsigned char  LZ4_byte;\n'
    r'  typedef uint16_t       LZ4_u16;\n'
    r'  typedef uint32_t       LZ4_u32;\n'
    r'#else\n'
    r'  typedef   signed char  LZ4_i8;\n'
    r'  typedef unsigned char  LZ4_byte;\n'
    r'  typedef unsigned short LZ4_u16;\n'
    r'  typedef unsigned int   LZ4_u32;\n'
    r'#endif'
)
replacement = (
    'typedef s8             LZ4_i8;\n'
    'typedef u8             LZ4_byte;\n'
    'typedef u16            LZ4_u16;\n'
    'typedef u32            LZ4_u32;'
)
s, n = pattern.subn(replacement, s, count=1)
if n != 1:
    raise SystemExit(f"error: failed to patch typedef block in {path}")

path.write_text(s)
PY
}

patch_kernel_lz4_h() {
	python3 - "$1" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
s = path.read_text()

old_stream = """typedef struct {
\tuint32_t hashTable[LZ4_HASH_SIZE_U32];
\tuint32_t currentOffset;
\tuint32_t initCheck;
\tconst uint8_t *dictionary;
\tuint8_t *bufferStart;
\tuint32_t dictSize;
} LZ4_stream_t_internal;"""
new_stream = """typedef struct LZ4_stream_t_internal {
\tuint32_t hashTable[LZ4_HASH_SIZE_U32];
\tconst uint8_t *dictionary;
\tconst struct LZ4_stream_t_internal *dictCtx;
\tuint32_t currentOffset;
\tuint32_t tableType;
\tuint32_t dictSize;
} LZ4_stream_t_internal;"""
old_decode = """typedef struct {
\tconst uint8_t *externalDict;
\tsize_t extDictSize;
\tconst uint8_t *prefixEnd;
\tsize_t prefixSize;
} LZ4_streamDecode_t_internal;"""
new_decode = """typedef struct {
\tconst uint8_t *externalDict;
\tconst uint8_t *prefixEnd;
\tsize_t extDictSize;
\tsize_t prefixSize;
} LZ4_streamDecode_t_internal;"""

if old_stream in s:
    s = s.replace(old_stream, new_stream, 1)
elif new_stream not in s:
    raise SystemExit(f"error: unrecognized LZ4 stream layout in {path}")

if old_decode in s:
    s = s.replace(old_decode, new_decode, 1)
elif new_decode not in s:
    raise SystemExit(f"error: unrecognized LZ4 decode layout in {path}")

path.write_text(s)
PY
}

patch_lz4_c() {
	python3 - "$1" <<'PY'
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
s = path.read_text()

pattern = re.compile(
    r'#include <limits.h>\n'
    r'#if defined\(__cplusplus\) \|\| \(defined \(__STDC_VERSION__\) && \(__STDC_VERSION__ >= 199901L\) /\* C99 \*/\)\n'
    r'# include <stdint.h>\n'
    r'  typedef unsigned char BYTE; /\*uint8_t not necessarily blessed to alias arbitrary type\*/\n'
    r'  typedef uint16_t      U16;\n'
    r'  typedef uint32_t      U32;\n'
    r'  typedef  int32_t      S32;\n'
    r'  typedef uint64_t      U64;\n'
    r'  typedef uintptr_t     uptrval;\n'
    r'#else\n'
    r'# if UINT_MAX != 4294967295UL\n'
    r'#   error "LZ4 code \(when not C\+\+ or C99\) assumes that sizeof\(int\) == 4"\n'
    r'# endif\n'
    r'  typedef unsigned char       BYTE;\n'
    r'  typedef unsigned short      U16;\n'
    r'  typedef unsigned int        U32;\n'
    r'  typedef   signed int        S32;\n'
    r'  typedef unsigned long long  U64;\n'
    r'  typedef size_t              uptrval;   /\* generally true, except OpenVMS-64 \*/\n'
    r'#endif'
)
replacement = (
    '#include <linux/limits.h>\n'
    'typedef unsigned char BYTE; /* u8 is not necessarily blessed to alias arbitrary type */\n'
    'typedef u16           U16;\n'
    'typedef u32           U32;\n'
    'typedef s32           S32;\n'
    'typedef u64           U64;\n'
    'typedef uintptr_t     uptrval;'
)
s, n = pattern.subn(replacement, s, count=1)
if n != 1:
    raise SystemExit(f"error: failed to patch type block in {path}")

path.write_text(s)
PY
}

patch_lz4_h "$linux_path/lib/lz4/lz4.h"
patch_lz4_c "$linux_path/lib/lz4/lz4.c"
patch_kernel_lz4_h "$linux_path/include/linux/lz4.h"

if [ "$run_kunit" -eq 1 ]; then
	[ -x "$linux_path/tools/testing/kunit/kunit.py" ] ||
		die "missing tools/testing/kunit/kunit.py"

	kcfg=$(mktemp "${TMPDIR:-/tmp}/lz4-kunit-config.XXXXXX")
	trap 'rm -f "$kcfg"; cleanup' EXIT INT TERM HUP
	cat > "$kcfg" <<'EOF_KUNIT'
CONFIG_KUNIT=y
CONFIG_KUNIT_TEST=y
CONFIG_LZ4_COMPRESS=y
CONFIG_LZ4_DECOMPRESS=y
CONFIG_LZ4_KUNIT_TEST=y
EOF_KUNIT

	"$linux_path/tools/testing/kunit/kunit.py" run \
		--arch=x86_64 \
		--kunitconfig="$kcfg" \
		--build_dir="$kunit_build_dir" \
		--timeout="$kunit_timeout"

	rm -f "$kcfg"
	trap cleanup EXIT INT TERM HUP
fi

cat > "$linux_path/lib/lz4/README.upstream" <<EOF_UPSTREAM
SPDX-License-Identifier: GPL-2.0

The imported lib/lz4/lz4.c and lib/lz4/lz4.h are from:

Repository: $repo
Commit: $lz4_commit
EOF_UPSTREAM

lz4_desc=$(git -C "$lz4_path" describe --tags --always 2>/dev/null || true)
lz4_ver=$(awk '
	/^#define LZ4_VERSION_(MAJOR|MINOR|RELEASE)/ {
		if (out != "")
			out = out "."
		out = out $3
	}
	END { print out }
' "$linux_path/lib/lz4/lz4.h")

echo
echo "Import complete."
echo "upstream: $lz4_desc"
echo "upstream commit: $lz4_commit"
echo "kernel lib/lz4 version: $lz4_ver"
echo "modified files:"
git -C "$linux_path" status --short -- \
	lib/Kconfig \
	include/linux/lz4.h \
	lib/lz4 \
	scripts/update-lz4.sh

if [ -n "$tmp_lz4" ] && [ "$keep_workdir" -eq 1 ]; then
	echo "temporary checkout kept at: $tmp_lz4"
fi
