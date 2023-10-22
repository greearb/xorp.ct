# libcli build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(libtecla)
add_subdirectory(tools)
add_subdirectory(tests)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(cli SHARED
                       cli_client.cc
                       cli_command.cc
                       cli_command_pipe.cc
                       cli_node.cc
                       cli_node_internal_commands.cc
                       cli_node_net.cc
                       xrl_cli_node.cc
                       )

add_dependencies(cli interfaces targets)
target_link_libraries(cli
                          xorp
                          tecla
                          proto
                          xipc
                          comm
                          xif_cli_processor
                          tgts_cli
                     )

install(TARGETS cli RUNTIME DESTINATION lib)
