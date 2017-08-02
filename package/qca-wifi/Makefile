
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG:=qca-wifi
PKG_NAME:=$(PKG)-10.4
# ifneq ($(BUILD_VARIANT),annapurnalabs)
# PKG_SOURCE_PROTO:=git
# PKG_SOURCE_URL:=ssh://qca-git01.qualcomm.com:29418/wifi/$(PKG_NAME).git
# PKG_BRANCH:=master
# endif
PKG_RELEASE:=1
PKG_VERSION=4.0.1756.225

LOCAL_SRC:=$(TOPDIR)/qca/src/qca-wifi-10.4

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
  PKG_VERSION:=g$(PKG_REV)
endif

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

FW_hw2_asic:=PACKAGE_qca-wifi-fw-hw2-10.4-asic
FW_hw3_asic:=PACKAGE_qca-wifi-fw-hw3-10.4-asic
FW_hw4_asic:=PACKAGE_qca-wifi-fw-hw4-10.4-asic
FW_hw4_emu_bb:=PACKAGE_qca-wifi-fw-hw4-10.4-emu_bb
FW_hw4_emu_m2m:=PACKAGE_qca-wifi-fw-hw4-10.4-emu_m2m
FW_hw5_asic:=PACKAGE_qca-wifi-fw-hw5-10.4-asic
FW_hw6_asic:=PACKAGE_qca-wifi-fw-hw6-10.4-asic
FW_hw5_emu_bb:=PACKAGE_qca-wifi-fw-hw5-10.4-emu_bb
FW_hw5_emu_m2m:=PACKAGE_qca-wifi-fw-hw5-10.4-emu_m2m
FW_hw7_asic:=PACKAGE_qca-wifi-fw-hw7-10.4-asic

FW_10_4:=$(FW_hw2_asic)||$(FW_hw3_asic)||$(FW_hw4_emu_bb)||$(FW_hw4_emu_m2m)||$(FW_hw4_asic)||$(FW_hw5_asic)||$(FW_hw6_asic)||$(FW_hw5_emu_bb)||$(FW_hw5_emu_m2m)||$(FW_hw7_asic)

# These two functions are used to define options based on WLAN config
if_opt_set=       $(if $(filter $(1)=1,$(QCAWLAN_MAKEOPTS)),$(2),)
if_opt_clear=     $(if $(filter $(1)=0,$(QCAWLAN_MAKEOPTS)),$(2),)

# Use the function below to add driver opts depending on menuconfig values
append_if_notnull=QCAWLAN_MAKEOPTS+=$(if $(call qstrip,$(1)),$(2),$(3))

#########################################################
############ WLAN DRIVER BUILD CONFIGURATION ############
#########################################################

QCAWLAN_MODULE_LIST:=

ifneq ($(BUILD_VARIANT),)
ifneq ($(BUILD_VARIANT),annapurnalabs)
QCAWLAN_MAKEOPTS:=$(shell cat $(LOCAL_SRC)/os/linux/configs/config.wlan.$(subst -,.,$(BUILD_VARIANT)))
endif
endif

ifeq ($(ARCH),mips)
HAL_TARGET:=mipsisa32-be-elf
QCAWLAN_MAKEOPTS+=BIG_ENDIAN_HOST=1
endif
ifeq ($(ARCH),arm)
HAL_TARGET:=armv4-le-elf
QCAWLAN_MAKEOPTS+=BUILD_ARM=y
QCAWLAN_MAKEOPTS+=BIG_ENDIAN_HOST=0
endif
ifeq ($(BUILD_VARIANT),annapurnalabs)
QCAWLAN_MAKEOPTS:=$(shell cat ./profiles/config.wlan.annapurnalabs)
HAL_TARGET:=armv7-le-elf
LIB_BSD:=-lbsd
QCAWLAN_MAKEOPTS+=BUILD_ARM=y
QCAWLAN_MAKEOPTS+=BIG_ENDIAN_HOST=0
QCAWLAN_MAKEOPTS+=ATH_DEFINE_ENDIAN=0
QCAWLAN_MAKEOPTS+=ARM_VERSION=7
QCAWLAN_MAKEOPTS+=PERE_IP_HDR_ALIGNMENT_WAR=1
QCAWLAN_MAKEOPTS+=BUS=PCI
endif
ifeq ($(ARCH_PACKAGES),ipq806x)
QCAWLAN_MAKEOPTS+=BUILD_ARM_MSM=y
endif
ifeq ($(ARCH_PACKAGES),ar71xx)
QCAWLAN_MAKEOPTS+= \
	BUS=dual \
	AH_CAL_IN_DRAM_FROM_HOST=1 \
	AH_CAL_RADIOS_AHB=1 \
	AH_CAL_RADIOS_PCI=1 \
	AH_SUPPORT_SCORPION=1
endif

QCAWLAN_MAKEOPTS+= \
	TOOLPATH= \
	TOOLPREFIX=$(TARGET_CROSS) \
	NO_SIMPLE_CONFIG=1 \
	USE_PLATFORM_FRAMEWORK=1 \
	TARGET=$(HAL_TARGET) \
	ARCH=$(LINUX_KARCH) \
	KERNELARCH=$(LINUX_KARCH) \
	KERNELVER=$(LINUX_VERSION) \
	FIRMWARE_REL_PATH=offload \
	PERF_PWR_OFFLOAD_DIR_PATH=$(PKG_BUILD_DIR)/offload \
	QCA_NSS_INCLUDE=$(STAGING_DIR)/usr/include/qca-nss-drv \
	KBUILDPATH=$(LINUX_SRC_DIR) \
	KERNELPATH=$(LINUX_DIR)


MAKEARCH:=$(MAKE) ARCH=$(LINUX_KARCH) CROSS_COMPILE=$(TARGET_CROSS)

ifeq ($(BUILD_VARIANT),annapurnalabs)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ext/wlan_ext.$(LINUX_KMOD_SUFFIX)
endif

# Module list
# This list is filled dynamically based on the WLAN configuration
# It depends on the content of the wlan config file (.profile)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/asf/asf.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/adf/adf.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/os/linux/ath_hal/ath_hal.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ratectrl/ath_rate_atheros.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, ATH_SUPPORT_DFS, \
	$(PKG_BUILD_DIR)/lmac/dfs/ath_dfs.$(LINUX_KMOD_SUFFIX)))
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, ATH_SUPPORT_SPECTRAL, \
	$(PKG_BUILD_DIR)/lmac/spectral/ath_spectral.$(LINUX_KMOD_SUFFIX)))
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, ATH_SUPPORT_TX99, \
	$(PKG_BUILD_DIR)/lmac/tx99/hst_tx99.$(LINUX_KMOD_SUFFIX)))
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ath_dev/ath_dev.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/umac/umac.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ath_pktlog/ath_pktlog.$(LINUX_KMOD_SUFFIX)

QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, UNIFIED_SMARTANTENNA, \
	$(PKG_BUILD_DIR)/smartantenna/smart_antenna.$(LINUX_KMOD_SUFFIX)))
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, ATH_SW_WOW_SUPPORT, \
	$(PKG_BUILD_DIR)/wow/sw_wow.$(LINUX_KMOD_SUFFIX)))


ifeq ($(BUILD_VARIANT),annapurnalabs)
PLATFORM_PATCH_DIR=patches-$(BUILD_VARIANT)

PKG_GIT_DIR=madwifi-11n.git
QCA_WIRELESS_GIT_TREEISH=3c1827c69ae031f8ab8f62221236ea961a2f562f

DNI_EXTRA_FLAGS+=-DENABLE_CLIENT_BRIDGE
DNI_EXTRA_FLAGS+=-DDNI_EXTEND_FUNCS
# DNI_EXTRA_FLAGS+=-DDISPLAY_SCAN_RESULT_IN_PROC
# DNI_EXTRA_FLAGS+=-DENABLE_DNI_SYSCTL

DNI_COMMON_CFLAGS= COMMON_CFLAGS="-DNETGEAR_ENHANCED_ACS -DDNI_NEW_STA_CHECK -DDNI_TPSCALE -DMORE_TPSCALE -DNETGEAR_ACL -DDNI_LED_CONTROL -DNETGEAR_CUSTOMISED_LED -DNETGEAR_WLAN_LOG -DENABLE_DNI_SYSCTL -DWPS_NOTIFICATION -DDNI_HOSTAP_JUDGE_PIN_LENGTH -DNETGEAR_DISABLE_WPS_IE -DDNI_SHOW_VAP_STATUS -DDNI_DFS_FUNC $(DNI_EXTRA_FLAGS)"

define Build/Prepare
	$(CP) $(GIT_HOME)/madwifi-11n.git/* $(PKG_BUILD_DIR)/
endef
endif

#########################################################
##################### CONFIG RULES ######################
#########################################################

define KernelPackage/$(PKG_NAME)/Default
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  TITLE:=QCA Wireless LAN Driver
  DEPENDS:=+libgcc +libc +wireless-tools +librt +iproute2 \
	  @$(FW_10_4) @(!(PACKAGE_kmod-ath&&PACKAGE_kmod-mac80211))
  FILES:= $(strip $(QCAWLAN_MODULE_LIST))
  AUTOLOAD:=$(call AutoLoad,33,$(notdir $(basename $(strip $(QCAWLAN_MODULE_LIST)))))
  MENU:=1
  PROVIDES:=kmod-qca-wifi-10.4
endef

$(eval $(call RequireCommand,uuencode, \
	Please install sharutils which provides uuencode required for build \
))

define KernelPackage/$(PKG_NAME)/Description/Default
  This package contains a driver for QCA WLAN chipsets
  It also installs the QCA WLAN tools, such as:
  - wlanconfig
  - athstats
  - pktlogconf
  - ...
endef

define KernelPackage/$(PKG_NAME)/config/Default
$(call Package/$(PKG_NAME)/override_source_path,kmod-$(PKG_NAME)$(1))
$(call Package/$(PKG_NAME)/override_version,kmod-$(PKG_NAME)$(1),$(PKG_SUPPORTED_VERSION))
endef

#########################################################
################### INSTALLDEV RULES ####################
#########################################################

################# BUILD/INSTALL RULES ###################
#########################################################

QCAWLAN_TOOL_LIST:= 80211stats athstats athstatsclr apstats pktlogconf pktlogdump wifitool wlanconfig thermaltool wps_enhc
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_DFS, radartool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_SPECTRAL, spectraltool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ASSOC_DENIAL_NOTIFY, assocdenialnotify)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_IBSS_PRIVATE_SECURITY, athadhoc)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SSID_STEERING, ssidsteering)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_AOW, mplay aowstatlog)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_TX99, tx99tool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, DEBUG_TOOLS, dumpregs reg)

Hooks/Prepare/Pre += qca-wifi_update_patches_version
define qca-wifi_update_patches_version

	[ ! -h patches ] || rm patches
	[ ! -d patches-$(PKG_BRANCH) ] || \
		ln -s patches-$(PKG_BRANCH) patches
endef

FWINCLUDES:= $(PKG_BUILD_DIR)/fwcommon
FWPROTOCOL_SUPPORT:=$(PKG_BUILD_DIR)/fwprotocol/support
FWPROTOCOL_TOOLS:=$(PKG_BUILD_DIR)/fwprotocol/tools
define Build/Compile
endef

define KernelPackage/$(PKG_NAME)/install/Default
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/usr/lib
	$(INSTALL_DIR) $(1)/lib/wifi
	$(INSTALL_DIR) $(1)/lib/sbin
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d

ifndef CONFIG_PACKAGE_qsdk-wifi-script
	$(INSTALL_DATA) ./files/qcawifi.sh $(1)/lib/wifi
else
	# qsdk-wifi-script expect the module file in /lib/wifi instead of /etc/modules.d
	mv $(1)/etc/modules.d/33-qca-wifi* $(1)/lib/wifi/
endif
	$(INSTALL_DATA) ./files/qcawifi_countrycode.txt $(1)/lib/wifi

	$(INSTALL_BIN) ./files/wifi_try $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/wifi_try $(1)/lib/sbin/

	$(INSTALL_CONF) ./files/ssid-steering.config $(1)/etc/config/ssid-steering
	$(INSTALL_BIN) ./files/ssid_steering.init $(1)/etc/init.d/ssid_steering

	[ ! -d files-$(PKG_BRANCH) ] || \
		$(CP) ./files-$(PKG_BRANCH)/* $(1)/
	$(INSTALL_BIN) $(foreach tool,$(QCAWLAN_TOOL_LIST),$(PKG_BUILD_DIR)/os/linux/tools/$(tool)) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/halphy_tools/host/testcmd_tlv/Linux/athtestcmd $(1)/usr/sbin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/halphy_tools/host/testcmd_tlv/Linux/*.so $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/offload/os/linux/tools/athdiag/athdiag $(1)/usr/sbin
endef

#########################################################
################# VARIANTS DEFINITION ###################
#########################################################

# Default variant (Akronite only)
define KernelPackage/$(PKG_NAME)-akronite-perf
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=akronite-perf
  DEPENDS+=@TARGET_ipq806x +TARGET_ipq806x:kmod-qca-nss-drv
  PKG_BUILD_DEPENDS+=qca-wifi-fw-hw4-10.4-asic
endef

define KernelPackage/$(PKG_NAME)-akronite-perf/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-akronite-perf/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-akronite-perf)
endef

define KernelPackage/$(PKG_NAME)-akronite-perf/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-akronite-perf))

# Emulation variant for ipq806x
define KernelPackage/$(PKG_NAME)-akronite-perf-emu
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=akronite-perf-emu
  DEPENDS+=@TARGET_ipq806x +TARGET_ipq806x:kmod-qca-nss-drv
endef

define KernelPackage/$(PKG_NAME)-akronite-perf-emu/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-akronite-perf-emu/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-akronite-perf-emu)
endef

define KernelPackage/$(PKG_NAME)-akronite-perf-emu/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-akronite-perf-emu))

# Perf variant
define KernelPackage/$(PKG_NAME)-perf
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=perf
  DEPENDS+=@TARGET_ar71xx
endef

define KernelPackage/$(PKG_NAME)-perf/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-perf/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-perf)
endef

define KernelPackage/$(PKG_NAME)-perf/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-perf))

# New datapath variant
define KernelPackage/$(PKG_NAME)-unified-perf
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=unified-perf
  DEPENDS+=@TARGET_ar71xx
endef

define KernelPackage/$(PKG_NAME)-unified-perf/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-unified-perf/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-unified-perf)
endef

define KernelPackage/$(PKG_NAME)-unified-perf/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-unified-perf))

# Dakota variant
define KernelPackage/$(PKG_NAME)-dakota-perf
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=dakota-perf
  DEPENDS+=@TARGET_ipq806x +TARGET_ipq806x:kmod-qca-nss-drv
  PKG_BUILD_DEPENDS+=qca-wifi-fw-hw5-10.4-asic
endef

define KernelPackage/$(PKG_NAME)-dakota-perf/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-dakota-perf/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-dakota-perf)
endef

define KernelPackage/$(PKG_NAME)-dakota-perf/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-dakota-perf))

# annapurnalabs variant
define KernelPackage/$(PKG_NAME)-annapurnalabs
$(call KernelPackage/$(PKG_NAME)/Default)
  VARIANT:=annapurnalabs
  PKG_BUILD_DEPENDS+=qca-wifi-fw-hw4-10.4-asic
endef

define KernelPackage/$(PKG_NAME)-annapurnalabs/Description
$(call define KernelPackage/$(PKG_NAME)/Description/Default)
endef

define KernelPackage/$(PKG_NAME)-annapurnalabs/config
$(call KernelPackage/$(PKG_NAME)/config/Default,-annapurnalabs)
endef

define KernelPackage/$(PKG_NAME)-annapurnalabs/install
$(call KernelPackage/$(PKG_NAME)/install/Default,$(1))
endef
$(eval $(call KernelPackage,$(PKG_NAME)-annapurnalabs))
