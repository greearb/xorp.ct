## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- No-install Libraries
noinst_LTLIBRARIES = libxorp_io.la

# -- Library Sources
libxorp_io_la_SOURCES		 = io_ip_socket.hh

libxorp_io_la_SOURCES		+= io_ip_socket.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
