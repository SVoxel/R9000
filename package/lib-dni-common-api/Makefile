include $(TOPDIR)/rules.mk

PKG_NAME:=libdni-common-api-module
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk


#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME) 

include $(INCLUDE_DIR)/package.mk


define Package/libdni-common-api-module
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=DNI common api module
endef

define Package/libdni-common-api-module/description
 This package contains a library for dni common api module.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += $(FPIC)

define Build/Compile
endef

define Build/Configure
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/lib*.so $(1)/usr/lib/
endef

define Package/libdni-common-api-module/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libdni-common-api-module))
