#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cifs-utils
PKG_VERSION:=6.6
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://ftp.samba.org/pub/linux-cifs/cifs-utils/$(PKG_NAME)
PKG_MD5SUM:=8b11350fade47d308b8f377e06cce243

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/cifs-utils
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GNU cifs-utilsc
  URL:=https://ftp.samba.org/
endef

define Package/cifs-utils/description
	Tar is a program for mount.cifs
endef

define Package/cifs-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/sbin/mount.cifs $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,cifs-utils))
