/*
 * Copyright (c) 2001-2004 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * DO NOT EDIT THIS FILE - IT IS PROGRAMMATICALLY GENERATED
 *
 * Generated by 'clnt-gen'.
 *
 * $XORP: xorp/xrl/interfaces/fea_fib_client_xif.hh,v 1.3 2004/06/10 22:41:59 hodson Exp $
 */

#ifndef __XRL_INTERFACES_FEA_FIB_CLIENT_XIF_HH__
#define __XRL_INTERFACES_FEA_FIB_CLIENT_XIF_HH__

#undef XORP_LIBRARY_NAME
#define XORP_LIBRARY_NAME "XifFeaFibClient"

#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxipc/xrl.hh"
#include "libxipc/xrl_error.hh"
#include "libxipc/xrl_sender.hh"


class XrlFeaFibClientV0p1Client {
public:
    XrlFeaFibClientV0p1Client(XrlSender* s) : _sender(s) {}
    virtual ~XrlFeaFibClientV0p1Client() {}

    typedef XorpCallback1<void, const XrlError&>::RefPtr AddRoute4CB;
    /**
     *  Send Xrl intended to:
     *
     *  Notification of a route being added.
     *
     *  @param tgt_name Xrl Target name
     *
     *  @param network the network address prefix of the route to add.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the routing metric toward the destination.
     *
     *  @param admin_distance the administratively defined distance toward the
     *  destination.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  route.
     *
     *  @param xorp_route true if this route was installed by XORP.
     */
    bool send_add_route4(
	const char*	target_name,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route,
	const AddRoute4CB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr AddRoute6CB;

    bool send_add_route6(
	const char*	target_name,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route,
	const AddRoute6CB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr ReplaceRoute4CB;
    /**
     *  Send Xrl intended to:
     *
     *  Notification of a route being replaced.
     *
     *  @param tgt_name Xrl Target name
     *
     *  @param network the network address prefix of the route to replace.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the routing metric toward the destination.
     *
     *  @param admin_distance the administratively defined distance toward the
     *  destination.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  route.
     *
     *  @param xorp_route true if this route was installed by XORP.
     */
    bool send_replace_route4(
	const char*	target_name,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route,
	const ReplaceRoute4CB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr ReplaceRoute6CB;

    bool send_replace_route6(
	const char*	target_name,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route,
	const ReplaceRoute6CB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr DeleteRoute4CB;
    /**
     *  Send Xrl intended to:
     *
     *  Notification of a route being deleted.
     *
     *  @param tgt_name Xrl Target name
     *
     *  @param network the network address prefix of the route to delete.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     */
    bool send_delete_route4(
	const char*	target_name,
	const IPv4Net&	network,
	const string&	ifname,
	const string&	vifname,
	const DeleteRoute4CB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr DeleteRoute6CB;

    bool send_delete_route6(
	const char*	target_name,
	const IPv6Net&	network,
	const string&	ifname,
	const string&	vifname,
	const DeleteRoute6CB&	cb
    );

protected:
    XrlSender* _sender;

private:
    void unmarshall_add_route4(
	const XrlError&	e,
	XrlArgs*	a,
	AddRoute4CB		cb
    );

    void unmarshall_add_route6(
	const XrlError&	e,
	XrlArgs*	a,
	AddRoute6CB		cb
    );

    void unmarshall_replace_route4(
	const XrlError&	e,
	XrlArgs*	a,
	ReplaceRoute4CB		cb
    );

    void unmarshall_replace_route6(
	const XrlError&	e,
	XrlArgs*	a,
	ReplaceRoute6CB		cb
    );

    void unmarshall_delete_route4(
	const XrlError&	e,
	XrlArgs*	a,
	DeleteRoute4CB		cb
    );

    void unmarshall_delete_route6(
	const XrlError&	e,
	XrlArgs*	a,
	DeleteRoute6CB		cb
    );

};

#endif /* __XRL_INTERFACES_FEA_FIB_CLIENT_XIF_HH__ */
