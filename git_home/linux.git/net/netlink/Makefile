#
# Makefile for the netlink driver.
#

KBUILD_CFLAGS := $(filter-out -O%,$(KBUILD_CFLAGS)) -O3

obj-y  				:= af_netlink.o genetlink.o

obj-$(CONFIG_NETLINK_DIAG)	+= netlink_diag.o
netlink_diag-y			:= diag.o
