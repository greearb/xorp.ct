# fea/data_plane/io build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(fea_io OBJECT
                   io_link_pcap.cc
                   io_tcpudp_socket.cc
                   io_ip_socket.cc)

if (FEA_DUMMY)
    target_sources(fea_io PRIVATE io_ip_dummy.cc
                                  io_link_dummy.cc
                                  io_tcpudp_dummy.cc)
endif()
#target_link_libraries(fea_io xorp xfdp)
# install(TARGETS fea_io LIBRARY DESTINATION ${XORP_LIBDIR})
set_target_properties(fea_io PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
