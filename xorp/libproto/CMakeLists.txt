# libproto build
# Copyright (c) 2021 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

file(GLOB LIBPROTO_C_SRCS checksum.c)
file(GLOB LIBPROTO_CXX_SRCS packet.cc
                            proto_node_cli.cc
                            proto_state.cc
                            proto_unit.cc
                            spt.cc)


add_library(proto SHARED
            ${LIBPROTO_C_SRCS} ${LIBPROTO_CXX_SRCS})


install(TARGETS proto RUNTIME DESTINATION lib)
