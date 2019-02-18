--[[
LuCI - Lua Configuration Interface

Copyright (c) 2015 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

local m, s = ...

------------------------------------------------------------------------------
--Wi-Fi link monitoring settings
------------------------------------------------------------------------------
s = m:section(NamedSection, "WiFiLink", "WiFiLink", translate("Wi-Fi Link Monitoring"))
s.anonymous = true

vl = s:option(Value, "MinAssocCheckPostWPS", translate("Minimum association checks post Wi-Fi Protected Setup (WPS)"))
vl.datatype = "uinteger"

vl = s:option(Value, "WPSTimeout", translate("Wi-Fi Protected Setup (WPS) Timeout"))
vl.datatype = "uinteger"

vl = s:option(Value, "AssociationTimeout", translate("Association Timeout"))
vl.datatype = "uinteger"

vl = s:option(Value, "RSSINumMeasurements", translate("Number of RSSI measurements for coverage check"))
vl.datatype = "uinteger"

vl = s:option(Value, "RSSIThresholdFar", translate("RSSI value below which RE is considered too far from root AP"))
vl.datatype = "integer"

vl = s:option(Value, "RSSIThresholdNear", translate("RSSI value above which RE is considered too close to root AP"))
vl.datatype = "integer"

vl = s:option(Value, "RSSIThresholdMin", translate("Minimum RSSI value for a good link when operating in client mode"))
vl.datatype = "integer"

