# fea/data_plane/managers build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(FEA_SOURCES "")
if (FEA_DUMMY)
list(APPEND FEA_SOURCES fea_data_plane_manager_dummy.cc)
endif()

if (CLICK)
list(APPEND FEA_SOURCES fea_data_plane_manager_click.cc)
endif()

add_library(fea_manager SHARED fea_data_plane_manager_linux.cc
                               fea_data_plane_manager_windows.cc
                               fea_data_plane_manager_bsd.cc
                               ${FEA_SOURCES})