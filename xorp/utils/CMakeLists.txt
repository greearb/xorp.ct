# utils build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(runit runit.cc)
install(TARGETS runit RUNTIME DESTINATION ${XORP_TOOLDIR})
if (BUILDDIR_RUN)
    add_custom_command(TARGET runit POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:runit>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:runit>)
endif()
