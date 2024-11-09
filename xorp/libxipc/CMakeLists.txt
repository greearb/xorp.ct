# libxipc build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_library(xipc finder_client.cc
                finder_client_observer.cc
                finder_client_xrl_target.cc
                finder_messenger.cc
                finder_msgs.cc
                finder_tcp.cc
                finder_tcp_messenger.cc
                hmac.cc
                hmac_md5.c
                permits.cc
                sockutil.cc
                xrl.cc
                xrl_args.cc
                xrl_atom.cc
                xrl_atom_encoding.cc
                xrl_atom_list.cc
                xrl_cmd_map.cc
                xrl_dispatcher.cc
                xrl_error.cc
                xrl_parser.cc
                xrl_parser_input.cc
                xrl_pf.cc
                xrl_pf_factory.cc
                xrl_pf_stcp.cc
                xrl_pf_stcp_ph.cc
                xrl_pf_unix.cc
                xrl_router.cc
                xrl_std_router.cc
                xrl_tokens.cc
                xuid.cc         # only for udp (and fea tcpudp mgr)
                fp64serial.c
                )

add_dependencies(xipc targets)
add_dependencies(xipc interfaces)
target_link_libraries(xipc
                           comm
                           proto
                           OpenSSL::Crypto
                           $<TARGET_NAME:xif_base>
                           $<TARGET_NAME:tgts_base>
                     )
# xif_finder
# xif_common
# tgts_finder_client

install(TARGETS xipc LIBRARY DESTINATION ${XORP_LIBDIR})

# Finder library
MESSAGE(STATUS "Targets dir is ${TARGETS_DIR}")

add_library(finder SHARED finder.cc
                    finder_server.cc
                    finder_xrl_queue.cc
                    finder_xrl_target.cc
                    )
add_dependencies(finder targets)
add_dependencies(finder interfaces)

target_link_libraries(finder
                             xipc
                             comm
                             xorp
                             proto
                             OpenSSL::Crypto
                             tgts_finder
                             xif_finder_event_observer
                             xif_finder_client
                     )

install(TARGETS finder LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_finder finder_main.cc)
target_link_libraries(xorp_finder finder xif_finder_client xif_finder xif_common OpenSSL::Crypto xipc comm xorp)
install(TARGETS xorp_finder RUNTIME DESTINATION ${XORP_MODULEDIR})

add_executable(call_xrl call_xrl.cc)
target_link_libraries(call_xrl finder xipc comm xorp)
install(TARGETS call_xrl RUNTIME DESTINATION ${XORP_SBINDIR})

if (RTLD_ORIGIN)
    foreach(T IN ITEMS finder xorp_finder xipc)
        set_target_properties(${T} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endforeach()
    set_target_properties(call_xrl PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib")
endif()

if (BUILDDIR_RUN)
    add_custom_command(TARGET call_xrl POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:call_xrl>
                        ${XORP_ALIAS_SBINDIR}/$<TARGET_FILE_NAME:call_xrl>)
    add_custom_command(TARGET xorp_finder POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_finder>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_finder>)
    add_custom_command(TARGET finder POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:finder>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:finder>)
    add_custom_command(TARGET xipc POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xipc>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:xipc>)
endif()
