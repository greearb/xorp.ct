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

#ifndef __WRAPPER_XRL_IO_HH__
#define __WRAPPER_XRL_IO_HH__

#include "libxipc/xrl_router.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/socket4_xif.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"


#include "io.hh"

class EventLoop;

class XrlIO : public IO {

//class XrlIO : public IO,
//              public ServiceChangeObserverBase {

public:

    /**
     * Construct an XrlIO instance.
     *
     * @param eventloop the event loop for the OLSR process.
     * @param xrl_router the name of the XRL router instance.
     * @param feaname the name of the FEA XRL target.
     * @param ribname the name of the RIB XRL target.
     */
    XrlIO(EventLoop& eventloop, XrlRouter& xrl_router,
          const string& feaname, const string& ribname, string protocol);

    /**
     * Destroy an XrlIO instance.
     */
    ~XrlIO();

    /**
     * Startup operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();
    int shutdown();
    int reset();

    bool doReg();
    int wstartup(Wrapper *wrapper);
    int restart(uint32_t admin_dist);
    void register_rib(uint32_t admin_dist);
    void wshutdown();
    void unregister_rib();
    void set_admin_dist(uint32_t admin_dist);

    void callbackI(const XrlError &e);
    void callbackIPv4(const XrlError &e, const IPv4 *data);
    void callbackStr(const XrlError &e, const string data);
    void callbackStrP(const XrlError &e, const string *data);

    void callbackStartup(const XrlError& error, bool up,
                         const char *comment);
    void forceclosecallback(const XrlError &e);
    void forceclose(string xrl_sock);

    void send_add_route(add_route_t * addroute, PolicyTags policytags,
                        void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_del_route(del_route_t * delroute,
                        void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_close(string xrl_sock, void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_open_udp(int domain, int type, int protocol,
                       void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_bind_udp(string xrl_sock, IPv4 local_addr, uint32_t local_port,
                       void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_open_bind_udp(IPv4 local_addr, uint32_t local_port,
                            void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_enable_recv(string xrl_sock,
                          void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_send_udp(string xrl_sock,IPv4 target_addr, uint32_t target_port, vector<uint8_t> payload,
                       void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_socket_option(string xrl_sock, int level, string optionName, uint32_t optValue, int length,
                            void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));
    void send_socket_option_to(string xrl_sock,int level, string optname, string devName, int length,
                               void (Wrapper::*call_back)(const XrlError &, const void *, uint32_t));


    void receive(const string& sockid, const string& if_name, const string& vif_name,
                 const IPv4& src_host, const uint32_t& src_port, const vector<uint8_t>& data);

    void push_routes();
    void fromXorp(int cmd, string network,  bool unicast, bool multicast, string nexthop, uint32_t metric);

private:
    EventLoop&          _eventloop;
    XrlRouter&          _xrl_router;
    string              _feaname;
    string              _ribname;
    string 		_protocol;

    uint32_t		_admin_distance;
    bool		_reg_admin_ok;

    XrlSocket4V0p1Client _xrl_socket;
    XrlRibV0p1Client 	_rib;
    bool		_init_callback_done;

    Wrapper * 		_wrapper;
    void (Wrapper::* wrapper_callback)(const XrlError &, const void *, uint32_t);

    /**
     * @short libfeaclient wrapper.
     */
    IfMgrXrlMirror      _ifmgr;

    /**
     * @short local copy of interface state obtained from libfeaclient.
     */
    IfMgrIfTree         _iftree;


};



#endif //__WRAPPER_XRL_IO_HH__
