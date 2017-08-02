# 
# Copyright (C) 2014 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=trend-micro
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/trend-micro
  CATEGORY:=Bigfoot
  TITLE:=trend micro script and configure files 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/trend-micro/install
	install -d -m0755 $(1)/iQoS
	install -d -m0755 $(1)/iQoS/R9000
	install -d -m0755 $(1)/iQoS/R9000/TM/
	install -d -m0755 $(1)/iQoS/R9000/tm_key/
	install -d -m0755 $(1)/iQoS/R9000/tm_pattern/
	install -m0755 $(PKG_BUILD_DIR)/priority $(1)/iQoS/R9000/TM/
	install -m0755 $(PKG_BUILD_DIR)/TM/* $(1)/iQoS/R9000/TM/
	install -m0755 $(PKG_BUILD_DIR)/tm_key/* $(1)/iQoS/R9000/tm_key
	install -m0755 $(PKG_BUILD_DIR)/tm_pattern/* $(1)/iQoS/R9000/tm_pattern/
	(cd $(1)/; $(LN) /iQoS/R9000/TM TM)
	(cd $(1)/; $(LN) /iQoS/R9000/tm_key tm_key)
	(cd $(1)/; $(LN) /iQoS/R9000/tm_pattern tm_pattern)
endef

$(eval $(call BuildPackage,trend-micro))
