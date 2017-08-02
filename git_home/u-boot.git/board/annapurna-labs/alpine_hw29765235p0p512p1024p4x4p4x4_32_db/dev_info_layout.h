#ifndef __DEV_INFO_LAYOUT_H__
#define __DEV_INFO_LAYOUT_H__

#define DEV_INFO_BASE                                       SRAM_DEV_INFO_ADDRESS

#define DEV_INFO_DEV_ID_0_OFFSET                            0

#define DEV_INFO_EARLY_INIT_ADDR_LSB_OFFSET                 10

#define DEV_INFO_EARLY_INIT_ADDR_MSB_OFFSET                 11

#define DEV_INFO_RSVD_XMODEM_LOAD_OFFSET                    12
#define DEV_INFO_RSVD_XMODEM_LOAD_SHIFT                     0
#define DEV_INFO_RSVD_XMODEM_LOAD_MASK                      0x01

#endif
