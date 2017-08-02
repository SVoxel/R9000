#
# Copyright (C) 2010-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=r9000
BOARDNAME:=R9000
FEATURES:=ubifs #fpu ramdisk
CPU_TYPE:=cortex-a15
CPU_SUBTYPE:=neon
#MAINTAINER:=Florian Fainelli <florian@openwrt.org>

CFLAGS:=-O2 -pipe -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mtune=cortex-a15

CUSTOMER:=dni
LINUX_VERSION:=3.10.20
LINUX_TREEISH:=linux-v3.10.20-dni1
NETFILTER_TREEISH:=dni-netfilter-v3.10.20-dni1

DEVICE_TYPE:=developerboard

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build images for ARM Ltd. Realview boards to be run with qemu
endef

KERNELNAME:="uImage"

$(eval $(call BuildTarget))
