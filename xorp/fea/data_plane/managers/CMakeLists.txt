# fea/data_plane/managers build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(MENAGERS_SRCS "")
if (FEA_DUMMY)
    list(APPEND MENAGERS_SRCS fea_data_plane_manager_dummy.cc)
endif()

if (CLICK)
    list(APPEND MENAGERS_SRCS fea_data_plane_manager_click.cc)
endif()

add_library(fea_managers OBJECT
                         fea_data_plane_manager_linux.cc
                         fea_data_plane_manager_windows.cc
                         fea_data_plane_manager_bsd.cc
                         ${MENAGERS_SRCS})

# target_link_libraries(fea_managers PUBLIC xorp ifconfig comm xipc)
# install(TARGETS fea_managers LIBRARY DESTINATION ${XORP_LIBDIR})
