# libcomm build
# Copyright (c) 2021 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

# add_subdirectory(test)


file(GLOB COMM_CSRC comm_sock.c comm_user.c)

add_library(comm SHARED ${COMM_CSRC})

install(TARGETS comm RUNTIME DESTINATION lib)