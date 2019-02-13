#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

SCRIPT_ROOT=$(dirname "${BASH_SOURCE}")

make defconfig

# HOTFIX for protobuf-cpp
curl --fail --retry 5 -L -o ./dl/protobuf-cpp-3.3.0.tar.gz https://mirror.sobukus.de/files/src/protobuf/protobuf-cpp-3.3.0.tar.gz

# compile upstream packages
./scripts/feeds update base packages
./scripts/feeds install protobuf curl

# update to later protobuf-c release
cp -a "${SCRIPT_ROOT}/openwrt/package/protobuf-c" package/

# compile custom snappy packages
cp -a "${SCRIPT_ROOT}/openwrt/package/snappy" package/

make package/prepare

# create dirs for lokicat package
mkdir -p package/lokicat/src
