# 
# Copyright (C) 2008 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dni-bond
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/dni-bond
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=Mini tool to operate bonding.
endef

define Package/dni-bond/description
	It is used for R7800 bonding to hardware setting
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Package/dni-bond/install

	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bond-monitor $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bond-ctrl    $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bond-set     $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bond-show    $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lacp-debug    $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/r9000-bond-init $(1)/etc/init.d/bond-init

	# install service by default
	$(LN) -sv ../init.d/bond-init $(1)/etc/rc.d/S80bond

endef

$(eval $(call BuildPackage,dni-bond))
