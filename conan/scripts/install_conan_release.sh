#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

conan install . --build=missing --profile=conan/profiles/linux-gcc-release.profile -of=build/release-gcc