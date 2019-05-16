include $(TOPDIR)/rules.mk

PKG_NAME:=usbmode
PKG_VERSION:=2017-12-19-f40f84c2
PKG_RELEASE=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=$(OPENWRT_GIT)/project/usbmode.git
PKG_SOURCE_VERSION:=f40f84c2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
CMAKE_INSTALL:=1

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>

PKG_DATA_VERSION:=20170806
PKG_DATA_URL:=http://www.draisberghof.de/usb_modeswitch
PKG_DATA_PATH:=usb-modeswitch-data-$(PKG_DATA_VERSION)
PKG_DATA_FILENAME:=$(PKG_DATA_PATH).tar.bz2

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Download/data
  FILE:=$(PKG_DATA_FILENAME)
  URL:=$(PKG_DATA_URL)
  MD5SUM:=fb50d15b52e909d742dd16f0a9882316
endef
$(eval $(call Download,data))

define Package/usb-modeswitch
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libubox +libblobmsg-json +libusb-1.0
  TITLE:=USB mode switching utility
endef

define Build/Prepare
	$(Build/Prepare/Default)
	tar xvfj $(DL_DIR)/$(PKG_DATA_FILENAME) -C $(PKG_BUILD_DIR)
	rm -f \
		$(PKG_BUILD_DIR)/$(PKG_DATA_PATH)/usb_modeswitch.d/05c6:1000:sVe=GT
	cp ./data/* $(PKG_BUILD_DIR)/$(PKG_DATA_PATH)/usb_modeswitch.d/
endef

define Package/usb-modeswitch/install
	$(INSTALL_DIR) $(1)/etc $(1)/sbin
	perl $(PKG_BUILD_DIR)/convert-modeswitch.pl \
		$(PKG_BUILD_DIR)/$(PKG_DATA_PATH)/usb_modeswitch.d/* \
		> $(1)/etc/usb-mode.json
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/usbmode $(1)/sbin/
endef

$(eval $(call BuildPackage,usb-modeswitch))
