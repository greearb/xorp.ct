# static routes build
# Copyright (c) 2022 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(static_routes  xorp_static_routes.cc
                    static_routes_node.cc
                    static_routes_varrw.cc
                    xrl_static_routes_node.cc)

target_include_directories(static_routes PRIVATE ${XORP_LIBDIR})
target_link_libraries(static_routes PRIVATE feaclient xipc xorp interfaces policy crypto proto)

