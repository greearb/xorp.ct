## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libxorp/Makefile.am,v 1.55 2008/10/03 03:15:37 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- No-install Programs
check_PROGRAMS	 = test_asyncio
#check_PROGRAMS	+= test_buffered_asyncio
check_PROGRAMS	+= test_callback
check_PROGRAMS	+= test_config_param
check_PROGRAMS	+= test_heap
check_PROGRAMS	+= test_ipnet
check_PROGRAMS	+= test_ipv4
check_PROGRAMS	+= test_ipv4net
check_PROGRAMS	+= test_ipv6
check_PROGRAMS	+= test_ipv6net
check_PROGRAMS	+= test_ipvx
check_PROGRAMS	+= test_ipvxnet
check_PROGRAMS	+= test_mac
check_PROGRAMS	+= test_observers
check_PROGRAMS	+= test_profile
check_PROGRAMS	+= test_ref_ptr
check_PROGRAMS	+= test_ref_trie
check_PROGRAMS	+= test_run_command
check_PROGRAMS	+= test_sched
check_PROGRAMS	+= test_service
check_PROGRAMS	+= test_task
check_PROGRAMS	+= test_test_main
check_PROGRAMS	+= test_time_slice
check_PROGRAMS	+= test_timer
check_PROGRAMS	+= test_timeval
check_PROGRAMS	+= test_trie
check_PROGRAMS	+= test_types
check_PROGRAMS	+= test_utils
check_PROGRAMS	+= test_vif

# -- Test Programs
TESTS	 = test_asyncio$(EXEEXT)
#TESTS	+= test_buffered_asyncio$(EXEEXT)
TESTS	+= test_callback$(EXEEXT)
TESTS	+= test_config_param$(EXEEXT)
TESTS	+= test_heap$(EXEEXT)
TESTS	+= test_ipnet$(EXEEXT)
TESTS	+= test_ipv4$(EXEEXT)
TESTS	+= test_ipv4net$(EXEEXT)
TESTS	+= test_ipv6$(EXEEXT)
TESTS	+= test_ipv6net$(EXEEXT)
TESTS	+= test_ipvx$(EXEEXT)
TESTS	+= test_ipvxnet$(EXEEXT)
TESTS	+= test_mac$(EXEEXT)
TESTS	+= test_observers$(EXEEXT)
TESTS	+= test_profile$(EXEEXT)
TESTS	+= test_ref_ptr$(EXEEXT)
TESTS	+= test_ref_trie$(EXEEXT)
TESTS	+= test_run_command.sh
TESTS	+= test_sched$(EXEEXT)
TESTS	+= test_service$(EXEEXT)
TESTS	+= test_task$(EXEEXT)
TESTS	+= test_test_main$(EXEEXT)
TESTS	+= test_time_slice$(EXEEXT)
TESTS	+= test_timer$(EXEEXT)
TESTS	+= test_timeval$(EXEEXT)
TESTS	+= test_trie$(EXEEXT)
TESTS	+= test_types$(EXEEXT)
TESTS	+= test_utils$(EXEEXT)
TESTS	+= test_vif$(EXEEXT)

# -- No-install Libraries
noinst_LTLIBRARIES = libxorp.la

# -- Libraries and library flags to link the binary programs
LDADD			 = $(noinst_LTLIBRARIES)

# -- Program Sources
test_asyncio_SOURCES	= test_asyncio.cc
#test_buffered_asyncio_SOURCES	= test_buffered_asyncio.cc
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
test_profile_SOURCES	= test_profile.cc
test_ref_ptr_SOURCES	= test_ref_ptr.cc
test_ref_trie_SOURCES	= test_ref_trie.cc
test_run_command_SOURCES = test_run_command.cc
test_sched_SOURCES	= test_sched.cc
test_service_SOURCES	= test_service.cc
test_task_SOURCES	= test_task.cc
test_test_main_SOURCES	= test_test_main.cc
test_time_slice_SOURCES	= test_time_slice.cc
test_timer_SOURCES	= test_timer.cc
test_timeval_SOURCES	= test_timeval.cc
test_trie_SOURCES	= test_trie.cc
test_types_SOURCES	= test_types.cc
test_utils_SOURCES	= test_utils.cc
test_vif_SOURCES	= test_vif.cc

# -- Test programs extra libraries
test_sched_LDADD	= $(top_builddir)/libcomm/libcomm.la
test_sched_LDADD       += $(top_builddir)/libxorp/libxorp.la

# -- Library Sources
libxorp_la_SOURCES  = libxorp_module.h
libxorp_la_SOURCES += daemon.h
libxorp_la_SOURCES += debug.h
libxorp_la_SOURCES += ether_compat.h
libxorp_la_SOURCES += random.h
libxorp_la_SOURCES += xlog.h
libxorp_la_SOURCES += xorp.h
libxorp_la_SOURCES += utility.h
libxorp_la_SOURCES += win_io.h

libxorp_la_SOURCES += asnum.hh
libxorp_la_SOURCES += asyncio.hh
libxorp_la_SOURCES += buffer.hh
libxorp_la_SOURCES += buffered_asyncio.hh
libxorp_la_SOURCES += c_format.hh
libxorp_la_SOURCES += callback.hh
libxorp_la_SOURCES += callback_nodebug.hh
libxorp_la_SOURCES += callback_debug.hh
libxorp_la_SOURCES += clock.hh
libxorp_la_SOURCES += config_param.hh
libxorp_la_SOURCES += eventloop.hh
libxorp_la_SOURCES += exceptions.hh
libxorp_la_SOURCES += heap.hh
libxorp_la_SOURCES += ioevents.hh
libxorp_la_SOURCES += ipnet.hh
libxorp_la_SOURCES += ipv4.hh
libxorp_la_SOURCES += ipv4net.hh
libxorp_la_SOURCES += ipv6.hh
libxorp_la_SOURCES += ipv6net.hh
libxorp_la_SOURCES += ipvx.hh
libxorp_la_SOURCES += ipvxnet.hh
libxorp_la_SOURCES += mac.hh
libxorp_la_SOURCES += nexthop.hh
libxorp_la_SOURCES += popen.hh
libxorp_la_SOURCES += profile.hh
libxorp_la_SOURCES += range.hh
libxorp_la_SOURCES += ref_ptr.hh
libxorp_la_SOURCES += round_robin.hh
libxorp_la_SOURCES += run_command.hh
libxorp_la_SOURCES += safe_callback_obj.hh
libxorp_la_SOURCES += selector.hh
libxorp_la_SOURCES += service.hh
libxorp_la_SOURCES += task.hh
libxorp_la_SOURCES += time_slice.hh
libxorp_la_SOURCES += timespent.hh
libxorp_la_SOURCES += timer.hh
libxorp_la_SOURCES += timeval.hh
libxorp_la_SOURCES += tlv.hh
libxorp_la_SOURCES += token.hh
libxorp_la_SOURCES += tokenize.hh
libxorp_la_SOURCES += transaction.hh
libxorp_la_SOURCES += trie.hh
libxorp_la_SOURCES += utils.hh
libxorp_la_SOURCES += vif.hh
libxorp_la_SOURCES += win_dispatcher.hh

libxorp_la_SOURCES += daemon.c
libxorp_la_SOURCES += debug.c
libxorp_la_SOURCES += ether_compat.c
libxorp_la_SOURCES += gai_strerror.c
libxorp_la_SOURCES += getopt.c
libxorp_la_SOURCES += inet_ntop.c
libxorp_la_SOURCES += inet_pton.c
libxorp_la_SOURCES += random.c
libxorp_la_SOURCES += strlcpy.c
libxorp_la_SOURCES += strptime.c
libxorp_la_SOURCES += utility.c
libxorp_la_SOURCES += win_io.c
libxorp_la_SOURCES += xlog.c

libxorp_la_SOURCES += asyncio.cc
libxorp_la_SOURCES += buffered_asyncio.cc
libxorp_la_SOURCES += c_format.cc
libxorp_la_SOURCES += callback.cc
libxorp_la_SOURCES += clock.cc
libxorp_la_SOURCES += eventloop.cc
libxorp_la_SOURCES += exceptions.cc
libxorp_la_SOURCES += heap.cc
libxorp_la_SOURCES += ipnet.cc
libxorp_la_SOURCES += ipv4.cc
libxorp_la_SOURCES += ipv6.cc
libxorp_la_SOURCES += ipvx.cc
libxorp_la_SOURCES += mac.cc
libxorp_la_SOURCES += nexthop.cc
libxorp_la_SOURCES += popen.cc
libxorp_la_SOURCES += profile.cc
libxorp_la_SOURCES += ref_ptr.cc
libxorp_la_SOURCES += round_robin.cc
libxorp_la_SOURCES += run_command.cc
libxorp_la_SOURCES += safe_callback_obj.cc
libxorp_la_SOURCES += selector.cc
libxorp_la_SOURCES += service.cc
libxorp_la_SOURCES += task.cc
libxorp_la_SOURCES += time_slice.cc
libxorp_la_SOURCES += timer.cc
libxorp_la_SOURCES += token.cc
libxorp_la_SOURCES += transaction.cc
libxorp_la_SOURCES += utils.cc
libxorp_la_SOURCES += vif.cc
libxorp_la_SOURCES += win_dispatcher.cc

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

@PYTHON_BUILD@callback_nodebug.hh: callback-gen.py
@PYTHON_BUILD@	$(PYTHON) $(srcdir)/callback-gen.py -b 6 -l 15 > $(srcdir)/callback_nodebug.hh

@PYTHON_BUILD@callback_debug.hh: callback-gen.py
@PYTHON_BUILD@	$(PYTHON) $(srcdir)/callback-gen.py -d -b 6 -l 15 > $(srcdir)/callback_debug.hh
