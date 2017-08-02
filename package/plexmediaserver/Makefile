#
# Copyright (C) 2015 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plexmediaserver
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/plexmediaserver
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Plex Media Server
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/plexmediaserver/install
	$(INSTALL_DIR) $(1)/etc/plexmediaserver
	$(CP) ./files/* $(1)/etc/plexmediaserver/
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/get_plex_pcode $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/plex_net_dev $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,plexmediaserver))
