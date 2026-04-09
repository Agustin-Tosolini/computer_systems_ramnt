#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="${PROJECT_DIR:-/workspace}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_DIR}/runtime/build}"
LOG_DIR="${LOG_DIR:-${PROJECT_DIR}/runtime/logs}"

mkdir -p "${BUILD_DIR}" "${LOG_DIR}"

nasm -f elf64 "${PROJECT_DIR}/float_to_int.asm" -o "${BUILD_DIR}/float_to_int.o"
gcc -shared -fPIC "${PROJECT_DIR}/interface_w_asm.c" "${BUILD_DIR}/float_to_int.o" -o "${BUILD_DIR}/libfloat_to_int.so"

python3 "${PROJECT_DIR}/api.py"
