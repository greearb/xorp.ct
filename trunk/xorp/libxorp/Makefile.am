## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libxorp/Makefile.am,v 1.11 2003/07/21 22:35:49 atanu Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
check_PROGRAMS	 = test_asyncio test_callback test_config_param
check_PROGRAMS	+= test_ipnet test_ipv4 test_ipv4net
check_PROGRAMS	+= test_ipv6 test_ipv6net test_ipvx test_ipvxnet
check_PROGRAMS	+= test_mac test_observers test_ref_ptr test_ref_trie 
check_PROGRAMS	+= test_test_main test_time_slice test_timer test_timeval 
check_PROGRAMS	+= test_trie test_types test_vif 
TESTS		 = $(check_PROGRAMS)

# -- No-install Programs
noinst_PROGRAMS  = 

# -- No-install Libraries
noinst_LIBRARIES = libxorp.a

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
test_asyncio_SOURCES	= test_asyncio.cc
test_callback_SOURCES	= test_callback.cc
test_config_param_SOURCES = test_config_param.cc
test_ipnet_SOURCES	= test_ipnet.cc
test_ipv4_SOURCES	= test_ipv4.cc
test_ipv4net_SOURCES	= test_ipv4net.cc
test_ipv6_SOURCES	= test_ipv6.cc
test_ipv6net_SOURCES	= test_ipv6net.cc
test_ipvx_SOURCES	= test_ipvx.cc
test_ipvxnet_SOURCES	= test_ipvxnet.cc
test_mac_SOURCES	= test_mac.cc
test_observers_SOURCES  = test_observers.cc
test_ref_ptr_SOURCES	= test_ref_ptr.cc
test_ref_trie_SOURCES	= test_ref_trie.cc
test_test_main_SOURCES	= test_test_main.cc
test_time_slice_SOURCES	= test_time_slice.cc
test_timer_SOURCES	= test_timer.cc
test_timeval_SOURCES	= test_timeval.cc
test_trie_SOURCES	= test_trie.cc
test_types_SOURCES	= test_types.cc
test_vif_SOURCES	= test_vif.cc


# -- Library Sources
libxorp_a_SOURCES  = libxorp_module.h
libxorp_a_SOURCES += debug.h
libxorp_a_SOURCES += ether_compat.h
libxorp_a_SOURCES += xlog.h
libxorp_a_SOURCES += xorp.h
libxorp_a_SOURCES += utility.h

libxorp_a_SOURCES += asnum.hh
libxorp_a_SOURCES += asyncio.hh
libxorp_a_SOURCES += buffer.hh
libxorp_a_SOURCES += c_format.hh
libxorp_a_SOURCES += callback.hh
libxorp_a_SOURCES += config_param.hh
libxorp_a_SOURCES += eventloop.hh
libxorp_a_SOURCES += exceptions.hh
libxorp_a_SOURCES += heap.hh
libxorp_a_SOURCES += ipnet.hh
libxorp_a_SOURCES += ipv4.hh
libxorp_a_SOURCES += ipv4net.hh
libxorp_a_SOURCES += ipv6.hh
libxorp_a_SOURCES += ipv6net.hh
libxorp_a_SOURCES += ipvx.hh
libxorp_a_SOURCES += ipvxnet.hh
libxorp_a_SOURCES += mac.hh
libxorp_a_SOURCES += nexthop.hh
libxorp_a_SOURCES += old_trie.hh
libxorp_a_SOURCES += ref_ptr.hh
libxorp_a_SOURCES += selector.hh
libxorp_a_SOURCES += time_slice.hh
libxorp_a_SOURCES += timer.hh
libxorp_a_SOURCES += timeval.hh
libxorp_a_SOURCES += token.hh
libxorp_a_SOURCES += transaction.hh
libxorp_a_SOURCES += trie.hh
libxorp_a_SOURCES += utils.hh
libxorp_a_SOURCES += vif.hh

libxorp_a_SOURCES += debug.c
libxorp_a_SOURCES += xlog.c

libxorp_a_SOURCES += asyncio.cc
libxorp_a_SOURCES += c_format.cc
libxorp_a_SOURCES += eventloop.cc
libxorp_a_SOURCES += exceptions.cc
libxorp_a_SOURCES += heap.cc
libxorp_a_SOURCES += ipv4.cc
libxorp_a_SOURCES += ipv6.cc
libxorp_a_SOURCES += ipvx.cc
libxorp_a_SOURCES += mac.cc
libxorp_a_SOURCES += nexthop.cc
libxorp_a_SOURCES += ref_ptr.cc
libxorp_a_SOURCES += selector.cc
libxorp_a_SOURCES += time_slice.cc
libxorp_a_SOURCES += timer.cc
libxorp_a_SOURCES += token.cc
libxorp_a_SOURCES += transaction.cc
libxorp_a_SOURCES += vif.cc

BUILT_SOURCES = callback.hh

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = callback-gen.py


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

debug.o: debug.c
	$(subst -Wnon-const-format,,$(COMPILE)) -c $<

xlog.o: xlog.c
	$(subst -Wnon-const-format,,$(COMPILE)) -c $<

# If this code is commented out, please upgrade to python2.0 or above.

@PY_BUILD@callback.hh: callback-gen.py
@PY_BUILD@	$(PYTHON) $(srcdir)/callback-gen.py -b 6 -l 13 > $(srcdir)/callback.hh

