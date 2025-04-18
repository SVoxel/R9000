#
# Copyright (C) 2010-2012 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=uhttpd
PKG_RELEASE:=31

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS := \
	CONFIG_PACKAGE_uhttpd-mod-tls \
	CONFIG_PACKAGE_uhttpd-mod-tls_cyassl \
	CONFIG_PACKAGE_uhttpd-mod-tls_openssl

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -Wno-int-conversion -Wno-implicit-function-declaration
TARGET_CFLAGS += -Wno-return-mismatch -Wno-incompatible-pointer-types

WEB_PATH:=www
PKG_GIT_TREEISH:=uhttpd-dni015
define Package/uhttpd/default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=uHTTPd - tiny, single threaded HTTP server
  MAINTAINER:=Jo-Philipp Wich <xm@subsignal.org>
endef

define Package/uhttpd
  $(Package/uhttpd/default)
  MENU:=1
endef

define Package/uhttpd/description
 uHTTPd is a tiny single threaded HTTP server with TLS, CGI and Lua
 support. It is intended as a drop-in replacement for the Busybox
 HTTP daemon.
endef


define Package/uhttpd-mod-tls
  $(Package/uhttpd/default)
  TITLE+= (TLS plugin)
  DEPENDS:=uhttpd +PACKAGE_uhttpd-mod-tls_cyassl:libcyassl +PACKAGE_uhttpd-mod-tls_openssl:libopenssl-1.0.2
endef

define Package/uhttpd-mod-tls/description
 The TLS plugin adds HTTPS support to uHTTPd.
endef

define Package/uhttpd-mod-tls/config
        choice
                depends on PACKAGE_uhttpd-mod-tls
                prompt "TLS Provider"
                default PACKAGE_uhttpd-mod-tls_cyassl

                config PACKAGE_uhttpd-mod-tls_cyassl
                        bool "CyaSSL"

                config PACKAGE_uhttpd-mod-tls_openssl
                        bool "OpenSSL"
        endchoice
endef

CGI_SUPPORT:= 1
LUA_SUPPORT:= 0
TLS_SUPPORT:= 1

UHTTPD_TLS:=
TLS_CFLAGS:=
TLS_LDFLAGS:=

ifneq ($(CONFIG_PACKAGE_uhttpd-mod-tls_cyassl),)
  UHTTPD_TLS:=cyassl
  TLS_CFLAGS:=-I$(STAGING_DIR)/usr/include/cyassl -DTLS_IS_CYASSL
  TLS_LDFLAGS:=-lcyassl -lm
endif

ifneq ($(CONFIG_PACKAGE_uhttpd-mod-tls_openssl),)
  UHTTPD_TLS:=openssl
  TLS_CFLAGS:=-DTLS_IS_OPENSSL
  TLS_LDFLAGS:=-lssl
  TARGET_CPPFLAGS:=-I$(STAGING_DIR)/usr/local/include $(TARGET_CPPFLAGS)
  TARGET_LDFLAGS:=-L$(STAGING_DIR)/usr/local/lib $(TARGET_LDFLAGS)
endif


define Package/uhttpd-mod-lua
  $(Package/uhttpd/default)
  TITLE+= (Lua plugin)
  DEPENDS:=uhttpd +liblua
endef

define Package/uhttpd-mod-lua/description
 The Lua plugin adds a CGI-like Lua runtime interface to uHTTPd.
endef


TARGET_CFLAGS += $(TLS_CFLAGS)
TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib
MAKE_VARS += \
	FPIC="$(FPIC)" \
	LUA_SUPPORT="$(if $(CONFIG_PACKAGE_uhttpd-mod-lua),1)" \
	TLS_SUPPORT="$(if $(CONFIG_PACKAGE_uhttpd-mod-tls),1)" \
	UHTTPD_TLS="$(UHTTPD_TLS)" \
	TLS_CFLAGS="$(TLS_CFLAGS)" \
	TLS_LDFLAGS="$(TLS_LDFLAGS)" \
	CGI_SUPPORT="$(CGI_SUPPORT)" \
	LUA_SUPPORT="$(LUA_SUPPORT)" \
	TLS_SUPPORT="$(TLS_SUPPORT)"

define Build/Prepare
	$(CP) $(GIT_HOME)/uhttpd.git/* $(PKG_BUILD_DIR)/
endef

define Package/uhttpd/conffiles
/etc/config/uhttpd
endef

#define Build/Configure
#	(cd $(PKG_BUILD_DIR)/files && \
#	./generate_crt.sh \
#	)
#endef

define Package/uhttpd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/uhttpd.init $(1)/etc/init.d/uhttpd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/uhttpd.config $(1)/etc/config/uhttpd
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/uhttpd.key $(1)/etc/uhttpd.key
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/uhttpd.crt $(1)/etc/uhttpd.crt
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd $(1)/usr/sbin/uhttpd
	$(INSTALL_DIR) $(1)/$(WEB_PATH)/cgi-bin
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/uhttpd.sh $(1)/$(WEB_PATH)/cgi-bin
	chmod a+x $(1)/$(WEB_PATH)/cgi-bin/uhttpd.sh
endef

define Package/uhttpd-mod-tls/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd_tls.so $(1)/usr/lib/
endef

define Package/uhttpd-mod-lua/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uhttpd_lua.so $(1)/usr/lib/
endef


$(eval $(call BuildPackage,uhttpd))
$(eval $(call BuildPackage,uhttpd-mod-tls))
$(eval $(call BuildPackage,uhttpd-mod-lua))
