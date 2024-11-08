# libmrt build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_library(xorp_mrt
                buffer.c
                mifset.cc
                mrib_table.cc)

target_link_libraries(xorp_mrt PUBLIC xorp)
install(TARGETS xorp_mrt LIBRARY DESTINATION ${XORP_LIBDIR})
if (RTLD_ORIGIN)
    set_target_properties(xorp_mrt PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET xorp_mrt POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_mrt>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:xorp_mrt>)
endif()
