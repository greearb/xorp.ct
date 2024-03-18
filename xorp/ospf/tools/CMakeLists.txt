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
