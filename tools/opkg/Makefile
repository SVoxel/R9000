#
# Copyright (c) 2013 The Linux Foundation. All rights reserved.
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=opkg
PKG_REV:=618
PKG_VERSION:=$(PKG_REV)
PKG_RELEASE:=5

PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=opkg-$(PKG_VERSION)
PKG_SOURCE_URL:=http://opkg.googlecode.com/svn/trunk/
PKG_SOURCE:=$(PKG_SOURCE_SUBDIR).tar.gz
PKG_FIXUP:=autoreconf libtool
PKG_REMOVE_FILES = autogen.sh aclocal.m4

PKG_MIRROR_MD5SUM:=e15f0cdbd93dff4e30a417f90acbdbcf
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1
HOST_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/host-build.mk


define Build/InstallDev
	mkdir -p $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libopkg $(1)/usr/include/
endef


HOST_CONFIGURE_ARGS+= \
	--disable-curl \
	--disable-gpg \
	--with-opkgetcdir=/etc \
	--with-opkglockfile=/tmp/opkg.lock

define Host/Compile
	+$(MAKE) $(HOST_JOBS) -C $(HOST_BUILD_DIR) CC="$(HOSTCC)" all
endef

define Host/Install
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/src/opkg-cl $(STAGING_DIR_HOST)/bin/opkg
endef

$(eval $(call HostBuild))
