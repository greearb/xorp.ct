# fea/data_plane/io build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(IO_SRCS "")

if (FEA_DUMMY)
list(APPEND IO_SRCS io_ip_dummy.cc
                    io_link_dummy.cc
                    io_tcpudp_dummy.cc)
endif()

add_library(io SHARED io_link_pcap.cc
                      io_tcpudp_socket.cc
                      io_ip_socket.cc)