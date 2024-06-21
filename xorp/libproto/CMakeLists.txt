# libproto build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl
if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()


add_library(proto checksum.c
                  packet.cc
                  proto_node_cli.cc
                  proto_state.cc
                  proto_unit.cc
                  spt.cc)


target_link_libraries(proto xorp)
install(TARGETS proto LIBRARY DESTINATION ${XORP_LIBDIR})
