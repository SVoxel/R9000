#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cifs-utils
PKG_VERSION:=6.8
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://download.samba.org/pub/linux-cifs/cifs-utils/
PKG_MD5SUM:=a385d60293e6f9e4cb0d4ac2093990d8

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/cifs-utils
  SECTION:=net
  CATEGORY:=Network
  TITLE:=CIFS mount utilities
  URL:=http://wiki.samba.org/index.php/LinuxCIFS_utils
endef

define Package/cifs-utils/description
	CIFS mount utilities
endef

define Package/cifs-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/sbin/mount.cifs $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,cifs-utils))
