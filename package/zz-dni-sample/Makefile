# 
#  Copyright (C) 2007-2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=zz-dni-sample
PKG_RELEASE:=1

PKG_BUILD_DEPENDS:=datalib
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/zz-dni-sample
	CATEGORY:=Bigfoot
	TITLE:=trend micro sample.bin sourcecode and configure files
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -rf ./sample_src/* $(PKG_BUILD_DIR)/
	$(call Build/Patch/Default)
endef


define Build/Compile
endef

define Package/zz-dni-sample/install
	$(INSTALL_DIR) $(1)/iQoS
	$(INSTALL_DIR) $(1)/iQoS/R9000
	$(INSTALL_DIR) $(1)/iQoS/R9000/TM/
	$(INSTALL_DIR) $(1)/iQoS/R9000/tm_pattern/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tdts_udb_sample/sample.bin $(1)/iQoS/R9000/TM/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tdts_udb_sample/sample.bin $(1)/iQoS/R9000/tm_pattern/
endef

$(eval $(call BuildPackage,zz-dni-sample))
