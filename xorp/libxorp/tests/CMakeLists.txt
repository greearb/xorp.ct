# libxorp/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(XORP_TESTS asyncio
               callback
               config_param
               heap
               ipnet
               ipv4
               ipv4net
               ipv6
               ipv6net
               ipvx
               ipvxnet
               mac
               observers
               ref_ptr
               ref_trie
               run_command
               sched
               service
               task
               test_main
               time_slice
               timer
               timeval
               trie
               types
               utils
               vif
              )

if(ENABLE_PROFILE)
    list(APPEND XORP_TESTS profile)
endif()

foreach(T IN ITEMS ${XORP_TESTS})
    add_executable(test_xorp_${T} test_${T}.cc)
    target_link_libraries(test_xorp_${T} xorp comm)
    target_include_directories(test_xorp_${T} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
    add_test(${T} COMMAND test_xorp_${T})
endforeach()
