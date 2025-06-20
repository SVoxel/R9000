#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

BASE_NAME=openssl
PKG_NAME:=$(BASE_NAME)-1.0.2
PKG_BASE:=1.0.2
PKG_BUGFIX:=u
PKG_VERSION:=$(PKG_BASE)$(PKG_BUGFIX)
PKG_RELEASE:=1
PKG_USE_MIPS16:=0
PKG_BUILD_DIR := $(BUILD_DIR)/$(BASE_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=0

PKG_SOURCE:=$(BASE_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.openssl.org/source/ \
	ftp://ftp.openssl.org/source/ \
	http://www.openssl.org/source/old/$(PKG_BASE)/ \
	ftp://ftp.funet.fi/pub/crypt/mirrors/ftp.openssl.org/source \
	ftp://ftp.sunet.se/pub/security/tools/net/openssl/source/
PKG_MD5SUM:=cdc2638f789ecc2db2c91488265686c1

PKG_LICENSE:=OpenSSL
PKG_LICENSE_FILES:=LICENSE
PKG_BUILD_DEPENDS:=ocf-crypto-headers
PKG_CONFIG_DEPENDS:= \
	CONFIG_OPENSSL_102_ENGINE_CRYPTO \
	CONFIG_OPENSSL_102_ENGINE_DIGEST \
	CONFIG_OPENSSL_102_WITH_EC \
	CONFIG_OPENSSL_102_WITH_EC2M \
	CONFIG_OPENSSL_102_WITH_SSL3 \
	CONFIG_OPENSSL_102_HARDWARE_SUPPORT

include $(INCLUDE_DIR)/package.mk

# GCC 12.x.x issue fixing (use of '-fvect-cost-model=unlimited' flag)
ifdef CONFIG_GCC_USE_VERSION_12
  TARGET_CFLAGS := $(subst -fvect-cost-model=unlimited,-fvect-cost-model=dynamic,$(TARGET_CFLAGS))
endif

# GCC 13.x.x issue fixing (use of '-fvect-cost-model=unlimited' flag)
ifdef CONFIG_GCC_USE_VERSION_13
  TARGET_CFLAGS := $(subst -fvect-cost-model=unlimited,-fvect-cost-model=dynamic,$(TARGET_CFLAGS))
endif

# GCC 14.x.x issue fixing (use of '-fvect-cost-model=unlimited' flag)
ifdef CONFIG_GCC_USE_VERSION_14
  TARGET_CFLAGS := $(subst -fvect-cost-model=unlimited,-fvect-cost-model=dynamic,$(TARGET_CFLAGS))
endif

# GCC 15.x.x issue fixing (use of '-fvect-cost-model=unlimited' flag)
ifdef CONFIG_GCC_USE_VERSION_15
  TARGET_CFLAGS := $(subst -fvect-cost-model=unlimited,-fvect-cost-model=dynamic,$(TARGET_CFLAGS))
endif

TARGET_CFLAGS := $(filter-out -O%,$(TARGET_CFLAGS)) -O3
TARGET_CFLAGS += $(FPIC) -ffunction-sections -fdata-sections
TARGET_ASFLAGS := $(TARGET_CFLAGS)
TARGET_LDFLAGS += -Wl,--gc-sections

ifneq ($(CONFIG_CCACHE),)
  HOSTCC=$(HOSTCC_NOCACHE)
  HOSTCXX=$(HOSTCXX_NOCACHE)
endif

define Package/openssl-1.0.2/Default
  TITLE:=Open source SSL toolkit
  URL:=http://www.openssl.org/
endef

define Package/libopenssl-1.0.2/config
  source "$(SOURCE)/Config.in"
endef

define Package/openssl-1.0.2/Default/description
  The OpenSSL Project is a collaborative effort to develop a robust,
  commercial-grade, full-featured, and Open Source toolkit implementing the Secure
  Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS v1) protocols as well
  as a full-strength general purpose cryptography library.
endef

define Package/libopenssl-1.0.2
$(call Package/openssl/Default)
  SECTION:=libs
  SUBMENU:=SSL
  CATEGORY:=Libraries
  DEPENDS:=+zlib
  TITLE+= (libraries)
  ABI_VERSION:=$(PKG_VERSION)
  MENU:=1
endef

define Package/libopenssl-1.0.2/description
  $(call Package/openssl-1.0.2/Default/description)
  This package contains the OpenSSL shared libraries, needed by other programs.
endef

define Package/openssl-util-1.0.2
  $(call Package/openssl-1.0.2/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libopenssl-1.0.2
  TITLE+= (utility)
endef

define Package/openssl-util-1.0.2/conffiles
  /etc/ssl/1.0.2/openssl.cnf
endef

define Package/openssl-util-1.0.2/description
  $(call Package/openssl-1.0.2/Default/description)
  This package contains the OpenSSL command-line utility.
endef


OPENSSL_NO_CIPHERS:= no-idea no-md2 no-mdc2 no-rc5 no-sha0 no-smime \
		     no-rmd160 no-aes192 no-ripemd no-camellia no-ans1 no-krb5
OPENSSL_OPTIONS:= shared no-err threads zlib-dynamic

ifdef CONFIG_OPENSSL_102_ENGINE_CRYPTO
  OPENSSL_OPTIONS += -DHAVE_CRYPTODEV
  ifdef CONFIG_OPENSSL_102_ENGINE_DIGEST
    OPENSSL_OPTIONS += -DUSE_CRYPTODEV_DIGESTS
  endif
else
  OPENSSL_OPTIONS += no-engines
endif

ifndef CONFIG_OPENSSL_102_WITH_EC
  OPENSSL_OPTIONS += no-ec
endif

ifndef CONFIG_OPENSSL_102_WITH_EC2M
  OPENSSL_OPTIONS += no-ec2m
endif

ifndef CONFIG_OPENSSL_102_WITH_SSL3
  OPENSSL_OPTIONS += no-ssl3
endif

ifndef CONFIG_OPENSSL_102_HARDWARE_SUPPORT
  OPENSSL_OPTIONS += no-hw
endif

ifeq ($(CONFIG_x86_64),y)
  OPENSSL_TARGET:=linux-x86_64-openwrt
  OPENSSL_MAKEFLAGS += LIBDIR=lib
else
  OPENSSL_OPTIONS+=no-sse2
  ifeq ($(CONFIG_mips)$(CONFIG_mipsel),y)
    OPENSSL_TARGET:=linux-mips-openwrt
  else ifeq ($(CONFIG_arm)$(CONFIG_armeb),y)
    OPENSSL_TARGET:=linux-armv4-openwrt
  else
    OPENSSL_TARGET:=linux-generic-openwrt
    OPENSSL_OPTIONS+=no-perlasm
  endif
endif

STAMP_CONFIGURED := $(STAMP_CONFIGURED)_$(subst $(space),_,$(OPENSSL_OPTIONS))

define Build/Configure
	[ -f $(STAMP_CONFIGURED) ] || { \
		rm -f $(PKG_BUILD_DIR)/*.so.* $(PKG_BUILD_DIR)/*.a; \
		find $(PKG_BUILD_DIR) -name \*.o | xargs rm -f; \
	}
	(cd $(PKG_BUILD_DIR); \
		./Configure $(OPENSSL_TARGET) \
			--prefix=/usr \
			--openssldir=/etc/ssl/1.0.2 \
			-D__ARM_MAX_ARCH__=7 \
			$(TARGET_CPPFLAGS) \
			$(TARGET_LDFLAGS) -ldl \
			-DNDEBUG \
			$(OPENSSL_NO_CIPHERS) \
			$(OPENSSL_OPTIONS) \
	)
	# XXX: OpenSSL "make depend" will look for installed headers before its own,
	# so remove installed stuff first
	-$(SUBMAKE) -j1 clean-staging
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		MAKEDEPPROG="$(TARGET_CROSS)gcc" \
		OPENWRT_OPTIMIZATION_FLAGS="$(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		depend
endef


define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		ASFLAGS="$(TARGET_ASFLAGS) -I$(PKG_BUILD_DIR)/crypto -c" \
		AR="$(TARGET_CROSS)ar r" \
		RANLIB="$(TARGET_CROSS)ranlib" \
		OPENWRT_OPTIMIZATION_FLAGS="$(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		all
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		ASFLAGS="$(TARGET_ASFLAGS) -I$(PKG_BUILD_DIR)/crypto -c" \
		AR="$(TARGET_CROSS)ar r" \
		RANLIB="$(TARGET_CROSS)ranlib" \
		OPENWRT_OPTIMIZATION_FLAGS="$(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		build-shared
	# Work around openssl build bug to link libssl.so with libcrypto.so.
	-rm $(PKG_BUILD_DIR)/libssl.so.*.*.*
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		OPENWRT_OPTIMIZATION_FLAGS="$(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		do_linux-shared
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		INSTALL_PREFIX="$(PKG_INSTALL_DIR)" \
		$(OPENSSL_MAKEFLAGS) \
		install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/local/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/openssl $(1)/usr/local/include/
	$(INSTALL_DIR) $(1)/usr/local/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{crypto,ssl}.{a,so*} $(1)/usr/local/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/openssl.pc $(1)/usr/lib/pkgconfig/openssl-1.0.2.pc
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libcrypto.pc $(1)/usr/lib/pkgconfig/libcrypto-1.0.2.pc
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libssl.pc $(1)/usr/lib/pkgconfig/libssl-1.0.2.pc
	[ -n "$(TARGET_LDFLAGS)" ] && $(SED) 's#$(TARGET_LDFLAGS)##g' $(1)/usr/lib/pkgconfig/{openssl-1.0.2,libcrypto-1.0.2,libssl-1.0.2}.pc || true
endef

define Package/libopenssl-1.0.2/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libcrypto.so.* $(1)/usr/lib/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libssl.so.* $(1)/usr/lib/
endef

define Package/openssl-util-1.0.2/install
	$(INSTALL_DIR) $(1)/etc/ssl/1.0.2
	$(CP) $(PKG_INSTALL_DIR)/etc/ssl/1.0.2/openssl.cnf $(1)/etc/ssl/1.0.2/
	$(LN) ../certs $(1)/etc/ssl/1.0.2/certs
	$(LN) ../private $(1)/etc/ssl/1.0.2/private
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/openssl $(1)/usr/bin/
endef

$(eval $(call BuildPackage,libopenssl-1.0.2))
$(eval $(call BuildPackage,openssl-util-1.0.2))
