# contrib/olsr/tools build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

foreach(T IN ITEMS clear_database print_databases)
    add_executable(olsr_${T} ${T}.cc)
    target_link_libraries(olsr_${T} xif_olsr4 xipc proto xorp comm)
    target_include_directories(olsr_${T} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../")
    install(TARGETS olsr_${T} RUNTIME DESTINATION ${XORP_TOOLDIR})
    if (RTLD_ORIGIN)
        set_target_properties(olsr_${T} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endforeach()
if (BUILDDIR_RUN)
    add_custom_command(TARGET olsr_clear_database POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:olsr_clear_database>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:olsr_clear_database>)
    add_custom_command(TARGET olsr_print_databases POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:olsr_print_databases>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:olsr_print_databases>)
endif()
