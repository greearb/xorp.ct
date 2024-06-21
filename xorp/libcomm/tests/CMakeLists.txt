# libcomm/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(test_comm test_comm.c)
target_include_directories(test_comm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(test_comm xorp comm)
add_test(NAME comm COMMAND test_comm)


add_executable(test_connect test_connect.cc)
target_include_directories(test_connect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(test_connect xorp comm)
add_test(NAME connect COMMAND test_connect)
