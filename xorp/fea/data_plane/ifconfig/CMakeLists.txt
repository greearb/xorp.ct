# fea/data_plane/ifconfig build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(IFCONFIG_SRCS "")
if (FEA_DUMMY)
    set(IFCONFIG_SRCS ifconfig_get_dummy.cc
                      ifconfig_observer_dummy.cc
                      ifconfig_property_dummy.cc
                      ifconfig_set_dummy.cc)
endif()

add_library(ifconfig OBJECT
                     ifconfig_get_click.cc
                     ifconfig_get_getifaddrs.cc
                     ifconfig_get_ioctl.cc
                     ifconfig_get_iphelper.cc
                     ifconfig_get_netlink_socket.cc
                     ifconfig_get_proc_linux.cc
                     ifconfig_get_sysctl.cc
                     ifconfig_media.cc
                     ifconfig_observer_iphelper.cc
                     ifconfig_observer_netlink_socket.cc
                     ifconfig_observer_routing_socket.cc
                     ifconfig_parse_getifaddrs.cc
                     ifconfig_parse_ioctl.cc
                     ifconfig_parse_routing_socket.cc
                     ifconfig_property_bsd.cc
                     ifconfig_property_linux.cc
                     ifconfig_property_solaris.cc
                     ifconfig_property_windows.cc
                     ifconfig_set.cc
                     ifconfig_set_click.cc
                     ifconfig_set_ioctl.cc
                     ifconfig_set_iphelper.cc
                     ifconfig_set_netlink_socket.cc
                     ifconfig_vlan_get_linux.cc
                     ifconfig_vlan_set_linux.cc
                     ${IFCONFIG_SRCS})

# target_link_libraries(ifconfig xorp comm proto)
#xipc finder_client OpenSSL::Crypto)
# install(TARGETS ifconfig LIBRARY DESTINATION ${XORP_LIBDIR})
set_target_properties(ifconfig PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
