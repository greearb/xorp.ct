# tmplates install
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(TEMPLATES fib2mrib.tp
              igmp.tp
              interfaces.tp
              mfea4.tp
              mld.tp
              pimsm4.tp
              plumbing.tp
              policy.tp
              protocols.tp
              rib.tp
              rtrmgr.tp
              static_routes.tp
)

set(COMMANDS fea.cmds
             host.cmds
             igmp.cmds
             mfea.cmds
             misc.cmds
             mld.cmds
             pim.cmds
             policy.cmds
             rib.cmds)

if (ENABLE_IPV6)
    list(APPEND TEMPLATES mfea6.tp
                          pimsm6.tp)
    list(APPEND COMMANDS mfea6.cmds
                         pim6.cmds)
    if (OSPF)
        list(APPEND COMMANDS ospfv3.cmds)
        list(APPEND TEMPLATES ospfv3.tp)
    endif()
endif()

if (FIREWALL)
    list(APPEND TEMPLATES firewall.tp)
endif()

if (OSPF)
    list(APPEND COMMANDS ospfv2.cmds)
    list(APPEND TEMPLATES ospfv2.tp)
endif()
if (OLSR)
    list(APPEND COMMANDS olsr4.cmds)
    list(APPEND TEMPLATES olsr4.tp)
endif()

if (BGP)
    list(APPEND COMMANDS bgp.cmds)
    list(APPEND TEMPLATES bgp.tp)
endif()

if (RIP)
    list(APPEND COMMANDS rip.cmds
                         ripng.cmds)
    list(APPEND TEMPLATES rip.tp
                         ripng.tp)
endif()

if (VRRP)
    list(APPEND COMMANDS vrrp.cmds)
    list(APPEND TEMPLATES vrrp.tp)
endif()

if (XORPSH)
    list(APPEND COMMANDS xorpsh.cmds)
endif()

# Generate fea.tp based on FIREWALL and CLICK options
add_custom_target(fea_template ALL
                  BYPRODUCTS fea.tp
                  COMMAND ${Python3_EXECUTABLE} ./gen_fea.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/fea.tp.raw
                  ${CMAKE_CURRENT_BINARY_DIR}/fea.tp
                  ${FIREWALL} ${CLICK}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  COMMENT "Gnerating fea template"
                  )


install(FILES ${TEMPLATES} DESTINATION ${XORP_TEMPLATEDIR})
install(FILES ${COMMANDS} DESTINATION ${XORP_TEMPLATEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fea.tp DESTINATION ${XORP_TEMPLATEDIR})

if (BUILDDIR_RUN)
    # fea.tp is already in build directory
    file(COPY ${TEMPLATES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${COMMANDS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
