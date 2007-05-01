## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/forwarding_plane/control_socket/Makefile.am,v 1.1 2007/05/01 02:33:32 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- No-install Libraries
noinst_LTLIBRARIES = libxorp_control_socket.la

# -- Library Sources
libxorp_control_socket_la_SOURCES	 = windows_routing_socket.h

libxorp_control_socket_la_SOURCES	+= click_socket.hh
libxorp_control_socket_la_SOURCES	+= netlink_socket.hh
libxorp_control_socket_la_SOURCES	+= netlink_socket_utilities.hh
libxorp_control_socket_la_SOURCES	+= routing_socket.hh
libxorp_control_socket_la_SOURCES	+= routing_socket_utilities.hh
libxorp_control_socket_la_SOURCES	+= system_utilities.hh
libxorp_control_socket_la_SOURCES	+= windows_rras_support.hh
libxorp_control_socket_la_SOURCES	+= windows_rtm_pipe.hh

libxorp_control_socket_la_SOURCES	+= click_socket.cc
libxorp_control_socket_la_SOURCES	+= netlink_socket.cc
libxorp_control_socket_la_SOURCES	+= netlink_socket_utilities.cc
libxorp_control_socket_la_SOURCES	+= routing_socket.cc
libxorp_control_socket_la_SOURCES	+= routing_socket_utilities.cc
libxorp_control_socket_la_SOURCES	+= windows_rras_support.cc
libxorp_control_socket_la_SOURCES	+= windows_rtm_pipe.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
