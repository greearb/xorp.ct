# ospf/tools build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(OSPFTOOLS_LIBS xif_ospfv2
                   xipc
                   xorp
                   proto
                   comm)

if (ENABLE_IPV6)
    list(APPEND OSPFTOOLS_LIBS xif_ospfv3)
endif()

add_executable(ospf_clear_database clear_database.cc)
target_link_libraries(ospf_clear_database ${OSPFTOOLS_LIBS})
install(TARGETS ospf_clear_database RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(ospf_print_lsas print_lsas.cc ../lsa.cc ../fletcher_checksum.cc)
target_link_libraries(ospf_print_lsas ${OSPFTOOLS_LIBS})
install(TARGETS ospf_print_lsas RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(ospf_print_neighbours print_neighbours.cc)
target_link_libraries(ospf_print_neighbours ${OSPFTOOLS_LIBS})
install(TARGETS ospf_print_neighbours RUNTIME DESTINATION ${XORP_TOOLDIR})
foreach(T IN ITEMS ospf_clear_database ospf_print_lsas ospf_print_neighbours)
    if (RTLD_ORIGIN)
        set_target_properties(${T} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endforeach()
if (BUILDDIR_RUN)
    add_custom_command(TARGET ospf_clear_database POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:ospf_clear_database>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:ospf_clear_database>)
    add_custom_command(TARGET ospf_print_lsas POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:ospf_print_lsas>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:ospf_print_lsas>)
    add_custom_command(TARGET ospf_print_neighbours POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:ospf_print_neighbours>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:ospf_print_neighbours>)
endif()
