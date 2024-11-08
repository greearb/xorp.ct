# cli/tools build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(CLI_LIBS xif_cli_processor
             xipc
             xorp
             proto
             comm)

add_executable(cli_generic cli_generic.cc)
target_link_libraries(cli_generic ${CLI_LIBS})
install(TARGETS cli_generic RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(cli_send_processor_xrl send_cli_processor_xrl.cc)
target_link_libraries(cli_send_processor_xrl ${CLI_LIBS})
install(TARGETS cli_send_processor_xrl RUNTIME DESTINATION ${XORP_TOOLDIR})
if (RTLD_ORIGIN)
    set_target_properties(cli_generic PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")

    set_target_properties(cli_send_processor_xrl PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                            INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()

if (BUILDDIR_RUN)
    add_custom_command(TARGET cli_generic POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:cli_generic>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:cli_generic>)
    add_custom_command(TARGET cli_send_processor_xrl POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:cli_send_processor_xrl>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:cli_send_processor_xrl>)
endif()
