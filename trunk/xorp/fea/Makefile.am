## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.10 2003/05/10 00:06:38 pavlin Exp $
##


bin_PROGRAMS		 = fea fea_dummy
bin_PROGRAMS 		+= demo_fea_ifmgr_client
check_PROGRAMS		 = test_rawsock4 test_ifmanager_transaction
TESTS			 = $(check_PROGRAMS)

demo_fea_ifmgr_client_SOURCES  = demo_fea_ifmgr_client.cc
demo_fea_ifmgr_client_LDADD    = $(top_builddir)/xrl/targets/libdemofeaifmgrclientbase.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxipc/libxipc.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libcomm/libcomm.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxorp/libxorp.a

# disabled test_fea

noinst_LIBRARIES	 = libfea.a

# -- Libraries and library flags to link the binary programs

# -- Program Sources

fea_SOURCES		= fea.cc
fea_LDADD		= $(noinst_LIBRARIES)
### FEA XRL libraries
fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
fea_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
### MFEA XRL libraries
fea_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
fea_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
fea_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
fea_LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
fea_LDADD		+= $(top_builddir)/cli/libcli.a
fea_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
fea_LDADD		+= -lcurses
fea_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
fea_LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
fea_LDADD		+= $(top_builddir)/libproto/libproto.a
fea_LDADD		+= $(top_builddir)/libxipc/libxipc.a
fea_LDADD		+= $(top_builddir)/libcomm/libcomm.a
fea_LDADD		+= $(top_builddir)/libxorp/libxorp.a

fea_dummy_SOURCES	= fea_dummy.cc
fea_dummy_LDADD		=  $(noinst_LIBRARIES)
### FEA XRL libraries
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
### MFEA XRL libraries
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
fea_dummy_LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
fea_dummy_LDADD		+= $(top_builddir)/cli/libcli.a
fea_dummy_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
fea_dummy_LDADD		+= -lcurses
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
fea_dummy_LDADD		+= $(top_builddir)/libproto/libproto.a
fea_dummy_LDADD		+= $(top_builddir)/libxipc/libxipc.a
fea_dummy_LDADD		+= $(top_builddir)/libcomm/libcomm.a
fea_dummy_LDADD		+= $(top_builddir)/libxorp/libxorp.a

test_ifmanager_transaction_SOURCES  = test_ifmanager_transaction.cc
test_ifmanager_transaction_SOURCES += iftree.cc
test_ifmanager_transaction_LDADD    = $(top_builddir)/libxorp/libxorp.a

test_rawsock4_SOURCES	 = test_rawsock4.cc rawsock4.cc
test_rawsock4_LDADD	 = $(top_builddir)/libxorp/libxorp.a

# -- Library Sources

libfea_a_SOURCES	 = fea_module.h
libfea_a_SOURCES	+= mfea_module.h
libfea_a_SOURCES	+= fte.hh
libfea_a_SOURCES	+= fti.hh
# libfea_a_SOURCES	+= fti_dummy.hh
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
libfea_a_SOURCES	+= xrl_rawsock4.hh
libfea_a_SOURCES	+= xrl_target.hh

# libfea_a_SOURCES	+= fti_dummy.cc
libfea_a_SOURCES	+= fti_transaction.cc
libfea_a_SOURCES	+= fticonfig.cc
libfea_a_SOURCES	+= fticonfig_entry_get.cc
libfea_a_SOURCES	+= fticonfig_entry_get_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_get_netlink.cc
libfea_a_SOURCES	+= fticonfig_entry_get_rtsock.cc
libfea_a_SOURCES	+= fticonfig_entry_observer.cc
libfea_a_SOURCES	+= fticonfig_entry_observer_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_observer_rtsock.cc
libfea_a_SOURCES	+= fticonfig_entry_parse_nlm.cc
libfea_a_SOURCES	+= fticonfig_entry_parse_rtm.cc
libfea_a_SOURCES	+= fticonfig_entry_set.cc
libfea_a_SOURCES	+= fticonfig_entry_set_dummy.cc
libfea_a_SOURCES	+= fticonfig_entry_set_rtsock.cc
libfea_a_SOURCES	+= fticonfig_table_get.cc
libfea_a_SOURCES	+= fticonfig_table_get_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_get_netlink.cc
libfea_a_SOURCES	+= fticonfig_table_get_sysctl.cc
libfea_a_SOURCES	+= fticonfig_table_observer.cc
libfea_a_SOURCES	+= fticonfig_table_observer_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_observer_rtsock.cc
libfea_a_SOURCES	+= fticonfig_table_parse_nlm.cc
libfea_a_SOURCES	+= fticonfig_table_parse_rtm.cc
libfea_a_SOURCES	+= fticonfig_table_set.cc
libfea_a_SOURCES	+= fticonfig_table_set_dummy.cc
libfea_a_SOURCES	+= fticonfig_table_set_rtsock.cc
libfea_a_SOURCES	+= ifconfig.cc
libfea_a_SOURCES	+= ifconfig_get.cc
libfea_a_SOURCES	+= ifconfig_get_dummy.cc
libfea_a_SOURCES	+= ifconfig_get_getifaddrs.cc
libfea_a_SOURCES	+= ifconfig_get_ioctl.cc
libfea_a_SOURCES	+= ifconfig_get_sysctl.cc
libfea_a_SOURCES	+= ifconfig_observer.cc
libfea_a_SOURCES	+= ifconfig_observer_dummy.cc
libfea_a_SOURCES	+= ifconfig_observer_rtsock.cc
libfea_a_SOURCES	+= ifconfig_parse_ifaddrs.cc
libfea_a_SOURCES	+= ifconfig_parse_ifreq.cc
libfea_a_SOURCES	+= ifconfig_parse_rtm.cc
libfea_a_SOURCES	+= ifconfig_set.cc
libfea_a_SOURCES	+= ifconfig_set_dummy.cc
libfea_a_SOURCES	+= ifconfig_set_ioctl.cc
libfea_a_SOURCES	+= ifmanager_transaction.cc
libfea_a_SOURCES	+= iftree.cc
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
libfea_a_SOURCES	+= xrl_rawsock4.cc
libfea_a_SOURCES	+= xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = test_xrls.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

