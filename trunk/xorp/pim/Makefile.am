## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/pim/Makefile.am,v 1.38 2007/05/19 01:52:45 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_pimsm4 xorp_pimsm6

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS	 = 

# -- No-install Libraries
noinst_LTLIBRARIES = libpim.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/xrl/targets/libpimbase.la
### XORP misc libraries
LDADD		+= $(top_builddir)/mrt/libmrt.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_pimsm4_SOURCES  = xorp_pimsm4.cc
xorp_pimsm6_SOURCES  = xorp_pimsm6.cc

# -- Library Sources
libpim_la_SOURCES  = pim_module.h
libpim_la_SOURCES += pim_proto.h

libpim_la_SOURCES += pim_bsr.hh
libpim_la_SOURCES += pim_mfc.hh
libpim_la_SOURCES += pim_mre.hh
libpim_la_SOURCES += pim_mre_task.hh
libpim_la_SOURCES += pim_mre_track_state.hh
libpim_la_SOURCES += pim_mrib_table.hh
libpim_la_SOURCES += pim_mrt.hh
libpim_la_SOURCES += pim_nbr.hh
libpim_la_SOURCES += pim_node.hh
libpim_la_SOURCES += pim_node_cli.hh
libpim_la_SOURCES += pim_proto_assert.hh
libpim_la_SOURCES += pim_proto_join_prune_message.hh
libpim_la_SOURCES += pim_rp.hh
libpim_la_SOURCES += pim_scope_zone_table.hh
libpim_la_SOURCES += pim_vif.hh
libpim_la_SOURCES += xrl_pim_node.hh

libpim_la_SOURCES += pim_bsr.cc
libpim_la_SOURCES += pim_config.cc
libpim_la_SOURCES += pim_mfc.cc
libpim_la_SOURCES += pim_mre.cc
libpim_la_SOURCES += pim_mre_assert.cc
libpim_la_SOURCES += pim_mre_data.cc
libpim_la_SOURCES += pim_mre_join_prune.cc
libpim_la_SOURCES += pim_mre_register.cc
libpim_la_SOURCES += pim_mre_rpf.cc
libpim_la_SOURCES += pim_mre_task.cc
libpim_la_SOURCES += pim_mre_track_state.cc
libpim_la_SOURCES += pim_mrib_table.cc
libpim_la_SOURCES += pim_mrt.cc
libpim_la_SOURCES += pim_mrt_mfc.cc
libpim_la_SOURCES += pim_mrt_task.cc
libpim_la_SOURCES += pim_nbr.cc
libpim_la_SOURCES += pim_node.cc
libpim_la_SOURCES += pim_node_cli.cc
libpim_la_SOURCES += pim_proto_assert.cc
libpim_la_SOURCES += pim_proto_bootstrap.cc
libpim_la_SOURCES += pim_proto_cand_rp_adv.cc
libpim_la_SOURCES += pim_proto_graft.cc
libpim_la_SOURCES += pim_proto_graft_ack.cc
libpim_la_SOURCES += pim_proto_hello.cc
libpim_la_SOURCES += pim_proto_join_prune.cc
libpim_la_SOURCES += pim_proto_join_prune_message.cc
libpim_la_SOURCES += pim_proto_register.cc
libpim_la_SOURCES += pim_proto_register_stop.cc
libpim_la_SOURCES += pim_rp.cc
libpim_la_SOURCES += pim_scope_zone_table.cc
libpim_la_SOURCES += pim_vif.cc
libpim_la_SOURCES += xrl_pim_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = DEVNOTES
EXTRA_DIST	 += command_pim
EXTRA_DIST	 += xrl_pim_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
