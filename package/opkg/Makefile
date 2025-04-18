#
# Copyright (c) 2013 The Linux Foundation. All rights reserved.
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk

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
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1
HOST_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/opkg/Default
  SECTION:=base
  CATEGORY:=Base system
  TITLE:=opkg package management system
  MAINTAINER:=Jo-Philipp Wich <xm@subsignal.org>
  URL:=http://wiki.openmoko.org/wiki/Opkg
endef

define Package/opkg
$(call Package/opkg/Default)
  VARIANT:=nocurl
  DEPENDS:=+zlib
endef

define Package/opkg-curl
$(call Package/opkg/Default)
  TITLE += (with curl support)
  DEPENDS:=+libopenssl-1.0.2 +libcurl +zlib
  VARIANT:=curl
endef

define Package/opkg/description
  Lightweight package management system
  opkg is the opkg Package Management System, for handling
  installation and removal of packages on a system. It can
  recursively follow dependencies and download all packages
  necessary to install a particular package.

  opkg knows how to install both .ipk and .deb packages.
endef

define Package/opkg-curl/description
$(call Package/opkg/description)

This is a variant with curl support
endef

define Package/opkg/conffiles
/etc/opkg.conf
endef

TARGET_CPPFLAGS:=-I$(STAGING_DIR)/usr/local/include $(TARGET_CPPFLAGS)
TARGET_LDFLAGS:=-L$(STAGING_DIR)/usr/local/lib $(TARGET_LDFLAGS)
TARGET_CFLAGS:=-L$(STAGING_DIR)/usr/local/lib $(TARGET_CFLAGS)
TARGET_CFLAGS += $(if $(CONFIG_GCC_VERSION_4_3)$(CONFIG_GCC_VERSION_4_4),-Wno-array-bounds)
TARGET_CFLAGS += -ffunction-sections -fdata-sections
EXTRA_CFLAGS += $(TARGET_CPPFLAGS) -L$(STAGING_DIR)/usr/lib -lz

CONFIGURE_ARGS += \
	--disable-gpg \
	--with-opkgetcdir=/etc \
	--with-opkglockfile=/var/lock/opkg.lock

ifeq ($(BUILD_VARIANT),nocurl)
	CONFIGURE_ARGS += --disable-curl
endif

MAKE_FLAGS = \
		CC="$(TARGET_CC)" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		HOST_CPU="$(PKGARCH)" \
		LDFLAGS="-Wl,--gc-sections" \

define Package/opkg/install
	$(INSTALL_DIR) $(1)/usr/lib/opkg
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) ./files/opkg.conf $(1)/etc/
	$(VERSION_SED) $(1)/etc/opkg.conf
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/opkg-cl $(1)/bin/opkg
endef

Package/opkg-curl/install = $(Package/opkg/install)

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

$(eval $(call BuildPackage,opkg))
$(eval $(call BuildPackage,opkg-curl))
$(eval $(call HostBuild))
