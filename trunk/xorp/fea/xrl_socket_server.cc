// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/xrl_socket_server.cc,v 1.4 2004/01/16 19:05:43 hodson Exp $"

#include "fea_module.h"

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xuid.hh"
#include "libcomm/comm_api.h"
#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl_socket_server.hh"

static const char* NOT_RUNNING_MSG = "SocketServer is not operational.";

/**
 * XrlRouter that informs XrlSocketServer of events.
 */
class ChattyXrlStdRouter : public XrlStdRouter {
public:
    ChattyXrlStdRouter(EventLoop&	e,
		       const char*	class_name,
		       IPv4	   	finder_host,
		       uint16_t		finder_port,
		       XrlSocketServer* parent)
	: XrlStdRouter(e, class_name, finder_host, finder_port), _p(parent)
    {}

    void
    finder_connect_event()
    {
	debug_msg("Finder connect event");
    }

    void
    finder_disconnect_event()
    {
	debug_msg("Finder disconnect event");
    }

    void
    finder_ready_event(const string& tgt_name)
    {
	debug_msg("target %s ready\n", tgt_name.c_str());
	_p->xrl_router_ready(tgt_name);
    }

private:
    XrlSocketServer* _p;
};


// ----------------------------------------------------------------------------
// RemoteSocketOwner

XrlSocketServer::RemoteSocketOwner::RemoteSocketOwner(
					XrlSocketServer&	ss,
					XrlSender&		xs,
					const string&		tname)
    : XrlSocketCommandDispatcher(xs), _ss(ss), _tname(tname), _sockets(0),
      _watched(false)
{
    _ss.add_owner_watch(_tname);
}

XrlSocketServer::RemoteSocketOwner::~RemoteSocketOwner()
{
    if (_watched) {
	_ss.remove_owner_watch(_tname);
    }
}

void
XrlSocketServer::RemoteSocketOwner::incr_socket_count()
{
    _sockets++;
}

void
XrlSocketServer::RemoteSocketOwner::decr_socket_count()
{
    XLOG_ASSERT(_sockets != 0);
    _sockets--;

    // Arrange for instance to be cleared up if no xrls are pending
    if (queue_empty())
	_ss.destroy_owner(_tname);
}

uint32_t
XrlSocketServer::RemoteSocketOwner::socket_count() const
{
    return _sockets;
}

void
XrlSocketServer::RemoteSocketOwner::xrl_cb(const XrlError& xe)
{
    if (xe != XrlError::OKAY()) {
	XLOG_FATAL("Unhandled Xrl error: %s\n", xe.str().c_str());
    }
    if (send_next() == false && _sockets == 0) {
	_ss.destroy_owner(_tname);
    }
}

XrlSocketServer::RemoteSocketOwner*
XrlSocketServer::find_or_create_owner(const string& target)
{
    map<string, RemoteSocketOwner>::iterator i = _socket_owners.find(target);
    if (i != _socket_owners.end()) {
	return &i->second;
    }
    pair<map<string, RemoteSocketOwner>::iterator, bool> ins =
	_socket_owners.insert(pair<string,RemoteSocketOwner>(target, RemoteSocketOwner(*this, *_r, target)));
    if (ins.second == false)
	return 0;
    i = ins.first;
    return &i->second;
}

XrlSocketServer::RemoteSocketOwner*
XrlSocketServer::find_owner(const string& target)
{
    map<string, RemoteSocketOwner>::iterator i = _socket_owners.find(target);
    if (i != _socket_owners.end())
	return &i->second;
    return 0;
}

void
XrlSocketServer::destroy_owner(const string& target)
{
    map<string, RemoteSocketOwner>::iterator i = _socket_owners.find(target);
    if (i != _socket_owners.end()) {
	_socket_owners.erase(i);
    }
}

void
XrlSocketServer::add_owner_watch(const string& target)
{
    if (_r == 0)
	return;

    XrlFinderEventNotifierV0p1Client c(_r);
    c.send_register_instance_event_interest(
	"finder", _r->instance_name(), target,
	callback(this, &XrlSocketServer::add_owner_watch_cb, target)
	);
}

void
XrlSocketServer::add_owner_watch_cb(const XrlError& e, string target)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not add watch on Xrl Target %s\n", target.c_str());
	remove_sockets_owned_by(target);
	return;
    }

    RemoteSocketOwner* rso = find_owner(target);
    if (rso == 0) {
	XLOG_ERROR("Could not find target for which watch added %s.",
		   target.c_str());
	return;
    }
    rso->set_watched(true);
}

void
XrlSocketServer::remove_owner_watch(const string& target)
{
    if (_r == 0)
	return;

    XrlFinderEventNotifierV0p1Client c(_r);
    c.send_deregister_instance_event_interest(
	"finder", _r->instance_name(), target,
	callback(this, &XrlSocketServer::remove_owner_watch_cb, target)
	);
}

void
XrlSocketServer::remove_owner_watch_cb(const XrlError& e, string target)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not remove watch on Xrl Target %s\n", target.c_str());
    }
    RemoteSocketOwner* rso = find_owner(target);
    if (rso) {
	rso->set_watched(false);
    }
    remove_sockets_owned_by(target);
    XLOG_ASSERT(find_owner(target) == 0);
}

void
XrlSocketServer::remove_sockets_owned_by(const string& target)
{
    V4Sockets::iterator i4 = _v4sockets.begin();
    while (i4 != _v4sockets.end()) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->owner().tgt_name() == target) {
	    _v4sockets.erase(i4++);
	    continue;
	}
	i4++;
    }

    V6Sockets::iterator i6 = _v6sockets.begin();
    while (i6 != _v6sockets.end()) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->owner().tgt_name() == target) {
	    _v6sockets.erase(i6++);
	    continue;
	}
	i6++;
    }
}


// ----------------------------------------------------------------------------
// RemoteSocket<A>

template <typename A>
XrlSocketServer::RemoteSocket<A>::RemoteSocket<A>(XrlSocketServer& ss,
			      RemoteSocketOwner&		   owner,
			      int				   fd,
			      const A&				   addr)
    : _ss(ss), _owner(owner), _fd(fd), _addr(addr), _sockid(XUID().str())
{
    _owner.incr_socket_count();
}

template <typename A>
XrlSocketServer::RemoteSocket<A>::~RemoteSocket()
{
    EventLoop& e = _ss.eventloop();
    e.remove_selector(_fd);
    comm_close(_fd);
    _owner.decr_socket_count();
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::set_data_recv_enable(bool en)
{
    EventLoop& e = _ss.eventloop();
    if (en) {
	e.add_selector(_fd, SEL_RD,
		       callback(this, &RemoteSocket::data_sel_cb));
    } else {
	e.remove_selector(_fd);
    }
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::data_sel_cb(int fd, SelectorMask)
{
    XLOG_ASSERT(fd == _fd);
    //
    // Create command.  We use buffer associated with this command type
    // to read data into (to avoid a copy).
    //
    Socket4UserSendRecvEvent* cmd =
	new Socket4UserSendRecvEvent(owner().tgt_name(), sockid());

    struct sockaddr sa;
    socklen_t sa_len = sizeof(sa);

    // XXX buffer is overprovisioned for normal case and size hard-coded...
    // It get's resized a little later on to amount of data read.
    cmd->data().resize(8000);
    ssize_t rsz = recvfrom(fd, &cmd->data()[0], cmd->data().size(), 0,
			   &sa, &sa_len);

    if (rsz < 0) {
	delete cmd;
	ref_ptr<XrlSocketCommandBase> ecmd = new
	    Socket4UserSendErrorEvent(owner().tgt_name(),
				      sockid(), strerror(errno), false);
	owner().enqueue(ecmd);
	return;
    }

    cmd->data().resize(rsz);

    XLOG_ASSERT(sa.sa_family == AF_INET);
    const sockaddr_in& sin = reinterpret_cast<const sockaddr_in&>(sa);

    IPv4 	src_addr(sin);
    uint16_t 	src_port(ntohs(sin.sin_port));

    cmd->set_source_host(src_addr);
    cmd->set_source_port(src_port);

    owner().enqueue(cmd);
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::set_connect_recv_enable(bool en)
{
    EventLoop& e = _ss.eventloop();
    if (en) {
	e.remove_selector(_fd, SEL_RD,
			  callback(this, &RemoteSocket::connect_sel_cb));
    } else {
	e.remove_selector(_fd);
    }
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::connect_sel_cb(int fd, SelectorMask)
{
    XLOG_ASSERT(fd == _fd);

    sockaddr sa;
    socklen_t sa_len = sizeof(sa);
    int afd = accept(_fd, sa, &sa_len);
    if (afd < 0) {
	ref_ptr<XrlSocketCommandBase*> ecmd = new
	    Socket4UserSendErrorEvent(owner()->tgt_name(),
				      sockid(), strerror(errno), false);
	owner().enqueue(ecmd);
	return;
    }

    XLOG_ASSERT(sa.sa_family == AF_INET);
    const sockaddr_in& sin = reinterpret_cast<const sockaddr_in&>(sa);
    uint16_t sin_port = ntohs(sin.sin_port);

    _ss._v4sockets.push_back(new RemoteSocket<IPv4>(_ss, owner(), afd, sin));

    ref_ptr<XrlSocketCommandBase*> cmd =
	new Socket4UserSendConnectEvent(&_ss, _owner.tgt_name(),
					_sockid, sin, sin_port,
					_v4sockets.back()->sockid());
    owner().enqueue(cmd);
}

void
XrlSocketServer::accept_connection(const string& sockid)
{
    for (V4Sockets::iterator i4 = _v4sockets.begin();
	 i4 != _v4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    rs->set_data_recv_enable(true);
	    return;
	}
    }
    for (V6Sockets::iterator i6 = _v6sockets.begin();
	 i6 != _v6sockets.end(); ++i6) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->sockid() == sockid) {
	    rs->set_data_recv_enable(true);
	    return;
	}
    }
}

void
XrlSocketServer::reject_connection(const string& sockid)
{
    for (V4Sockets::iterator i4 = _v4sockets.begin();
	 i4 != _v4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    _v4sockets.erase(i4);
	    return;
	}
    }
    for (V6Sockets::iterator i6 = _v6sockets.begin();
	 i6 != _v6sockets.end(); ++i6) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->sockid() == sockid) {
	    _v6sockets.erase(i6);
	    return;
	}
    }
}

// ----------------------------------------------------------------------------
//

XrlSocketServer::XrlSocketServer(EventLoop&		e,
				 AddressTableBase&	atable,
				 const IPv4&		finder_host,
				 uint16_t		finder_port)
    : ServiceBase("Socket Server"), _e(e), _atable(atable)
{
    const char* class_name = "socket_server";
    _r = new ChattyXrlStdRouter(e, class_name, finder_host, finder_port, this);
    if (set_command_map(_r) == false) {
	XLOG_FATAL("Could not set command map.");
    }
    _atable.add_observer(this);
}

XrlSocketServer::~XrlSocketServer()
{
    _atable.remove_observer(this);
    set_command_map(0);
    delete _r;
    _r = 0;
}

void
XrlSocketServer::startup()
{
    _r->finalize();
    set_status(STARTING);
}

void
XrlSocketServer::shutdown()
{
    set_status(SHUTTING_DOWN);
}

uint32_t
XrlSocketServer::socket_owner_count() const
{
    return _socket_owners.size();
}

uint32_t
XrlSocketServer::ipv4_socket_count() const
{
    return _v4sockets.size();
}

uint32_t
XrlSocketServer::ipv6_socket_count() const
{
    return _v6sockets.size();
}


// ----------------------------------------------------------------------------
// common/0.1 interface implementation

XrlCmdError
XrlSocketServer::common_0_1_get_target_name(string& name)
{
    name = this->name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::common_0_1_get_version(string& version)
{
    version = this->version();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::common_0_1_get_status(uint32_t& status,
				       string&	 reason)
{
    // XXX TODO
    ProcessStatus p = PROC_READY;
    status = p;
    reason = "";
    return XrlCmdError::OKAY();
    // XXX
}

XrlCmdError
XrlSocketServer::common_0_1_shutdown()
{
    shutdown();
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// finder_event_observer/0.1 interface implementation

XrlCmdError
XrlSocketServer::finder_event_observer_0_1_xrl_target_birth(
    const string& clsname,
    const string& instance
    )
{
    debug_msg("birth event %s/%s\n", clsname.c_str(), instance.c_str());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::finder_event_observer_0_1_xrl_target_death(
    const string& clsname,
    const string& instance
    )
{
    debug_msg("death event %s/%s\n", clsname.c_str(), instance.c_str());

    RemoteSocketOwner* rso = find_owner(instance);
    if (rso) {
	rso->set_watched(false);
	remove_sockets_owned_by(instance);
	XLOG_ASSERT(find_owner(instance) == 0);
	// destroy_owner(instance);	// no-op, socket death clears up
    }

    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// socket4/0.1 implementation

static const string&
last_comm_error()
{
    static string last_err("Consult xlog output.");
    // Placeholder until libcomm has saner error handling.
    return last_err;
}

static bool
valid_addr_port(const AddressTableBase&	atable,
		const IPv4&		addr,
		const uint32_t&		port,
		string&			err)
{
    if (atable.address_valid(addr) == false) {
	err = "Invalid Address.";
	return false;
    }
    if (port > 0xffff) {
	err = "Port out of range.";
	return false;
    }
    return true;
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_open_and_bind(const string&	creator,
					       const IPv4&	local_addr,
					       const uint32_t&	local_port,
					       string&		sockid)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);
    int fd = comm_bind_tcp4(&ia, htons(local_port));
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _v4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    sockid = _v4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_and_bind(const string&	creator,
					       const IPv4&	local_addr,
					       const uint32_t&	local_port,
					       string&		sockid)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);
    int fd = comm_bind_udp4(&ia, htons(local_port));
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _v4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _v4sockets.back()->set_data_recv_enable(true);
    sockid = _v4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_bind_join(const string&	creator,
						const IPv4&	local_addr,
						const uint32_t&	local_port,
						const IPv4&	mcast_addr,
						const uint32_t&	ttl,
						const bool&	reuse,
						string&		sockid)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    in_addr grp;
    mcast_addr.copy_out(grp);

    int fd = comm_bind_join_udp4(&grp, &ia, htons(local_port), reuse);
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    if (comm_set_iface4(fd, &ia) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting interface.");
    }
    if (comm_set_ttl(fd, ttl) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting TTL failed.");
    }
    if (comm_set_loopback(fd, 0) != XORP_OK) {
	XLOG_WARNING("Could not turn off loopback.");
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner.");
    }
    _v4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _v4sockets.back()->set_data_recv_enable(true);
    sockid = _v4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_open_bind_connect(
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    string&		sockid
    )
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    int fd = comm_bind_tcp4(&ia, htons(local_port));
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    in_addr ra;
    remote_addr.copy_out(ra);
    if (comm_sock_connect4(fd, &ra, htons(remote_port)) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Connect failed.");
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _v4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _v4sockets.back()->set_data_recv_enable(true);
    sockid = _v4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_bind_connect(
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t& 	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    string&		sockid
    )
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    in_addr ra;
    remote_addr.copy_out(ra);

    int fd = comm_bind_connect_udp4(&ia, htons(local_port),
				    &ra, htons(remote_port));
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _v4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _v4sockets.back()->set_data_recv_enable(true);
    sockid = _v4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_close(const string& sockid)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::iterator i4;
    for (i4 = _v4sockets.begin(); i4 != _v4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    _v4sockets.erase(i4);
	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_listen(const string&	sockid,
					const uint32_t&	backlog)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::const_iterator ci;
    for (ci = _v4sockets.begin(); ci != _v4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int x = listen(rs->fd(), backlog);
	    if (x == 0) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}


XrlCmdError
XrlSocketServer::socket4_0_1_send(
    const string&		sockid,
    const vector<uint8_t>&	data
    )
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::const_iterator ci;
    for (ci = _v4sockets.begin(); ci != _v4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int out = send(rs->fd(), &data[0], data.size(), 0);
	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}

XrlCmdError
XrlSocketServer::socket4_0_1_send_with_flags(
    const string&		sockid,
    const vector<uint8_t>& 	data,
    const bool&			out_of_band,
    const bool&			end_of_record,
    const bool&			end_of_file
    )
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::const_iterator ci;
    for (ci = _v4sockets.begin(); ci != _v4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() != sockid)
	    continue;

	int flags = 0;
#ifdef MSG_OOB
	if (out_of_band)
	    flags |= MSG_OOB;
#else
	if (out_of_band)
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_OOB\n");
#endif

#ifdef MSG_EOR
	if (end_of_record)
	    flags |= MSG_EOR;
#else
	if (end_of_file)
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR\n");
#endif

#ifdef MSG_EOF
	if (end_of_file)
	    flags |= MSG_EOF;
#else
	if (end_of_file)
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF\n");
#endif

	int out = send(rs->fd(), &data[0], data.size(), flags);
	if (out == (int)data.size()) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}

XrlCmdError
XrlSocketServer::socket4_0_1_send_to(const string&		sockid,
				     const IPv4&		remote_addr,
				     const uint32_t&		remote_port,
				     const vector<uint8_t>&	data)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::const_iterator ci;
    for (ci = _v4sockets.begin(); ci != _v4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    sockaddr_in sai;
	    remote_addr.copy_out(sai);
	    if (remote_port > 0xffff) {
		return XrlCmdError::COMMAND_FAILED("Port out of range.");
	    }
	    sai.sin_port = htons(remote_port);
	    int out = sendto(rs->fd(), &data[0], data.size(), 0,
			     reinterpret_cast<const sockaddr*>(&sai),
			     sizeof(sai));

	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}

XrlCmdError
XrlSocketServer::socket4_0_1_send_to_with_flags(const string&	sockid,
						const IPv4&	remote_addr,
						const uint32_t&	remote_port,
						const vector<uint8_t>&	data,
						const bool&	out_of_band,
						const bool&	end_of_record,
						const bool&	end_of_file)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    V4Sockets::const_iterator ci;
    for (ci = _v4sockets.begin(); ci != _v4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() != sockid)
	    continue;

	sockaddr_in sai;
	remote_addr.copy_out(sai);
	if (remote_port > 0xffff) {
	    return XrlCmdError::COMMAND_FAILED("Port out of range.");
	}
	sai.sin_port = htons(static_cast<uint16_t>(remote_port));

	int flags = 0;
#ifdef MSG_OOB
	if (out_of_band)
	    flags |= MSG_OOB;
#else
	if (out_of_band)
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_OOB\n");
#endif

#ifdef MSG_EOR
	if (end_of_record)
	    flags |= MSG_EOR;
#else
	if (end_of_file)
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR\n");
#endif

#ifdef MSG_EOF
	if (end_of_file)
	    flags |= MSG_EOF;
#else
	if (end_of_file)
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF\n");
#endif
	int out = sendto(rs->fd(), &data[0], data.size(), flags,
			 reinterpret_cast<const sockaddr*>(&sai),
			 sizeof(sai));
	if (out == (int)data.size()) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED("Socket not found");
}

XrlCmdError
XrlSocketServer::socket4_0_1_set_socket_option(const string&	/* sockid */,
					       const string&	/* optname */,
					       const uint32_t&	/* optval */)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    return XrlCmdError::COMMAND_FAILED("Not implemented");
}

XrlCmdError
XrlSocketServer::socket4_0_1_get_socket_option(const string&	/* sockid */,
					       const string&	/* optname */,
					       uint32_t&	/* optval */)
{
    if (status() != RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    return XrlCmdError::COMMAND_FAILED("Not implemented");
}


// ----------------------------------------------------------------------------
// XrlSocketServer AddressTableEventObserver methods

void
XrlSocketServer::invalidate_address(const IPv4& addr, const string& why)
{
    V4Sockets::iterator i = _v4sockets.begin();
    while (i != _v4sockets.end()) {
	ref_ptr<RemoteSocket<IPv4> > rp4 = *i;
	if (rp4->addr_is(addr)) {
	    RemoteSocketOwner& o = rp4->owner();
	    o.enqueue(new
		      Socket4UserSendCloseEvent(o.tgt_name().c_str(),
						rp4->sockid(),
						why));
	    _v4sockets.erase(i++);
	    continue;
	}
	++i;
    }
}

void
XrlSocketServer::invalidate_address(const IPv6&, const string&)
{
    XLOG_ERROR("Not implemented.");
    // XXX Copy IPv4 code and s/4/6/.
}


const string&
XrlSocketServer::instance_name() const
{
    return _r->instance_name();
}

void
XrlSocketServer::xrl_router_ready(const string& iname)
{
    if (iname == _r->instance_name())
	set_status(RUNNING);
}

