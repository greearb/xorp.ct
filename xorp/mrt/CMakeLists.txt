# libmrt build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_library(xorp_mrt
                buffer.c
                mifset.cc
                mrib_table.cc)

install(TARGETS xorp_mrt LIBRARY DESTINATION ${XORP_LIBDIR})
