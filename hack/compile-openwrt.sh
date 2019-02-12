#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

SCRIPT_ROOT=$(dirname "${BASH_SOURCE}")

BUILD_IMAGE_NAME=simonswine/lokicat:build-openwrt

BUILD_ROOT=${1:-/opt/openwrt-mvebu-cortexa9}

# ensure dockerfile is ready
docker build -t "${BUILD_IMAGE_NAME}" -f "${SCRIPT_ROOT}/../Dockerfile.openwrt" "${SCRIPT_ROOT}/.."

# create container instance
container_id=$(docker create --workdir "${BUILD_ROOT}" ${BUILD_IMAGE_NAME} make package/lokicat/compile)
cleanup_container() {
    docker rm -f "${container_id}"
}
trap "cleanup_container" EXIT SIGINT

tmpfile=$(mktemp /tmp/lokicat-hack.XXXXXX)
cleanup_tmpfile() {
    rm -f "${tmpfile}"
}
trap "cleanup_tmpfile" EXIT SIGINT
cat > ${tmpfile} <<\EOFOUTER
#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lokicat
PKG_VERSION:=0.1.0
PKG_RELEASE:=1

PKG_BUILD_PARALLEL:=1

PKG_LICENSE:=Apache License

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/lokicat
  SECTION:=net
  CATEGORY:=Network
  TITLE:=A log forwarder to loki
  URL:=https://github.com/simonswine/lokicat/
  DEPENDS:=+snappy +libprotobuf-c +curl
endef



define Package/lokicat/description
 A log forwarder to loki
endef

define Package/lokicat/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/local/bin/lokicat $(1)/usr/bin/
endef

$(eval $(call BuildPackage,lokicat))

EOFOUTER
# copy sources into container
docker cp "${tmpfile}" "${container_id}:${BUILD_ROOT}/package/lokicat/Makefile"

# copy sources into container
git ls-files '**.proto' '**.c' '**.h' 'Makefile' | xargs tar -cv | docker cp - "${container_id}:${BUILD_ROOT}/package/lokicat/src"

docker start -a "${container_id}"
