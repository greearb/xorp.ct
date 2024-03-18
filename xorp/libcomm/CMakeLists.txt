# libcomm build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

# add_subdirectory(test)


file(GLOB COMM_CSRC comm_sock.c comm_user.c)

add_library(comm ${COMM_CSRC})

install(TARGETS comm LIBRARY DESTINATION ${XORP_LIBDIR})
set_target_properties(comm PROPERTIES SOVERSION 0.0)
