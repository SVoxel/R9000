#
# Copyright (C) 2007-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iperf
PKG_VERSION:=3.19
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://downloads.es.net/pub/iperf
PKG_MD5SUM:=4704c31b68668f74d75a6d192599f53c

PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>
PKG_LICENSE:=BSD-3-Clause

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS := $(filter-out -O%,$(TARGET_CFLAGS)) -O0

DISABLE_NLS:=

define Package/iperf3/default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Internet Protocol bandwidth measuring tool
  URL:=https://github.com/esnet/iperf
  DEPENDS:=+libpthread
endef

define Package/iperf3
$(call Package/iperf3/default)
  VARIANT:=nossl
endef

define Package/iperf3-ssl
$(call Package/iperf3/default)
  TITLE+= with iperf_auth support
  VARIANT:=ssl
  DEPENDS:= +libopenssl
endef

TARGET_CFLAGS += -std=gnu17 -D_GNU_SOURCE
CONFIGURE_ARGS += --disable-shared

ifeq ($(BUILD_VARIANT),ssl)
	CONFIGURE_ARGS += --with-openssl="$(STAGING_DIR)/usr"
else
	CONFIGURE_ARGS += --without-openssl
endif

MAKE_FLAGS += noinst_PROGRAMS=

define Package/iperf3/description
 Iperf is a modern alternative for measuring TCP and UDP bandwidth
 performance, allowing the tuning of various parameters and
 characteristics.
endef

# autoreconf fails if the README file isn't present
define Build/Prepare
	$(call Build/Prepare/Default)
	touch $(PKG_BUILD_DIR)/README
endef

define Package/iperf3/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/iperf3 $(1)/usr/bin/
endef

define Package/iperf3-ssl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/iperf3 $(1)/usr/bin/
endef

$(eval $(call BuildPackage,iperf3))
$(eval $(call BuildPackage,iperf3-ssl))
