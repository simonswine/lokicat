#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

SCRIPT_ROOT=$(dirname "${BASH_SOURCE}")

BUILD_IMAGE_NAME=simonswine/lokicat:build-openwrt

TARGET=${1:-mvebu-cortexa9}

# ensure dockerfile is ready
docker build -t "${BUILD_IMAGE_NAME}" -f "${SCRIPT_ROOT}/../Dockerfile.openwrt" "${SCRIPT_ROOT}/.."

# create volume container if missing
volume_container_name="simonswine-lokicat-openwrt-${TARGET}"

docker inspect "${volume_container_name}" > /dev/null 2> /dev/null || docker run --name "${volume_container_name}" -v /opt/openwrt-vol "${BUILD_IMAGE_NAME}" rsync -av /opt/openwrt-image/ /opt/openwrt-vol/

# create container instance
container_id=$(docker create --volumes-from ${volume_container_name} --workdir /opt/openwrt-vol ${BUILD_IMAGE_NAME} make package/lokicat/compile V=99)
cleanup_container() {
    docker rm -f "${container_id}"
}
trap "cleanup_container" EXIT SIGINT

tmpfile=$(mktemp /tmp/lokicat-hack.XXXXXX)
cleanup_tmpfile() {
    rm -f "${tmpfile}"
}
trap "cleanup_tmpfile" EXIT SIGINT
cat > ${tmpfile} <<\EOF
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
EOF

# copy sources into container
docker cp "${tmpfile}" "${container_id}:/opt/openwrt-vol/package/lokicat/Makefile"

# copy sources into containerc
git ls-files '**.proto' '**.c' '**.h' 'Makefile' | xargs tar -cv | docker cp - "${container_id}:/opt/openwrt-vol/package/lokicat/src"

# run build
docker start -a ${container_id}

# copy packages back
docker cp "${container_id}:/opt/openwrt-vol/bin/packages" - > output.tar
