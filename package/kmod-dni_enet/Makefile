include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-dni_enet
#PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=ssh://qca-git01.qualcomm.com:29418/oss/lklm/nss-gmac
#PKG_BRANCH:=master
#PKG_RELEASE:=1

ifeq ($(DUMP)$(PKG_VERSION),)
#  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE_VERSION:=$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

ENET_AUTOLOAD:= dni_enet 

define KernelPackage/dni_enet
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  DEPENDS:=@TARGET_r9000
  TITLE:=Kernel driver for NSS gmac
  FILES:=$(PKG_BUILD_DIR)/dni_enet.ko
  AUTOLOAD:=$(call AutoLoad,32,$(ENET_AUTOLOAD))
endef

define KernelPackage/kmod-dni_enet/Description
This package contains a NSS driver for QCA chipset
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/	
endef

define Build/Compile
endef

$(eval $(call KernelPackage,dni_enet))
