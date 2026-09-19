#!/usr/bin/env sh
# PvZ-Portable native build script (Linux/macOS)
#
# Usage: ./build.sh [release|debug] [clean]
#
#   release   Release build (default)
#   debug     Debug build
#   clean     Delete the build directory before configuring
#
# Environment overrides:
#   BUILD_DIR        build directory (default: build)
#   CMAKE_GENERATOR  CMake generator (default: Ninja)
#   BUILD_STATIC     static link option passed to CMake (default: OFF)

set -eu

usage() {
	printf '%s\n' 'Usage: ./build.sh [release|debug] [clean]'
}

die() {
	printf 'Error: %s\n' "$*" >&2
	exit 1
}

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR" || die 'cannot enter the repository directory.'

BUILD_TYPE=Release
DO_CLEAN=false

for theArgument in "$@"; do
	case "$theArgument" in
		release) BUILD_TYPE=Release ;;
		debug) BUILD_TYPE=Debug ;;
		clean) DO_CLEAN=true ;;
		*)
			printf 'Error: unknown argument "%s".\n\n' "$theArgument" >&2
			usage >&2
			exit 2
			;;
	esac
done

BUILD_DIR=${BUILD_DIR:-build}
CMAKE_GENERATOR=${CMAKE_GENERATOR:-Ninja}
BUILD_STATIC=${BUILD_STATIC:-OFF}

case "$BUILD_DIR" in
	build|build-*) ;;
	*)
		if [ "$DO_CLEAN" = true ]; then
			die "refusing to delete \"$BUILD_DIR\"; only build and build-* are allowed."
		fi
		;;
esac

printf '%s\n' \
	'==========================================================' \
	' PvZ-Portable native build' \
	"   Build type : $BUILD_TYPE" \
	"   Static     : $BUILD_STATIC" \
	"   Build dir  : $BUILD_DIR" \
	"   Generator  : $CMAKE_GENERATOR" \
	'==========================================================' \
	''

command -v cmake >/dev/null 2>&1 || die 'CMake was not found in PATH.'
if [ "$CMAKE_GENERATOR" = Ninja ]; then
	command -v ninja >/dev/null 2>&1 || die 'Ninja was not found in PATH.'
fi
command -v python3 >/dev/null 2>&1 || die 'Python 3 was not found in PATH; CMake needs it to pack main.pak.'

if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
	CACHED_GENERATOR=$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)
	if [ -n "$CACHED_GENERATOR" ] && [ "$CACHED_GENERATOR" != "$CMAKE_GENERATOR" ]; then
		die "\"$BUILD_DIR\" is configured for generator \"$CACHED_GENERATOR\", not \"$CMAKE_GENERATOR\". Run ./build.sh clean to reconfigure from scratch."
	fi
fi

if [ "$DO_CLEAN" = true ] && [ -d "$BUILD_DIR" ]; then
	printf 'Removing "%s" ...\n' "$BUILD_DIR"
	rm -rf -- "$BUILD_DIR"
	[ ! -e "$BUILD_DIR" ] || die "could not remove \"$BUILD_DIR\"."
	printf 'Cleaned "%s".\n\n' "$BUILD_DIR"
fi

printf '%s\n' '[1/3] Configuring with CMake ...'
cmake -S . -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DBUILD_STATIC="$BUILD_STATIC"

printf '%s\n' '' '[2/3] Building ...'
START_TIME=$(date +%s)
cmake --build "$BUILD_DIR" --parallel
ELAPSED=$(( $(date +%s) - START_TIME ))

printf '%s\n' '' '[3/3] Verifying output ...'
EXECUTABLE_PATH=
for theCandidate in "dist/pvz-portable" "dist/pvz-portable.exe" "$BUILD_DIR/pvz-portable" "$BUILD_DIR/pvz-portable.exe"; do
	if [ -f "$theCandidate" ]; then
		EXECUTABLE_PATH=$theCandidate
		break
	fi
done

[ -n "$EXECUTABLE_PATH" ] || die 'the build finished but no pvz-portable executable was found.'
printf '  Executable : %s (%s bytes)\n' "$(cd "$(dirname "$EXECUTABLE_PATH")" && pwd)/$(basename "$EXECUTABLE_PATH")" "$(wc -c < "$EXECUTABLE_PATH" | tr -d ' ')"

if [ -d dist ]; then
	if [ -f dist/main.pak ]; then
		printf '  Resources  : %s (%s bytes)\n' "$(pwd)/dist/main.pak" "$(wc -c < dist/main.pak | tr -d ' ')"
	else
		printf '%s\n' '  Resources  : dist/main.pak is missing (check res/main).'
	fi
	if [ -d dist/properties ]; then
		printf '%s\n' '  Properties : dist/properties found.'
	else
		printf '%s\n' '  Properties : dist/properties is missing (check res/properties).'
	fi
fi

printf '\nBuild succeeded in ~%ss.\n' "$ELAPSED"
