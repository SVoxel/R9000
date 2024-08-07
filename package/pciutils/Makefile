#
# Copyright (C) 2007-2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=pciutils
PKG_VERSION:=3.13.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@KERNEL/software/utils/pciutils
PKG_MD5SUM:=b6596d1926ead879b4af00f7665c0f8c

PKG_MAINTAINER:=Lucian Cristian <lucian.cristian@gmail.com>
PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/pciutils
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Linux PCI Utilities
  URL:=http://mj.ucw.cz/pciutils.shtml
  DEPENDS:=+libpci
endef

define Package/pciutils/description
 contains collection of programs for inspecting and manipulating configuration
 of PCI devices
endef

define Package/libpci
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Linux PCI Libraries
  URL:=http://mj.ucw.cz/pciutils.shtml
endef

TARGET_CFLAGS += $(FPIC)

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
	PREFIX="/usr" \
	HOST="Linux" \
	HWDB="no" \
	ZLIB="no" \
	SHARED="yes"

ifneq ($(CONFIG_USE_GLIBC),)
TARGET_LDFLAGS += -lresolv
endif

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpci.so.3 \
		$(PKG_INSTALL_DIR)/usr/lib/libpci.so
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_BUILD_DIR)/lib/libpci.pc $(1)/usr/lib/pkgconfig
	$(SED) 's,/usr/include,$$$${prefix}/include,g' $(1)/usr/lib/pkgconfig/libpci.pc
	$(SED) 's,/usr/lib,$$$${prefix}/lib,g' $(1)/usr/lib/pkgconfig/libpci.pc
	$(INSTALL_DIR) $(1)/usr/include/pci
	$(CP) $(foreach i,pci.h config.h header.h types.h, \
		$(PKG_BUILD_DIR)/lib/$(i)) $(1)/usr/include/pci
endef

define Package/pciutils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/{lspci,setpci,update-pciids} $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/share/hwdata
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/hwdata/pci.ids $(1)/usr/share/hwdata/
endef

define Package/libpci/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef


$(eval $(call BuildPackage,libpci))
$(eval $(call BuildPackage,pciutils))
