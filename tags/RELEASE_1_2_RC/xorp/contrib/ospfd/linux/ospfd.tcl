###############################################################
# OSPFD routing daemon
# Copyright (C) 1998, 1999 by John T. Moy
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
###############################################################

global {global_att}
global {areas}
global {routes}
global {area_att}
global {ifc_att}
global {aggr_att}
global {host_att}
global {vl_att}
global {nbr_att}
global {exrt_att}
global {md5_att}
global {thisarea}
global {thisifc}
global {thisaggr}
global {route_att}
global {route_index}
global {thisrt}

set areas {}
set routes {}
set route_index 0
set thisarea "0.0.0.0"
set global_att(lsdb_limit) 0
set global_att(mospf_enabled) 0
set global_att(inter_area_mc) 1
set global_att(ovfl_int) 300
set global_att(new_flood_rate) 200
set global_att(max_rxmt_window) 8
set global_att(max_dds) 4
set global_att(base_level) 4
set global_att(host) 0
set global_att(refresh_rate) 0
set global_att(PPAdjLimit) 0
set global_att(random_refresh) 0

set IGMP_OFF 0
set IGMP_ON 1
set IGMP_DFLT 2

###############################################################
# Top-level commands to set global parameters
# Router ID is set by "routerid _id_"
# top-level commands:
#	ospfExtLsdbLimit %no
#	host_mode
#	mospf
#	no_inter_area_mc
#	ospfExitOverflowInterval %no
#	ase_orig_rate %no
#	lsu_rxmt_window %no
#	dd_sessions %no
#	log_level %no
#	refresh_rate %seconds
#	PPAdjLimit %no
#	random_refresh
###############################################################

proc ospfExtLsdbLimit {val} {
    global global_att
    set global_att(lsdb_limit) $val
}
proc host_mode {} {
    global global_att
    set global_att(host) 1
}
proc mospf {} {
    global global_att
    set global_att(mospf_enabled) 1
}
proc no_inter_area_mc {} {
    global global_att
    set global_att(inter_area_mc) 0
}
proc ospfExitOverflowInterval {val} {
    global global_att
    set global_att(ovfl_int) $val
}
proc ase_orig_rate {val} {
    global global_att
    set global_att(new_flood_rate) $val
}
proc lsu_rxmt_window {val} {
    global global_att
    set global_att(max_rxmt_window) $val
}
proc dd_sessions {val} {
    global global_att
    set global_att(max_dds) $val
}
proc log_level {level} {
    global global_att
    set global_att(base_level) $level
}
proc refresh_rate {secs} {
    global global_att
    set global_att(refresh_rate) $secs
}
proc PPAdjLimit {nadj} {
    global global_att
    set global_att(PPAdjLimit) $nadj
}
proc random_refresh {} {
    global global_att
    set global_att(random_refresh) 1
}

###############################################################
# Area configuration:
#	area _id_
# subcommands:
#	stub _default_cost_
#	no_summaries
###############################################################

proc area {area_id} {
    global areas area_att thisarea
    if {[lsearch $areas $area_id] != -1} {
	return;
    }
    lappend areas $area_id
    set thisarea $area_id
    set area_att($area_id,interfaces) {}
    set area_att($area_id,aggregates) {}
    set area_att($area_id,hosts) {}
    set area_att($area_id,vls) {}
    set area_att($area_id,stub) 0
    set area_att($area_id,dflt_cost) 0
    set area_att($area_id,import_summs) 1
}

proc stub {arg} {
    global area_att thisarea
    set area_att($thisarea,stub) 1
    set area_att($thisarea,dflt_cost) $arg
}
proc no_summaries {} {
    global area_att thisarea
    set area_att($thisarea,import_summs) 0
}

###############################################################
# Interface configuration:
#	interface _address_ _cost_
# subordinate to area configuration
# subcommands:
#	mtu %no
#	IfIndex %no
#	nbma
#	ptmp
#	ospfIfRtrPriority %no
#	ospfIfTransitDelay %no
#	ospfIfRetransInterval %no
#	ospfIfHelloInterval %no
#	ospfIfRtrDeadInterval %no
#	ospfIfPollInterval %no
#	ospfIfAuthType %no
#	ospfIfAuthKey %string
#	ospfIfMulticastForwarding %special
#	on-demand
#	passive
###############################################################

proc interface {address cost} {
    global thisarea thisifc area_att ifc_att
    global IGMP_DFLT
    if {[lsearch $area_att($thisarea,interfaces) $address] != -1} {
	return;
    }
    lappend area_att($thisarea,interfaces) $address
    set thisifc $address
    set ifc_att($thisarea,$address,nbrs) {}
    set ifc_att($thisarea,$thisifc,keys) {}
    set ifc_att($thisarea,$address,mtu) 0
    set ifc_att($thisarea,$address,IfIndex) [llength interfaces]
    set ifc_att($thisarea,$address,iftype) 0
    set ifc_att($thisarea,$address,dr_pri) 1
    set ifc_att($thisarea,$address,xmt_dly) 1
    set ifc_att($thisarea,$address,rxmt_int) 5
    set ifc_att($thisarea,$address,hello_int) 10
    set ifc_att($thisarea,$address,cost) $cost
    set ifc_att($thisarea,$address,dead_int) 40
    set ifc_att($thisarea,$address,poll_int) 120
    set ifc_att($thisarea,$address,auth_type) 0
    set ifc_att($thisarea,$address,auth_key) ""
    set ifc_att($thisarea,$address,mc_fwd) 1
    set ifc_att($thisarea,$address,demand) 0
    set ifc_att($thisarea,$address,passive) 0
    set ifc_att($thisarea,$address,igmp) $IGMP_DFLT
}

proc mtu {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,mtu) $val
}
proc IfIndex {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,IfIndex) $val
}
proc nbma {} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,iftype) 3
}
proc ptmp {} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,iftype) 4
}
proc ospfIfRtrPriority {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,dr_pri) $val
}
proc ospfIfTransitDelay {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,xmt_dly) $val
}
proc ospfIfRetransInterval {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,rxmt_int) $val
}
proc ospfIfHelloInterval {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,hello_int) $val
}
proc ospfIfRtrDeadInterval {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,dead_int) $val
}
proc ospfIfPollInterval {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,poll_int) $val
}
proc ospfIfAuthType {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,auth_type) $val
}
proc ospfIfAuthKey {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,auth_key) $val
}
proc ospfIfMulticastForwarding {val} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,mc_fwd) $val
}
proc on-demand {} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,demand) 1
}
proc passive {} {
    global thisarea thisifc ifc_att
    set ifc_att($thisarea,$thisifc,passive) 1
}
proc igmp {} {
    global thisarea thisifc ifc_att
    global IGMP_ON
    set ifc_att($thisarea,$thisifc,igmp) $IGMP_ON
}

###############################################################
# Area aggregate configuration:
#	aggregate _prefix_
# subordinate to area configuration
# subcommands:
#	suppress
###############################################################

proc aggregate {prefix} {
    global thisarea thisaggr area_att aggr_att
    if {[lsearch $area_att($thisarea,aggregates) $prefix] != -1} {
	return;
    }
    lappend area_att($thisarea,aggregates) $prefix
    set thisaggr $prefix
    set aggr_att($thisarea,$prefix,suppress) 0
}

proc suppress {} {
    global thisarea thisaggr aggr_att
    set aggr_att($thisarea,$thisaggr,suppress) 1
}

###############################################################
# Host or loopback configuration:
#	host _prefix_ _cost_
# subordinate to area configuration
###############################################################

proc host {prefix cost} {
    global thisarea area_att host_att
    if {[lsearch $area_att($thisarea,hosts) $prefix] != -1} {
	return;
    }
    lappend area_att($thisarea,hosts) $prefix
    set host_att($thisarea,$prefix,cost) $cost
}

###############################################################
# Virtual link configuration:
#	vlink _endpoint_
# subordinate to (transit) area configuration
# subcommands:
#	ospfVirtIfTransitDelay
#	ospfVirtIfRetransInterval
#	ospfVirtIfHelloInterval
#	ospfVirtIfRtrDeadInterval
#	ospfVirtIfAuthType
#	ospfVirtIfAuthKey
###############################################################

proc vlink {endpoint} {
    global thisarea thisvl area_att vl_att
    if {[lsearch $area_att($thisarea,vls) $endpoint] != -1} {
	return;
    }
    lappend area_att($thisarea,vls) $endpoint
    set thisvl $endpoint
    set vl_att($thisarea,$endpoint,xmt_dly) 1
    set vl_att($thisarea,$endpoint,rxmt_int) 5
    set vl_att($thisarea,$endpoint,hello_int) 10
    set vl_att($thisarea,$endpoint,dead_int) 60
    set vl_att($thisarea,$endpoint,auth_type) 0
    set vl_att($thisarea,$endpoint,auth_key) ""
}

proc ospfVirtIfTransitDelay {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,xmt_dly) $val
}

proc ospfVirtIfRetransInterval {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,rxmt_int) $val
}

proc ospfVirtIfHelloInterval {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,hello_int) $val
}

proc ospfVirtIfRtrDeadInterval {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,dead_int) $val
}

proc ospfVirtIfAuthType {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,auth_type) $val
}

proc ospfVirtIfAuthKey {val} {
    global thisarea thisvl area_att vl_att
    set vl_att($thisarea,$thisvl,auth_key) $val
}

###############################################################
# Neighbor configuration:
#	neighbor _address_ _priority_
# subordinate to interface configuration
###############################################################

proc neighbor {address priority} {
    global thisarea thisifc area_att ifc_att
    global nbr_att
    if {[lsearch $ifc_att($thisarea,$thisifc,nbrs) $address] != -1} {
	return;
    }
    lappend ifc_att($thisarea,$thisifc,nbrs) $address
    set nbr_att($thisarea,$thisifc,$address,pri) $priority
}

###############################################################
# External route configuration:
#	route _prefix_ _nh_ _exttype_ _cost_
# subordinate to area configuration
# subcommands:
#	mcsource	- it's a multicast source
#	tag		- external route tag
###############################################################

proc route {pr nexth type metric} {
    global routes route_att thisrt route_index
    lappend routes $route_index
    set thisrt $route_index
    incr route_index
    set route_att($thisrt,prefix) $pr
    set route_att($thisrt,nh) $nexth
    set route_att($thisrt,exttype) $type
    set route_att($thisrt,cost) $metric
    set route_att($thisrt,mcsrc) 0
    set route_att($thisrt,exttag) 0
}

proc mcsource {} {
    global route_att thisrt
    set route_att($thisrt,mcsrc) 1
}
proc tag {val} {
    global route_att thisrt
    set route_att($thisrt,exttag) $val
}

###############################################################
# Interface authentication configuration:
#	md5key _keyid_ _key_
# subordinate to interface configuration
# subcommands:
#	startaccept _date_
#	startgenerate _date_
#	stopgenerate _date_
#	stopaccept _date_
###############################################################

proc md5key {keyid key} {
    global thisarea thisifc area_att ifc_att
    global md5_att thiskey
    if {[lsearch $ifc_att($thisarea,$thisifc,keys) $keyid] != -1} {
	return;
    }
    lappend ifc_att($thisarea,$thisifc,keys) $keyid
    set thiskey $keyid
    set md5_att($thisarea,$thisifc,$keyid,key) $key
    set md5_att($thisarea,$thisifc,$keyid,startacc) 0
    set md5_att($thisarea,$thisifc,$keyid,startgen) 0
    set md5_att($thisarea,$thisifc,$keyid,stopgen) 0
    set md5_att($thisarea,$thisifc,$keyid,stopacc) 0
}

###############################################################
# Send entire configuration to the
# OSPF application.
###############################################################

proc sendcfg {} {
    global global_att areas area_att ifc_att routes route_att
    global aggr_att host_att vl_att nbr_att md5_att
    sendgen $global_att(lsdb_limit) $global_att(mospf_enabled) \
	    $global_att(inter_area_mc) $global_att(ovfl_int) \
	    $global_att(new_flood_rate) $global_att(max_rxmt_window) \
	    $global_att(max_dds) $global_att(base_level) \
	    $global_att(host) $global_att(refresh_rate) \
	    $global_att(PPAdjLimit) $global_att(random_refresh)
    foreach a $areas {
	sendarea $a $area_att($a,stub) $area_att($a,dflt_cost) \
		$area_att($a,import_summs)
	foreach i $area_att($a,interfaces) {
	    sendifc $i $ifc_att($a,$i,mtu) $ifc_att($a,$i,IfIndex) \
		    $a $ifc_att($a,$i,iftype) \
		    $ifc_att($a,$i,dr_pri) $ifc_att($a,$i,xmt_dly) \
		    $ifc_att($a,$i,rxmt_int) $ifc_att($a,$i,hello_int) \
		    $ifc_att($a,$i,cost) $ifc_att($a,$i,dead_int) \
		    $ifc_att($a,$i,poll_int) $ifc_att($a,$i,auth_type) \
		    $ifc_att($a,$i,auth_key) $ifc_att($a,$i,mc_fwd) \
		    $ifc_att($a,$i,demand) $ifc_att($a,$i,passive) \
		    $ifc_att($a,$i,igmp)
	    foreach nbr $ifc_att($a,$i,nbrs) {
		sendnbr $nbr $nbr_att($a,$i,$nbr,pri)
	    }
	    foreach key $ifc_att($a,$i,keys) {
		sendmd5 $i $key $md5_att($a,$i,$key,key) \
			$md5_att($a,$i,$key,startacc) \
			$md5_att($a,$i,$key,startgen) \
			$md5_att($a,$i,$key,stopgen) \
			$md5_att($a,$i,$key,stopacc)
	    }
	}
	foreach aggr $area_att($a,aggregates) {
	    sendagg $aggr $a $aggr_att($a,$aggr,suppress)
	}
	foreach host $area_att($a,hosts) {
	    sendhost $host $a $host_att($a,$host,cost)
	}
	foreach vl $area_att($a,vls) {
	    sendvl $vl $a $vl_att($a,$vl,xmt_dly) \
		    $vl_att($a,$vl,rxmt_int) $vl_att($a,$vl,hello_int) \
		    $vl_att($a,$vl,dead_int) $vl_att($a,$vl,auth_type) \
		    $vl_att($a,$vl,auth_key)
	}
    }
    foreach r $routes {
	sendextrt $route_att($r,prefix) $route_att($r,nh) \
		$route_att($r,exttype)  $route_att($r,cost) \
		$route_att($r,mcsrc) $route_att($r,exttag)
    }
}

    

