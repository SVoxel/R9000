#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=radvd
PKG_VERSION:=1.5
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=$(DNI_DL_WEB) \
#		http://v6web.litech.org/radvd/dist \
#		http://download.sourcemage.org/mirror
#PKG_MD5SUM:=78598b60ed9d64920739189c3dc0934d

PKG_GIT_TREEISH=radvd-1.5-dni15

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -Wno-implicit-function-declaration

define Package/radvd
  SECTION:=ipv6
  CATEGORY:=IPv6
  TITLE:=IPv6 Routing Advertisement Daemon
  URL:=http://v6web.litech.org/radvd/
  DEPENDS:=+kmod-ipv6
endef

define Package/radvd/description
IPv6 Routing Advertisement Daemon.
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/radvd.git/* $(PKG_BUILD_DIR)/
	touch -t 201705170001.01 $(PKG_BUILD_DIR)/*
endef

define Build/Configure
	$(call Build/Configure/Default,\
		--with-configfile=/etc/radvd.conf \
		--with-logfile=/var/log/radvd.log \
		--with-pidfile=/var/run/radvd.pid \
	)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		radvd_LDADD=""
	$(MAKE) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		install
endef

define Package/radvd/conffiles
/etc/config/radvd
endef

define Package/radvd/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/radvd.config $(1)/etc/config/radvd
	$(INSTALL_DATA) ./files/radvd.conf $(1)/etc/
	#$(INSTALL_DIR) $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/radvd.init $(1)/etc/init.d/radvd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/radvd $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/radvdump $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,radvd))
