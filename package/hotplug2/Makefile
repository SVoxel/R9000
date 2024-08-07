#
# Copyright (c) 2013 The Linux Foundation. All rights reserved.
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=hotplug2
PKG_REV:=201
PKG_VERSION:=$(PKG_REV)
PKG_RELEASE:=4

PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=hotplug2-$(PKG_VERSION)
PKG_SOURCE_URL:=http://svn.nomi.cz/svn/isteve/hotplug2
PKG_SOURCE:=$(PKG_SOURCE_SUBDIR).tar.gz
#PKG_SOURCE_URL:=http://isteve.bofh.cz/~isteve/hotplug2
#PKG_MD5SUM:=ea2c01d027b4002e4e6b0ff266f51a51

PKG_MIRROR_MD5SUM:=9a8e64f89558998bb824525105cdbe6b

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -Wno-implicit-function-declaration

define Package/hotplug2
  SECTION:=base
  CATEGORY:=Base system
  VERSION:=1.0-beta-$(PKG_RELEASE)
  TITLE:=Version 1.0 Dynamic device management subsystem for embedded systems
  URL:=http://isteve.bofh.cz/~isteve/hotplug2/
  DEPENDS:=+!USE_UCLIBC:libbsd
endef

define Package/hotplug2/description
 Hotplug2 is a trivial replacement of some of the UDev functionality
 in a tiny pack, intended for Linux early userspace: Init RAM FS and InitRD.
 This is an implementation of Hotplug2-1.0-beta
endef

define Package/hotplug2/conffiles
/etc/hotplug2.rules
endef

MAKE_FLAGS += \
	COPTS="$(TARGET_CFLAGS)" \
	STATIC_WORKER="fork"

ifneq ($(CONFIG_USE_EGLIBC),)
  TARGET_LDFLAGS += -lbsd
endif

define Build/Compile
	$(call Build/Compile/Default)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/udevtrigger src/udevtrigger.c
endef

define Package/hotplug2/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DATA) ./files/hotplug2.rules $(1)/etc/
	$(INSTALL_BIN) ./files/usb_modem_hotplug $(1)/etc/
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udevtrigger $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hotplug2 $(1)/sbin/
	$(INSTALL_BIN) ./files/hotplug2.mount $(1)/sbin/
	$(INSTALL_BIN) ./files/hotplug2.umount $(1)/sbin/
	$(INSTALL_BIN) ./files/umount_unapproved $(1)/sbin/
	$(INSTALL_BIN) ./files/app_mount $(1)/sbin/
	$(INSTALL_BIN) ./files/hotplug2.testbusy $(1)/sbin/
	-$(INSTALL_BIN) ./files/green_download_path $(1)/sbin/
endef

$(eval $(call BuildPackage,hotplug2))
