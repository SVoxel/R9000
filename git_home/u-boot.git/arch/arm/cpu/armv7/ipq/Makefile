# Copyright (c) 2012 The Linux Foundation. All rights reserved.

obj-y = timer.o
obj-y += clock.o
obj-y += gpio.o
#obj-y += smem.o
obj-$(CONFIG_HW29764958P0P128P512P3X3P4X4) += smem_hw29764958p0p128p512p3x3p4x4.o
obj-y += scm.o
#obj-y += cmd_bootipq.o
obj-$(CONFIG_HW29764958P0P128P512P3X3P4X4) += cmd_bootipq_hw29764958p0p128p512p3x3p4x4.o
ifeq ($(CONFIG_IPQ_APPSBL_DLOAD),y)
obj-y += cmd_dumpipq_data.o
endif

