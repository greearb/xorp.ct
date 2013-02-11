// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// ===========================================================================

#ifndef __WRAPPER_IO_HH__
#define __WRAPPER_IO_HH__

#include "policy/backend/policytags.hh"
#include "wrapper.hh"

class IO : public ServiceBase {
public:
    IO() {}
    virtual ~IO() {}

    virtual  int wstartup(Wrapper *wrapper) = 0;
    virtual  int restart(uint32_t admin_dist) = 0;
    virtual  void wshutdown() = 0;
    virtual  void set_admin_dist(uint32_t admin_dist) = 0;

    virtual void forceclose(string xrl_sock) = 0;

    virtual void send_add_route(add_route_t * addroute, PolicyTags policytags,
                                void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_del_route(del_route_t * delroute,
                                void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_close(string xrl_sock,
                            void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_open_udp(int domain, int type, int protocol,
                               void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_bind_udp(string xrl_sock, IPv4 local_addr, uint32_t local_port,
                               void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_open_bind_udp(IPv4 local_addr, uint32_t local_port,
                                    void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_enable_recv(string xrl_sock,
                                  void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_send_udp(string xrl_sock,IPv4 target_addr, uint32_t target_port, vector<uint8_t> payload,
                               void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_socket_option(string xrl_sock, int level, string optionName, uint32_t optValue, int length,
                                    void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void send_socket_option_to(string xrl_sock,int level, string optname, string devName, int length,
                                       void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t)) = 0;

    virtual void push_routes() = 0;
    virtual void fromXorp(int cmd, string network,  bool unicast, bool multicast, string nexthop, uint32_t metric) = 0;


};


#endif
