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
if (RTLD_ORIGIN)
    set_target_properties(proto PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                           INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if(BUILDDIR_RUN)
    add_custom_command(TARGET proto POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:proto>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:proto>)
endif()
