#
# Copyright (C) 2008-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=check
PKG_VERSION:=0.15.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/libcheck/check/releases/download/$(PKG_VERSION)
PKG_MD5SUM:=50fcafcecde5a380415b12e9c574e0b2

PKG_MAINTAINER:=Eduardo Abinader <eduardoabinader@gmail.com>
PKG_LICENSE:=LGPL-2.1-or-later
PKG_LICENSE_FILES:=COPYING.LESSER

CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

TARGET_CFLAGS += -Wno-implicit-function-declaration -Wno-int-conversion

define Package/check
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Unit testing framework for C
  URL:=https://libcheck.github.io/check/
  DEPENDS:= +libpthread +librt
endef

define Package/check/description
  Check features a simple interface for defining unit tests, putting little in
  the way of the developer. Tests are run in a separate address space, so Check
  can catch both assertion failures and code errors that cause segmentation
  faults or other signals. The output from unit tests can be used within source
  code editors and IDEs.
endef

CMAKE_OPTIONS += \
	-DBUILD_TESTING=OFF \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON

define Package/check/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcheck.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,check))
