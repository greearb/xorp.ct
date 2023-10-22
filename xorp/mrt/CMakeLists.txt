# libmrt build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(mrt
                buffer.c
                mifset.cc
                mrib_table.cc)

install(TARGETS mrt RUNTIME DESTINATION lib)