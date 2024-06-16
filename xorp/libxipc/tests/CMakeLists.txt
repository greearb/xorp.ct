# libxipc/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(XIPC_TESTS
                xipc
                # proto
                xif_finder_event_notifier
                xif_test_xrls
                tgts_test_finder_events
                tgts_test_xrls
                xorp
                comm
                finder
        )

foreach(T IN ITEMS
                    "finder"		# XXX test target needed
                    "finder_events"
                    "finder_messenger"
                    "finder_msgs"
                    "finder_tcp"
                    "finder_to"
                    "lemming"
                    "stcp"
                    "stcppf"
                    "xrl"
                    "xrl_args"
                    "xrl_atom"
                    "xrl_error"
                    "xrl_parser"
                    "xrl_router"
                    "fp64"
                    )
    add_executable(test_xipc_${T} test_${T}.cc)
    target_link_libraries(test_xipc_${T} ${XIPC_TESTS})
    target_include_directories(test_xipc_${T} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
    add_test(${T} COMMAND test_xipc_${T})
endforeach()
configure_file(test_xrl_parser.data ${CMAKE_CURRENT_BINARY_DIR}/test_xrl_parser.data COPYONLY)
add_test(NAME finder_deaths COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_finder_deaths.sh WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME leaks COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_leaks.sh WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME test_parser COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_xrl_parser.sh WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
if (PROFILE)
# Require profiler in libxorp to compile
add_executable(test_xipc_xrl_sender test_xrl_sender.cc test_receiver.cc)
target_link_libraries(test_xipc_xrl_sender ${XIPC_TESTS})
target_include_directories(test_xipc_xrl_sender PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(${T} COMMAND test_xipc_xrl_sender)

add_executable(test_xipc_xrl_receiver test_xrl_receiver.cc test_receiver.cc)
target_link_libraries(test_xipc_xrl_receiver ${XIPC_TESTS})
target_include_directories(test_xipc_xrl_receiver PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(${T} COMMAND test_xipc_xrl_receiver)
endif()
