## Process this file with automake to produce Makefile.in.
##
## # $XORP: xorp/libxipc/Makefile.am,v 1.6 2003/01/24 02:48:21 hodson Exp $
##

# - The library
noinst_LIBRARIES = libxipc.a

# -- Programs
bin_PROGRAMS  =	call_xrl finder finder_ng

# -- Test Programs - clunkiness is due to automake's inability to handle
#    variables itself
noinst_PROGRAMS  = test_finder_client test_finder_ipc test_finder_ipc2
noinst_PROGRAMS += test_finder_msg test_finder_transport test_header
noinst_PROGRAMS += test_finder_msgs test_finder_tcp test_finder_messenger
noinst_PROGRAMS += test_inproc test_lemming test_sudp test_stcp
noinst_PROGRAMS += test_xrl_error test_xrl test_xrl_atom test_xrl_args 
noinst_PROGRAMS += test_xrl_router test_xrl_parser

TESTS = $(noinst_PROGRAMS) test_xrl_parser.sh

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a

# -- Program Sources
call_xrl_SOURCES		= call_xrl.cc
finder_SOURCES			= finder.cc

finder_ng_SOURCES		 = finder_ng.hh 
finder_ng_SOURCES		+= finder_ng.cc
finder_ng_SOURCES		+= finder_ng_main.cc
finder_ng_SOURCES		+= finder_base.hh
finder_ng_SOURCES		+= finder_base.cc
finder_ng_SOURCES		+= finder_ng_xrl_target.hh
finder_ng_SOURCES		+= finder_ng_xrl_target.cc

test_finder_client_SOURCES	= test_finder_client.cc
test_finder_ipc_SOURCES		= test_finder_ipc.cc
test_finder_ipc2_SOURCES	= test_finder_ipc2.cc
test_finder_messenger_SOURCES	= test_finder_messenger.cc
test_finder_msg_SOURCES		= test_finder_msg.cc
test_finder_msgs_SOURCES	= test_finder_msgs.cc
test_finder_tcp_SOURCES		= test_finder_tcp.cc
test_finder_transport_SOURCES	= test_finder_transport.cc
test_header_SOURCES		= test_header.cc
test_inproc_SOURCES		= test_inproc.cc
test_lemming_SOURCES		= test_lemming.cc
test_stcp_SOURCES		= test_stcp.cc
test_sudp_SOURCES		= test_sudp.cc
test_xrl_SOURCES		= test_xrl.cc
test_xrl_atom_SOURCES		= test_xrl_atom.cc
test_xrl_args_SOURCES		= test_xrl_args.cc
test_xrl_parser_SOURCES		= test_xrl_parser.cc
test_xrl_router_SOURCES		= test_xrl_router.cc
test_xrl_error_SOURCES		= test_xrl_error.cc

# -- Library Sources
libxipc_a_SOURCES  = finder_module.h
libxipc_a_SOURCES += ipc_module.h
libxipc_a_SOURCES += xrl_module.h
libxipc_a_SOURCES += hmac_md5.h
libxipc_a_SOURCES += md5.h
libxipc_a_SOURCES += finder_client.hh
libxipc_a_SOURCES += finder_ipc.hh
libxipc_a_SOURCES += finder_messenger.hh
libxipc_a_SOURCES += finder_msg.hh
libxipc_a_SOURCES += finder_msgs.hh
libxipc_a_SOURCES += finder_server.hh
libxipc_a_SOURCES += finder_tcp_messenger.hh
libxipc_a_SOURCES += finder_transport.hh
libxipc_a_SOURCES += finder_tcp.hh
libxipc_a_SOURCES += header.hh
libxipc_a_SOURCES += hmac.hh
libxipc_a_SOURCES += sockutil.hh
libxipc_a_SOURCES += xrl.hh
libxipc_a_SOURCES += xrl_parser.hh
libxipc_a_SOURCES += xrl_parser_input.hh
libxipc_a_SOURCES += xrl_tokens.hh
libxipc_a_SOURCES += xrl_atom.hh
libxipc_a_SOURCES += xrl_atom_encoding.hh
libxipc_a_SOURCES += xrl_atom_list.hh
libxipc_a_SOURCES += xrl_args.hh
libxipc_a_SOURCES += xrl_cmd_map.hh
libxipc_a_SOURCES += xrl_pf_factory.hh
libxipc_a_SOURCES += xrl_pf_inproc.hh
libxipc_a_SOURCES += xrl_pf_stcp.hh
libxipc_a_SOURCES += xrl_pf_stcp_ph.hh
libxipc_a_SOURCES += xrl_pf_sudp.hh
libxipc_a_SOURCES += xrl_pf.hh
libxipc_a_SOURCES += xrl_router.hh
libxipc_a_SOURCES += xrl_sender.hh
libxipc_a_SOURCES += xrl_std_router.hh
libxipc_a_SOURCES += xuid.hh
libxipc_a_SOURCES += xrl_error.hh

libxipc_a_SOURCES += hmac_md5.c
libxipc_a_SOURCES += md5.c

libxipc_a_SOURCES += finder_client.cc
libxipc_a_SOURCES += finder_ipc.cc
libxipc_a_SOURCES += finder_messenger.cc
libxipc_a_SOURCES += finder_msg.cc
libxipc_a_SOURCES += finder_msgs.cc
libxipc_a_SOURCES += finder_server.cc
libxipc_a_SOURCES += finder_tcp.cc
libxipc_a_SOURCES += finder_tcp_messenger.cc
libxipc_a_SOURCES += finder_transport.cc
libxipc_a_SOURCES += header.cc
libxipc_a_SOURCES += hmac.cc
libxipc_a_SOURCES += sockutil.cc
libxipc_a_SOURCES += xrl.cc
libxipc_a_SOURCES += xrl_parser.cc
libxipc_a_SOURCES += xrl_parser_input.cc
libxipc_a_SOURCES += xrl_tokens.cc
libxipc_a_SOURCES += xrl_atom.cc
libxipc_a_SOURCES += xrl_atom_encoding.cc
libxipc_a_SOURCES += xrl_atom_list.cc
libxipc_a_SOURCES += xrl_args.cc
libxipc_a_SOURCES += xrl_cmd_map.cc
libxipc_a_SOURCES += xrl_pf_factory.cc
libxipc_a_SOURCES += xrl_pf_inproc.cc
libxipc_a_SOURCES += xrl_pf_stcp.cc
libxipc_a_SOURCES += xrl_pf_stcp_ph.cc
libxipc_a_SOURCES += xrl_pf_sudp.cc
libxipc_a_SOURCES += xrl_router.cc
libxipc_a_SOURCES += xrl_std_router.cc
libxipc_a_SOURCES += xrl_error.cc
libxipc_a_SOURCES += xuid.cc

# magic-script:			magic-script.in
# 	sed -e 's/whatever//' < $(srcdir)/magic-script.in > magic-script
# 	chmod +x magic-script


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# -- Generated Sources used in libxipc (see explanation at end of file)
BUILT_SOURCES += common_xif.hh
BUILT_SOURCES += common_xif.cc
BUILT_SOURCES += finder_client_base.hh
BUILT_SOURCES += finder_client_base.cc
BUILT_SOURCES += finder_base.hh
BUILT_SOURCES += finder_base.cc
BUILT_SOURCES += finder_xif.hh
BUILT_SOURCES += finder_xif.cc

CLEANFILES = finder_client_base.hh finder_client_base.cc 		      \
	     finder_xif.hh finder_xif.cc 				      \
             finder_base.hh finder_base.cc

libxipc_a_SOURCES += common_xif.hh
libxipc_a_SOURCES += common_xif.cc
libxipc_a_SOURCES += finder_client_base.hh
libxipc_a_SOURCES += finder_client_base.cc
libxipc_a_SOURCES += finder_base.hh
libxipc_a_SOURCES += finder_base.cc
libxipc_a_SOURCES += finder_xif.hh
libxipc_a_SOURCES += finder_xif.cc

###############################################################################
#
# XXX This is somewhat nasty and very likely source of pain.
# Autogenerated Xrl calling and handling code is used from the Xrl
# build dir.  For all other Xrl senders and targets this happens much
# later in the build process. But, as FinderClient itself uses and
# supports an Xrl Interface we need to auto-generate the appropriate
# files.  They are sym-linked here since we are not using libtool and
# automake (rightly) dislikes cross directory builds.
#

IDIR=$(top_builddir)/xrl/interfaces
TDIR=$(top_builddir)/xrl/targets

common_xif.%: $(IDIR)/common_xif.%
	cd $(IDIR) && $(MAKE) $(notdir $@)
	$(RM) $@
	$(LN_S) $(IDIR)/$@ .

finder_xif.%: $(IDIR)/finder_xif.%
	cd $(IDIR) && $(MAKE) $(notdir $@)
	$(RM) $@
	$(LN_S) $(IDIR)/$@ .

finder_client_base.%: $(TDIR)/finder_client_base.%
	cd $(TDIR) && $(MAKE) $(notdir $@)
	$(RM) $@
	$(LN_S) $(TDIR)/$@ .

finder_base.%: $(TDIR)/finder_base.%
	cd $(TDIR) && $(MAKE) $(notdir $@)
	$(RM) $@
	$(LN_S) $(TDIR)/$@ .









