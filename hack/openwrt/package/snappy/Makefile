#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=snappy
PKG_VERSION:=1.1.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/google/snappy/archive/
PKG_MD5SUM:=ee9086291c9ae8deb4dac5e0b85bf54a

PKG_BUILD_PARALLEL:=1

PKG_LICENSE:=BSD-3-Clause

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/snappy
  DEPENDS:=+libstdcpp
  SECTION:=libs
  CATEGORY:=Libaries
  TITLE:=A fast compressor/decompressor library
  URL:=https://google.github.io/snappy/
endef

define Package/snappy-dev
  DEPENDS:=snappy
  SECTION:=devel
  CATEGORY:=Development
  SUBMENU:=Libraries
  TITLE:=Development files for the snappy library
  URL:=https://google.github.io/snappy/
endef

define Package/snappy/description
 A fast compressor/decompressor library
endef

define Package/snappy-dev/description
 A fast compressor/decompressor library
 This package includes the development support files.
endef

CMAKE_OPTIONS += \
	-DBUILD_SHARED_LIBS=ON

define Package/snappy/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsnappy.so $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsnappy.so.* $(1)/usr/lib/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/include $(1)/usr/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsnappy.so* $(1)/usr/lib/
endef

define Package/snappy-dev/install
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/snappy.h \
	  $(1)/usr/include/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/snappy-c.h \
	  $(1)/usr/include/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/snappy-sinksource.h \
	  $(1)/usr/include/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/snappy-stubs-public.h \
	  $(1)/usr/include/
endef

$(eval $(call BuildPackage,snappy))
$(eval $(call BuildPackage,snappy-dev))
