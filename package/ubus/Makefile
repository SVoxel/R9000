include $(TOPDIR)/rules.mk

PKG_NAME:=ubus
PKG_VERSION:=2025-05-16-88e63250
PKG_RELEASE=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=$(PROJECT_GIT)/project/ubus.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=88e63250
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_MIRROR_MD5SUM:=263d5ef39d450719d8ae81b8f0da588c
CMAKE_INSTALL:=1

PKG_LICENSE:=LGPLv2.1
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/ubus
  SECTION:=luci2
  CATEGORY:=LuCI2
  SUBMENU:=System
  DEPENDS:=+libubus +libblobmsg-json +ubusd
  TITLE:=OpenWrt RPC client utility
endef

define Package/ubusd
  SECTION:=luci2
  CATEGORY:=LuCI2
  SUBMENU:=System
  TITLE:=OpenWrt RPC daemon
  DEPENDS:=+libubox
endef

define Package/libubus
  SECTION:=luci2
  CATEGORY:=LuCI2
  SUBMENU:=Libraries
  DEPENDS:=+libubox
  TITLE:=OpenWrt RPC client library
endef

define Package/libubus-lua
  SECTION:=luci2
  CATEGORY:=LuCI2
  SUBMENU:=Libraries
  DEPENDS:=+libubus +liblua
  TITLE:=Lua binding for the OpenWrt RPC client
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include

CMAKE_OPTIONS = \
	-DLUAPATH=/usr/lib/lua

define Package/ubus/install
	$(INSTALL_DIR) $(1)/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ubus $(1)/bin/
endef

define Package/ubusd/install
	$(INSTALL_DIR) $(1)/sbin $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ubus.init $(1)/etc/init.d/ubus
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/ubusd $(1)/sbin/
endef

define Package/libubus/install
	$(INSTALL_DIR) $(1)/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/lib/
endef

define Package/libubus-lua/install
	$(INSTALL_DIR) $(1)/usr/lib/lua
	$(CP) $(PKG_BUILD_DIR)/lua/ubus.so $(1)/usr/lib/lua/
endef

$(eval $(call BuildPackage,ubus))
$(eval $(call BuildPackage,ubusd))
$(eval $(call BuildPackage,libubus))
$(eval $(call BuildPackage,libubus-lua))
