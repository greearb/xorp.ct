# libcomm build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_library(comm comm_sock.c comm_user.c)
target_link_libraries(comm xorp)
install(TARGETS comm LIBRARY DESTINATION ${XORP_LIBDIR})
if (RTLD_ORIGIN)
    set_target_properties(comm PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET comm POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:comm>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:comm>)
endif()
