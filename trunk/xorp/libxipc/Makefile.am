## Process this file with automake to produce Makefile.in.
##
## # $XORP: xorp/libxipc/Makefile.am,v 1.38 2004/11/12 02:44:42 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# Redefine the sbin installation directory
# XXX: the trailing "bin" is not a mistake, but a hack to install
# "call_xrl" into a different directory. Sigh...
sbindir = $(prefix)/bin

# -- Real Programs
bin_PROGRAMS	= xorp_finder
sbin_PROGRAMS	= call_xrl

# -- No-install Libtool Libraries
noinst_LTLIBRARIES = libxipc.la libfinder.la

# -- Test Programs and scripts (order of increasing complexity)
test_programs  = test_xrl_error test_xrl test_xrl_atom test_xrl_args
test_programs += test_header test_inproc test_sudp test_stcp
test_programs += test_lemming test_xrl_router
test_programs += test_finder_tcp test_finder_messenger test_finder_msgs
test_programs += test_finder test_finder_events

test_extra_programs  = test_xrl_parser test_finder_to
test_extra_programs += test_xrl_receiver test_xrl_sender

test_scripts = test_xrl_parser.sh test_finder_deaths.sh test_leaks.sh

TESTS = $(test_programs) $(test_scripts)

# -- No-install Programs
check_PROGRAMS = $(test_programs) $(test_extra_programs)
check_SCRIPTS = $(test_scripts)

# -- Libraries and library flags to link the binary programs
LDADD		 = ./libfinder.la
LDADD		+= ./libxipc.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la

xorp_finder_LDADD		 = libfinder.la
xorp_finder_LDADD		+= ./libxipc.la
xorp_finder_LDADD		+= $(top_builddir)/libxorp/libxorp.la
xorp_finder_LDADD		+= $(top_builddir)/libcomm/libcomm.la

# -- Program Sources
call_xrl_SOURCES		 = call_xrl.cc
xorp_finder_SOURCES		 = finder_main.cc

test_finder_messenger_SOURCES	 = test_finder_messenger.cc
test_finder_msgs_SOURCES	 = test_finder_msgs.cc
test_finder_tcp_SOURCES		 = test_finder_tcp.cc

test_header_SOURCES		 = test_header.cc
test_inproc_SOURCES		 = test_inproc.cc
test_lemming_SOURCES		 = test_lemming.cc
test_stcp_SOURCES		 = test_stcp.cc
test_sudp_SOURCES		 = test_sudp.cc
test_xrl_SOURCES		 = test_xrl.cc
test_xrl_atom_SOURCES		 = test_xrl_atom.cc
test_xrl_args_SOURCES		 = test_xrl_args.cc
test_xrl_parser_SOURCES		 = test_xrl_parser.cc
test_xrl_error_SOURCES		 = test_xrl_error.cc

test_xrl_router_SOURCES		 = test_xrl_router.cc
test_xrl_router_LDADD		 = libfinder.la $(LDADD)

test_finder_SOURCES		 = test_finder.cc
test_finder_LDADD		 = libfinder.la $(LDADD)

test_finder_events_SOURCES	 = test_finder_events.cc
test_finder_events_SOURCES	+= test_finder_events_base.hh
test_finder_events_SOURCES	+= test_finder_events_base.cc
test_finder_events_SOURCES	+= finder_event_notifier_xif.hh
test_finder_events_SOURCES	+= finder_event_notifier_xif.cc
test_finder_events_LDADD	 = libfinder.la $(LDADD)

test_finder_to_SOURCES		 = test_finder_to.cc
test_finder_to_LDADD		 = libfinder.la $(LDADD)

test_xrl_receiver_SOURCES	 = test_xrl_receiver.cc
test_xrl_receiver_SOURCES	+= test_xrls_base.hh
test_xrl_receiver_SOURCES	+= test_xrls_base.cc

test_xrl_sender_SOURCES		 = test_xrl_sender.cc
test_xrl_sender_SOURCES		+= test_xrls_xif.hh
test_xrl_sender_SOURCES		+= test_xrls_xif.cc
# XXX: extra source files if both sender and receiver are within same binary
test_xrl_sender_SOURCES		+= test_xrls_base.hh
test_xrl_sender_SOURCES		+= test_xrls_base.cc

# -- Library Sources (libxipc)
libxipc_la_SOURCES  = finder_module.h
libxipc_la_SOURCES += hmac_md5.h
libxipc_la_SOURCES += ipc_module.h
libxipc_la_SOURCES += xrl_module.h
libxipc_la_SOURCES += finder_client.hh
libxipc_la_SOURCES += finder_client_observer.hh
libxipc_la_SOURCES += finder_client_xrl_target.hh
libxipc_la_SOURCES += finder_constants.hh
libxipc_la_SOURCES += finder_ipc.hh
libxipc_la_SOURCES += finder_messenger.hh
libxipc_la_SOURCES += finder_msgs.hh
libxipc_la_SOURCES += finder_tcp.hh
libxipc_la_SOURCES += finder_tcp_messenger.hh
libxipc_la_SOURCES += header.hh
libxipc_la_SOURCES += hmac.hh
libxipc_la_SOURCES += permits.hh
libxipc_la_SOURCES += sockutil.hh
libxipc_la_SOURCES += xrl.hh
libxipc_la_SOURCES += xrl_args.hh
libxipc_la_SOURCES += xrl_atom.hh
libxipc_la_SOURCES += xrl_atom_encoding.hh
libxipc_la_SOURCES += xrl_atom_list.hh
libxipc_la_SOURCES += xrl_cmd_map.hh
libxipc_la_SOURCES += xrl_dispatcher.hh
libxipc_la_SOURCES += xrl_error.hh
libxipc_la_SOURCES += xrl_parser.hh
libxipc_la_SOURCES += xrl_parser_input.hh
libxipc_la_SOURCES += xrl_pf.hh
libxipc_la_SOURCES += xrl_pf_factory.hh
libxipc_la_SOURCES += xrl_pf_inproc.hh
libxipc_la_SOURCES += xrl_pf_kill.hh
libxipc_la_SOURCES += xrl_pf_stcp.hh
libxipc_la_SOURCES += xrl_pf_stcp_ph.hh
libxipc_la_SOURCES += xrl_pf_sudp.hh
libxipc_la_SOURCES += xrl_router.hh
libxipc_la_SOURCES += xrl_sender.hh
libxipc_la_SOURCES += xrl_std_router.hh
libxipc_la_SOURCES += xrl_tokens.hh
libxipc_la_SOURCES += xuid.hh

libxipc_la_SOURCES += hmac_md5.c

libxipc_la_SOURCES += finder_client.cc
libxipc_la_SOURCES += finder_client_observer.cc
libxipc_la_SOURCES += finder_client_xrl_target.cc
libxipc_la_SOURCES += finder_messenger.cc
libxipc_la_SOURCES += finder_msgs.cc
libxipc_la_SOURCES += finder_tcp.cc
libxipc_la_SOURCES += finder_tcp_messenger.cc
libxipc_la_SOURCES += header.cc
libxipc_la_SOURCES += hmac.cc
libxipc_la_SOURCES += permits.cc
libxipc_la_SOURCES += sockutil.cc
libxipc_la_SOURCES += xrl.cc
libxipc_la_SOURCES += xrl_args.cc
libxipc_la_SOURCES += xrl_atom.cc
libxipc_la_SOURCES += xrl_atom_encoding.cc
libxipc_la_SOURCES += xrl_atom_list.cc
libxipc_la_SOURCES += xrl_cmd_map.cc
libxipc_la_SOURCES += xrl_dispatcher.cc
libxipc_la_SOURCES += xrl_error.cc
libxipc_la_SOURCES += xrl_parser.cc
libxipc_la_SOURCES += xrl_parser_input.cc
libxipc_la_SOURCES += xrl_pf.cc
libxipc_la_SOURCES += xrl_pf_factory.cc
libxipc_la_SOURCES += xrl_pf_inproc.cc
libxipc_la_SOURCES += xrl_pf_kill.cc
libxipc_la_SOURCES += xrl_pf_stcp.cc
libxipc_la_SOURCES += xrl_pf_stcp_ph.cc
libxipc_la_SOURCES += xrl_pf_sudp.cc
libxipc_la_SOURCES += xrl_router.cc
libxipc_la_SOURCES += xrl_std_router.cc
libxipc_la_SOURCES += xrl_tokens.cc
libxipc_la_SOURCES += xuid.cc

# -- Library Sources (libxipc - built sources)
libxipc_la_SOURCES += common_xif.hh
libxipc_la_SOURCES += common_xif.cc
libxipc_la_SOURCES += finder_client_base.hh
libxipc_la_SOURCES += finder_client_base.cc
libxipc_la_SOURCES += finder_xif.hh
libxipc_la_SOURCES += finder_xif.cc

# -- Library Sources (libfinder)
libfinder_la_SOURCES  = finder.hh
libfinder_la_SOURCES += finder_constants.hh
libfinder_la_SOURCES += finder_server.hh
libfinder_la_SOURCES += finder_xrl_queue.hh
libfinder_la_SOURCES += finder_xrl_target.hh

libfinder_la_SOURCES += finder.cc
libfinder_la_SOURCES += finder_server.cc
libfinder_la_SOURCES += finder_xrl_queue.cc
libfinder_la_SOURCES += finder_xrl_target.cc

# -- Library Sources (libfinder - built sources)
libfinder_la_SOURCES += finder_base.hh
libfinder_la_SOURCES += finder_base.cc
libfinder_la_SOURCES += finder_client_xif.hh
libfinder_la_SOURCES += finder_client_xif.cc
libfinder_la_SOURCES += finder_event_observer_xif.hh
libfinder_la_SOURCES += finder_event_observer_xif.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = bench_ipc.awk
EXTRA_DIST	+= bench_ipc.sh
EXTRA_DIST	+= bench_ipc_gnuplot.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

# -- Generated Sources used in libxipc and libfinder
# (see explanation at end of file)
BUILT_SOURCES  = common_xif.hh
BUILT_SOURCES += common_xif.cc
BUILT_SOURCES += finder_base.hh
BUILT_SOURCES += finder_base.cc
BUILT_SOURCES += finder_client_base.hh
BUILT_SOURCES += finder_client_base.cc
BUILT_SOURCES += finder_client_xif.hh
BUILT_SOURCES += finder_client_xif.cc
BUILT_SOURCES += finder_event_notifier_xif.hh
BUILT_SOURCES += finder_event_notifier_xif.cc
BUILT_SOURCES += finder_event_observer_xif.hh
BUILT_SOURCES += finder_event_observer_xif.cc
BUILT_SOURCES += finder_xif.hh
BUILT_SOURCES += finder_xif.cc
BUILT_SOURCES += test_finder_events_base.hh
BUILT_SOURCES += test_finder_events_base.cc
BUILT_SOURCES += test_xrls_base.hh
BUILT_SOURCES += test_xrls_base.cc
BUILT_SOURCES += test_xrls_xif.hh
BUILT_SOURCES += test_xrls_xif.cc

CLEANFILES = $(BUILT_SOURCES)


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

###############################################################################
#
# Build interfaces
#
###############################################################################

# Basic build rule for Xrl Target interface files
%_xif.o: %_xif.cc
	$(CXXCOMPILE) -c $<

common_xif.%: $(top_srcdir)/xrl/interfaces/common_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/common_xif.hh				\
$(top_srcdir)/xrl/interfaces/common_xif.cc:				\
    $(top_srcdir)/xrl/interfaces/common.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

finder_xif.%: $(top_srcdir)/xrl/interfaces/finder_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/finder_xif.hh				\
$(top_srcdir)/xrl/interfaces/finder_xif.cc:				\
    $(top_srcdir)/xrl/interfaces/finder.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

finder_client_xif.%: $(top_srcdir)/xrl/interfaces/finder_client_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/finder_client_xif.hh			\
$(top_srcdir)/xrl/interfaces/finder_client_xif.cc:			\
    $(top_srcdir)/xrl/interfaces/finder_client.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

finder_event_notifier_xif.%:						\
    $(top_srcdir)/xrl/interfaces/finder_event_notifier_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/finder_event_notifier_xif.hh		\
$(top_srcdir)/xrl/interfaces/finder_event_notifier_xif.cc:		\
    $(top_srcdir)/xrl/interfaces/finder_event_notifier.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

finder_event_observer_xif.%:						\
    $(top_srcdir)/xrl/interfaces/finder_event_observer_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/finder_event_observer_xif.hh		\
$(top_srcdir)/xrl/interfaces/finder_event_observer_xif.cc:		\
    $(top_srcdir)/xrl/interfaces/finder_event_observer.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

test_xrls_xif.%:							\
    $(top_srcdir)/xrl/interfaces/test_xrls_xif.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/interfaces/$@ $(srcdir)

$(top_srcdir)/xrl/interfaces/test_xrls_xif.hh				\
$(top_srcdir)/xrl/interfaces/test_xrls_xif.cc:				\
    $(top_srcdir)/xrl/interfaces/test_xrls.xif
	cd $(top_builddir)/xrl/interfaces && $(MAKE) $(notdir $@)

###############################################################################
#
# Build handler bases
#
# NB The .xif file dependencies are specified MANUALLY for C++ Xrl Target
# Base below.  These MUST be kept up to date to be useful.
#
###############################################################################

# Basic build rule for Xrl Target base files
%_base.o: %_base.cc
	$(CXXCOMPILE) -c $<

# Finder base

finder_base.%: $(top_srcdir)/xrl/targets/finder_base.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/targets/$@ $(srcdir)

$(top_srcdir)/xrl/targets/finder_base.hh				\
$(top_srcdir)/xrl/targets/finder_base.cc:				\
    $(top_srcdir)/xrl/targets/finder.tgt				\
    $(top_srcdir)/xrl/interfaces/common.xif				\
    $(top_srcdir)/xrl/interfaces/finder.xif				\
    $(top_srcdir)/xrl/interfaces/finder_event_notifier.xif
	cd $(top_builddir)/xrl/targets && $(MAKE) $(notdir $@)

# Finder client base

finder_client_base.%: $(top_srcdir)/xrl/targets/finder_client_base.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/targets/$@ $(srcdir)

$(top_srcdir)/xrl/targets/finder_client_base.hh				\
$(top_srcdir)/xrl/targets/finder_client_base.cc:			\
    $(top_srcdir)/xrl/targets/finder_client.tgt				\
    $(top_srcdir)/xrl/interfaces/common.xif				\
    $(top_srcdir)/xrl/interfaces/finder_client.xif
	cd $(top_builddir)/xrl/targets && $(MAKE) $(notdir $@)

# Test Finder events base

test_finder_events_base.%:						\
    $(top_srcdir)/xrl/targets/test_finder_events_base.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/targets/$@ $(srcdir)

$(top_srcdir)/xrl/targets/test_finder_events_base.hh			\
$(top_srcdir)/xrl/targets/test_finder_events_base.cc:			\
    $(top_srcdir)/xrl/targets/test_finder_events.tgt			\
    $(top_srcdir)/xrl/interfaces/common.xif				\
    $(top_srcdir)/xrl/interfaces/finder_event_observer.xif
	cd $(top_builddir)/xrl/targets && $(MAKE) $(notdir $@)

# Test XRLs base

test_xrls_base.%:							\
    $(top_srcdir)/xrl/targets/test_xrls_base.%
	$(RM) $(srcdir)/$(notdir $@)
	$(LN_S) $(top_srcdir)/xrl/targets/$@ $(srcdir)

$(top_srcdir)/xrl/targets/test_xrls_base.hh				\
$(top_srcdir)/xrl/targets/test_xrls_base.cc:				\
    $(top_srcdir)/xrl/targets/test_xrls.tgt				\
    $(top_srcdir)/xrl/interfaces/common.xif				\
    $(top_srcdir)/xrl/interfaces/test_xrls.xif
	cd $(top_builddir)/xrl/targets && $(MAKE) $(notdir $@)
