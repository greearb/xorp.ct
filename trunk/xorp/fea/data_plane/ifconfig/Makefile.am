## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- No-install Libraries
noinst_LTLIBRARIES = libxorp_ifconfig.la

# -- Library Sources
libxorp_ifconfig_la_SOURCES	 = ifconfig_get_click.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_getifaddrs.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_proc_linux.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_get_sysctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_observer_routing_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_getifaddrs.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_netlink_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_parse_routing_socket.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_click.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_dummy.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_ioctl.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_iphelper.cc
libxorp_ifconfig_la_SOURCES	+= ifconfig_set_netlink_socket.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
