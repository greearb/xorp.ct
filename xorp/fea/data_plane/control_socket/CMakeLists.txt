# data-plane/control_socket build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(control_socket SHARED netlink_socket.cc
                                  netlink_socket_utilities.cc
                                  routing_socket.cc
                                  routing_socket_utilities.cc
                                  windows_rras_support.cc
                                  windows_rtm_pipe.cc)

if (CLICK)
    target_sources(control_socket click_socket.cc)
endif()


