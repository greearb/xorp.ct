## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libxorp/Makefile.am,v 1.25 2004/08/18 09:04:37 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_asyncio
TESTS		+= test_callback
TESTS		+= test_config_param
TESTS		+= test_heap
TESTS		+= test_ipnet
TESTS		+= test_ipv4
TESTS		+= test_ipv4net
TESTS		+= test_ipv6
TESTS		+= test_ipv6net
TESTS		+= test_ipvx
TESTS		+= test_ipvxnet
TESTS		+= test_mac
TESTS		+= test_observers
TESTS		+= test_ref_ptr
TESTS		+= test_ref_trie
TESTS		+= test_service
TESTS		+= test_test_main
TESTS		+= test_time_slice
TESTS		+= test_timer
TESTS		+= test_timeval
TESTS		+= test_trie
TESTS		+= test_types
TESTS		+= test_vif

# -- No-install Programs
noinst_PROGRAMS	 = test_asyncio
noinst_PROGRAMS	+= test_callback
noinst_PROGRAMS	+= test_config_param
noinst_PROGRAMS	+= test_heap
noinst_PROGRAMS	+= test_ipnet
noinst_PROGRAMS	+= test_ipv4
noinst_PROGRAMS	+= test_ipv4net
noinst_PROGRAMS	+= test_ipv6
noinst_PROGRAMS	+= test_ipv6net
noinst_PROGRAMS	+= test_ipvx
noinst_PROGRAMS	+= test_ipvxnet
noinst_PROGRAMS	+= test_mac
noinst_PROGRAMS	+= test_observers
noinst_PROGRAMS	+= test_ref_ptr
noinst_PROGRAMS	+= test_ref_trie
noinst_PROGRAMS	+= test_service
noinst_PROGRAMS	+= test_test_main
noinst_PROGRAMS	+= test_time_slice
noinst_PROGRAMS	+= test_timer
noinst_PROGRAMS	+= test_timeval
noinst_PROGRAMS	+= test_trie
noinst_PROGRAMS	+= test_types
noinst_PROGRAMS	+= test_vif

# -- No-install Libraries
noinst_LIBRARIES = libxorp.a

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
test_asyncio_SOURCES	= test_asyncio.cc
test_callback_SOURCES	= test_callback.cc
test_config_param_SOURCES = test_config_param.cc
test_heap_SOURCES	= test_heap.cc
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
test_service_SOURCES	= test_service.cc
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
libxorp_a_SOURCES += callback_nodebug.hh
libxorp_a_SOURCES += callback_debug.hh
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
libxorp_a_SOURCES += safe_callback_obj.hh
libxorp_a_SOURCES += selector.hh
libxorp_a_SOURCES += service.hh
libxorp_a_SOURCES += time_slice.hh
libxorp_a_SOURCES += timespent.hh
libxorp_a_SOURCES += timer.hh
libxorp_a_SOURCES += timeval.hh
libxorp_a_SOURCES += token.hh
libxorp_a_SOURCES += tokenize.hh
libxorp_a_SOURCES += transaction.hh
libxorp_a_SOURCES += trie.hh
libxorp_a_SOURCES += utils.hh
libxorp_a_SOURCES += vif.hh

libxorp_a_SOURCES += debug.c
# libxorp_a_SOURCES += ether_compat.c
libxorp_a_SOURCES += utility.c
libxorp_a_SOURCES += xlog.c

libxorp_a_SOURCES += asyncio.cc
libxorp_a_SOURCES += c_format.cc
libxorp_a_SOURCES += callback.cc
libxorp_a_SOURCES += eventloop.cc
libxorp_a_SOURCES += exceptions.cc
libxorp_a_SOURCES += heap.cc
libxorp_a_SOURCES += ipv4.cc
libxorp_a_SOURCES += ipv6.cc
libxorp_a_SOURCES += ipvx.cc
libxorp_a_SOURCES += mac.cc
libxorp_a_SOURCES += nexthop.cc
libxorp_a_SOURCES += ref_ptr.cc
libxorp_a_SOURCES += safe_callback_obj.cc
libxorp_a_SOURCES += selector.cc
libxorp_a_SOURCES += service.cc
libxorp_a_SOURCES += time_slice.cc
libxorp_a_SOURCES += timer.cc
libxorp_a_SOURCES += token.cc
libxorp_a_SOURCES += transaction.cc
libxorp_a_SOURCES += vif.cc

BUILT_SOURCES	   = callback_nodebug.hh callback_debug.hh

# -- Extra files and directories to add to the distribution
EXTRA_DIST	   = callback_gen.py

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

# If this code is commented out, please upgrade to python2.0 or above.

@PY_BUILD@callback_nodebug.hh: callback-gen.py
@PY_BUILD@	$(PYTHON) $(srcdir)/callback-gen.py -b 6 -l 15 > $(srcdir)/callback_nodebug.hh

@PY_BUILD@callback_debug.hh: callback-gen.py
@PY_BUILD@	$(PYTHON) $(srcdir)/callback-gen.py -d -b 6 -l 15 > $(srcdir)/callback_debug.hh
