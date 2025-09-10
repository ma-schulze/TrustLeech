#!/usr/bin/env bash
set -euo pipefail

# Make sure the current working directory is the location of the script
cd "$(dirname "$0")" || exit 1

SOURCE="../arm-trusted-firmware/include/export/"
TARGET="arm-trusted-firmware/include/arm-trusted-firmware"

if ! git diff --quiet --exit-code -- "$TARGET"; then
	echo "There are unstaged changes in the target dir '${TARGET}', aborting..."
	exit 1
fi

rsync --archive --verbose --exclude "README" --delete "$SOURCE" "$TARGET"
