# libcli/tests build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(test_cli test_cli.cc)
target_include_directories(test_cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(test_cli cli tecla xif_cli_processor tgts_cli finder xipc comm xorp proto)

add_test(cli COMMAND test_cli)
