/*
 * Copyright (c) 2001-2007 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * DO NOT EDIT THIS FILE - IT IS PROGRAMMATICALLY GENERATED
 *
 * Generated by 'clnt-gen'.
 */

#ident "$XORP: xorp/xrl/interfaces/socket6_xif.cc,v 1.19 2007/08/21 00:10:38 pavlin Exp $"

#include "socket6_xif.hh"

bool
XrlSocket6V0p1Client::send_tcp_open(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const TcpOpenCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/tcp_open");
    x.args().add("creator", creator);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_tcp_open, cb));
}


/* Unmarshall tcp_open */
void
XrlSocket6V0p1Client::unmarshall_tcp_open(
	const XrlError&	e,
	XrlArgs*	a,
	TcpOpenCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_udp_open(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const UdpOpenCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_open");
    x.args().add("creator", creator);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_open, cb));
}


/* Unmarshall udp_open */
void
XrlSocket6V0p1Client::unmarshall_udp_open(
	const XrlError&	e,
	XrlArgs*	a,
	UdpOpenCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_tcp_open_and_bind(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const TcpOpenAndBindCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/tcp_open_and_bind");
    x.args().add("creator", creator);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_tcp_open_and_bind, cb));
}


/* Unmarshall tcp_open_and_bind */
void
XrlSocket6V0p1Client::unmarshall_tcp_open_and_bind(
	const XrlError&	e,
	XrlArgs*	a,
	TcpOpenAndBindCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_udp_open_and_bind(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const UdpOpenAndBindCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_open_and_bind");
    x.args().add("creator", creator);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_open_and_bind, cb));
}


/* Unmarshall udp_open_and_bind */
void
XrlSocket6V0p1Client::unmarshall_udp_open_and_bind(
	const XrlError&	e,
	XrlArgs*	a,
	UdpOpenAndBindCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_udp_open_bind_join(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	mcast_addr,
	const uint32_t&	ttl,
	const bool&	reuse,
	const UdpOpenBindJoinCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_open_bind_join");
    x.args().add("creator", creator);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    x.args().add("mcast_addr", mcast_addr);
    x.args().add("ttl", ttl);
    x.args().add("reuse", reuse);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_open_bind_join, cb));
}


/* Unmarshall udp_open_bind_join */
void
XrlSocket6V0p1Client::unmarshall_udp_open_bind_join(
	const XrlError&	e,
	XrlArgs*	a,
	UdpOpenBindJoinCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_tcp_open_bind_connect(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	const TcpOpenBindConnectCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/tcp_open_bind_connect");
    x.args().add("creator", creator);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    x.args().add("remote_addr", remote_addr);
    x.args().add("remote_port", remote_port);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_tcp_open_bind_connect, cb));
}


/* Unmarshall tcp_open_bind_connect */
void
XrlSocket6V0p1Client::unmarshall_tcp_open_bind_connect(
	const XrlError&	e,
	XrlArgs*	a,
	TcpOpenBindConnectCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_udp_open_bind_connect(
	const char*	dst_xrl_target_name,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	const UdpOpenBindConnectCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_open_bind_connect");
    x.args().add("creator", creator);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    x.args().add("remote_addr", remote_addr);
    x.args().add("remote_port", remote_port);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_open_bind_connect, cb));
}


/* Unmarshall udp_open_bind_connect */
void
XrlSocket6V0p1Client::unmarshall_udp_open_bind_connect(
	const XrlError&	e,
	XrlArgs*	a,
	UdpOpenBindConnectCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e, 0);
	return;
    } else if (a && a->size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(1));
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    string sockid;
    try {
	a->get("sockid", sockid);
    } catch (const XrlArgs::BadArgs& e) {
	XLOG_ERROR("Error decoding the arguments: %s", e.str().c_str());
	cb->dispatch(XrlError::BAD_ARGS(), 0);
	return;
    }
    cb->dispatch(e, &sockid);
}

bool
XrlSocket6V0p1Client::send_bind(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const BindCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/bind");
    x.args().add("sockid", sockid);
    x.args().add("local_addr", local_addr);
    x.args().add("local_port", local_port);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_bind, cb));
}


/* Unmarshall bind */
void
XrlSocket6V0p1Client::unmarshall_bind(
	const XrlError&	e,
	XrlArgs*	a,
	BindCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_udp_join_group(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const IPv6&	mcast_addr,
	const IPv6&	join_if_addr,
	const UdpJoinGroupCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_join_group");
    x.args().add("sockid", sockid);
    x.args().add("mcast_addr", mcast_addr);
    x.args().add("join_if_addr", join_if_addr);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_join_group, cb));
}


/* Unmarshall udp_join_group */
void
XrlSocket6V0p1Client::unmarshall_udp_join_group(
	const XrlError&	e,
	XrlArgs*	a,
	UdpJoinGroupCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_udp_leave_group(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const IPv6&	mcast_addr,
	const IPv6&	leave_if_addr,
	const UdpLeaveGroupCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/udp_leave_group");
    x.args().add("sockid", sockid);
    x.args().add("mcast_addr", mcast_addr);
    x.args().add("leave_if_addr", leave_if_addr);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_udp_leave_group, cb));
}


/* Unmarshall udp_leave_group */
void
XrlSocket6V0p1Client::unmarshall_udp_leave_group(
	const XrlError&	e,
	XrlArgs*	a,
	UdpLeaveGroupCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_close(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const CloseCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/close");
    x.args().add("sockid", sockid);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_close, cb));
}


/* Unmarshall close */
void
XrlSocket6V0p1Client::unmarshall_close(
	const XrlError&	e,
	XrlArgs*	a,
	CloseCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_tcp_listen(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const uint32_t&	backlog,
	const TcpListenCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/tcp_listen");
    x.args().add("sockid", sockid);
    x.args().add("backlog", backlog);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_tcp_listen, cb));
}


/* Unmarshall tcp_listen */
void
XrlSocket6V0p1Client::unmarshall_tcp_listen(
	const XrlError&	e,
	XrlArgs*	a,
	TcpListenCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_send(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const vector<uint8_t>&	data,
	const SendCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/send");
    x.args().add("sockid", sockid);
    x.args().add("data", data);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_send, cb));
}


/* Unmarshall send */
void
XrlSocket6V0p1Client::unmarshall_send(
	const XrlError&	e,
	XrlArgs*	a,
	SendCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_send_to(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	const vector<uint8_t>&	data,
	const SendToCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/send_to");
    x.args().add("sockid", sockid);
    x.args().add("remote_addr", remote_addr);
    x.args().add("remote_port", remote_port);
    x.args().add("data", data);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_send_to, cb));
}


/* Unmarshall send_to */
void
XrlSocket6V0p1Client::unmarshall_send_to(
	const XrlError&	e,
	XrlArgs*	a,
	SendToCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_send_from_multicast_if(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const IPv6&	group_addr,
	const uint32_t&	group_port,
	const IPv6&	ifaddr,
	const vector<uint8_t>&	data,
	const SendFromMulticastIfCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/send_from_multicast_if");
    x.args().add("sockid", sockid);
    x.args().add("group_addr", group_addr);
    x.args().add("group_port", group_port);
    x.args().add("ifaddr", ifaddr);
    x.args().add("data", data);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_send_from_multicast_if, cb));
}


/* Unmarshall send_from_multicast_if */
void
XrlSocket6V0p1Client::unmarshall_send_from_multicast_if(
	const XrlError&	e,
	XrlArgs*	a,
	SendFromMulticastIfCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}

bool
XrlSocket6V0p1Client::send_set_socket_option(
	const char*	dst_xrl_target_name,
	const string&	sockid,
	const string&	optname,
	const uint32_t&	optval,
	const SetSocketOptionCB&	cb
)
{
    Xrl x(dst_xrl_target_name, "socket6/0.1/set_socket_option");
    x.args().add("sockid", sockid);
    x.args().add("optname", optname);
    x.args().add("optval", optval);
    return _sender->send(x, callback(this, &XrlSocket6V0p1Client::unmarshall_set_socket_option, cb));
}


/* Unmarshall set_socket_option */
void
XrlSocket6V0p1Client::unmarshall_set_socket_option(
	const XrlError&	e,
	XrlArgs*	a,
	SetSocketOptionCB		cb
)
{
    if (e != XrlError::OKAY()) {
	cb->dispatch(e);
	return;
    } else if (a && a->size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != %u)", XORP_UINT_CAST(a->size()), XORP_UINT_CAST(0));
	cb->dispatch(XrlError::BAD_ARGS());
	return;
    }
    cb->dispatch(e);
}
