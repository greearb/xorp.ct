## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/data_plane/ifconfig/Makefile.am,v 1.6 2007/12/28 05:12:37 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Libraries
lib_LTLIBRARIES = libxorp_ifconfig.la

# -- Library Sources
libxorp_ifconfig_la_SOURCES	 = ifconfig_get_click.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_getifaddrs.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_ioctl.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_iphelper.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_netlink_socket.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_proc_linux.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_sysctl.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_media.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_iphelper.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_netlink_socket.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_routing_socket.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_bsd.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_linux.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_solaris.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_windows.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_click.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_ioctl.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_iphelper.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_netlink_socket.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_bsd.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_linux.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_bsd.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_dummy.hh
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_linux.hh

libxorp_ifconfig_la_SOURCES	+= ifconfig_get_click.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_getifaddrs.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_proc_linux.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_sysctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_media.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_routing_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_getifaddrs.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_routing_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_bsd.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_linux.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_solaris.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_property_windows.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_click.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_bsd.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_get_linux.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_bsd.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_vlan_set_linux.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
