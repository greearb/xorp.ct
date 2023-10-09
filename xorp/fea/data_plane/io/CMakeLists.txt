# fea/data_plane/io build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(fea_io SHARED io_link_pcap.cc
                      io_tcpudp_socket.cc
                      io_ip_socket.cc)

if (FEA_DUMMY)
target_sources(fea_io PUBLIC io_ip_dummy.cc
                             io_link_dummy.cc
                             io_tcpudp_dummy.cc)
endif()

install(TARGETS fea_io RUNTIME DESTINATION lib)
