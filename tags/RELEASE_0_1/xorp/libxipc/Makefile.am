## Process this file with automake to produce Makefile.in.
##
## # $XORP: xorp/libxipc/Makefile.am,v 1.37 2002/12/06 00:41:41 hodson Exp $
##

# - The library
noinst_LIBRARIES = libxipc.a

# -- Programs
bin_PROGRAMS  =	call_xrl finder

# -- Test Programs - clunkiness is due to automake's inability to handle
#    variables itself
noinst_PROGRAMS  = test_finder_client test_finder_ipc test_finder_ipc2
noinst_PROGRAMS += test_finder_msg test_finder_transport test_header
noinst_PROGRAMS += test_inproc test_lemming test_sudp test_stcp
noinst_PROGRAMS += test_xrlerror test_xrl test_xrlatom test_xrlargs 
noinst_PROGRAMS += test_xrlrouter test_xrlparser

TESTS = $(noinst_PROGRAMS) test_xrlparser.sh

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
call_xrl_SOURCES		= call_xrl.cc
finder_SOURCES			= finder.cc

test_finder_client_SOURCES	= test_finder_client.cc
test_finder_ipc_SOURCES		= test_finder_ipc.cc
test_finder_ipc2_SOURCES	= test_finder_ipc2.cc
test_finder_msg_SOURCES		= test_finder_msg.cc
test_finder_transport_SOURCES	= test_finder_transport.cc
test_header_SOURCES		= test_header.cc
test_inproc_SOURCES		= test_inproc.cc
test_lemming_SOURCES		= test_lemming.cc
test_stcp_SOURCES		= test_stcp.cc
test_sudp_SOURCES		= test_sudp.cc
test_xrl_SOURCES		= test_xrl.cc
test_xrlatom_SOURCES		= test_xrlatom.cc
test_xrlargs_SOURCES		= test_xrlargs.cc
test_xrlparser_SOURCES		= test_xrlparser.cc
test_xrlrouter_SOURCES		= test_xrlrouter.cc
test_xrlerror_SOURCES		= test_xrlerror.cc

# -- Library Sources
libxipc_a_SOURCES  = finder_module.h
libxipc_a_SOURCES += ipc_module.h
libxipc_a_SOURCES += xrl_module.h
libxipc_a_SOURCES += hmac_md5.h
libxipc_a_SOURCES += md5.h
libxipc_a_SOURCES += finder-client.hh
libxipc_a_SOURCES += finder-ipc.hh
libxipc_a_SOURCES += finder-msg.hh
libxipc_a_SOURCES += finder-server.hh
libxipc_a_SOURCES += finder-transport.hh
libxipc_a_SOURCES += header.hh
libxipc_a_SOURCES += hmac.hh
libxipc_a_SOURCES += sockutil.hh
libxipc_a_SOURCES += xrl.hh
libxipc_a_SOURCES += xrl-parser.hh
libxipc_a_SOURCES += xrl-parser-input.hh
libxipc_a_SOURCES += xrl-tokens.hh
libxipc_a_SOURCES += xrlatom.hh
libxipc_a_SOURCES += xrlatom-encoding.hh
libxipc_a_SOURCES += xrlatom-list.hh
libxipc_a_SOURCES += xrlargs.hh
libxipc_a_SOURCES += xrlcmdmap.hh
libxipc_a_SOURCES += xrlpf-factory.hh
libxipc_a_SOURCES += xrlpf-inproc.hh
libxipc_a_SOURCES += xrlpf-stcp.hh
libxipc_a_SOURCES += xrlpf-stcp-ph.hh
libxipc_a_SOURCES += xrlpf-sudp.hh
libxipc_a_SOURCES += xrlpf.hh
libxipc_a_SOURCES += xrlrouter.hh
libxipc_a_SOURCES += xrlstdrouter.hh
libxipc_a_SOURCES += xuid.hh
libxipc_a_SOURCES += xrlerror.hh

libxipc_a_SOURCES += hmac_md5.c
libxipc_a_SOURCES += md5.c

libxipc_a_SOURCES += finder-client.cc
libxipc_a_SOURCES += finder-ipc.cc
libxipc_a_SOURCES += finder-msg.cc
libxipc_a_SOURCES += finder-server.cc
libxipc_a_SOURCES += finder-transport.cc
libxipc_a_SOURCES += header.cc
libxipc_a_SOURCES += hmac.cc
libxipc_a_SOURCES += sockutil.cc
libxipc_a_SOURCES += xrl.cc
libxipc_a_SOURCES += xrl-parser.cc
libxipc_a_SOURCES += xrl-parser-input.cc
libxipc_a_SOURCES += xrl-tokens.cc
libxipc_a_SOURCES += xrlatom.cc
libxipc_a_SOURCES += xrlatom-encoding.cc
libxipc_a_SOURCES += xrlatom-list.cc
libxipc_a_SOURCES += xrlargs.cc
libxipc_a_SOURCES += xrlcmdmap.cc
libxipc_a_SOURCES += xrlpf-factory.cc
libxipc_a_SOURCES += xrlpf-inproc.cc
libxipc_a_SOURCES += xrlpf-stcp.cc
libxipc_a_SOURCES += xrlpf-stcp-ph.cc
libxipc_a_SOURCES += xrlpf-sudp.cc
libxipc_a_SOURCES += xrlrouter.cc
libxipc_a_SOURCES += xrlstdrouter.cc
libxipc_a_SOURCES += xrlerror.cc
libxipc_a_SOURCES += xuid.cc

# magic-script:			magic-script.in
# 	sed -e 's/whatever//' < $(srcdir)/magic-script.in > magic-script
# 	chmod +x magic-script


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##










