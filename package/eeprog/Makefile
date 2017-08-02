# 
#  Copyright (C) 2007-2008 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=eeprog
PKG_VERSION:=0.7.6-tear5
PKG_MD5SUM:=d6408cd2c7010ebc653c24cdc2364629
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://darkswarm.org/eeprog-0.7.6-tear5.tar.gz
#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/eeprog
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=@TARGET_r9000
	TITLE:=Tool for accessing EEPROM
endef

define Build/Prepare
	tar zxvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR)
	mv $(PKG_BUILD_DIR)/eeprog-0.7.6-tear12/* $(PKG_BUILD_DIR)
	rm -r $(PKG_BUILD_DIR)/eeprog-0.7.6-tear12/
endef

define Package/eeprog/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/eeprog $(1)/usr/sbin/
	install -d -m0755 $(1)/etc/init.d
	install -m0755 ./files/check_eeprom $(1)/etc/init.d/
  #	install -m0755 ./files/check_eeprom $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,eeprog))
