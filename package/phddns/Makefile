# 
#  Copyright (C) 2011 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=phddns
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
#PKG_SOURCE_URL:=http://open.oray.com/doc/phddns-sdk-1.0.4.17314.tar.gz

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -Wno-implicit-function-declaration -Wno-incompatible-pointer-types
TARGET_CFLAGS += -Wno-int-conversion -Wno-implicit-int

define Package/phddns
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=Oray DDNS utility.
endef

define Build/Prepare
	$(CP) -r ./src/* $(PKG_BUILD_DIR)/
endef

define Package/phddns/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/phddns $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,phddns))
