#!/bin/sh
# Builds web/rubiksrays.wasm. Needs emcc and GLM headers (GLM=/path/to/glm to override).
set -e
cd "$(dirname "$0")"
GLM=${GLM:-/usr/include/glm}
mkdir -p inc
ln -sfn "$GLM" inc/glm
emcc -std=c++20 -O3 -flto -fno-exceptions -fno-rtti \
	-I. -Iinc \
	-sSTANDALONE_WASM --no-entry \
	-o rubiksrays.wasm ../main.cpp
ls -l rubiksrays.wasm
