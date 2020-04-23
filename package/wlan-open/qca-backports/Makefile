include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-backports

PKG_BACKPORTS_VERSION:=9828be0
PKG_BACKPORTS_SOURCE_URL:=git://git.kernel.org/pub/scm/linux/kernel/git/backports/backports.git
PKG_BACKPORTS_BRANCH:=master

PKG_KERNEL_VERSION:=2dd6906
PKG_KERNEL_SOURCE_URL:=https://github.com/kvalo/ath.git
PKG_KERNEL_BRANCH:=ath-qca

PKG_VERSION:=20150515
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1

PKG_DRIVERS = ath9k-common ath9k ath9k-htc ath10k ath

PKG_CONFIG_DEPENDS:= \
	CONFIG_PACKAGE_kmod-qca-mac80211 \
	$(patsubst %,CONFIG_PACKAGE_kmod-qca-%,$(PKG_DRIVERS)) \
	CONFIG_PACKAGE_QCA_MAC80211_MESSAGE_TRACING \
	CONFIG_PACKAGE_QCA_MAC80211_DEBUGFS \
	CONFIG_PACKAGE_QCA_MAC80211_MESH \
	CONFIG_PACKAGE_QCA_ATH_DEBUG \
	CONFIG_QCA_ATH_USER_REGD

include $(INCLUDE_DIR)/package.mk

WMENU:=QCA mac80211 Wireless Drivers

define KernelPackage/qca-mac80211/Default
  SUBMENU:=$(WMENU)
  SECTION:=QCA
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  DEPENDS:=@(!(TARGET_avr32||TARGET_ps3||TARGET_pxcab)||BROKEN)
endef

define KernelPackage/qca-cfg80211
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=cfg80211 - wireless configuration API
  DEPENDS+= +iw-full +kmod-crypto-core +kmod-crypto-manager +kmod-crypto-aead2
  FILES:= \
	$(PKG_BUILD_DIR)/compat.ko \
	$(PKG_BUILD_DIR)/cfg80211.ko
  AUTOLOAD:=$(call AutoLoad,20,compat cfg80211)
endef

define KernelPackage/qca-cfg80211/description
cfg80211 is the Linux wireless LAN (802.11) configuration API.
endef

define KernelPackage/qca-mac80211
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=Linux 802.11 Wireless Networking Stack
  DEPENDS+= +kmod-crypto-arc4 +kmod-crypto-aes +kmod-qca-cfg80211 +hostapd-common \
	    +kmod-crypto-gcm
  KCONFIG:=\
	CONFIG_AVERAGE=y
  FILES:= $(PKG_BUILD_DIR)/net/mac80211/mac80211.ko
  AUTOLOAD:=$(call AutoLoad,21,mac80211)
  MENU:=1
endef

define KernelPackage/qca-mac80211/config
  if PACKAGE_kmod-qca-mac80211

	config PACKAGE_QCA_MAC80211_DEBUGFS
		bool "Export mac80211 internals in DebugFS"
		select KERNEL_DEBUG_FS
		select KERNEL_RELAY if PACKAGE_kmod-qca-ath9k-common
		default y
		help
		  Select this to see extensive information about
		  the internal state of mac80211 in debugfs.

	config PACKAGE_QCA_MAC80211_MESH
		bool "Enable 802.11s mesh support"
		default y

	config PACKAGE_QCA_MAC80211_MESSAGE_TRACING
		bool "Enable mac80211 message tracing"
		default n
		help
		  Select this option to enable tracing.

	config PACKAGE_QCA_MAC80211_DEBUG_MENU
		bool "Select mac80211 debugging features"
		default n
		help
		  This option collects various mac80211 debug settings.

	menu "mac80211 debugging features"
		depends on PACKAGE_QCA_MAC80211_DEBUG_MENU

	config PACKAGE_QCA_MAC80211_VERBOSE_DEBUG
		bool "Verbose debugging output"
		default n
		help
		  Selecting this option causes mac80211 to print out
		  many debugging messages.

	config PACKAGE_QCA_MAC80211_MLME_DEBUG
		bool "Verbose managed MLME output"
		default n
		help
		  Selecting this option causes mac80211 to print out
		  debugging messages for the managed-mode MLME.

	config PACKAGE_QCA_MAC80211_STA_DEBUG
		bool "Verbose station debugging"
		default n
		help
		  Selecting this option causes mac80211 to print out
		  debugging messages for station addition/removal.

	config PACKAGE_QCA_MAC80211_HT_DEBUG
		bool "Verbose HT debugging"
		default n
		help
		  This option enables 802.11n High Throughput features
		  debug tracing output.

	config PACKAGE_QCA_MAC80211_IBSS_DEBUG
		bool "Verbose IBSS debugging"
		default n
		help
		  Selecting this option causes mac80211 to print out
		  very verbose IBSS debugging messages.

	config PACKAGE_QCA_MAC80211_PS_DEBUG
		bool "Verbose powersave mode debugging"
		default n
		help
		  Selecting this option causes mac80211 to print out very
		  verbose power save mode debugging messages (when mac80211
		  is an AP and has power saving stations.)

	endmenu
  endif
endef

define KernelPackage/qca-mac80211/description
Generic IEEE 802.11 Networking Stack (mac80211)
endef

PKG_LINUX_FIRMWARE_NAME:=linux-firmware
PKG_LINUX_FIRMWARE_VERSION:=f404336
PKG_LINUX_FIRMWARE_BRANCH:=master
PKG_LINUX_FIRMWARE_SOURCE:=$(PKG_LINUX_FIRMWARE_NAME)-$(PKG_LINUX_FIRMWARE_VERSION).tar.bz2
PKG_LINUX_FIRMWARE_PROTO:=git
PKG_LINUX_FIRMWARE_SOURCE_URL:=git://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git
PKG_LINUX_FIRMWARE_SUBDIR:=$(PKG_LINUX_FIRMWARE_NAME)-$(PKG_LINUX_FIRMWARE_VERSION)
PKG_LINUX_FIRMWARE_MIRROR_MD5SUM:=837a1a9456c1ec8b428cc0b2b08a331b

define Download/qca-linux-firmware
  FILE:=$(PKG_LINUX_FIRMWARE_SOURCE)
  URL:=$(PKG_LINUX_FIRMWARE_SOURCE_URL)
  MD5SUM:=$(PKG_LINUX_FIRMWARE_MD5SUM)
  PROTO:=$(PKG_LINUX_FIRMWARE_PROTO)
  VERSION:=$(PKG_LINUX_FIRMWARE_VERSION)
  SUBDIR:=$(PKG_LINUX_FIRMWARE_SUBDIR)
  MIRROR_MD5SUM:=$(PKG_LINUX_FIRMWARE_MIRROR_MD5SUM)
endef
$(eval $(call Download,qca-linux-firmware))

ifneq ($(call qstrip,$(CONFIG_KERNEL_GIT_CLONE_URI)),)
# --reference option doesn't work on git trees synced with "repo", so instead
# we're manually using clone & fetch to speed up sync time
  define FastCloneKernel
	git clone $(CONFIG_KERNEL_GIT_CLONE_URI) $(2)
	(cd $(2); git remote add src $(1); git fetch src)
  endef
else
  define FastCloneKernel
	git clone $(1) $(2)
  endef
endif

define Download/backports
  FILE:=$(PKG_SOURCE)
  SUBDIR:=$(PKG_SUBDIR)
endef
$(eval $(call DownloadBackports,backports))

define KernelPackage/qca-ath/config
  if PACKAGE_kmod-qca-ath
	config QCA_ATH_USER_REGD
		bool "Force Atheros drivers to respect the user's regdomain settings"
		help
		  Atheros' idea of regulatory handling is that the EEPROM of the card defines
		  the regulatory limits and the user is only allowed to restrict the settings
		  even further, even if the country allows frequencies or power levels that
		  are forbidden by the EEPROM settings.

		  Select this option if you want the driver to respect the user's decision about
		  regulatory settings.

	config PACKAGE_QCA_ATH_DEBUG
		bool "Atheros wireless debugging"
		help
		  Say Y, if you want to debug atheros wireless drivers.
		  Right now only ath9k makes use of this.
  endif
endef

define KernelPackage/qca-ath
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA common driver part
  DEPENDS+= @PCI_SUPPORT||USB_SUPPORT||TARGET_ar71xx||TARGET_atheros +kmod-qca-mac80211
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath.ko
  AUTOLOAD:=$(call AutoLoad,26,ath)
  MENU:=1
endef

define KernelPackage/qca-ath/description
 This module contains some common parts needed by QCA Wireless drivers.
endef

define KernelPackage/qca-ath9k-common
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA 802.11n wireless devices (common code for ath9k and ath9k_htc)
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @PCI_SUPPORT||USB_SUPPORT||TARGET_ar71xx +kmod-qca-ath +@DRIVER_11N_SUPPORT
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_common.ko \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_hw.ko
  AUTOLOAD:=$(call AutoLoad,27,ath9k_hw ath9k_common)
endef

define KernelPackage/qca-ath9k
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA 802.11n PCI wireless cards support
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @PCI_SUPPORT||TARGET_ar71xx +kmod-qca-ath9k-common
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k.ko
  AUTOLOAD:=$(call AutoLoad,28,ath9k)
endef

define KernelPackage/qca-ath9k/description
This module adds support for wireless adapters based on
QCA IEEE 802.11n AR5008, AR9001,AR9002, AR9003, AR9004 family of chipsets.
endef

define KernelPackage/qca-ath9k-htc
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA 802.11n USB device support
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @USB_SUPPORT +kmod-qca-ath9k-common +kmod-usb-core
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_htc.ko
  AUTOLOAD:=$(call AutoLoad,55,ath9k_htc)
endef

define KernelPackage/qca-ath9k-htc/description
This module adds support for wireless adapters based on
QCA USB AR9271 and AR7010 family of chipsets.
endef

define KernelPackage/qca-ath10k
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA 802.11ac wireless cards support
  URL:=http://wireless.kernel.org/en/users/Drivers/ath10k
  DEPENDS+= @PCI_SUPPORT +kmod-qca-ath +@DRIVER_11N_SUPPORT
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath10k/ath10k_core.ko \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath10k/ath10k_pci.ko
  AUTOLOAD:=$(call AutoLoad,55,ath10k_core ath10k_pci)
endef

define KernelPackage/qca-ath10k/description
This module adds support for wireless adapters based on
QCA IEEE 802.11ac family of chipsets. For now only
PCI is supported.
endef

define KernelPackage/qca-wil6210
  $(call KernelPackage/qca-mac80211/Default)
  TITLE:=QCA 802.11ad wireless cards support
  DEPENDS+= @PCI_SUPPORT +kmod-qca-cfg80211 +TARGET_annapurnalabs:kmod-qca-nss-drv
  FILES:= \
	$(PKG_BUILD_DIR)/wil6210.ko
endef

define KernelPackage/qca-wil6210/description
This module adds support for wireless adapters based on
60GHz QCA Wilocity family of chipsets. These support 802.11d.
endef

config_package=$(if $(CONFIG_PACKAGE_kmod-$(1)),m)

config-y:= \
	WLAN \
	NL80211_TESTMODE \
	CFG80211_WEXT \
	CFG80211_INTERNAL_REGDB \
	CFG80211_CERTIFICATION_ONUS \
	MAC80211_RC_MINSTREL \
	MAC80211_RC_MINSTREL_HT \
	MAC80211_RC_DEFAULT_MINSTREL

config-$(call config_package,qca-cfg80211) += CFG80211

config-$(call config_package,qca-mac80211) += MAC80211
config-$(CONFIG_PACKAGE_QCA_MAC80211_MESH) += MAC80211_MESH
ifdef CONFIG_PACKAGE_QCA_MAC80211_DEBUGFS
  config-y += \
	CFG80211_DEBUGFS \
	MAC80211_DEBUGFS \
	ATH9K_DEBUGFS \
	ATH9K_HTC_DEBUGFS \
	ATH10K_DEBUGFS
endif

config-$(CONFIG_PACKAGE_QCA_MAC80211_MESSAGE_TRACING) += MAC80211_MESSAGE_TRACING ATH10K_TRACING
config-$(CONFIG_PACKAGE_QCA_MAC80211_DEBUG_MENU) += MAC80211_DEBUG_MENU
config-$(CONFIG_PACKAGE_QCA_MAC80211_VERBOSE_DEBUG) += MAC80211_VERBOSE_DEBUG
config-$(CONFIG_PACKAGE_QCA_MAC80211_MLME_DEBUG) += MAC80211_MLME_DEBUG
config-$(CONFIG_PACKAGE_QCA_MAC80211_STA_DEBUG) += MAC80211_STA_DEBUG
config-$(CONFIG_PACKAGE_QCA_MAC80211_HT_DEBUG) += MAC80211_HT_DEBUG
config-$(CONFIG_PACKAGE_QCA_MAC80211_IBSS_DEBUG) += MAC80211_IBSS_DEBUG
config-$(CONFIG_PACKAGE_QCA_MAC80211_PS_DEBUG) += MAC80211_PS_DEBUG

config-$(call config_package,qca-ath) += ATH_CARDS ATH_COMMON
config-$(CONFIG_PACKAGE_QCA_ATH_DEBUG) += ATH_DEBUG ATH10K_DEBUG

config-$(call config_package,qca-ath9k) += ATH9K
config-$(call config_package,qca-ath9k-common) += ATH9K_COMMON
config-$(CONFIG_TARGET_ar71xx) += ATH9K_AHB
config-$(CONFIG_PCI) += ATH9K_PCI
config-$(CONFIG_QCA_ATH_USER_REGD) += ATH_USER_REGD \
				      ATH_REG_DYNAMIC_USER_REG_HINTS \
				      ATH_REG_DYNAMIC_USER_CERT_TESTING

config-$(call config_package,qca-ath9k-htc) += ATH9K_HTC
config-$(call config_package,qca-ath10k) += ATH10K ATH10K_PCI ATH10K_DFS_CERTIFIED
config-$(call config_package,qca-wil6210) += ATH_CARDS WIL6210

config-$(CONFIG_LEDS_TRIGGERS) += MAC80211_LEDS

EXTRA_KCONFIG:= CONFIG_COMMON_CLK=y

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

MAKE_OPTS:= -C "$(PKG_BUILD_DIR)" \
	CROSS_COMPILE="$(KERNEL_CROSS)" \
	ARCH="$(LINUX_KARCH)" \
	EXTRA_CFLAGS="-I$(PKG_BUILD_DIR)/include $(EXTRA_CFLAGS)"  \
	KLIB_BUILD="$(LINUX_DIR)" \
	MODPROBE=true \
	KLIB=$(TARGET_MODULES_DIR) \
	KERNEL_SUBLEVEL=$(lastword $(subst ., ,$(KERNEL_PATCHVER))) \
	KBUILD_LDFLAGS_MODULE_PREREQ=

ifneq ($(findstring c,$(OPENWRT_VERBOSE)),)
  MAKE_OPTS += V=1
endif

define ConfigVars
$(subst $(space),,$(foreach opt,$(config-$(1)),CPTCFG_$(opt)=$(1)
))
endef

define mac80211_config
$(call ConfigVars,m)$(call ConfigVars,y)
endef
$(eval $(call shexport,mac80211_config))

define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
	rm -rf \
		$(PKG_BUILD_DIR)/include/linux/ssb \
		$(PKG_BUILD_DIR)/include/linux/bcma \
		$(PKG_BUILD_DIR)/include/net/bluetooth

	rm -f \
		$(PKG_BUILD_DIR)/include/linux/cordic.h \
		$(PKG_BUILD_DIR)/include/linux/crc8.h \
		$(PKG_BUILD_DIR)/include/linux/eeprom_93cx6.h \
		$(PKG_BUILD_DIR)/include/linux/wl12xx.h \
		$(PKG_BUILD_DIR)/include/linux/spi/libertas_spi.h \
		$(PKG_BUILD_DIR)/include/net/ieee80211.h

	echo 'compat-wireless-$(PKG_VERSION)-$(PKG_RELEASE)-$(REVISION)' > $(PKG_BUILD_DIR)/compat_version
	$(CP) ./files/regdb.txt $(PKG_BUILD_DIR)/db.txt
endef

ifneq ($(CONFIG_PACKAGE_kmod-qca-cfg80211)$(CONFIG_PACKAGE_kmod-lib80211),)
 define Build/Compile/kmod
#	rm -rf $(PKG_BUILD_DIR)/modules
#	+$(MAKE) $(PKG_JOBS) $(MAKE_OPTS) modules
 endef
endif

define Build/Compile
#	$(SH_FUNC) var2file "$(call shvar,mac80211_config)" $(PKG_BUILD_DIR)/.config
#	$(MAKE) $(MAKE_OPTS) olddefconfig
#	$(call Build/Compile/kmod)
endef

define Build/InstallDev
#	mkdir -p \
#		$(1)/usr/include/mac80211 \
#		$(1)/usr/include/mac80211-backport \
#		$(1)/usr/include/mac80211/ath \
#		$(1)/usr/include/net/mac80211
#	$(CP) $(PKG_BUILD_DIR)/net/mac80211/*.h $(PKG_BUILD_DIR)/include/* $(1)/usr/include/mac80211/
#	$(CP) $(PKG_BUILD_DIR)/backport-include/* $(1)/usr/include/mac80211-backport/
#	$(CP) $(PKG_BUILD_DIR)/net/mac80211/rate.h $(1)/usr/include/net/mac80211/
#	$(CP) $(PKG_BUILD_DIR)/drivers/net/wireless/ath/*.h $(1)/usr/include/mac80211/ath/
endef

define KernelPackage/qca-wil6210/install
	$(INSTALL_DIR) $(1)/etc/modules.d
	$(INSTALL_DATA) ./files/33-qca-wil6210 $(1)/etc/modules.d

	# Install 11ad linkloss watchdog script
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) ./files/11ad_linkloss_wd.sh $(1)/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/wigig_linkloss_wd.init $(1)/etc/init.d/wigig_linkloss_wd
endef

ifndef CONFIG_PACKAGE_qsdk-wifi-script
define KernelPackage/qca-cfg80211/install
	$(INSTALL_DIR) $(1)/lib/wifi
	$(INSTALL_DATA) ./files/lib/wifi/mac80211.sh $(1)/lib/wifi
endef
endif

define KernelPackage/qca-ath9k-htc/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/htc_9271.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/htc_7010.fw \
		$(1)/lib/firmware/
endef

define KernelPackage/qca-ath10k/install
	$(INSTALL_DIR) $(1)/lib/firmware/ath10k/QCA988X/hw2.0
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/ath10k/QCA988X/hw2.0/firmware-4.bin \
		$(1)/lib/firmware/ath10k/QCA988X/hw2.0/firmware-4.bin
endef

$(eval $(call KernelPackage,qca-cfg80211))
$(eval $(call KernelPackage,qca-mac80211))
$(eval $(call KernelPackage,qca-ath9k-common))
$(eval $(call KernelPackage,qca-ath9k))
$(eval $(call KernelPackage,qca-ath9k-htc))
$(eval $(call KernelPackage,qca-ath10k))
$(eval $(call KernelPackage,qca-ath))
$(eval $(call KernelPackage,qca-wil6210))
