## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.25 2003/10/14 02:17:11 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS		 = xorp_fea xorp_fea_dummy

# -- Test Programs
TESTS			 = test_rawsock4 test_ifmanager_transaction

# -- No-install Programs
noinst_PROGRAMS 	 = demo_fea_ifmgr_client
noinst_PROGRAMS 	+= test_mfea
noinst_PROGRAMS		+= test_rawsock4
noinst_PROGRAMS		+= test_ifmanager_transaction

# -- No-install Libraries
noinst_LIBRARIES	 = libfea.a

# -- Sources and Linkage for xorp_fea
xorp_fea_SOURCES	= fea.cc
xorp_fea_LDADD		= $(noinst_LIBRARIES)
### FEA XRL libraries
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
### MFEA XRL libraries
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
xorp_fea_LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
xorp_fea_LDADD		+= $(top_builddir)/cli/libcli.a
xorp_fea_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
xorp_fea_LDADD		+= @CLI_CURSES_LIB@
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### libfeaclient libraries
xorp_fea_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.a
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
### XORP misc libraries
xorp_fea_LDADD		+= $(top_builddir)/libproto/libproto.a
xorp_fea_LDADD		+= $(top_builddir)/libxipc/libxipc.a
xorp_fea_LDADD		+= $(top_builddir)/libcomm/libcomm.a
xorp_fea_LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Sources and Linkage for xorp_fea_dummy
xorp_fea_dummy_SOURCES	= fea_dummy.cc
xorp_fea_dummy_LDADD	=  $(noinst_LIBRARIES)
### FEA XRL libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/targets/libfeabase.a
### MFEA XRL libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/cli/libcli.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/cli/libtecla/libtecla.a
xorp_fea_dummy_LDADD	+= @CLI_CURSES_LIB@
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/targets/libclibase.a
### libfeaclient libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/libfeaclient/libfeaclient.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
### XORP misc libraries
xorp_fea_dummy_LDADD	+= $(top_builddir)/libproto/libproto.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/libxipc/libxipc.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/libcomm/libcomm.a
xorp_fea_dummy_LDADD	+= $(top_builddir)/libxorp/libxorp.a

# -- Sources and Linkage for test_mfea
test_mfea_SOURCES	= test_mfea.cc
test_mfea_LDADD		= $(noinst_LIBRARIES)
# FEA-specific begin
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
# FEA-specific end
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
#test_mfea_LDADD	+= $(top_builddir)/xrl/interfaces/libcommonxif.a
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
test_mfea_LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
test_mfea_LDADD		+= $(top_builddir)/cli/libcli.a
test_mfea_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
test_mfea_LDADD		+= @CLI_CURSES_LIB@
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
test_mfea_LDADD		+= $(top_builddir)/libproto/libproto.a
test_mfea_LDADD		+= $(top_builddir)/libxipc/libfinder.a
test_mfea_LDADD		+= $(top_builddir)/libxipc/libxipc.a
test_mfea_LDADD		+= $(top_builddir)/libcomm/libcomm.a
test_mfea_LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Sources and Linkage for test_ifmanager_transaction
test_ifmanager_transaction_SOURCES  = test_ifmanager_transaction.cc
test_ifmanager_transaction_SOURCES += iftree.cc
test_ifmanager_transaction_LDADD    = $(top_builddir)/libxorp/libxorp.a

# -- Sources and Linkages for test_rawsock4
test_rawsock4_SOURCES	 = test_rawsock4.cc rawsock4.cc
test_rawsock4_LDADD	 = $(top_builddir)/libxorp/libxorp.a

# -- Sources and Linkage for demo_fea_ifmgr_client
demo_fea_ifmgr_client_SOURCES  = demo_fea_ifmgr_client.cc
demo_fea_ifmgr_client_LDADD    = $(top_builddir)/xrl/targets/libdemofeaifmgrclientbase.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxipc/libxipc.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libcomm/libcomm.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxorp/libxorp.a

# -- Library Sources
libfea_a_SOURCES	 = fea_module.h
libfea_a_SOURCES	+= mfea_module.h
libfea_a_SOURCES	+= fte.hh
libfea_a_SOURCES	+= fti.hh
libfea_a_SOURCES	+= fti_transaction.hh
libfea_a_SOURCES	+= fticonfig.hh
libfea_a_SOURCES	+= fticonfig_entry_get.hh
libfea_a_SOURCES	+= fticonfig_entry_observer.hh
libfea_a_SOURCES	+= fticonfig_entry_set.hh
libfea_a_SOURCES	+= fticonfig_table_get.hh
libfea_a_SOURCES	+= fticonfig_table_observer.hh
libfea_a_SOURCES	+= fticonfig_table_set.hh
libfea_a_SOURCES	+= ifconfig.hh
libfea_a_SOURCES	+= ifconfig_get.hh
libfea_a_SOURCES	+= ifconfig_observer.hh
libfea_a_SOURCES	+= ifconfig_set.hh
libfea_a_SOURCES	+= ifmanager.hh
libfea_a_SOURCES	+= ifmanager_transaction.hh
libfea_a_SOURCES	+= iftree.hh
libfea_a_SOURCES	+= kernel_utils.hh
libfea_a_SOURCES	+= libfeaclient_bridge.hh
libfea_a_SOURCES	+= mfea_dataflow.hh
libfea_a_SOURCES	+= mfea_kernel_messages.hh
libfea_a_SOURCES	+= mfea_mrouter.hh
libfea_a_SOURCES	+= mfea_node.hh
libfea_a_SOURCES	+= mfea_node_cli.hh
libfea_a_SOURCES	+= mfea_osdep.hh
libfea_a_SOURCES	+= mfea_proto_comm.hh
libfea_a_SOURCES	+= mfea_vif.hh
libfea_a_SOURCES	+= netlink_socket.hh
libfea_a_SOURCES	+= netlink_socket_utils.hh
libfea_a_SOURCES	+= rawsock4.hh
libfea_a_SOURCES	+= routing_socket.hh
libfea_a_SOURCES	+= routing_socket_utils.hh
libfea_a_SOURCES	+= xrl_fti.hh
libfea_a_SOURCES	+= xrl_ifmanager.hh
libfea_a_SOURCES	+= xrl_ifupdate.hh
libfea_a_SOURCES	+= xrl_mfea_node.hh
libfea_a_SOURCES	+= xrl_mfea_vif_manager.hh
libfea_a_SOURCES	+= xrl_rawsock4.hh
libfea_a_SOURCES	+= xrl_target.hh

libfea_a_SOURCES	+= fti_transaction.cc
libfea_a_SOURCES	+= fticonfig.cc
libfea_a_SOURCES	+= fticonfig_entry_get.cc
libfea_a_SOURCES	+= fticonfig_entry_get_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_get_netlink.cc
libfea_a_SOURCES	+= fticonfig_entry_get_rtsock.cc
libfea_a_SOURCES	+= fticonfig_entry_observer.cc
libfea_a_SOURCES	+= fticonfig_entry_observer_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_observer_netlink.cc
libfea_a_SOURCES	+= fticonfig_entry_observer_rtsock.cc
libfea_a_SOURCES	+= fticonfig_entry_parse_nlm.cc
libfea_a_SOURCES	+= fticonfig_entry_parse_rtm.cc
libfea_a_SOURCES	+= fticonfig_entry_set.cc
libfea_a_SOURCES	+= fticonfig_entry_set_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_set_netlink.cc
libfea_a_SOURCES	+= fticonfig_entry_set_rtsock.cc
libfea_a_SOURCES	+= fticonfig_table_get.cc
libfea_a_SOURCES	+= fticonfig_table_get_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_get_netlink.cc
libfea_a_SOURCES	+= fticonfig_table_get_sysctl.cc
libfea_a_SOURCES	+= fticonfig_table_observer.cc
libfea_a_SOURCES	+= fticonfig_table_observer_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_observer_netlink.cc
libfea_a_SOURCES	+= fticonfig_table_observer_rtsock.cc
libfea_a_SOURCES	+= fticonfig_table_parse_nlm.cc
libfea_a_SOURCES	+= fticonfig_table_parse_rtm.cc
libfea_a_SOURCES	+= fticonfig_table_set.cc
libfea_a_SOURCES	+= fticonfig_table_set_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_set_netlink.cc
libfea_a_SOURCES	+= fticonfig_table_set_rtsock.cc
libfea_a_SOURCES	+= ifconfig.cc
libfea_a_SOURCES	+= ifconfig_get.cc
libfea_a_SOURCES	+= ifconfig_get_dummy.cc
libfea_a_SOURCES	+= ifconfig_get_getifaddrs.cc
libfea_a_SOURCES	+= ifconfig_get_ioctl.cc
libfea_a_SOURCES	+= ifconfig_get_netlink.cc
libfea_a_SOURCES	+= ifconfig_get_proc_linux.cc
libfea_a_SOURCES	+= ifconfig_get_sysctl.cc
libfea_a_SOURCES	+= ifconfig_observer.cc
libfea_a_SOURCES	+= ifconfig_observer_dummy.cc
libfea_a_SOURCES	+= ifconfig_observer_netlink.cc
libfea_a_SOURCES	+= ifconfig_observer_rtsock.cc
libfea_a_SOURCES	+= ifconfig_parse_ifaddrs.cc
libfea_a_SOURCES	+= ifconfig_parse_ifreq.cc
libfea_a_SOURCES	+= ifconfig_parse_nlm.cc
libfea_a_SOURCES	+= ifconfig_parse_rtm.cc
libfea_a_SOURCES	+= ifconfig_set.cc
libfea_a_SOURCES	+= ifconfig_set_dummy.cc
libfea_a_SOURCES	+= ifconfig_set_ioctl.cc
libfea_a_SOURCES	+= ifconfig_set_netlink.cc
libfea_a_SOURCES	+= ifmanager_transaction.cc
libfea_a_SOURCES	+= iftree.cc
libfea_a_SOURCES	+= libfeaclient_bridge.cc
libfea_a_SOURCES	+= mfea_config.cc
libfea_a_SOURCES	+= mfea_dataflow.cc
libfea_a_SOURCES	+= mfea_mrouter.cc
libfea_a_SOURCES	+= mfea_node.cc
libfea_a_SOURCES	+= mfea_node_cli.cc
libfea_a_SOURCES	+= mfea_proto_comm.cc
libfea_a_SOURCES	+= mfea_vif.cc
libfea_a_SOURCES	+= netlink_socket.cc
libfea_a_SOURCES	+= netlink_socket_utils.cc
libfea_a_SOURCES	+= rawsock4.cc
libfea_a_SOURCES	+= routing_socket.cc
libfea_a_SOURCES	+= routing_socket_utils.cc
libfea_a_SOURCES	+= xrl_fti.cc
libfea_a_SOURCES	+= xrl_ifmanager.cc
libfea_a_SOURCES	+= xrl_ifupdate.cc
libfea_a_SOURCES	+= xrl_mfea_node.cc
libfea_a_SOURCES	+= xrl_mfea_vif_manager.cc
libfea_a_SOURCES	+= xrl_rawsock4.cc
libfea_a_SOURCES	+= xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = DEVNOTES.mfea
EXTRA_DIST	+= command_mfea
EXTRA_DIST	+= configure_mfea
EXTRA_DIST	+= test_xrls.sh
EXTRA_DIST	+= xrl_mfea_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

