--[[
LuCI - Lua Configuration Interface

Copyright (c) 2014 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

local m, s = ...
------------------------------------------------------------------------------------------------
--Basic Settings - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "config_Adv", "config_Adv", translate("Basic Advanced"))
s.anonymous = true

vl = s:option(Value, "AgeLimit", translate("Maximum number of seconds elapsed allowed for a 'recent' measurement"))
vl.datatype = "uinteger"

------------------------------------------------------------------------------------------------
--Station database - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "StaDB_Adv", "StaDB_Adv", translate("Station Database Advanced"))
s.anonymous = true

vl = s:option(Value, "AgingSizeThreshold", translate("Size Threshold For Aging Timer"))
vl.datatype = "uinteger"
vl = s:option(Value, "AgingFrequency", translate("Aging Timer Frequency (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "OutOfNetworkMaxAge", translate("Max Age for Out-of-Network Client (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "InNetworkMaxAge", translate("Max Age for In-Network Client (s)"))
vl.datatype = "uinteger"

------------------------------------------------------------------------------------------------
--Station Monitor Setting - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "StaMonitor_Adv", "StaMonitor_Adv", translate("Post-association steering decision maker"))
s.anonymous = true
vl = s:option(Value, "RSSIMeasureSamples_W2", translate("Number of RSSI measurements on 2.4 GHz band"))
vl.datatype = "uinteger"
vl = s:option(Value, "RSSIMeasureSamples_W5", translate("Number of RSSI measurements on 5 GHz band"))
vl.datatype = "uinteger"

------------------------------------------------------------------------------------------------
--Band Monitor Settings - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "BandMonitor_Adv", "BandMonitor_Adv", translate("Utilization Monitor Advanced Settings"))
s.anonymous = true

vl = s:option(Value, "ProbeCountThreshold", translate("Number of probe requests required for the RSSI averaging"))
vl.datatype = "uinteger"

vl_mu_interval_w2 = s:option(Value, "MUCheckInterval_W2", translate("The frequency to check medium utilization on 2.4 GHz (s)"))
vl_mu_interval_w2.datatype = "uinteger"
vl_mu_interval_w5 = s:option(Value, "MUCheckInterval_W5", translate("The frequency to check medium utilization on 5 GHz (s)"))
vl_mu_interval_w5.datatype = "uinteger"

------------------------------------------------------------------------------------------------
--Estimator Settings - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "Estimator_Adv", "Estimator_Adv", translate("Rate estimation"))
s.anonymous = true
vl = s:option(Value, "RSSIDiff_EstW5FromW2", translate("Difference when estimating 5 GHz RSSI value from the one measured on 2.4 GHz"))
vl.datatype = "integer"
vl = s:option(Value, "RSSIDiff_EstW2FromW5", translate("Difference when estimating 2.4 GHz RSSI value from the one measured on 5 GHz"))
vl.datatype = "integer"
vl = s:option(Value, "ProbeCountThreshold", translate("Number of probe requests required for the RSSI averaging"))
vl.datatype = "uinteger"
vl = s:option(Value, "StatsSampleInterval", translate("Seconds between successive stats samples for estimating data rate"))
vl.datatype = "uinteger"
vl = s:option(Value, "11kProhibitTime", translate("Time to wait before sending a 802.11k beacon report request after last one (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "PhyRateScalingForAirtime", translate("Scaling factor (as percentage) for converting PHY rate to upper layer rate for airtime computations"))
vl.datatype = "uinteger"
vl = s:option(Flag, "EnableContinuousThroughput", translate("Continously measure throughput (for demo purposes only)"))
vl.rmempty = false
vl_bcnrpt_active_duration = s:option(Value, "BcnrptActiveDuration", translate("Active scan duration used in 802.11k Beacon Report (s)"))
vl_bcnrpt_active_duration.datatype = "uinteger"
vl_bcnrpt_passive_duration = s:option(Value, "BcnrptPassiveDuration", translate("Passive scan duration used in 802.11k Beacon Report request (s)"))
vl_bcnrpt_passive_duration.datatype = "uinteger"

------------------------------------------------------------------------------------------------
--Steer Executor Settings - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "SteerExec_Adv", "SteerExec_Adv", translate("Steering Executor Advanced Settings"))
s.anonymous = true
vl = s:option(Value, "TSteering", translate("Maximum time for client to associate on target band before AP aborts steering (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "InitialAuthRejCoalesceTime", translate("Time to coalesce multiple authentication rejects down to a single one (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "AuthRejMax", translate("Max consecutive authentication rejects after which the device is marked as steering unfriendly"))
vl.datatype = "uinteger"
vl = s:option(Value, "SteeringUnfriendlyTime", translate("The base amount of time a device is considered steering unfriendly before another attempt (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "MaxSteeringUnfriendly", translate("The maximum time used for backoff for steering unfriendly STAs.  Total amount of backoff is calculated as min(MaxSteeringUnfriendly, SteeringUnfriendlyTime * 2 ^ CountConsecutiveFailures) (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "TargetLowRSSIThreshold_W2", translate("RSSI threshold indicating 2.4 GHz band is not strong enough for association (dB)"))
vl.datatype = "uinteger"
vl = s:option(Value, "TargetLowRSSIThreshold_W5", translate("RSSI threshold indicating 5 GHz band is not strong enough for association (dB)"))
vl.datatype = "uinteger"
vl = s:option(Value, "BlacklistTime", translate("The amount of time (in seconds) before automatically removing the blacklist (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "BTMResponseTime", translate("The amount of time to wait for a BTM response (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "BTMAssociationTime", translate("The amount of time to wait for an association on the correct band after receiving a BTM response (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "BTMAlsoBlacklist", translate("If set to 1, will also setup blacklists when attempting to steer a client via BSS Transition Management"))
vl.datatype = "uinteger"
vl = s:option(Value, "BTMUnfriendlyTime", translate("The base amount of time a device is considered BTM-steering unfriendly before another attempt to steer via BTM (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "MaxBTMUnfriendly", translate("The maximum time used for backoff for BTM unfriendly STAs.  Total amount of backoff is calculated as min(MaxBTMUnfriendly, BTMUnfriendlyTime * 2 ^ CountConsecutiveFailures) (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "MaxBTMActiveUnfriendly", translate("The maximum time used for backoff for BTM STAs that fail active steering.  Total amount of backoff is calculated as min(MaxBTMActiveUnfriendly, BTMUnfriendlyTime * 2 ^ CountConsecutiveFailures) (s)"))
vl.datatype = "uinteger"
vl = s:option(Value, "MinRSSIBestEffort", translate("The minimum RSSI, below which lbd will only steer clients via best effort (no blacklists, failures do not mark as unfriendly) (dB)"))
vl.datatype = "uinteger"
vl = s:option(Value, "LowRSSIXingThreshold", translate("RSSI threshold to generate an indication when a client crosses it (dB)"))
vl.datatype = "uinteger"


------------------------------------------------------------------------------------------------
--Steer Algorithm Settings - Advanced
------------------------------------------------------------------------------------------------
s = m:section(NamedSection, "SteerAlg_Adv", "SteerAlg_Adv", translate("Steering Algorithm Advanced Settings"))
s.anonymous = true
vl = s:option(Value, "MinTxRateIncreaseThreshold", translate("Downlink rate (in Mbps) should exceed at least LowTxRateXingThreshold + this value when steering from 2.4GHz to 5GHz due to overload"))
vl.datatype = "uinteger"
