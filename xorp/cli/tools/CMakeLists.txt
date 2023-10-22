# cli/tools build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(CLI_LIBS xif_cli_processor
             xipc
             xorp
             proto
             comm)

add_executable(cli_generic cli_generic.cc)
target_link_libraries(cli_generic ${CLI_LIBS})
install(TARGETS cli_generic RUNTIME DESTINATION bin)

add_executable(cli_send_processor_xrl send_cli_processor_xrl.cc)
target_link_libraries(cli_send_processor_xrl ${CLI_LIBS})
install(TARGETS cli_send_processor_xrl RUNTIME DESTINATION bin)
