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

#include "wrapper_module.h"

#include "wrapper_types.hh"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/socket4_xif.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"

#include "xorp_io.hh"

// --------------------------------------------------------------------------
// XrlIO

XrlIO::XrlIO(EventLoop& eventloop, XrlRouter& xrl_router,
             const string& feaname, const string& ribname, string protocol)
    : _eventloop(eventloop),
      _xrl_router(xrl_router),
      _feaname(feaname),
      _ribname(ribname),
      _protocol(protocol),
      _xrl_socket(&xrl_router),
      _rib(&xrl_router),
      _ifmgr(eventloop, feaname.c_str(), _xrl_router.finder_address(),
             _xrl_router.finder_port())
{
    _admin_distance = 202;
    _reg_admin_ok = false;
}

XrlIO::~XrlIO()
{
//    _ifmgr.detach_hint_observer(this);
//    _ifmgr.unset_observer(this);

}
void
XrlIO::callbackStartup(const XrlError& error, bool up,
                       const char *comment)
{
    UNUSED(error);
    UNUSED(up);
    UNUSED(comment);
    _init_callback_done = true;
}
void
XrlIO::forceclosecallback(const XrlError &e)
{
    UNUSED(e);
    _init_callback_done = true;
}

void
XrlIO::forceclose(string xrl_sock)
{
//fprintf(stderr,"force close xrl_sock: >%s<\n",xrl_sock.c_str());
    bool sent = _xrl_socket.send_close(_feaname.c_str(),
                                       xrl_sock,
                                       callback(this,&XrlIO::forceclosecallback));

    if (sent) {
        _init_callback_done = false;
        while (!_init_callback_done) {
            _eventloop.run();
        }
    }
}

int
XrlIO::startup()
{
    return XORP_OK;
}

int
XrlIO::wstartup(Wrapper *wrapper)
{
    _wrapper = wrapper;

    ServiceBase::set_status(SERVICE_STARTING);

    return (XORP_OK);
}

int
XrlIO::reset()
{
    return restart(_admin_distance);
}

int
XrlIO::restart(uint32_t admin_dist)
{
    unregister_rib();
    ServiceBase::set_status(SERVICE_STARTING);
    register_rib(admin_dist);

    return (XORP_OK);
}

void XrlIO::set_admin_dist(uint32_t admin_dist)
{
    _reg_admin_ok = true;
    _admin_distance = admin_dist;
}

bool XrlIO::doReg()
{
    if (!_reg_admin_ok) return false;
    register_rib(_admin_distance);
    return true;
}

void
XrlIO::register_rib(uint32_t admin_dist)
{
    if (!_reg_admin_ok) {
        if (! _rib.send_set_protocol_admin_distance(
                    _ribname.c_str(),
                    _protocol,  // protocol
                    true,       // ipv4
                    false,      // ipv6
                    true,       // unicast
                    false,      // multicast
                    admin_dist, // admin_distance
                    callback(this,
                             &XrlIO::callbackStartup,true,
                             "set_protocol_admin_distance"))) {
            XLOG_WARNING("Failed to set admin distance in RIB");
        } else {
            _init_callback_done = false;
            while (!_init_callback_done) {
                _eventloop.run();
            }
        }
    }
    if (! _rib.send_add_igp_table4(
                _ribname.c_str(),                   // RIB target name
                _protocol,                          // protocol name
                _xrl_router.class_name(),           // our class
                _xrl_router.instance_name(),        // our instance
                true,                               // unicast
                false,                              // multicast
                callback(this,
                         &XrlIO::callbackStartup,true,
                         "add_igp_table4"))) {
        XLOG_FATAL("Failed to add table(s) to IPv4 RIB");
    } else {
        _init_callback_done = false;
        while (!_init_callback_done) {
            _eventloop.run();
        }
    }
}

void
XrlIO::unregister_rib()
{
    if (! _rib.send_delete_igp_table4(
                _ribname.c_str(),                   // RIB target name
                _protocol,                          // protocol name
                _xrl_router.class_name(),           // our class
                _xrl_router.instance_name(),        // our instance
                true,                               // unicast
                false,                              // multicast
                callback(this,
                         &XrlIO::callbackStartup,
                         false,
                         "delete_igp_table4"))) {
        XLOG_FATAL("Failed to delete Wrapper table(s) from IPv4 RIB");
    }
}

int
XrlIO::shutdown()
{
    return XORP_OK;
}

void
XrlIO::wshutdown()
{
    ServiceBase::set_status(SERVICE_SHUTTING_DOWN);
    unregister_rib();
}

void
XrlIO::callbackI(const XrlError &e)
{
    (_wrapper->*wrapper_callback)(e,NULL,0);
}

void
XrlIO::callbackIPv4(const XrlError &e, const IPv4 *data)
{
    (_wrapper->*wrapper_callback)(e,(void *)((data->str()).c_str()),(data->str().length()+1));
}

void
XrlIO::callbackStr(const XrlError &e, const string data)
{
    (_wrapper->*wrapper_callback)(e,(void *)(data.c_str()),data.length()+1);
}

void
XrlIO::callbackStrP(const XrlError &e, const string *data)
{
    if (data==NULL)
        (_wrapper->*wrapper_callback)(e,NULL,0);
    else {
        (_wrapper->*wrapper_callback)(e,(void *)(data->c_str()),data->length()+1);
    }
}

void
XrlIO::send_add_route(add_route_t * addroute, PolicyTags policytags,
                      void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;

//fprintf(stderr,"add route: %s dst: %s  next: %s %d  %s\n",_protocol.c_str(),addroute->dst,addroute->next_hop,(addroute->metric),addroute->ifname);

    const string cptl(_protocol);
    const string ifname(addroute->ifname);
    const string vifname(addroute->vifname);
    uint32_t 	mtc = (uint32_t)(addroute->metric);
    IPv4Net 	dst(addroute->dst);
    IPv4 	nexthop(addroute->next_hop);

    bool sent =  _rib.send_add_interface_route4(_ribname.c_str(),
                 _protocol,
                 addroute->unicast,
                 addroute->multicast,
                 dst,
                 dst.contains(nexthop)?IPv4::ZERO():nexthop,
                 ifname, // ifname
                 vifname, // vifname
                 mtc,
                 policytags.xrl_atomlist(),
                 callback(this,&XrlIO::callbackStr,cptl));

    if (!sent)
        fprintf(stderr,"rib add route %s failed",
                (addroute->dst));

}


void
XrlIO::send_del_route(del_route_t * delroute,
                      void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;

//fprintf(stderr,"del route: %s dst: %s  \n",_protocol.c_str(),delroute->dst);
    const string cptl(_protocol);
    const IPv4Net dst(delroute->dst);
    const bool uni(delroute->unicast);
    const bool mul(delroute->multicast);
    bool sent = _rib.send_delete_route4(_ribname.c_str(),
                                        cptl,
                                        uni,
                                        mul,
                                        dst,
                                        callback(this,&XrlIO::callbackStr,_protocol));
    if (!sent)
        fprintf(stderr,"rib delete route %s failed",
                (delroute->dst));

}

void
XrlIO::send_close(string xrl_sock, void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;
    bool sent = _xrl_socket.send_close(_feaname.c_str(),
                                       xrl_sock,
                                       callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to close %s\n",xrl_sock.c_str());
}

void
XrlIO::send_open_udp(int domain, int type, int protocol,
                     void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;
    bool sent = _xrl_socket.send_udp_open(_feaname.c_str(),
                                          _xrl_router.instance_name(),
                                          callback(this,&XrlIO::callbackStrP));
    if (!sent)
        fprintf(stderr,"fail to open udp %d %d %d\n",domain,type,protocol);
}

void
XrlIO::send_bind_udp(string xrl_sock, IPv4 local_addr, uint32_t local_port,
                     void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;
//fprintf(stderr,"bind:   %s  ip: %s  port: %d\n",xrl_sock.c_str(),local_addr.str().c_str(),local_port);
    bool sent = _xrl_socket.send_bind(_feaname.c_str(),
                                      xrl_sock,
                                      local_addr,
                                      local_port,
                                      callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to bind udp 0x%.8x %u\n",local_addr.addr(),local_port);
}

void
XrlIO::send_open_bind_udp(IPv4 local_addr, uint32_t local_port,
                          void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;
//fprintf(stderr,"Bind:   ip: %s  port: %d\n",local_addr.str().c_str(),local_port);
    bool sent = _xrl_socket.send_udp_open_and_bind(_feaname.c_str(),
                _xrl_router.instance_name(),
                local_addr,
                local_port,
                "",  		// local device
                true,    	// reuse
                callback(this,&XrlIO::callbackStrP));
    if (!sent)
        fprintf(stderr,"fail to open udp 0x%.8x %u\n",local_addr.addr(),local_port);
}

void
XrlIO::send_enable_recv(string xrl_sock,
                        void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;
    bool sent = _xrl_socket.send_udp_enable_recv(_feaname.c_str(),
                xrl_sock,
                callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to enable recv for %s \n",xrl_sock.c_str());
}

void
XrlIO::send_send_udp(string xrl_sock,IPv4 target_addr, uint32_t target_port, vector<uint8_t> payload,
                     void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    wrapper_callback = call_back;

    bool sent = _xrl_socket.send_send_to(_feaname.c_str(),
                                         xrl_sock,
                                         target_addr,
                                         target_port,
                                         payload,
                                         callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to send for %s 0x%.8x %u\n",xrl_sock.c_str(),target_addr.addr(),target_port);
}

void
XrlIO::send_socket_option(string xrl_sock, int level, string optionName, uint32_t optValue, int length,
                          void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    UNUSED(level);
    UNUSED(length);
    wrapper_callback = call_back;
    bool sent = _xrl_socket.send_set_socket_option(_feaname.c_str(),
                xrl_sock,
                optionName,
                optValue,
                callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to set option %s %s %d\n",xrl_sock.c_str(),optionName.c_str(),optValue);
}

void
XrlIO::send_socket_option_to(string xrl_sock,int level, string optname, string devName, int length,
                             void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t))
{
    UNUSED(level);
    UNUSED(length);
    wrapper_callback = call_back;
    bool sent = _xrl_socket.send_set_socket_option_txt(_feaname.c_str(),
                xrl_sock,
                optname,
                devName,
                callback(this,&XrlIO::callbackI));
    if (!sent)
        fprintf(stderr,"fail to set option %s %s %s\n",xrl_sock.c_str(),optname.c_str(),devName.c_str());
}

void
XrlIO::receive(const string& sockid, const string& if_name, const string& vif_name,
               const IPv4& src_host, const uint32_t& src_port, const vector<uint8_t>& data)
{
    xrl_recv_udp_t udppacket;
    string    srchost = src_host.str();
    udppacket.sock_id.len = sockid.length();
    memcpy(udppacket.sock_id.xorp_sock_name, sockid.c_str(), udppacket.sock_id.len+1);
    memcpy(udppacket.ifname, if_name.c_str(), if_name.length()+1);
    memcpy(udppacket.vifname,vif_name.c_str(), vif_name.length()+1);
    memcpy(udppacket.src_addr,srchost.c_str(),srchost.length()+1);
    udppacket.src_port = src_port;
    udppacket.data = &(*data.begin());
    udppacket.data_len = data.size();
    _wrapper->udp_recv(&udppacket);
}

void
XrlIO::push_routes()
{
    xrl_policy_t policy;
    policy.cmd = LDWRAPPER_PUSH_ROUTE;
    _wrapper->policy(&policy);
}

void
XrlIO::fromXorp(int cmd, string   network,  bool   unicast,
                bool   multicast, string   nexthop,  uint32_t  metric)
{
    xrl_policy_t policy;

    policy.cmd = cmd;
    if (cmd == POLICY_DEL_ROUTE) {
        policy.unicast = unicast;
        policy.multicast = multicast;
        memcpy(policy.network,network.c_str(),network.length()+1);
    } else if (cmd == POLICY_ADD_ROUTE) {
        policy.unicast = unicast;
        policy.multicast = multicast;
        memcpy(policy.network,network.c_str(),network.length()+1);
        memcpy(policy.next_hop,nexthop.c_str(),nexthop.length()+1);
        policy.metric = metric;
    }

    _wrapper->policy(&policy);
}
