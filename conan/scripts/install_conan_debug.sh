#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

conan install . --build=missing --profile:all=conan/profiles/linux-gcc-debug.profile -of=build/debug-gcc