# vrrp build
# Copyright (c) 2021 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

file(GLOB VRRP_CXX_SRCS arpd.cc
                        vrrp.cc
                        vrrp_packet.cc
                        vrrp_target.cc
                        vrrp_vif.cc)

# add_executable(vrrp ${VRRP_CXX_SRCS})
# target_link_libraries(vrrp xorp xipc comm )


add_library(vrrp SHARED ${VRRP_CXX_SRCS})

install(TARGETS vrrp RUNTIME DESTINATION lib)