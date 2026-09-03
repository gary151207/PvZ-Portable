#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESOURCE_DIR="${SCRIPT_DIR}/../Plants vs. Zombies (AIO v1.0)/Plants vs. Zombies GOTY EN/1.2.0.1073 EN Origin"
EXECUTABLE="${SCRIPT_DIR}/build/pvz-portable.exe"
OUTPUT_PATH="${SCRIPT_DIR}/dist/pvz-portable.zip"

if ! command -v zip >/dev/null 2>&1; then
    echo "Error: zip command not found." >&2
    exit 1
fi

MAIN_PAK="${RESOURCE_DIR}/main.pak"
PROPERTIES_DIR="${RESOURCE_DIR}/properties"

if [[ ! -f "${EXECUTABLE}" ]]; then
    echo "Error: executable not found: ${EXECUTABLE}" >&2
    exit 1
fi

if [[ ! -f "${MAIN_PAK}" ]]; then
    echo "Error: resource file not found: ${MAIN_PAK}" >&2
    exit 1
fi

if [[ ! -d "${PROPERTIES_DIR}" ]]; then
    echo "Error: resource directory not found: ${PROPERTIES_DIR}" >&2
    exit 1
fi

STAGING_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "${STAGING_DIR}"
}
trap cleanup EXIT

cp "${EXECUTABLE}" "${STAGING_DIR}/pvz-portable.exe"
cp "${MAIN_PAK}" "${STAGING_DIR}/main.pak"
cp -R "${PROPERTIES_DIR}" "${STAGING_DIR}/properties"

mkdir -p "$(dirname "${OUTPUT_PATH}")"
rm -f "${OUTPUT_PATH}"
(
    cd "${STAGING_DIR}"
    zip -qr "${OUTPUT_PATH}" pvz-portable.exe main.pak properties
)

echo "Created: ${OUTPUT_PATH}"
