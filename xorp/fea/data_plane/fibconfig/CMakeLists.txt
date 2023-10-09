# fibconfig build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(FIB_SRCS fibconfig_entry_get_iphelper.cc
             fibconfig_entry_get_routing_socket.cc
             fibconfig_entry_get_rtmv2.cc
             fibconfig_entry_observer_iphelper.cc
             fibconfig_entry_observer_routing_socket.cc
             fibconfig_entry_observer_rtmv2.cc
             fibconfig_entry_parse_routing_socket.cc
             fibconfig_entry_set_iphelper.cc
             fibconfig_entry_set_routing_socket.cc
             fibconfig_entry_set_rtmv2.cc
             fibconfig_forwarding_proc_linux.cc
             fibconfig_forwarding_solaris.cc
             fibconfig_forwarding_sysctl.cc
             fibconfig_forwarding_windows.cc
             fibconfig_table_get_iphelper.cc
             fibconfig_table_get_sysctl.cc
             fibconfig_table_observer_iphelper.cc
             fibconfig_table_observer_routing_socket.cc
             fibconfig_table_observer_rtmv2.cc
             fibconfig_table_parse_routing_socket.cc
             fibconfig_table_set_iphelper.cc
             fibconfig_table_set_routing_socket.cc
             fibconfig_table_set_rtmv2.cc)

if (NOT MINGW)
list(APPEND FIB_SRCS fibconfig_table_set_netlink_socket.cc
                     fibconfig_table_get_netlink_socket.cc
                     fibconfig_table_parse_netlink_socket.cc
                     fibconfig_table_observer_netlink_socket.cc
                     fibconfig_entry_get_netlink_socket.cc
                     fibconfig_entry_set_netlink_socket.cc
                     fibconfig_entry_observer_netlink_socket.cc)
endif()

if (CLICK)
list(APPEND FIB_SRCS fibconfig_entry_get_click.cc
                     fibconfig_entry_set_click.cc
                     fibconfig_table_set_click.cc
                     fibconfig_table_get_click.cc)
endif()

if (FEA_DUMMY)
list(APPEND FIB_SRCS fibconfig_entry_get_dummy.cc
                     fibconfig_entry_observer_dummy.cc
                     fibconfig_entry_set_dummy.cc
                     fibconfig_forwarding_dummy.cc
                     fibconfig_table_get_dummy.cc
                     fibconfig_table_observer_dummy.cc
                     fibconfig_table_set_dummy.cc)
endif()

add_library(fibconfig SHARED ${FIB_SRCS})
install(TARGETS fibconfig RUNTIME DESTINATION lib)
