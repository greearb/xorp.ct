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


install(FILES ${TEMPLATES} DESTINATION ${XORP_TEMPLATEDIR})
install(FILES ${COMMANDS} DESTINATION ${XORP_TEMPLATEDIR})
# TODO: find way to separate config options firewall/click from fea.tp form
install(FILES fea.tp.raw DESTINATION ${XORP_TEMPLATEDIR} RENAME fea.tp)

if (BUILD_RUN)
    file(COPY ${TEMPLATES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${COMMANDS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY fea.tp.raw DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/fea.tp.raw ${CMAKE_CURRENT_BINARY_DIR}/fea.tp)
endif()
