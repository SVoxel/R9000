#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifeq ($(CONFIG_AL_ETH),y)
-include $(HAL_TOP)/file_list_base.mk
ccflags-y += -Iboard/$(CONFIG_SYS_VENDOR)/common/
ccflags-y += -Iboard/$(CONFIG_SYS_VENDOR)/$(BOARD)/
ccflags-y += $(HAL_USER_INCLUDE_PATH) $(HAL_DRIVER_INCLUDE_PATH) $(HAL_INIT_INCLUDE_PATH)
ccflags-$(CONFIG_ALPINE) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V1)
ccflags-$(CONFIG_ALPINE_V2_32) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V2)
ccflags-$(CONFIG_ALPINE_HW29765235P0P512P1024P4X4P4X4_32) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_HW29765235P0P512P1024P4X4P4X4)
ccflags-$(CONFIG_ALPINE_HW29765294P0P128P512P8X8P4X4_32) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_HW29765294P0P128P512P8X8P4X4)
ccflags-$(CONFIG_ALPINE_V2_64) += $(HAL_PLATFORM_INCLUDE_PATH_ALPINE_V2)
endif

obj-$(CONFIG_PPC4xx_EMAC) += 4xx_enet.o
obj-$(CONFIG_ALTERA_TSE) += altera_tse.o
obj-$(CONFIG_ARMADA100_FEC) += armada100_fec.o
obj-$(CONFIG_DRIVER_AT91EMAC) += at91_emac.o
obj-$(CONFIG_DRIVER_AX88180) += ax88180.o
obj-$(CONFIG_BCM_SF2_ETH) += bcm-sf2-eth.o
obj-$(CONFIG_BCM_SF2_ETH_GMAC) += bcm-sf2-eth-gmac.o
obj-$(CONFIG_BFIN_MAC) += bfin_mac.o
obj-$(CONFIG_CALXEDA_XGMAC) += calxedaxgmac.o
obj-$(CONFIG_CS8900) += cs8900.o
obj-$(CONFIG_TULIP) += dc2114x.o
obj-$(CONFIG_DESIGNWARE_ETH) += designware.o
obj-$(CONFIG_DRIVER_DM9000) += dm9000x.o
obj-$(CONFIG_DNET) += dnet.o
obj-$(CONFIG_E1000) += e1000.o
obj-$(CONFIG_E1000_SPI) += e1000_spi.o
obj-$(CONFIG_EEPRO100) += eepro100.o
obj-$(CONFIG_SUNXI_EMAC) += sunxi_emac.o
obj-$(CONFIG_ENC28J60) += enc28j60.o
obj-$(CONFIG_EP93XX) += ep93xx_eth.o
obj-$(CONFIG_ETHOC) += ethoc.o
obj-$(CONFIG_FEC_MXC) += fec_mxc.o
obj-$(CONFIG_FSLDMAFEC) += fsl_mcdmafec.o mcfmii.o
obj-$(CONFIG_FTGMAC100) += ftgmac100.o
obj-$(CONFIG_FTMAC110) += ftmac110.o
obj-$(CONFIG_FTMAC100) += ftmac100.o
obj-$(CONFIG_GRETH) += greth.o
obj-$(CONFIG_DRIVER_TI_KEYSTONE_NET) += keystone_net.o
obj-$(CONFIG_DRIVER_KS8695ETH) += ks8695eth.o
obj-$(CONFIG_KS8851_MLL) += ks8851_mll.o
obj-$(CONFIG_LAN91C96) += lan91c96.o
obj-$(CONFIG_MACB) += macb.o
obj-$(CONFIG_MCFFEC) += mcffec.o mcfmii.o
obj-$(CONFIG_MPC5xxx_FEC) += mpc5xxx_fec.o
obj-$(CONFIG_MPC512x_FEC) += mpc512x_fec.o
obj-$(CONFIG_MVGBE) += mvgbe.o
obj-$(CONFIG_MVNETA) += mvneta.o
obj-$(CONFIG_NATSEMI) += natsemi.o
obj-$(CONFIG_DRIVER_NE2000) += ne2000.o ne2000_base.o
obj-$(CONFIG_DRIVER_AX88796L) += ax88796.o ne2000_base.o
obj-$(CONFIG_NETCONSOLE) += netconsole.o
obj-$(CONFIG_NS8382X) += ns8382x.o
obj-$(CONFIG_PCNET) += pcnet.o
obj-$(CONFIG_RTL8139) += rtl8139.o
obj-$(CONFIG_RTL8169) += rtl8169.o
obj-$(CONFIG_SH_ETHER) += sh_eth.o
obj-$(CONFIG_SMC91111) += smc91111.o
obj-$(CONFIG_SMC911X) += smc911x.o
obj-$(CONFIG_DRIVER_TI_EMAC) += davinci_emac.o
obj-$(CONFIG_TSEC_ENET) += tsec.o fsl_mdio.o
obj-$(CONFIG_DRIVER_TI_CPSW) += cpsw.o
obj-$(CONFIG_FMAN_ENET) += fsl_mdio.o
obj-$(CONFIG_TSI108_ETH) += tsi108_eth.o
obj-$(CONFIG_ULI526X) += uli526x.o
obj-$(CONFIG_VSC7385_ENET) += vsc7385.o
obj-$(CONFIG_XILINX_AXIEMAC) += xilinx_axi_emac.o
obj-$(CONFIG_XILINX_EMACLITE) += xilinx_emaclite.o
obj-$(CONFIG_XILINX_LL_TEMAC) += xilinx_ll_temac.o xilinx_ll_temac_mdio.o \
		xilinx_ll_temac_fifo.o xilinx_ll_temac_sdma.o
obj-$(CONFIG_SYNOPSYS_GEMAC) += nss/synopGMAC_network_interface.o nss/synopGMAC_Dev.o nss/uboot_skb.o nss/mii_gpio.o nss/synop_mdio_acc.o
#obj-$(CONFIG_SYNOPSYS_GEMAC) += ../../board/qcom/common/athrs17_phy.o
obj-$(CONFIG_HW29764958P0P128P512P3X3P4X4) += ipq/ipq_gmac_eth_hw29764958p0p128p512p3x3p4x4.o
obj-$(CONFIG_IPQ_MDIO) += ipq/ipq_mdio.o
#obj-$(CONFIG_IPQ_SWITCH_ATHRS17) += ../../board/qcom/common/athrs17_phy.o
obj-$(CONFIG_HW29764958P0P128P512P3X3P4X4) += ../../board/qcom/common/athrs17_phy_hw29764958p0p128p512p3x3p4x4.o
obj-$(CONFIG_IPQ_SWITCH_QCA8511) += ../../board/qcom/common/qca8511.o
obj-$(CONFIG_ZYNQ_GEM) += zynq_gem.o
obj-$(CONFIG_FSL_MC_ENET) += fsl_mc/
obj-$(CONFIG_AL_ETH) += al_eth_pci.o al_eth.o
