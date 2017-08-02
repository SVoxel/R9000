-include $(HAL_TOP)/file_list_base.mk

ccflags-y += -Iarch/$(CONFIG_SYS_ARCH)/include/asm/arch-$(CONFIG_SYS_SOC)/
ccflags-y += -Iboard/$(CONFIG_SYS_VENDOR)/common/
ccflags-y += $(HAL_USER_INCLUDE_PATH) $(HAL_DRIVER_INCLUDE_PATH)
ccflags-$(CONFIG_ALPINE) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V1)
ccflags-$(CONFIG_ALPINE_V2_32) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V2)
ccflags-$(CONFIG_ALPINE_V2_64) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V2)

obj-y += lowlevel_init.o
obj-y += board.o
