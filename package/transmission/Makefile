#
# Copyright (C) 2009-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=transmission
PKG_VERSION:=3.00
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@GITHUB/transmission/transmission-releases/master
PKG_MD5SUM:=a23a32672b83c89b9b61e90408f53d98
PKG_MAINTAINER:=Rosen Penev <rosenp@gmail.com>
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

TARGET_CPPFLAGS:=-I$(STAGING_DIR)/usr/local/include $(TARGET_CPPFLAGS)
TARGET_LDFLAGS:=-L$(STAGING_DIR)/usr/local/lib $(TARGET_LDFLAGS)
TARGET_CFLAGS += -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -Wl,--gc-sections

define Package/transmission/template
  SUBMENU:=BitTorrent
  SECTION:=net
  CATEGORY:=Network
  TITLE:=BitTorrent client
  URL:=http://www.transmissionbt.com
  DEPENDS:=+ca-bundle +libcurl +libevent2 +libminiupnpc +libnatpmp +libpthread +librt +zlib
endef

define Package/transmission-daemon/Default
  $(call Package/transmission/template)
  USERID:=transmission=224:transmission=224
endef

define Package/transmission-daemon-openssl
  $(call Package/transmission-daemon/Default)
  TITLE+= (with OpenSSL)
  DEPENDS+=+libopenssl-1.0.2
  VARIANT:=openssl
endef

define Package/transmission-daemon-mbedtls
  $(call Package/transmission-daemon/Default)
  TITLE+= (with mbed TLS)
  DEPENDS+=+libmbedtls
  VARIANT:=mbedtls
endef

define Package/transmission-cli-openssl
  $(call Package/transmission/template)
  TITLE+= (with OpenSSL)
  DEPENDS+=+libopenssl-1.0.2
  VARIANT:=openssl
endef

define Package/transmission-cli-mbedtls
  $(call Package/transmission/template)
  TITLE+= (with mbed TLS)
  DEPENDS+=+libmbedtls
  VARIANT:=mbedtls
endef

define Package/transmission-remote-openssl
  $(call Package/transmission/template)
  TITLE+= (with OpenSSL)
  DEPENDS+=+libopenssl-1.0.2
  VARIANT:=openssl
endef

define Package/transmission-remote-mbedtls
  $(call Package/transmission/template)
  TITLE+= (with mbed TLS)
  DEPENDS+=+libmbedtls
  VARIANT:=mbedtls
endef

define Package/transmission-web
  $(call Package/transmission/template)
  TITLE+= (webinterface)
  DEPENDS:=@(PACKAGE_transmission-daemon-openssl||PACKAGE_transmission-daemon-mbedtls)
endef


define Package/transmission-daemon/Default/description
 Transmission is a simple BitTorrent client.
 It features a very simple, intuitive interface
 on top on an efficient, cross-platform back-end.
 This package contains the daemon itself.
endef
Package/transmission-daemon-openssl/description = $(Package/transmission-daemon/Default/description)
Package/transmission-daemon-mbedtls/description = $(Package/transmission-daemon/Default/description)

define Package/transmission-cli/Default/description
 CLI utilities for transmission.
endef
Package/transmission-cli-openssl/description = $(Package/transmission-cli/Default/description)
Package/transmission-cli-mbedtls/description = $(Package/transmission-cli/Default/description)

define Package/transmission-remote/Default/description
 CLI remote interface for transmission.
endef
Package/transmission-remote-openssl/description = $(Package/transmission-remote/Default/description)
Package/transmission-remote-mbedtls/description = $(Package/transmission-remote/Default/description)

define Package/transmission-web/description
 Webinterface resources for transmission.
endef

define Package/transmission-daemon-openssl/conffiles
/etc/config/transmission
endef
Package/transmission-daemon-mbedtls/conffiles = $(Package/transmission-daemon-openssl/conffiles)

CONFIGURE_VARS += \
	LIBEVENT_LIBS="$(STAGING_DIR)/usr/lib/libevent-2.1.so.7.0.1"

CONFIGURE_ARGS += \
	--enable-daemon \
	--enable-cli \
	--without-gtk \
	--enable-largefile \
	--enable-lightweight

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) -D_GNU_SOURCE -I$(STAGING_DIR)/usr/include/libevent2/ -std=c99"

ifeq ($(BUILD_VARIANT),openssl)
  CONFIGURE_ARGS += \
	--with-crypto=openssl
endif

ifeq ($(BUILD_VARIANT),mbedtls)
  CONFIGURE_ARGS += \
	--with-crypto=polarssl
  CONFIGURE_VARS += \
	MBEDTLS_CFLAGS="-I$(STAGING_DIR)/usr/include/mbedtls" \
	MBEDTLS_LIBS="-lmbedtls -lmbedcrypto"
endif

define Package/transmission-daemon-openssl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/transmission-daemon $(1)/usr/bin/
	$(INSTALL_BIN) files/transbt.sh $(1)/usr/bin
	$(INSTALL_BIN) files/WhenDone.sh $(1)/usr/bin
	$(INSTALL_BIN) files/transbt-poptsk.sh $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) files/transmission.init $(1)/etc/init.d/transmission
	$(CP) files/etc/transmission $(1)/etc/
endef
Package/transmission-daemon-mbedtls/install = $(Package/transmission-daemon-openssl/install)

define Package/transmission-cli-openssl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/transmission-daemon $(1)/usr/bin/
	$(INSTALL_BIN) files/transbt.sh $(1)/usr/bin
	$(INSTALL_BIN) files/WhenDone.sh $(1)/usr/bin
	$(INSTALL_BIN) files/transbt-poptsk.sh $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) files/transmission.init $(1)/etc/init.d/transmission
	$(CP) files/etc/transmission $(1)/etc/
endef
Package/transmission-cli-mbedtls/install = $(Package/transmission-cli-openssl/install)

define Package/transmission-remote-openssl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/transmission-remote $(1)/usr/bin/
endef
Package/transmission-remote-mbedtls/install = $(Package/transmission-remote-openssl/install)

define Package/transmission-web/install
	$(INSTALL_DIR) $(1)/usr/share/transmission
	$(CP) $(PKG_INSTALL_DIR)/usr/share/transmission/web $(1)/usr/share/transmission/
endef

$(eval $(call BuildPackage,transmission-daemon-openssl))
$(eval $(call BuildPackage,transmission-daemon-mbedtls))
$(eval $(call BuildPackage,transmission-cli-openssl))
$(eval $(call BuildPackage,transmission-cli-mbedtls))
$(eval $(call BuildPackage,transmission-remote-openssl))
$(eval $(call BuildPackage,transmission-remote-mbedtls))
$(eval $(call BuildPackage,transmission-web))
