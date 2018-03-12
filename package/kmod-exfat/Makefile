#
# Copyright (C) 2014 Bruno Randolf <br1@einfach.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=exfat-nofuse
PKG_RELEASE:=1
PKG_GIT_TREEISH=de4c760bc9a05ead83bc3ec6eec6cf1fb106f523

PKG_SOURCE:=$(PKG_NAME).tar.gz

NF=nf
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/exfat
	SUBMENU:=Other modules
	TITLE:=ExFAT Kernel driver
	FILES:=$(PKG_BUILD_DIR)/exfat.ko
	AUTOLOAD:=$(call AutoLoad,30,exfat,1)
	DEPENDS:=+kmod-nls-base @BUILD_PATENTED
endef

define KernelPackage/exfat/description
	Kernel module for ExFAT Filesytems
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	M="$(PKG_BUILD_DIR)"

define Package/kmod-exfat
#define Build/Prepare
#	test x$(GIT_HOME) != x
#	@echo "Plz sync with https://github.com/dorimanx/exfat-nofuse.git"
#	test -d $(GIT_HOME)/$(PKG_NAME).git
#	(cd $(GIT_HOME)/$(PKG_NAME).git; git-cat-file -e $(PKG_GIT_TREEISH))
#	(cd $(KERNEL_BUILD_DIR); git-archive --format=tar --prefix=$(PKG_NAME)/ --remote=$(GIT_HOME)/$(PKG_NAME).git $(PKG_GIT_TREEISH) | tar -xvf -)
#	$(call Build/Patch/Default)
	$(call Package/exfat-fuse/template)
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		CONFIG_EXFAT_FS=m \
		modules
endef

$(eval $(call KernelPackage,exfat))
