# 
#  Copyright (C) 2007 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=netdrive
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/netdrive
	SECTION:=utils
	CATEGORY:=Network
	TITLE:= map public services e.g. Dropbox to act as a network drive on the LAN.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/netdrive/install
	install -d -m0755 $(1)/usr/sbin
	install -d -m0755 $(1)/etc
	install -m0755 $(PKG_BUILD_DIR)/netdrive $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/netdrive.sh $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/nusb_right.sh $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/getpcode $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/file_notify $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/am_listen $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/drive_login_link $(1)/etc/
endef

$(eval $(call BuildPackage,netdrive))
