#!/bin/sh
# Project home: https://github.com/puuu/MQTT433gateway/
# Script to prepare MQTT433gateway binaries for deploying
# Copyright (c) 2018 Puuu
# Permission to copy and modify is granted under the MIT license

# Skipping non-tagged commit
[ -z "$(git tag --contains)" ] && exit 0

VERSION=$(git describe --abbrev=8 --dirty --always --tags)

echo "Preparing release for tagged version: $VERSION"

for file in .pio/build/*/firmware.bin; do
  env=$(echo "$file" | cut -f3 -d'/')
  cp "$file" "dist/mqtt433gateway_${env}-${VERSION}.bin"
done
