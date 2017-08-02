# 
# Copyright (C) 2016 Opensource
#
# This is free software, See COPYING for more information.
# download from http://github.com/downloads/rvoicilas/inotify-tools/
#
include $(TOPDIR)/rules.mk

PKG_NAME:=inotify-tools
PKG_RELEASE:=1
PKG_VERSION:=3.14

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk

define Package/inotify-tools
	SECTION:=utils
	CATEGORY:=Base system
	TITLE:=fs inotify tool
endef

define Build/Configure
			$(call Build/Configure/Default, \
			--enable-shared \
			--enable-static \
			)
endef

define Package/inotify-tools/install
	install -d -m0755 $(1)/usr/sbin
	install -d -m0755 $(1)/usr/lib
	install -m0755 $(PKG_BUILD_DIR)/src/.libs/inotifywait $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/src/.libs/inotifywatch $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/libinotifytools/src/.libs/libinotifytools.so.0 $(1)/usr/lib/
endef

$(eval $(call BuildPackage,inotify-tools))
