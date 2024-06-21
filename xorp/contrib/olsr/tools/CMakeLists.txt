# contrib/olsr/tools build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

foreach(T IN ITEMS "clear_database" "print_databases")
    add_executable(olsr_${T} ${T}.cc)
    target_link_libraries(olsr_${T} xif_olsr4 xipc proto xorp comm)
    target_include_directories(olsr_${T} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../")
    install(TARGETS olsr_${T} RUNTIME DESTINATION ${XORP_TOOLDIR})
endforeach()
