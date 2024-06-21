# libproto/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/spt_graph1
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

foreach(T IN ITEMS "checksum" "config_node_id" "packet")
    add_executable(test_proto_${T} test_${T}.cc)
    target_link_libraries(test_proto_${T} xorp comm proto)
    target_include_directories(test_proto_${T} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
    add_test(${T} COMMAND test_proto_${T} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
