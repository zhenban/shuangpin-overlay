#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds the RPM from the committed tree (HEAD). The packages end up in
# the directory given, ./rpms by default.
#
#   packaging/build-rpm.sh [output directory]
set -eu

root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
out=$(mkdir -p "${1:-rpms}" && cd "${1:-rpms}" && pwd)
spec=$root/packaging/shuangpin-overlay.spec
name=$(rpmspec -q --srpm --qf '%{name}' "$spec")
version=$(rpmspec -q --srpm --qf '%{version}' "$spec")

top=$(mktemp -d)
trap 'rm -rf "$top"' EXIT
mkdir -p "$top/SOURCES"
git -C "$root" archive --prefix="$name-$version/" -o "$top/SOURCES/$name-$version.tar.gz" HEAD

rpmbuild -ba "$spec" --define "_topdir $top"
find "$top/RPMS" "$top/SRPMS" -name '*.rpm' ! -name '*-debug*' -exec cp {} "$out/" \;
ls "$out"
