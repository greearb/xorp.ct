## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/data_plane/managers/Makefile.am,v 1.1 2007/07/11 22:18:17 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Libraries
lib_LTLIBRARIES = libxorp_data_plane_managers.la

# -- Library Sources
libxorp_data_plane_managers_la_SOURCES	 = fea_data_plane_manager_bsd.hh
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_click.hh
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_dummy.hh
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_linux.hh
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_windows.hh

libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_bsd.cc
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_click.cc
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_dummy.cc
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_linux.cc
libxorp_data_plane_managers_la_SOURCES	+= fea_data_plane_manager_windows.cc


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
