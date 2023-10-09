# libxipc build

if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()
# find_library(crypto)
add_library(xipc SHARED finder_client.cc
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
# ${XIPC_SRCS})
add_dependencies(xipc targets)
add_dependencies(xipc interfaces)
target_link_libraries(xipc
                        comm
                        crypto
                        xif_finder_client
                        xif_finder
                        xif_common
                        tgts_finder_client)
# add_dependencies(xipc crypto)
install(TARGETS xipc RUNTIME DESTINATION lib)
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
                        crypto
                        tgts_finder
                        xif_finder_event_observer
                        xif_finder_client)

install(TARGETS finder RUNTIME DESTINATION lib)
