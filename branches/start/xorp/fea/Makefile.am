## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.49 2002/12/09 09:43:55 pavlin Exp $
##

if FEA_BUILD_RTSOCK
# Absence of whitespace here is due to automake's inability to parse
# whitespace at this point...
host_programs	 	 = fea_rtsock

fea_rtsock_SOURCES	 = fea_rtsock.cc
fea_rtsock_LDADD	 = ./libfea.a
fea_rtsock_LDADD	+= $(top_builddir)/xrl/targets/libfeabase.a
fea_rtsock_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
fea_rtsock_LDADD	+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
fea_rtsock_LDADD	+= $(top_builddir)/libxipc/libxipc.a
fea_rtsock_LDADD	+= $(top_builddir)/libxorp/libxorp.a

host_test_programs	 = test_ifconfig_rtsock

test_ifconfig_rtsock_SOURCES	 = test_ifconfig_rtsock.cc
test_ifconfig_rtsock_LDADD	 = ./libfea.a
test_ifconfig_rtsock_LDADD	+= $(top_builddir)/libxorp/libxorp.a

host_lib_sources	 = fti_rtsock.hh
host_lib_sources	+= fti_rtsock.cc
host_lib_sources	+= ifconfig_rtsock.hh
host_lib_sources	+= ifconfig_rtsock.cc
host_lib_sources	+= rtsock.hh
host_lib_sources	+= rtsock.cc
FEA_BINARY		 = fea_rtsock
else
host_programs 		 = 
host_test_programs 	 =
host_lib_sources 	 =
FEA_BINARY		 = fea_dummy
endif

bin_PROGRAMS		 = fea fea_dummy
bin_PROGRAMS 		+= $(host_programs) demo_fea_ifmgr_client
check_PROGRAMS		+= test_rawsock4 test_ifmanager_transaction
check_PROGRAMS		+= $(host_test_programs)
TESTS			 = $(check_PROGRAMS)

demo_fea_ifmgr_client_SOURCES += demo_fea_ifmgr_client.cc
demo_fea_ifmgr_client_LDADD    = $(top_builddir)/xrl/targets/libdemofeaifmgrclientbase.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxipc/libxipc.a
demo_fea_ifmgr_client_LDADD   += $(top_builddir)/libxorp/libxorp.a

# disabled test_fea

noinst_LIBRARIES	 = libfea.a

# -- Libraries and library flags to link the binary programs
LDFLAGS			 = -all-static

# -- Program Sources

fea_dummy_SOURCES	 = fea_dummy.cc
fea_dummy_LDADD		 = ./libfea.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
fea_dummy_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
fea_dummy_LDADD		+= $(top_builddir)/libxipc/libxipc.a
fea_dummy_LDADD		+= $(top_builddir)/libxorp/libxorp.a

test_ifmanager_transaction_SOURCES  = test_ifmanager_transaction.cc
test_ifmanager_transaction_SOURCES += iftree.cc
test_ifmanager_transaction_LDADD    = $(top_builddir)/libxorp/libxorp.a

test_rawsock4_SOURCES	 = test_rawsock4.cc rawsock4.cc
test_rawsock4_LDADD	+= $(top_builddir)/libxorp/libxorp.a

# -- Library Sources

libfea_a_SOURCES	 = fea_module.h
libfea_a_SOURCES	+= fti.hh
libfea_a_SOURCES	+= fti_dummy.hh
libfea_a_SOURCES	+= fti_transaction.hh
libfea_a_SOURCES	+= ifconfig.hh
libfea_a_SOURCES	+= ifconfig_dummy.hh
libfea_a_SOURCES	+= ifmanager.hh
libfea_a_SOURCES	+= ifmanager_transaction.hh
libfea_a_SOURCES	+= iftree.hh
libfea_a_SOURCES	+= rawsock4.hh
libfea_a_SOURCES	+= xrl_fti.hh
libfea_a_SOURCES	+= xrl_ifmanager.hh
libfea_a_SOURCES	+= xrl_ifupdate.hh
libfea_a_SOURCES	+= xrl_rawsock4.hh
libfea_a_SOURCES	+= xrl_target.hh

libfea_a_SOURCES	+= fti_dummy.cc
libfea_a_SOURCES	+= fti_transaction.cc
libfea_a_SOURCES	+= ifconfig.cc
libfea_a_SOURCES	+= ifconfig_dummy.cc
libfea_a_SOURCES	+= ifmanager_transaction.cc
libfea_a_SOURCES	+= iftree.cc
libfea_a_SOURCES	+= rawsock4.cc
libfea_a_SOURCES	+= xrl_fti.cc
libfea_a_SOURCES	+= xrl_ifmanager.cc
libfea_a_SOURCES	+= xrl_ifupdate.cc
libfea_a_SOURCES	+= xrl_rawsock4.cc
libfea_a_SOURCES	+= xrl_target.cc
libfea_a_SOURCES 	+= $(host_lib_sources)

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = test_xrls.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

fea: 
	$(LN_S) $(FEA_BINARY) $@
