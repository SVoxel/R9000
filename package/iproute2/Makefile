#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iproute2
PKG_VERSION:=4.4.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@KERNEL/linux/utils/net/iproute2
PKG_MD5SUM:=d762653ec3e1ab0d4a9689e169ca184f
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/iproute2/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=http://www.linuxfoundation.org/collaborate/workgroups/networking/iproute2
  SUBMENU:=Routing and Redirection
  MAINTAINER:=Russell Senior <russell@personaltelco.net>
  DEPENDS:= +libnl-tiny
endef

define Package/ip-tiny
$(call Package/iproute2/Default)
 TITLE:=Routing control utility (Minimal)
 VARIANT:=tiny
 PROVIDES:=ip
 ALTERNATIVES:=200:/usr/sbin/ip:/usr/sbin/ip-tiny
endef

define Package/ip-full
$(call Package/iproute2/Default)
 TITLE:=Routing control utility (Full)
 VARIANT:=full
 PROVIDES:=ip
 ALTERNATIVES:=300:/usr/sbin/ip:/usr/sbin/ip-full
endef

define Package/tc
$(call Package/iproute2/Default)
  TITLE:=Traffic control utility
  DEPENDS:=+kmod-sched-core
endef

define Package/genl
$(call Package/iproute2/Default)
  TITLE:=General netlink utility frontend
endef

define Package/ip-bridge
$(call Package/iproute2/Default)
  TITLE:=Bridge configuration utility from iproute2
endef

define Package/ss
$(call Package/iproute2/Default)
  TITLE:=Socket statistics utility
endef

define Package/nstat
$(call Package/iproute2/Default)
  TITLE:=Network statistics utility
endef

ifeq ($(BUILD_VARIANT),tiny)
  IP_CONFIG_TINY:=y
endif

define Build/Configure
	$(SED) "s,-I/usr/include/db3,," $(PKG_BUILD_DIR)/Makefile
	$(SED) "s,^KERNEL_INCLUDE.*,KERNEL_INCLUDE=$(LINUX_DIR)/include," \
		$(PKG_BUILD_DIR)/Makefile
	$(SED) "s,^LIBC_INCLUDE.*,LIBC_INCLUDE=$(STAGING_DIR)/include," \
		$(PKG_BUILD_DIR)/Makefile
	echo "static const char SNAPSHOT[] = \"$(PKG_VERSION)-$(PKG_RELEASE)-lede\";" \
		> $(PKG_BUILD_DIR)/include/SNAPSHOT.h
endef

ifdef CONFIG_USE_GLIBC
  TARGET_CFLAGS += -DHAVE_SETNS
endif
ifdef CONFIG_USE_MUSL
  TARGET_CFLAGS += -DHAVE_SETNS
endif

TARGET_CFLAGS += -ffunction-sections -fdata-sections -Wno-implicit-function-declaration
TARGET_CFLAGS := $(filter-out -O%,$(TARGET_CFLAGS)) -O3

MAKE_FLAGS += \
	EXTRA_CCOPTS="$(TARGET_CFLAGS) -I../include -I$(STAGING_DIR)/usr/include/libnl-tiny" \
	KERNEL_INCLUDE="$(LINUX_DIR)/include" \
	SHARED_LIBS="" \
	LDFLAGS="$(TARGET_LDFLAGS) -Wl,--gc-sections" \
	IP_CONFIG_TINY=$(IP_CONFIG_TINY) \
	FPIC="$(FPIC)"

define Build/Compile
	+$(MAKE_VARS) $(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) $(MAKE_FLAGS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/include/{libgenl,libnetlink,ll_map}.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/lib/libnetlink.a $(1)/usr/lib/
endef

define Package/ip-tiny/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/iproute2
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/etc/iproute2/rt_tables $(1)/etc/iproute2
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ip/ip $(1)/usr/sbin/
endef

define Package/ip-full/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/iproute2
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/etc/iproute2/rt_tables $(1)/etc/iproute2
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ip/ip $(1)/usr/sbin/ip
endef

define Package/tc/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tc/tc $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) ./files/15-teql $(1)/etc/hotplug.d/iface/
	$(INSTALL_DIR) $(1)/lib/debug
	$(INSTALL_BIN) ./files/tc.debug $(1)/lib/debug/tc
endef

define Package/genl/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/genl/genl $(1)/usr/sbin/
endef

define Package/ip-bridge/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bridge/bridge $(1)/usr/sbin/
endef

define Package/ss/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/misc/ss $(1)/usr/sbin/
endef

define Package/nstat/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/misc/nstat $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ip-tiny))
$(eval $(call BuildPackage,ip-full))
$(eval $(call BuildPackage,tc))
$(eval $(call BuildPackage,genl))
$(eval $(call BuildPackage,ip-bridge))
$(eval $(call BuildPackage,ss))
$(eval $(call BuildPackage,nstat))
