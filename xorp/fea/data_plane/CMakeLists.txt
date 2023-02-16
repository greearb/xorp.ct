# data_plane directory build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(control_socket)
add_subdirectory(fibconfig)
if (FIREWALL)
add_subdirectory(firewall)
endif()
add_subdirectory(ifconfig)
add_subdirectory(io)
add_subdirectory(managers)
