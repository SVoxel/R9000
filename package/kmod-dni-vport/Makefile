include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-dni-vport
PKG_VERSION:=
PKG_RELEASE:=

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_GIT_TREEISH=HEAD
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/kmod-dni-vport

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dni-vport
  SUBMENU:=Other modules
  TITLE:=Module to support virtual ports for L2 managment protocol.
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/dni-vport.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,39,dni-vport)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
endef

define KernelPackage/dni-vport/install
	#mkdir -p $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/dni-vport.init $(1)/etc/init.d/dni-vport
endef

$(eval $(call KernelPackage,dni-vport))
