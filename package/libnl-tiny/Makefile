#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnl-tiny
PKG_VERSION:=2023-12-05-965c4bf4
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=$(PROJECT_GIT)/project/libnl-tiny.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=965c4bf4
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_MIRROR_MD5SUM:=5791a65ee47970fbe4728afb66750302
CMAKE_INSTALL:=1

PKG_LICENSE:=LGPL-2.1
PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libnl-tiny
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=netlink socket library
endef

define Package/libnl-tiny/description
 This package contains a stripped down version of libnl
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig $(1)/usr/include/libnl-tiny
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libnl-tiny/* $(1)/usr/include/libnl-tiny
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libnl-tiny.so $(1)/usr/lib/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libnl-tiny.pc $(1)/usr/lib/pkgconfig
endef

define Package/libnl-tiny/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnl-tiny.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnl-tiny))
