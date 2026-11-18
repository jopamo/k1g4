#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

usage() {
	cat <<USAGE
Usage: $(basename "$0") [options]

Sync a patch branch on top of the latest upstream Linux branch.

Options:
  --remote <name>          Upstream remote name (default: upstream)
  --url <url>              Upstream remote URL (default: https://github.com/torvalds/linux.git)
  --upstream-ref <ref>     Upstream branch/ref (default: master)
  --branch <name>          Patch branch to rebase (default: current branch)
  --fetch-only             Only fetch upstream; do not rebase
  --no-rebase-merges       Rebase without preserving merges
  -h, --help               Show this help
USAGE
}

die() {
	echo "error: $*" >&2
	exit 1
}

note() {
	echo "==> $*"
}

remote="upstream"
url="https://github.com/torvalds/linux.git"
upstream_ref="master"
branch=""
fetch_only=0
rebase_merges=1

while [[ $# -gt 0 ]]; do
	case "$1" in
	--remote)
		[[ $# -ge 2 ]] || die "--remote requires a value"
		remote="$2"
		shift 2
		;;
	--url)
		[[ $# -ge 2 ]] || die "--url requires a value"
		url="$2"
		shift 2
		;;
	--upstream-ref)
		[[ $# -ge 2 ]] || die "--upstream-ref requires a value"
		upstream_ref="$2"
		shift 2
		;;
	--branch)
		[[ $# -ge 2 ]] || die "--branch requires a value"
		branch="$2"
		shift 2
		;;
	--fetch-only)
		fetch_only=1
		shift
		;;
	--no-rebase-merges)
		rebase_merges=0
		shift
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		die "unknown option: $1"
		;;
	esac
done

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "run this inside a git repository"

if [[ -z "$branch" ]]; then
	branch="$(git rev-parse --abbrev-ref HEAD)"
fi

[[ "$branch" != "HEAD" ]] || die "detached HEAD; pass --branch"

git rev-parse --verify --quiet "$branch" >/dev/null || die "branch '$branch' does not exist"

if [ -n "$(git status --porcelain --untracked-files=normal --ignore-submodules=all)" ]; then
	die "working tree has changes or untracked files; commit, stash, or clean first"
fi

if git remote get-url "$remote" >/dev/null 2>&1; then
	current_url="$(git remote get-url "$remote")"
	if [[ "$current_url" != "$url" ]]; then
		note "remote '$remote' exists at '$current_url' (ignoring requested URL '$url')"
	fi
else
	note "adding remote '$remote' -> $url"
	git remote add "$remote" "$url"
fi

note "fetching $remote/$upstream_ref"
git fetch "$remote" "$upstream_ref"

upstream="$remote/$upstream_ref"
git rev-parse --verify --quiet "$upstream" >/dev/null || die "missing fetched ref '$upstream'"

patch_count_before="$(git rev-list --count "$upstream..$branch")"
note "patch commits to replay: $patch_count_before"

if ((fetch_only)); then
	note "fetch-only mode complete"
	exit 0
fi

current_branch="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$current_branch" != "$branch" ]]; then
	note "checking out $branch"
	git checkout "$branch"
fi

rebase_args=()
if ((rebase_merges)); then
	rebase_args+=(--rebase-merges)
fi

note "rebasing $branch onto $upstream"
if git rebase "${rebase_args[@]}" "$upstream"; then
	patch_count_after="$(git rev-list --count "$upstream..$branch")"
	note "rebase complete"
	note "patch commits after rebase: $patch_count_after"
	note "next: git push --force-with-lease origin $branch"
else
	die "rebase stopped on conflicts; resolve then run 'git rebase --continue' or abort with 'git rebase --abort'"
fi
