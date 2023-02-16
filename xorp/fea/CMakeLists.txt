# fea build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(data_plane)
# add_library(static  static_routes_node.cc
#                     static_routes_varrw.cc
#                     xrl_static_routes_node.cc)

# target_include_directories(static PRIVATE ${XORP_LIBDIR})
# target_link_libraries(static PRIVATE feaclient xipc xorp interfaces policy crypto proto)

