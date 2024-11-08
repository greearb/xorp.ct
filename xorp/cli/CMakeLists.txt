# libcli build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
if (NOT EXTERNAL_TECLA)
     add_subdirectory(libtecla)
endif()

add_subdirectory(tools)
if (TESTS_PROGRAMS)
     add_subdirectory(tests)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(cli
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

install(TARGETS cli LIBRARY DESTINATION ${XORP_LIBDIR})

if (RTLD_ORIGIN)
     set_target_properties(cli PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                          INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
 endif()
if (BUILDDIR_RUN)
     add_custom_command(TARGET cli POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:cli>
                         ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:cli>)
endif()
