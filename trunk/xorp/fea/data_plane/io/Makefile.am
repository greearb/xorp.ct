## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/data_plane/io/Makefile.am,v 1.4 2007/08/09 00:47:00 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Libraries
lib_LTLIBRARIES = libxorp_io.la

# -- Library Sources
libxorp_io_la_SOURCES		 = io_ip_dummy.hh
libxorp_io_la_SOURCES		+= io_ip_socket.hh
libxorp_io_la_SOURCES		+= io_link_dummy.hh
libxorp_io_la_SOURCES		+= io_link_pcap.hh
libxorp_io_la_SOURCES		+= io_tcpudp_dummy.hh
libxorp_io_la_SOURCES		+= io_tcpudp_socket.hh

libxorp_io_la_SOURCES		+= io_ip_dummy.cc
libxorp_io_la_SOURCES		+= io_ip_socket.cc
libxorp_io_la_SOURCES		+= io_link_dummy.cc
libxorp_io_la_SOURCES		+= io_link_pcap.cc
libxorp_io_la_SOURCES		+= io_tcpudp_dummy.cc
libxorp_io_la_SOURCES		+= io_tcpudp_socket.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
