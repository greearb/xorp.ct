# mrt/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(MRTTESTS
            xorp_mrt
            xorp
            comm
        )

foreach(T IN ITEMS "mrt" "mrib")
    add_executable(test_mrt_${T} test_${T}.cc)
    target_link_libraries(test_mrt_${T} ${MRTTESTS})
    target_include_directories(test_mrt_${T} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
    add_test(${T} COMMAND test_mrt_${T})
endforeach()
