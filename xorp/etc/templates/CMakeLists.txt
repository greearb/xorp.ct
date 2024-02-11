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

if (IPV6)
list(APPEND TEMPLATES mfea6.tp
                      pimsm6.tp)
list(APPEND COMMANDS mfea6.cmds
                      pim6.cmds)
    if (OSPF)
        list(APPEND COMMANDS ospfv3.cmds)
        list(APPEND TEMPLATE ospfv3.tp)
    endif()
endif()

if (FIREWALL)
    list(APPEND TEMPLATE firewall.tp)
endif()

if (OSPF)
    list(APPEND COMMANDS ospfv2.cmds)
    list(APPEND TEMPLATE ospfv2.tp)
endif()
if (OLSR)
    list(APPEND COMMANDS olsr4.cmds)
    list(APPEND TEMPLATE olsr4.tp)
endif()

if (BGP)
    list(APPEND COMMANDS bgp.cmds)
    list(APPEND TEMPLATE bgp.tp)
endif()

if (RIP)
    list(APPEND COMMANDS rip.cmds
                         ripng.cmds)
    list(APPEND TEMPLATE rip.tp
                         ripng.tp)
endif()

if (VRRP)
    list(APPEND COMMANDS vrrp.cmds)
    list(APPEND TEMPLATE vrrp.tp)
endif()

if (XORPSH)
    list(APPEND COMMANDS xorpsh.cmds)
endif()


install(FILES ${TEMPLATES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/templates)
install(FILES ${COMMANDS} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/templates)
# TODO: find way to separate config options firewall/click from fea.tp form
install(FILES fea.tp.raw DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/templates RENAME fea.tp)
