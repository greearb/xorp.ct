## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mfea/Makefile.am,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS	 =

# -- Test Programs
bin_PROGRAMS	+= test_mfea

noinst_LIBRARIES = libmfea.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
### MFEA libraries
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.a
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= -lcurses
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_mfea_SOURCES	= test_mfea.cc

# -- Library Sources
libmfea_a_SOURCES  = mfea_module.h
libmfea_a_SOURCES += mfea_dataflow.hh
libmfea_a_SOURCES += mfea_node.hh
libmfea_a_SOURCES += mfea_node_cli.hh
libmfea_a_SOURCES += mfea_private.hh
libmfea_a_SOURCES += mfea_unix_comm.hh
libmfea_a_SOURCES += mfea_unix_kernel_messages.hh
libmfea_a_SOURCES += mfea_unix_osdep.hh
libmfea_a_SOURCES += mfea_vif.hh
libmfea_a_SOURCES += xrl_mfea_node.hh

libmfea_a_SOURCES += mfea_config.cc
libmfea_a_SOURCES += mfea_dataflow.cc
libmfea_a_SOURCES += mfea_node.cc
libmfea_a_SOURCES += mfea_node_cli.cc
libmfea_a_SOURCES += mfea_unix_comm.cc
libmfea_a_SOURCES += mfea_unix_if_getifaddrs.cc
libmfea_a_SOURCES += mfea_unix_if_ioctl.cc
libmfea_a_SOURCES += mfea_unix_if_sysctl.cc
libmfea_a_SOURCES += mfea_unix_mrib_netlink.cc
libmfea_a_SOURCES += mfea_unix_mrib_rawsock.cc
libmfea_a_SOURCES += mfea_unix_rtread_sysctl.cc
libmfea_a_SOURCES += mfea_unix_rtread_netlink.cc
libmfea_a_SOURCES += mfea_vif.cc
libmfea_a_SOURCES += xrl_mfea_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = DEVNOTES
EXTRA_DIST	 += configure_mfea
EXTRA_DIST	 += xrl_mfea_shell_funcs.sh
EXTRA_DIST	 += xrl_shell_lib.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

