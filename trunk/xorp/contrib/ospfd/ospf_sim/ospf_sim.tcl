###############################################################
# OSPFD routing simulator
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

global {node_att}
global {routers}
global {router_att}
global {areas}
global {area_att}
global {port_index}
global {networks}
global {network_att}
global {interface_index}
global {ifc_att}
global {vlink_index}
global {vlink_att}
global {aggregate_index}
global {aggr_att}
global {route_index}
global {route_att}
global {host_index}
global {host_att}
global {neighbor_index}
global {nbr_att}
global {vlinks}
global {sessions}
global {session_ids}
global {random_refresh_flag}

set routers {}
set areas {}
set networks {}
set vlinks {}
set sessions {}
set interface_index 0
set pplink_index 0
set vlink_index 0
set aggregate_index 0
set route_index 0
set host_index 0
set neighbor_index 0
set port_index 0
set session_ids 0
set random_refresh_flag 0

set PING 0
set TRACEROUTE 1
set MTRACE 2
set DEST_UNREACH 3
set TTL_EXPIRED 11

#############################################################
# Commands that can be present in the simulation
# configuration file:
# 	router %rtr_id %x %y %mospf
# 	host %rtr_id %x %y
#	interface %rtr_id %addr %cost %passive %run_ospf
#	broadcast %prefix %area %x %y
#	nbma %prefix %area %x %y
#	ptmp %prefix %area %x %y
#	pplink %rtr_id1 %addr1 %cost1 %rtr_id2 %addr2 %cost2 %area
#	vlink %rtr_id1 %rtr_id2 %area
#       drpri %rtr_id %addr %priority
#	aggr %rtr_id %area %prefix %noadv
#	stub %area %default_cost %import
#	extrt %rtr_id %prefix %nh %etype %cost %noadv
#	loopback %rtr_id %prefix %area
#	neighbor %rtr_id %addr %drpri
#	membership %prefix %group
#	PPAdjLimit %rtr_id %nadj
#
# Almost all of these operations can also be accomplished
# through the GUI.
################################################################

proc router {rtr_id x y mospf} {
    global router_att
    set router_att($rtr_id,host) 0
    set router_att($rtr_id,mospf) $mospf
    set router_att($rtr_id,PPAdjLimit) 0
    router_or_host $rtr_id $x $y
}

proc host {rtr_id x y} {
    global router_att
    set router_att($rtr_id,host) 1
    set router_att($rtr_id,mospf) 0
    set router_att($rtr_id,PPAdjLimit) 0
    router_or_host $rtr_id $x $y
}

proc router_or_host {rtr_id x y} {
    global routers router_att node_att
    if {[lsearch $routers $rtr_id] == -1} {
	set node_att($rtr_id,interfaces) {}
	set router_att($rtr_id,areas) {}
	set router_att($rtr_id,pplinks) {}
	set router_att($rtr_id,vlinks) {}
	set router_att($rtr_id,aggrs) {}
	set router_att($rtr_id,extrts) {}
	set router_att($rtr_id,hosts) {}
	set router_att($rtr_id,nbrs) {}
    }
    set node_att($rtr_id,x) $x
    set node_att($rtr_id,y) $y
    draw_router $rtr_id
    if {[lsearch $routers $rtr_id] == -1} {
	lappend routers $rtr_id
	startrtr $rtr_id
    }
}

###############################################################
# Broadcast, NBMA and Point-to-MultiPoint
# networks are are built from the network command.
# If the area has not yet been defined, define it
# here.
###############################################################

proc network {prefix area x y demand} {
    global networks network_att port_index areas area_att node_att
    if {[lsearch $networks $prefix] != -1} {
	return 0;
    }
    lappend networks $prefix
    incr port_index
    set node_att($prefix,x) $x
    set node_att($prefix,y) $y
    set node_att($prefix,interfaces) {}
    set network_att($prefix,port) $port_index
    set network_att($prefix,type) "broadcast"
    set network_att($prefix,area) $area
    set network_att($prefix,demand) $demand
    set network_att($prefix,groups) {}
    if {[lsearch $areas $area] == -1} {
	lappend areas $area
	set area_att($area,stub) 0
	set area_att($area,default_cost) 1
	set area_att($area,import) 1
    }
    draw_network $prefix
    return -1
}

proc broadcast {prefix area x y demand} {
    global network_att
    if {[network $prefix $area $x $y $demand] != -1} {
	return;
    }
    set network_att($prefix,type) "broadcast"
}

proc nbma {prefix area x y demand} {
    global network_att
    if {[network $prefix $area $x $y $demand] != -1} {
	return;
    }
    set network_att($prefix,type) "nbma"
}

proc ptmp {prefix area x y demand} {
    global network_att
    if {[network $prefix $area $x $y $demand] != -1} {
	return;
    }
    set network_att($prefix,type) "ptmp"
}

###############################################################
# The interface command is used to define attachments
# from a router to a broadcast, NBMA or Point-to-MultiPoint
# network.
# The network must be defined before
# giving this command. The prefix_match command is
# implemented in C++, and does the obvious.
###############################################################

proc interface {rtr_id addr cost passive ospf} {
    global networks router_att interface_index node_att
    global network_att ifc_att
    set net_found 0
    foreach prefix $networks {
	if {[prefix_match $prefix $addr] == 0} {
	    set net_found 1
	    break;
	}
    }
    if {$net_found == 0} {
	put_message "Can't find net for $addr"
	return;
    }
    incr interface_index
    lappend node_att($rtr_id,interfaces) $interface_index
    lappend node_att($prefix,interfaces) $interface_index
    set ifc_att($interface_index,port) $network_att($prefix,port)
    set ifc_att($interface_index,type) $network_att($prefix,type)
    set ifc_att($interface_index,area) $network_att($prefix,area)
    set ifc_att($interface_index,addr) $addr
    set ifc_att($interface_index,prefix) $prefix
    set ifc_att($interface_index,cost) $cost
    set ifc_att($interface_index,demand) $network_att($prefix,demand)
    set ifc_att($interface_index,enabled) 1
    set ifc_att($interface_index,passive) $passive
    set ifc_att($interface_index,drpri) 1
    set ifc_att($interface_index,ospf) $ospf
    set ifc_att($interface_index,rtr) $rtr_id
    set area $network_att($prefix,area)
    if {[lsearch $router_att($rtr_id,areas) $area] == -1} {
	lappend router_att($rtr_id,areas) $area
    }
    draw_interface $interface_index $rtr_id $prefix
    add_mapping $addr $rtr_id
    add_net_membership $network_att($prefix,port) $rtr_id
}

###############################################################
#  Point-to-point link between two routers.
#  If unnumbered, set addr1 and addr2 to 0.0.0.0
###############################################################

proc pplink {rtr_id1 addr1 cost1 rtr_id2 addr2 cost2 area demand} {
    global router_att ifc_att interface_index port_index node_att
    global areas area_att
    if {[lsearch $areas $area] == -1} {
	lappend areas $area
	set area_att($area,stub) 0
	set area_att($area,default_cost) 1
	set area_att($area,import) 1
    }
    incr interface_index
    set index1 $interface_index
    incr port_index
    lappend node_att($rtr_id1,interfaces) $interface_index
    set ifc_att($interface_index,oifc) [expr $interface_index+1]
    set ifc_att($interface_index,ortr) $rtr_id2
    set ifc_att($interface_index,port) $port_index
    set ifc_att($interface_index,type) "pp"
    set ifc_att($interface_index,area) $area
    set ifc_att($interface_index,addr) $addr1
    set ifc_att($interface_index,prefix) "0.0.0.0/0"
    set ifc_att($interface_index,cost) $cost1
    set ifc_att($interface_index,demand) $demand
    set ifc_att($interface_index,enabled) 1
    set ifc_att($interface_index,passive) 0
    set ifc_att($interface_index,drpri) 0
    set ifc_att($interface_index,ospf) 1
    set ifc_att($interface_index,rtr) $rtr_id1
    add_mapping $addr1 $rtr_id1
    add_net_membership $port_index $rtr_id1
    if {[lsearch $router_att($rtr_id1,areas) $area] == -1} {
	lappend router_att($rtr_id1,areas) $area
    }
    incr interface_index
    lappend node_att($rtr_id2,interfaces) $interface_index
    set ifc_att($interface_index,oifc) 0
    set ifc_att($interface_index,ortr) $rtr_id1
    set ifc_att($interface_index,port) $port_index
    set ifc_att($interface_index,type) "pp"
    set ifc_att($interface_index,area) $area
    set ifc_att($interface_index,addr) $addr2
    set ifc_att($interface_index,prefix) "0.0.0.0/0"
    set ifc_att($interface_index,cost) $cost2
    set ifc_att($interface_index,demand) $demand
    set ifc_att($interface_index,enabled) 1
    set ifc_att($interface_index,passive) 0
    set ifc_att($interface_index,drpri) 0
    set ifc_att($interface_index,ospf) 1
    set ifc_att($interface_index,rtr) $rtr_id2
    add_mapping $addr2 $rtr_id2
    add_net_membership $port_index $rtr_id2
    if {[lsearch $router_att($rtr_id2,areas) $area] == -1} {
	lappend router_att($rtr_id2,areas) $area
    }
    draw_pplink $index1 $rtr_id1 $interface_index $rtr_id2
}

###############################################################
#  Virtual link between two routers
#  Specify router ID of two endpoints and the transit
#  area.
###############################################################

proc vlink {rtr_id1 rtr_id2 area} {
    global router_att vlink_index vlink_att vlinks
    incr vlink_index
    lappend router_att($rtr_id1,vlinks) $vlink_index
    lappend vlinks $vlink_index
    set vlink_att($vlink_index,area) $area
    set vlink_att($vlink_index,endpt) $rtr_id2
    set vlink_att($vlink_index,end0) $rtr_id1
    incr vlink_index
    lappend router_att($rtr_id2,vlinks) $vlink_index
    set vlink_att($vlink_index,area) $area
    set vlink_att($vlink_index,endpt) $rtr_id1
}

###############################################################
#  Set DR priority of a given interface, identified
#  by the router's Router ID and the interface's address
###############################################################

proc drpri {rtr_id addr priority} {
    global ifc_att node_att
    foreach i $node_att($rtr_id,interfaces) {
	if {$ifc_att($i,addr) == $addr} {
	    set ifc_att($i,drpri) $priority
	}
    }
}

###############################################################
#  Address aggregation in an area border router
#  Last argument says whether to advertise (noadv=0)
#  or suppress the aggregate,
###############################################################

proc aggr {rtr_id area prefix noadv} {
    global router_att aggregate_index aggr_att
    incr aggregate_index
    lappend router_att($rtr_id,aggrs) $aggregate_index
    set aggr_att($aggregate_index,area) $area
    set aggr_att($aggregate_index,prefix) $prefix
    set aggr_att($aggregate_index,noadv) $noadv
}

###############################################################
# Configure an area as a stub
# Must specify default cost to advertise,
# and whether to import summary-LSAs.
###############################################################

proc stub {area default_cost import} {
    global areas area_att
    if {[lsearch $areas $area] == -1} {
	lappend areas $area
    }
    set area_att($area,stub) 1
    set area_att($area,default_cost) $default_cost
    set area_att($area,import) $import
}

###############################################################
# Add an external route.
# To be advertised by a specific router.
###############################################################

proc extrt {rtr_id prefix nh etype cost noadv} {
    global router_att route_index route_att
    incr route_index
    lappend router_att($rtr_id,extrts) $route_index
    set route_att($route_index,prefix) $prefix
    set route_att($route_index,nh) $nh
    set route_att($route_index,etype) $etype
    set route_att($route_index,cost) $cost
    set route_att($route_index,noadv) $noadv
}

###############################################################
# Add a loopback address. Area that the
# address belongs to is specified, but the
# cost is assumed to be 0.
###############################################################

proc loopback {rtr_id prefix area} {
    global router_att host_index host_att
    incr host_index
    lappend router_att($rtr_id,hosts) $host_index
    set host_att($host_index,prefix) $prefix
    set host_att($host_index,area) $area
    add_mapping $prefix $rtr_id
}

###############################################################
# Add a statically configured neighbor.
# Used on NBMA or non-broadcast interfaces.
# DR priority (really, whether it is non-zero)
# must also be specified.
###############################################################

proc neighbor {rtr_id addr drpri} {
    global router_att neighbor_index nbr_att
    incr neighbor_index
    lappend router_att($rtr_id,nbrs) $neighbor_index
    set nbr_att($neighbor_index,addr) $addr
    set nbr_att($neighbor_index,drpri) $drpri
}

###############################################################
# Add a group member to a particular network segment
###############################################################

proc membership {prefix group} {
    global networks
    global network_att
    if {[lsearch $networks $prefix] != -1} {
	lappend network_att($prefix,groups) $group
	put_message "membership $prefix $group"
    }
}

###############################################################
# Limit the number of p-p adjacencies from this router to
# any of its neighbors
###############################################################

proc PPAdjLimit {rtr_id nadj} {
    global router_att
    set router_att($rtr_id,PPAdjLimit) $nadj
}

###############################################################
# Set *all* simulated routers to randomly refresh.
# By default, this function is disabled.
###############################################################

proc random_refresh {} {
    global random_refresh_flag
    set random_refresh_flag 1
}
###############################################################
# Send entire configuration to a
# given router (i.e., simulated ospfd).
###############################################################

proc sendcfg {rtr_id} {
    global router_att area_att ifc_att vlink_att
    global aggr_att route_att host_att nbr_att node_att
    global networks network_att random_refresh_flag
    sendgen $rtr_id $router_att($rtr_id,host) $router_att($rtr_id,mospf) \
	    $router_att($rtr_id,PPAdjLimit) $random_refresh_flag
    foreach a $router_att($rtr_id,areas) {
	sendarea $rtr_id $a $area_att($a,stub) $area_att($a,default_cost) \
		$area_att($a,import)
    }
    foreach i $node_att($rtr_id,interfaces) {
	sendifc $rtr_id $ifc_att($i,port) $ifc_att($i,type) \
		$ifc_att($i,area) $ifc_att($i,addr) $ifc_att($i,cost) \
		$i $ifc_att($i,prefix) $ifc_att($i,demand) \
		$ifc_att($i,enabled) $ifc_att($i,passive) $ifc_att($i,ospf) \
		$ifc_att($i,drpri)
	set prefix $ifc_att($i,prefix)
	if {[lsearch $networks $prefix] != -1} {
	    foreach group $network_att($prefix,groups) {
		sendgrp $rtr_id $group $ifc_att($i,port)
	    }
	}
    }
    foreach vl $router_att($rtr_id,vlinks) {
	sendvl $rtr_id $vlink_att($vl,area) $vlink_att($vl,endpt)
    }
    foreach nbr $router_att($rtr_id,nbrs) {
	sendnbr $rtr_id $nbr_att($nbr,addr) $nbr_att($nbr,drpri)
    }
    foreach aggr $router_att($rtr_id,aggrs) {
	sendagg $rtr_id $aggr_att($aggr,area) $aggr_att($aggr,prefix) \
		$aggr_att($aggr,noadv)
    }
    foreach host $router_att($rtr_id,hosts) {
	sendhost $rtr_id $host_att($host,prefix) $host_att($host,area)
    }
    foreach r $router_att($rtr_id,extrts) {
	sendextrt $rtr_id $route_att($r,prefix) $route_att($r,nh) \
		$route_att($r,etype) $route_att($r,cost) $route_att($r,noadv)
    }
}


###############################################################
# TCL code for the OSPF simulator
###############################################################

global {node_binding}
global {ifc_binding}

###############################################################
# Draw a new OSPF router
# id is the OSPF Router ID
# label is how it will be named on the map
# x and y will be its coordinates, in pixels
###############################################################

proc draw_router {id} {
    global node_att node_binding
    # draw router
    set x $node_att($id,x)
    set y $node_att($id,y)
    set label $id
    set new [.map create rectangle [expr $x-25] [expr $y-32] \
	    [expr $x+25] [expr $y+32] -fill red \
	    -tags [list node rtr $id rtrbox$id]]
    set node_binding($new) $id
    set new [.map create line [expr $x-25] [expr $y+26] [expr $x+25] \
	    [expr $y+26] -tags [list node rtr $id]]
    set node_binding($new) $id
    set new [.map create line [expr $x-25] [expr $y+20] [expr $x+25] \
	    [expr $y+20] -tags [list node rtr $id]]
    set node_binding($new) $id
    set new [.map create line [expr $x-25] [expr $y+14] [expr $x+25] \
	    [expr $y+14] -tags [list node rtr $id]]
    set node_binding($new) $id
    set new [.map create line [expr $x-25] [expr $y+8] [expr $x+25] \
	    [expr $y+8] -tags [list node rtr $id]]
    set node_binding($new) $id
    set new [.map create rectangle  [expr $x+8] [expr $y-27] [expr $x+20] \
	    [expr $y-15] -fill black -tags [list node rtr $id]]
    set node_binding($new) $id
    set new [.map create text [expr $x] [expr $y-8] -text $label \
	    -anchor center -tags [list node rtr $id]]
    set node_binding($new) $id
}

###############################################################
# Draw a new network
# cidr is its cidr address (e.g., 10/8)
# x and y will be its coordinates, in pixels
###############################################################

proc draw_network {cidr} {
    global node_att node_binding
    set x $node_att($cidr,x)
    set y $node_att($cidr,y)
    set new [.map create oval [expr $x-35] [expr $y-15] \
	    [expr $x+35] [expr $y+15] -fill white \
	    -tags [list node net $cidr]]
    set node_binding($new) $cidr
    set new [.map create text $x $y -text $cidr -anchor center \
	    -tags [list node net $cidr]]
    set node_binding($new) $cidr
}

###############################################################
# Draw an interface between a router and a network
# index is the index into the ifc_att array
# id is the router's OSPF router ID
# cidr is the network's CIDR address
###############################################################

proc draw_interface {index id cidr} {
    global node_att ifc_att
    global ifc_binding
    set new [.map create line $node_att($id,x) $node_att($id,y) \
	    $node_att($cidr,x) $node_att($cidr,y) \
	    -tags [list ifc]]
    .map lower $new
    set ifc_att($index,end1) $id
    set ifc_att($index,end2) $cidr
    set ifc_att($index,mapid) $new
    set ifc_binding($new) $index
}

###############################################################
# Draw a pplink
# interfaces at either end occupy separate
# entries in ifc_att
###############################################################

proc draw_pplink {index1 id1 index2 id2} {
    global node_att ifc_att
    global ifc_binding
    set new [.map create line $node_att($id1,x) $node_att($id1,y) \
	    $node_att($id2,x) $node_att($id2,y) \
	    -tags [list ifc]]
    .map lower $new
    set ifc_att($index1,end1) $id1
    set ifc_att($index1,end2) $id2
    set ifc_att($index1,mapid) $new
    set ifc_att($index2,end1) $id2
    set ifc_att($index2,end2) $id1
    set ifc_att($index2,mapid) $new
    set ifc_binding($new) $index1
}

###############################################################
# Move a router or network on the map
# called internally due to mouse manipulations on the map
# Moves all interfaces attached to the object as well
###############################################################

proc move_node {id xincr yincr} {
    global node_att ifc_att
    .map move $id $xincr $yincr
    set node_att($id,x) [expr $node_att($id,x)+$xincr]
    set node_att($id,y) [expr $node_att($id,y)+$yincr]
    foreach ifc $node_att($id,interfaces) {
	redraw_interface $ifc
    }
}

###############################################################
# Redraw an interface, probably because one
# of its endpoints has moved.
###############################################################

proc redraw_interface {index} {
    global ifc_att node_att
    set mapid $ifc_att($index,mapid)
    set id1 $ifc_att($index,end1)
    set id2 $ifc_att($index,end2)
    if {$ifc_att($index,enabled) == 1} {
	set color "black"
    } else {
	set color "red"
    }
    .map coords $mapid $node_att($id1,x) $node_att($id1,y) \
	    $node_att($id2,x) $node_att($id2,y)
    .map itemconfigure $mapid -fill $color
}

###############################################################
# put_message string
# Place a particular message at the bottom of the window
###############################################################

proc put_message {m} {
    .trailer.message configure -text $m
}

###############################################################
# Show simulated time
# Must be changed if TICKS_PER_SECOND is changed in sim.h
###############################################################

proc show_time {ticks} {
    global sec tenths
    set sec [expr $ticks/20]
    set tenths [expr ($ticks%20)/2]
    .trailer.time configure -text "Time $sec.$tenths"
}

###############################################################
# Select a router using the mouse,
# and then execute the specified command
###############################################################

proc SelectRouter {cmd} {
    global selectrouter_cmd
    put_message "Double left click on router"
    set selectrouter_cmd $cmd
    .map bind rtr <Double-Button-1> {
	global node_binding
	global selectrouter_cmd
	set curNode [.map find withtag current]
	put_message ""
	$selectrouter_cmd $node_binding($curNode)
	.map bind rtr <Double-Button-1> {}
    }
}

###############################################################
# Add a router, from the graphical interface
# Multi-step process:
#	AddRouter - finds place on map
#	add_router_menu - inputs parameters (like Router ID)
#	add_router_complete - finishes up
###############################################################

proc AddRouter {} {
    put_message "left click on router's position"
    bind .map <Button-1> {add_router_menu %x %y}
}

proc add_router_menu {x y} {
    global add_router_x add_router_y add_router_id add_router_mospf
    bind .map <Button-1> {}
    put_message ""
    set add_router_x [.map canvasx $x]
    set add_router_y [.map canvasy $y]
    set w .addrouter
    toplevel $w
    wm title $w "Add Router"

    frame $w.f0
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "Router ID :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable add_router_id
    pack  $w.f0.id_e -side right
    checkbutton $w.mospf -text "mospf" -variable add_router_mospf
    pack $w.mospf -side top
    button $w.f2.ok -text "OK" -command [list add_router_complete $w]
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc add_router_complete {w} {
    global add_router_x add_router_y add_router_id add_router_mospf
    destroy $w
    put_message "router $add_router_id $add_router_x $add_router_y $add_router_mospf"
    router $add_router_id $add_router_x $add_router_y $add_router_mospf
}

###############################################################
# Add a network, from the graphical interface
# Like adding a router, a multi-step process:
#	AddNetwork - finds place on map
#	add_network_menu - inputs parameters (like Router ID)
#	add_network_complete - finishes up
###############################################################

proc AddNetwork {} {
    put_message "left click on network's position"
    bind .map <Button-1> {add_network_menu %x %y}
}

proc add_network_menu {x y} {
    global add_network_x add_network_y add_network_prefix
    global add_network_type add_network_area
    bind .map <Button-1> {}
    put_message ""
    set add_network_x [.map canvasx $x]
    set add_network_y [.map canvasy $y]
    set w .addnetwork
    toplevel $w
    wm title $w "Add Network"

    frame $w.f0
    frame $w.fl
    frame $w.fr
    frame $w.f2
    frame $w.f3
    pack $w.f3 -side bottom -fill x

    pack $w.f0 -fill x
    pack $w.f2 -fill x
    pack $w.fl $w.fr -side left -expand yes
    label $w.f0.id -text "Network Prefix :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable add_network_prefix
    pack  $w.f0.id_e -side right
    label $w.f2.id -text "Area ID :      "
    pack  $w.f2.id -side left
    entry $w.f2.id_e -width 20 -textvariable add_network_area
    pack  $w.f2.id_e -side right
    label $w.fl.type -text "Network type :"
    pack $w.fl.type -side left
    radiobutton $w.fr.broadcast -text "broadcast" -variable add_network_type -value 0
    radiobutton $w.fr.nbma -text "nbma" -variable add_network_type -value 1
    radiobutton $w.fr.ptmp -text "Point-to-MultiPoint" -variable add_network_type -value 2
    pack $w.fr.broadcast $w.fr.nbma $w.fr.ptmp -side top -anchor w
    button $w.f3.ok -text "OK" -command [list add_network_complete $w]
    button $w.f3.quit -text "Quit" -command [list destroy $w]
    pack $w.f3.quit $w.f3.ok -side left -expand 1
}

proc add_network_complete {w} {
    global add_network_x add_network_y add_network_prefix
    global add_network_type add_network_area
    destroy $w
    if {$add_network_type == 0} {
	put_message "broadcast $add_network_prefix $add_network_x $add_network_y 0"
	broadcast $add_network_prefix $add_network_area $add_network_x $add_network_y 0
    }
    if {$add_network_type == 1} {
	put_message "nbma $add_network_prefix $add_network_x $add_network_y 0"
	nbma $add_network_prefix $add_network_area $add_network_x $add_network_y 0
    }
    if {$add_network_type == 2} {
	put_message "ptmp $add_network_prefix $add_network_x $add_network_y 0"
	ptmp $add_network_prefix $add_network_area $add_network_x $add_network_y 0
    }
}

###############################################################
# Add a network interface
# Point-to-point links and virtual links handled elsewhere
# Takes place in the following steps
#	AddInterface - clicks on router
#	add_interface_menu - inputs parameters (address, cost)
#	add_interface_complete - finishes up
###############################################################

proc add_interface_menu {rtr} {
    global add_interface_router add_interface_addr
    global add_interface_cost
    global add_interface_passive add_interface_ospf
    set add_interface_router $rtr
    set w .addinterface
    toplevel $w
    wm title $w "Add Interface"

    label $w.hdr -wraplength 2i -justify left -text "You are adding an interface on router $add_interface_router."
    pack $w.hdr -side top

    frame $w.f0
    frame $w.f1
    frame $w.fb
    pack $w.fb -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "Interface address :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable add_interface_addr
    pack  $w.f0.id_e -side right
    pack $w.f1 -fill x
    label $w.f1.id -text "Interface Cost :      "
    pack  $w.f1.id -side left
    entry $w.f1.id_e -width 20 -textvariable add_interface_cost
    pack  $w.f1.id_e -side right
    set add_interface_ospf 1
    checkbutton $w.passive -text "passive" -variable add_interface_passive
    checkbutton $w.ospf -text "OSPF enabled" -variable add_interface_ospf
    pack $w.passive $w.ospf -side top
    button $w.fb.ok -text "OK" -command [list add_interface_complete $w]
    button $w.fb.quit -text "Quit" -command [list destroy $w]
    pack $w.fb.quit $w.fb.ok -side left -expand 1
}

proc add_interface_complete {w} {
    global add_interface_router add_interface_addr
    global add_interface_cost interface_index ifc_att
    global add_interface_passive add_interface_ospf
    destroy $w
    put_message "interface $add_interface_router $add_interface_addr $add_interface_cost $add_interface_passive $add_interface_ospf"
    interface $add_interface_router $add_interface_addr $add_interface_cost \
	    $add_interface_passive $add_interface_ospf
    set i $interface_index
    sendifc $add_interface_router $ifc_att($i,port) $ifc_att($i,type) \
	    $ifc_att($i,area) $ifc_att($i,addr) $ifc_att($i,cost) \
	    $i $ifc_att($i,prefix) $ifc_att($i,demand) $ifc_att($i,enabled) \
	    $ifc_att($i,passive) $ifc_att($i,ospf) $ifc_att($i,drpri)
}

###############################################################
# Add a point-to-point interface
# Takes place in the following steps
#	AddPP - clicks on router #1
#	add_PP_menu - inputs parameters (address, cost)
#	AddPP - clicks on router #2
#	add_PP_menu - inputs parameters (address, cost)
#	add_PP_complete - finishes up
###############################################################

proc AddPP {rtrno w} {
    global add_pp_rtrno
    set add_pp_rtrno $rtrno
    put_message "Double left click on router no. $rtrno"
    .map bind rtr <Double-Button-1> {
	global node_binding
	set curNode [.map find withtag current]
	add_PP_menu $node_binding($curNode)
    }
    if {$rtrno == 2} {
	destroy $w
    }
}

proc add_PP_menu {id} {
    global add_pp_rtrno add_pp_router1 add_pp_router2
    global add_pp_addr1 add_pp_addr2
    global add_pp_cost1 add_pp_cost2
    global add_pp_area add_pp_demand
    .map bind rtr <Double-Button-1> {}
    put_message ""
    set w .addinterface
    toplevel $w
    wm title $w "Add Point-to-Point interface"

    if {$add_pp_rtrno == 1} {
	set add_pp_router1 $id
	set rtr $add_pp_router1
	set cmd [list AddPP 2 $w]
    } else {
	set add_pp_router2 $id
	set rtr $add_pp_router2
	set cmd [list add_PP_complete $w]
    }
    label $w.hdr -wraplength 2i -justify left -text "You are adding a point-to-point interface on router $rtr."
    pack $w.hdr -side top

    frame $w.f0
    frame $w.f1
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "Interface address :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable add_pp_addr$add_pp_rtrno
    pack  $w.f0.id_e -side right
    pack $w.f1 -fill x
    label $w.f1.id -text "Interface Cost :      "
    pack  $w.f1.id -side left
    entry $w.f1.id_e -width 20 -textvariable add_pp_cost$add_pp_rtrno
    pack  $w.f1.id_e -side right
    if {$add_pp_rtrno == 1} {
	frame $w.f3
	pack $w.f3 -fill x
	label $w.f3.id -text "Area ID :      "
	pack  $w.f3.id -side left
	entry $w.f3.id_e -width 20 -textvariable add_pp_area
	pack  $w.f3.id_e -side right
	checkbutton $w.demand -text "Demand link" -variable add_pp_demand
	pack $w.demand -side top -pady 2 -anchor e
    }
    button $w.f2.ok -text "OK" -command $cmd
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc add_PP_complete {w} {
    global add_pp_router1 add_pp_router2
    global add_pp_addr1 add_pp_addr2
    global add_pp_cost1 add_pp_cost2
    global add_pp_area add_pp_demand
    global interface_index ifc_att
    destroy $w
    put_message "pplink $add_pp_router1 $add_pp_addr1 $add_pp_cost1 $add_pp_router2 $add_pp_addr2 $add_pp_cost2 $add_pp_area $add_pp_demand"
    pplink $add_pp_router1 $add_pp_addr1 $add_pp_cost1 $add_pp_router2 $add_pp_addr2 $add_pp_cost2 $add_pp_area $add_pp_demand
    set i [expr $interface_index-1]
    sendifc $add_pp_router1 $ifc_att($i,port) $ifc_att($i,type) \
	    $ifc_att($i,area) $ifc_att($i,addr) $ifc_att($i,cost) \
	    $i $ifc_att($i,prefix) $ifc_att($i,demand) \
	    $ifc_att($i,enabled) $ifc_att($i,passive) $ifc_att($i,ospf) \
	    $ifc_att($i,drpri)
    incr i
    sendifc $add_pp_router2 $ifc_att($i,port) $ifc_att($i,type) \
	    $ifc_att($i,area) $ifc_att($i,addr) $ifc_att($i,cost) \
	    $i $ifc_att($i,prefix) $ifc_att($i,demand) \
	    $ifc_att($i,enabled) $ifc_att($i,passive) $ifc_att($i,ospf) \
	    $ifc_att($i,drpri)
}

###############################################################
# Add a static route to a simulated router.
###############################################################

proc add_static_menu {rtr} {
    global add_static_router
    global add_static_prefix add_static_nexth add_static_cost
    global add_static_type2 add_static_noadv
    set add_static_router $rtr
    set w .addstatic
    toplevel $w
    wm title $w "Add Static Route"

    label $w.hdr -wraplength 2i -justify left -text "You are adding a static route on router $add_static_router."
    pack $w.hdr -side top

    frame $w.f0
    frame $w.f1
    frame $w.f2
    frame $w.fb
    pack $w.fb -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "IP Prefix :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable add_static_prefix
    pack  $w.f0.id_e -side right
    pack $w.f1 -fill x
    label $w.f1.id -text "Next Hop Address :      "
    pack  $w.f1.id -side left
    entry $w.f1.id_e -width 20 -textvariable add_static_nexth
    pack  $w.f1.id_e -side right
    pack $w.f2 -fill x
    label $w.f2.id -text "Cost :      "
    pack  $w.f2.id -side left
    entry $w.f2.id_e -width 20 -textvariable add_static_cost
    pack  $w.f2.id_e -side right
    set add_static_type2 1
    checkbutton $w.type2 -text "Type 2 metric" -variable add_static_type2
    checkbutton $w.noadv -text "Don't advertise" -variable add_static_noadv
    pack $w.type2 $w.noadv -side top
    button $w.fb.ok -text "OK" -command [list add_static_complete $w]
    button $w.fb.quit -text "Quit" -command [list destroy $w]
    pack $w.fb.quit $w.fb.ok -side left -expand 1
}

proc add_static_complete {w} {
    global add_static_router
    global add_static_prefix add_static_nexth add_static_cost
    global add_static_type2 add_static_noadv
    global route_index route_att
    destroy $w
    set type2 [expr $add_static_type2+1]
    put_message "extrt $add_static_router $add_static_prefix $add_static_nexth $type2 $add_static_cost $add_static_noadv"
    extrt $add_static_router $add_static_prefix $add_static_nexth $type2 $add_static_cost $add_static_noadv
    set r $route_index
    sendextrt $add_static_router $route_att($r,prefix) $route_att($r,nh) \
	    $route_att($r,etype) $route_att($r,cost) $route_att($r,noadv)

}

###############################################################
# Toggle the operational status of an interface
# Mostly done in C++, here we just have to have
# the user click on the router
###############################################################

proc ToggleInterface {} {
    put_message "Double left click on interface/link"
    .map bind ifc <Double-Button-1> {
	global ifc_binding
	set curIfc [.map find withtag current]
	set i $ifc_binding($curIfc)
	if {$ifc_att($i,enabled) == 0} {
	    set ifc_att($i,enabled) 1
	} else {
	    set ifc_att($i,enabled) 0
	}
	redraw_interface $i
	sendifc $ifc_att($i,rtr) $ifc_att($i,port) $ifc_att($i,type) \
		$ifc_att($i,area) $ifc_att($i,addr) $ifc_att($i,cost) \
		$i $ifc_att($i,prefix) $ifc_att($i,demand) \
		$ifc_att($i,enabled) $ifc_att($i,passive) $ifc_att($i,ospf) \
		$ifc_att($i,drpri)
	put_message ""
    }
}

###############################################################
# Start a ping/traceroute session from a particular router
# Query for the destination and TTL
###############################################################

proc ping {rtrid} {
    global session_type session_command
    global PING
    set session_type $PING
    set session_command "ping"
    start_session $rtrid
}

proc tr {rtrid} {
    global session_type session_command
    global TRACEROUTE
    set session_type $TRACEROUTE
    set session_command "traceroute"
    start_session $rtrid
}

proc start_session {rtrid} {
    global session_src session_src_addr session_dest session_ttl
    global session_type session_command
    bind .map <Button-1> {}
    put_message ""
    set session_src $rtrid
    set session_src_addr "0.0.0.0"
    set session_ttl "64"
    set w .session_menu
    toplevel $w
    wm title $w "Start $session_command on $rtrid"

    frame $w.f0
    frame $w.f1
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    if {$session_command == "ping"} {
	frame $w.f3
	pack $w.f3 -fill x
	label $w.f3.id -text "Source :      "
	pack  $w.f3.id -side left
	entry $w.f3.id_e -width 20 -textvariable session_src_addr
	pack  $w.f3.id_e -side right
    }
    pack $w.f0 -fill x
    label $w.f0.id -text "Destination :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable session_dest
    pack  $w.f0.id_e -side right
    pack $w.f1 -fill x
    label $w.f1.id -text "Maximum TTL :      "
    pack  $w.f1.id -side left
    entry $w.f1.id_e -width 3 -textvariable session_ttl
    pack  $w.f1.id_e -side right
    button $w.f2.ok -text "OK" -command [list ${session_command}_menu_complete $w]
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc ping_menu_complete {w} {
    global session_src session_src_addr session_dest session_ttl
    global session_ids session_stats
    global sec
    session_menu_complete $w
    set session_stats($session_ids,src) $session_src
    set session_stats($session_ids,dest) $session_dest
    set session_stats($session_ids,start) $sec
    set session_stats($session_ids,rcvd) 0
    put_message "start_ping $session_src $session_src_addr $session_dest $session_ttl $session_ids"
    start_ping $session_src $session_src_addr $session_dest $session_ttl $session_ids
    print_session $session_ids "% ping -s $session_src_addr $session_dest \n"
}

proc traceroute_menu_complete {w} {
    global session_src session_src_addr session_dest session_ttl session_ids
    session_menu_complete $w
    put_message "start_traceroute $session_src $session_src_addr $session_dest $session_ttl $session_ids"
    start_traceroute $session_src $session_src_addr $session_dest $session_ttl $session_ids
    print_session $session_ids "% traceroute $session_dest \n"
}

###############################################################
# Multicast traceroute. After selecting the router
# to run the mtrace, click on the destination network and then
# input the source, group, and TTL
###############################################################

proc mtrace {rtrid} {
    global session_src
    global session_type session_command
    global MTRACE
    set session_type $MTRACE
    set session_command "mtrace"
    set session_src $rtrid
    put_message "Double left click on destination network"
    .map bind net <Double-Button-1> {
	global node_binding
	set curNode [.map find withtag current]
	start_mtrace_session $node_binding($curNode)
	put_message ""
	.map bind net <Double-Button-1> {}
    }
}

proc start_mtrace_session {cidr} {
    global session_src session_src_addr session_dest
    global session_type session_command mtrace_group mtrace_cidr
    bind .map <Button-1> {}
    put_message ""
    set mtrace_cidr $cidr
    set w .session_menu
    toplevel $w
    wm title $w "Start mtrace on $session_src"

    frame $w.f0
    frame $w.f1
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    frame $w.f3
    pack $w.f3 -fill x
    label $w.f3.id -text "Source :      "
    pack  $w.f3.id -side left
    entry $w.f3.id_e -width 20 -textvariable session_src_addr
    pack  $w.f3.id_e -side right
    pack $w.f0 -fill x
    label $w.f0.id -text "Multicast Group :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable mtrace_group
    pack  $w.f0.id_e -side right
    button $w.f2.ok -text "OK" -command [list mtrace_menu_complete $w]
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc mtrace_menu_complete {w} {
    session_menu_complete $w
    global session_src session_src_addr session_ids
    global session_type session_command mtrace_group mtrace_cidr
    global network_att
    put_message "start_mtrace $session_src $session_src_addr $mtrace_cidr $mtrace_group $session_ids $network_att($mtrace_cidr,port)"
    start_mtrace $session_src $session_src_addr $mtrace_cidr $mtrace_group $session_ids $network_att($mtrace_cidr,port)
    print_session $session_ids "% mtrace $session_src_addr $mtrace_cidr $mtrace_group \n"
}

proc session_menu_complete {w} {
    global session_src session_src_addr session_dest session_ttl
    global session_ids
    global sessions session_attr
    global session_type session_command session_group
    destroy $w
    incr session_ids
    lappend sessions $session_ids
    set session_attr($session_ids,type) $session_type

    set w .session$session_ids
    toplevel $w
    wm title $w "Router $session_src"
    frame $w.f1
    frame $w.f2
    text $w.f1.text -relief raised -bd 2 -yscrollcommand "$w.f1.scroll set"
    scrollbar $w.f1.scroll -command "$w.f1.text yview"
    button $w.f2.stop -text "Stop" -command [list sess_summ $session_ids]
    pack $w.f1 -side top -expand 1 -fill both
    pack $w.f2 -side bottom -fill x
    pack $w.f1.scroll -side right -fill y
    pack $w.f1.text -side left
    pack $w.f2.stop -side right
}

###############################################################
# Summarize the ping session, and then stop it
###############################################################

proc ping_reply {s_id src seq ttl milli} {
    global session_stats session_attr PING
    if {$session_attr($s_id,type) != $PING} {
	return
    }
    incr session_stats($s_id,rcvd)
    print_session $s_id "Echo Reply from $src, icmp_seq=$seq, ttl=$ttl, time=$milli ms\n"
}

###############################################################
# Summarize the ping session, and then stop it
###############################################################

proc sess_summ {s_id} {
    global sessions session_stats session_attr
    global sec
    global PING
    if {$session_attr($s_id,type) != $PING} {
	return
    }
    print_session $s_id "\n"
    print_session $s_id "---- $session_stats($s_id,dest) ping statistics ----\n"
    set x [expr $sec-$session_stats($s_id,start)]
    print_session $s_id "Duration: $x seconds, Responses received: $session_stats($s_id,rcvd) \n"
    set i [lsearch $sessions $s_id]
    set sessions [lreplace $sessions $i $i]
    stop_ping $session_stats($s_id,src) $s_id
}

###############################################################
# Process ICMP errors received for a particular session
# Could be for a ping, or for a traceroute.
###############################################################

proc icmp_error {s_id src type code milli} {
    global sessions session_attr
    global PING DEST_UNREACH TTL_EXPIRED TRACEROUTE
    if {[lsearch $sessions $s_id] == -1} {
	return;
    }
    if {$session_attr($s_id,type) == $PING} {
	if {$type == $DEST_UNREACH} {
	    print_session $s_id "Destination unreachable from $src \n"
	}
	if {$type == $TTL_EXPIRED} {
	    print_session $s_id "TTL expired from $src \n"
	}
    }
    if {$session_attr($s_id,type) == $TRACEROUTE} {
	if {$session_attr($s_id,response_from) != $src} {
	    set session_attr($s_id,response_from) $src
	    print_session $s_id " $src"
	}
	print_session $s_id " $milli ms"
	if {$type == $DEST_UNREACH} {
	    print_session $s_id " !H"
	}
    }
}

##############################################################
# New set of probes are being sent for a traceroute.
# Display the TTL, and clear the response address so that
# a new one will be displayed.
##############################################################

proc traceroute_ttl {s_id ttl} {
    global sessions session_attr
    if {[lsearch $sessions $s_id] == -1} {
	return;
    }
    set session_attr($s_id,response_from) "0.0.0.0"
    set msg [format "\n%2d " $ttl]
    print_session $s_id $msg
}

###############################################################
# Log ping/traceroute stats into a scrolling text window
###############################################################

proc print_session {s_id str} {
    global sessions
    if {[lsearch $sessions $s_id] == -1} {
	return;
    }
    set w .session$s_id
    $w.f1.text insert end "$str"
    $w.f1.text yview moveto 1.0
}

###############################################################
# Select a router using the mouse,
# and then execute the specified command
###############################################################

proc Group {join} {
    global group_cmd
    put_message "Double left click on network"
    set group_cmd $join
    .map bind net <Double-Button-1> {
	global node_binding
	set curNode [.map find withtag current]
	GroupMenu $node_binding($curNode)
	put_message ""
	.map bind net <Double-Button-1> {}
    }
}

proc GroupMenu {cidr} {
    global group_cmd group_addr source_net
    set w .group_menu
    toplevel $w
    set source_net $cidr
    if {$group_cmd == 0} {
	wm title $w "Join group on $cidr"
    } else {
	wm title $w "Leave group on $cidr"
    }
    frame $w.f0
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "Group :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable group_addr
    pack  $w.f0.id_e -side right
    button $w.f2.ok -text "OK" -command [list group_menu_complete $w]
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc group_menu_complete {w} {
    global group_cmd group_addr source_net
    global network_att node_att ifc_att
    destroy $w
    set prefix $source_net
    if {$group_cmd == 0} {
	if {[lsearch $network_att($prefix,groups) $group_addr] != -1} {
	    return
	}
	lappend network_att($prefix,groups) $group_addr
	set command "sendgrp"
    } else {
	if {[lsearch $network_att($prefix,groups) $group_addr] == -1} {
	    return
	}
	set j [lsearch $network_att($prefix,groups) $group_addr]
	set network_att($prefix,groups) [lreplace $network_att($prefix,groups) $j $j]
	set command "leavegrp"
    }
    foreach i $node_att($prefix,interfaces) {
	set rtr_id $ifc_att($i,rtr)
	$command $rtr_id $group_addr $ifc_att($i,port)
    }
}

###############################################################
# Color a router a particular color
###############################################################

proc color_router {id color} {
    .map itemconfigure rtrbox$id -fill $color
}

###############################################################
# Quit the simulation
###############################################################

proc ExitSim {} {
    destroy .
}

###############################################################
# Store the name of the config file
###############################################################

proc ConfigFile {file} {
    global config_file
    set config_file $file
    wm title . "OSPF Simulator: $file"
    .mbar.file entryconfigure "Save" -state normal
}

###############################################################
# Save the current map to a new file of the user's choosing
###############################################################

proc SaveMaptoFile {} {
    global config_file new_file
    set w .saveas
    toplevel $w
    wm title $w "Save As"

    label $w.hdr -wraplength 2i -justify left -text "Specify the new config file in which to save the map. This file name will also be used in all future save commands."
    pack $w.hdr -side top

    frame $w.f0
    frame $w.f2
    pack $w.f2 -side bottom -fill x

    pack $w.f0 -fill x
    label $w.f0.id -text "New config file :      "
    pack  $w.f0.id -side left
    entry $w.f0.id_e -width 20 -textvariable new_file
    pack  $w.f0.id_e -side right

    button $w.f2.ok -text "OK" -command [list SaveAsDone $w]
    button $w.f2.quit -text "Quit" -command [list destroy $w]
    pack $w.f2.quit $w.f2.ok -side left -expand 1
}

proc SaveAsDone {w} {
    global new_file
    destroy $w
    ConfigFile $new_file
    SaveMap
}

###############################################################
# Save the current map to the same file that the user originally
# opened, or if specified, the file that the map was saved to
# This should mirror the logic in sendcfg{}
###############################################################

proc SaveMap {} {
    global config_file
    global routers networks
    global router_att area_att ifc_att vlink_att vlinks
    global aggr_att route_att host_att nbr_att node_att
    global network_att aggr_att route_att nbr_att
    global host_att random_refresh_flag

    set f [open $config_file w]
    if {$random_refresh_flag != 0} {
	puts $f "random_refresh"
    }
    foreach id $routers {
	if {$router_att($id,host) == 0} {
	    puts $f [concat "router" $id $node_att($id,x) $node_att($id,y) \
		    $router_att($id,mospf)]
	} else {
	    puts $f [concat "host" $id $node_att($id,x) $node_att($id,y) \
		    $router_att($id,mospf)]
	}
	if {$router_att($id,PPAdjLimit) != 0} {
	    puts $f [concat "PPAdjLimit" $id $router_att($id,PPAdjLimit)]
	}
    }
    foreach net $networks {
	if {$network_att($net,type) == "broadcast"} {
	    puts $f [concat "broadcast" $net $network_att($net,area) $node_att($net,x) \
		    $node_att($net,y) $network_att($net,demand)]
	}
	if {$network_att($net,type) == "nbma"} {
	    puts $f [concat "nbma" $net $network_att($net,area) $node_att($net,x) \
		    $node_att($net,y) $network_att($net,demand)]
	}
	if {$network_att($net,type) == "ptmp"} {
	    puts $f [concat "ptmp" $net $network_att($net,area) $node_att($net,x) \
		    $node_att($net,y) $network_att($net,demand)]
	}
	foreach group $network_att($net,groups) {
	    puts $f [concat "membership" $net $group]
	}
    }
    foreach id $routers {
	foreach ifc $node_att($id,interfaces) {
	    if {$ifc_att($ifc,type) != "pp"} {
		puts $f [concat "interface" $id $ifc_att($ifc,addr) \
			$ifc_att($ifc,cost) $ifc_att($ifc,passive) \
			$ifc_att($ifc,ospf)] 
		if {$ifc_att($ifc,drpri) != 1} {
		    puts $f [concat "drpri" $id $ifc_att($ifc,addr) \
			    $ifc_att($ifc,drpri)]
		}
	    } else {
		if {$ifc_att($ifc,oifc) != 0} {
		    set oid $ifc_att($ifc,ortr)
		    set oifc $ifc_att($ifc,oifc)
		    puts $f [concat "pplink" $id $ifc_att($ifc,addr) \
			    $ifc_att($ifc,cost) $oid $ifc_att($oifc,addr) \
			    $ifc_att($oifc,cost) $ifc_att($ifc,area) \
			    $ifc_att($ifc,demand)]
		}
	    }
	}
	foreach n $router_att($id,nbrs) {
	    puts $f [concat "neighbor" $id $nbr_att($n,addr) \
		    $nbr_att($n,drpri)]
	}
	foreach agg $router_att($id,aggrs) {
	    puts $f [concat "aggr" $id $aggr_att($agg,area) \
		    $aggr_att($agg,prefix) $aggr_att($agg,noadv)]
	}
	foreach r $router_att($id,extrts) {
	    puts $f [concat "extrt" $id $route_att($r,prefix) \
		    $route_att($r,nh) $route_att($r,etype) \
		    $route_att($r,cost) $route_att($r,noadv)]
	}
	foreach h $router_att($id,hosts) {
	    puts $f [concat loopback $id $host_att($h,prefix) \
		    $host_att($h,area)]
	}
    }
    foreach vl $vlinks {
	puts $f [concat vlink $vlink_att($vl,end0) $vlink_att($vl,endpt) \
		$vlink_att($vl,area)]
    }
    close $f
}

# Add a virtual link, from the graphical interface

proc AddVL {} {
}

# main routine
# create the map and all the bindings
wm title . "OSPF Simulator: *scratch*"
wm iconname . "ospf"

menu .mbar -tearoff 0
menu .mbar.file -tearoff 0
.mbar add cascade -label "File" -underline 0 -menu .mbar.file
.mbar.file add command -label "Save" -command "SaveMap" -state disabled
.mbar.file add command -label "Save As" -command "SaveMaptoFile"
.mbar.file add command -label "Exit" -command "ExitSim" -underline 0
menu .mbar.add -tearoff 0
.mbar add cascade -label "Add" -underline 0 -menu .mbar.add
.mbar.add add command -label "Router" -command "AddRouter"
.mbar.add add command -label "Network" -command "AddNetwork"
.mbar.add add command -label "Interface" -command [list SelectRouter add_interface_menu]
.mbar.add add command -label "PPlink" -command [list AddPP 1 -1]
#.mbar.add add command -label "Neighbor" -command "AddNeighbor"
.mbar.add add command -label "Virtual Link" -command "AddVL"
.mbar.add add command -label "Static Route" -command [list SelectRouter add_static_menu]
menu .mbar.toggle -tearoff 0
.mbar add cascade -label "Router" -underline 0 -menu .mbar.toggle
.mbar.toggle add command -label "Toggle" -command [list SelectRouter togglertr]
.mbar.toggle add command -label "Restart" -command [list SelectRouter rstrtr]
.mbar.toggle add command -label "Hitless Restart" -command [list SelectRouter hitlessrtr]
.mbar.toggle add command -label "Toggle Interface" -command "ToggleInterface"
menu .mbar.sim
.mbar add cascade -label "Simulation" -underline 0 -menu .mbar.sim
.mbar.sim add command -label "Resume" -command "time_resume"
.mbar.sim add command -label "Stop" -command "time_stop" -underline 2
menu .mbar.utilities -tearoff 0
.mbar add cascade -label "Utilities" -underline 0 -menu .mbar.utilities
.mbar.utilities add command -label "Ping" -command [list SelectRouter ping]
.mbar.utilities add command -label "Traceroute" -command [list SelectRouter tr]
.mbar.utilities add command -label "MTrace" -command [list SelectRouter mtrace]
.mbar.utilities add command -label "Join Group" -command [list Group 0]
.mbar.utilities add command -label "Leave Group" -command [list Group 1]
. configure -menu .mbar

# then the map
frame .mapgrid
scrollbar .hscroll -orient horiz -command ".map xview"
scrollbar .vscroll -command ".map yview"
canvas .map -height 350 -width 600 -scrollregion {0 0 2000 2000} \
	-xscrollcommand ".hscroll set" \
	-yscrollcommand ".vscroll set"

pack .mapgrid -side top -expand 1 -fill both
grid rowconfig .mapgrid 0 -weight 1 -minsize 0
grid columnconfig .mapgrid 0 -weight 1 -minsize 0
grid .map -padx 1 -in .mapgrid -pady 1 \
	-row 0 -column 0  -rowspan 1 -columnspan 1 -sticky news
grid .hscroll -padx 1 -in .mapgrid -pady 1 \
	-row 1 -column 0  -rowspan 1 -columnspan 1 -sticky news
grid .vscroll -padx 1 -in .mapgrid -pady 1 \
	-row 0 -column 1  -rowspan 1 -columnspan 1 -sticky news

# then the messages, and time, at the bottom
frame .trailer -relief raised -bd 2
pack .trailer -fill x
message .trailer.message -width 16c -justify left -text "Welcome to the OSPF Simulator!"
pack .trailer.message -fill x -side left
label .trailer.time -text "Time 0.0"
pack .trailer.time -side right

# button bindings

.map bind node <Button-1> {
    set curX %x
    set curY %y
}

.map bind node <B1-Motion> {
    global node_binding
    set curNode [.map find withtag current]
    set id $node_binding($curNode)
    move_node $id [expr %x-$curX] [expr %y-$curY]
    set curX %x
    set curY %y
}


    

