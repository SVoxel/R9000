#!/bin/sh

# Collect basi debug information

iwpriv ath0 dbgLVL 0x11C00180
iwpriv ath1 dbgLVL 0x11C00180
iwpriv ath01 dbgLVL 0x11C00180
iwpriv ath11 dbgLVL 0x11C00180
iwpriv ath0 dbgLVL_high 0x400
iwpriv ath1 dbgLVL_high 0x400
iwpriv ath01 dbgLVL_high 0x400
iwpriv ath11 dbgLVL_high 0x400

while [ 1 ]
do
	wlanconfig ath0 list
	wlanconfig ath1 list
	wlanconfig ath01 list
	wlanconfig ath11 list
	sleep 1
	athstats
	iwpriv ath0 txrx_fw_stats 3
	iwpriv ath0 txrx_fw_stats 6
	sleep 1
	iwpriv ath1 txrx_fw_stats 3
	iwpriv ath1 txrx_fw_stats 6
	sleep 1
	iwpriv ath01 txrx_fw_stats 3
	iwpriv ath01 txrx_fw_stats 6
	sleep 1
	iwpriv ath11 txrx_fw_stats 3
	iwpriv ath11 txrx_fw_stats 6
	sleep 1
	
	sleep 300
done

