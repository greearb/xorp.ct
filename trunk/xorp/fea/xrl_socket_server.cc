// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/xrl_socket_server.cc,v 1.38 2007/04/19 22:04:40 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"
#include "libxipc/xuid.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl_socket_server.hh"

static const char* NOT_RUNNING_MSG = "SocketServer is not operational.";
static const char* NOT_FOUND_MSG   = "Socket not found.";
static const char* NO_PIF_MSG 	   = "Could not find interface index "
				     "associated with address.";
static const char* NO_IPV6_MSG = "IPv6 is not available on host";

// XXX: This is a horrible hack. This file needs considerable
// cleanup before it can be merged with a cross-platform build
#ifdef HOST_OS_WINDOWS
#define _FEA_BUF_CONST_CAST(x)	\
	(const_cast<char *>(reinterpret_cast<const char *>((x))))
#else
#define _FEA_BUF_CONST_CAST(x)	(x)
#endif

/**
 * XrlRouter that informs XrlSocketServer of events.
 */
class ChattyXrlStdRouter : public XrlStdRouter {
public:
    ChattyXrlStdRouter(EventLoop&	eventloop,
		       const char*	class_name,
		       IPv4	   	finder_host,
		       uint16_t		finder_port,
		       XrlSocketServer* parent)
	: XrlStdRouter(eventloop, class_name, finder_host, finder_port),
	  _p(parent)
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
// Utility methods

/**
 * Extract IP port from sockaddr.
 */
template <typename A>
inline uint16_t sockaddr_ip_port(const sockaddr& sa);

template <>
inline uint16_t
sockaddr_ip_port<IPv4>(const sockaddr& sa)
{
    XLOG_ASSERT(sa.sa_family == IPv4::af());
    const sockaddr_in* sin = sockaddr2sockaddr_in(&sa);
    return ntohs(sin->sin_port);
}

template <>
inline uint16_t
sockaddr_ip_port<IPv6>(const sockaddr& sa)
{
    XLOG_ASSERT(sa.sa_family == IPv6::af());
    const sockaddr_in6* sin6 = sockaddr2sockaddr_in6(&sa);
    return ntohs(sin6->sin6_port);
}

template <typename A>
static bool
valid_addr_port(const AddressTableBase&	atable,
		const A&		addr,
		const uint32_t&		port,
		string&			err)
{
    if (atable.address_valid(addr) == false) {
	err = c_format("Invalid Address (%s).", addr.str().c_str());
	return false;
    }
    if (port > 0xffff) {
	err = c_format("Port %u out of range.", port);
	return false;
    }
    return true;
}


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

    //
    // Arrange for instance to be cleared up if no XRLs are pending
    // and there are no remaining sockets.
    //
    if (queue_empty() && _sockets == 0)
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
	XLOG_FATAL("Unhandled Xrl error: %s", xe.str().c_str());
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
	_socket_owners.insert(pair<string, RemoteSocketOwner>(
				  target, 
				  RemoteSocketOwner(*this, *_r, target)));
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
	//
	// Method remove_sockets_owned_by(target) needs to be called before
	// the owner is destroyed, otherwise remove_owner_watch (which calls 
	// remove_sockets_owned_by at an arbitrary future time) might find
	// RemoteSockets with their owners destroyed, thus any owner() accesses
	// will cause a segfault.
	//
	remove_sockets_owned_by(target);
	_socket_owners.erase(i);
    }
}

template <>
void
XrlSocketServer::destroy_socket<IPv4>(const string& sockid,
				      const IPv4& dummy_addr)
{
    IPv4Sockets::iterator i4;

    // XXX: dummy_addr is needed to get around a compilation issue on gcc-2.9x
    UNUSED(dummy_addr);

    for (i4 = _ipv4sockets.begin(); i4 != _ipv4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    _ipv4sockets.erase(i4);
	    return;
	}
    }
}

template <>
void
XrlSocketServer::destroy_socket<IPv6>(const string& sockid,
				      const IPv6& dummy_addr)
{
    IPv6Sockets::iterator i6;

    // XXX: dummy_addr is needed to get around a compilation issue on gcc-2.9x
    UNUSED(dummy_addr);

    for (i6 = _ipv6sockets.begin(); i6 != _ipv6sockets.end(); ++i6) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->sockid() == sockid) {
	    _ipv6sockets.erase(i6);
	    return;
	}
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
	callback(this, &XrlSocketServer::add_owner_watch_cb, target));
}

void
XrlSocketServer::add_owner_watch_cb(const XrlError& e, string target)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not add watch on Xrl Target %s", target.c_str());
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
	callback(this, &XrlSocketServer::remove_owner_watch_cb, target));
}

void
XrlSocketServer::remove_owner_watch_cb(const XrlError& e, string target)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Could not remove watch on Xrl Target %s", target.c_str());
    }
    RemoteSocketOwner* rso = find_owner(target);
    if (rso) {
    XLOG_ASSERT(rso->socket_count() == 0);
	rso->set_watched(false);
    }
    XLOG_ASSERT(find_owner(target) == 0);
}

void
XrlSocketServer::remove_sockets_owned_by(const string& target)
{
    IPv4Sockets::iterator i4 = _ipv4sockets.begin();
    while (i4 != _ipv4sockets.end()) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->owner().tgt_name() == target) {
	    _ipv4sockets.erase(i4++);
	    continue;
	}
	i4++;
    }

    IPv6Sockets::iterator i6 = _ipv6sockets.begin();
    while (i6 != _ipv6sockets.end()) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->owner().tgt_name() == target) {
	    _ipv6sockets.erase(i6++);
	    continue;
	}
	i6++;
    }
}


// ----------------------------------------------------------------------------
// RemoteSocket<A>

template <typename A>
XrlSocketServer::RemoteSocket<A>::RemoteSocket(XrlSocketServer&		ss,
					       RemoteSocketOwner&	owner,
					       XorpFd			fd,
					       const A&			addr)
    : _ss(ss), _owner(owner), _fd(fd), _addr(addr), _sockid(XUID().str())
{
    _owner.incr_socket_count();
}

template <typename A>
XrlSocketServer::RemoteSocket<A>::RemoteSocket(XrlSocketServer&	ss,
			      RemoteSocketOwner&		owner,
			      XorpFd				fd)
    : _ss(ss), _owner(owner), _fd(fd), _addr(A()), _sockid(XUID().str())
{
    _owner.incr_socket_count();
}

template <typename A>
XrlSocketServer::RemoteSocket<A>::~RemoteSocket()
{
    EventLoop& eventloop = _ss.eventloop();
    eventloop.remove_ioevent_cb(_fd);
    comm_close(_fd);
    _owner.decr_socket_count();
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::set_data_recv_enable(bool en)
{
    EventLoop& eventloop = _ss.eventloop();
    if (en) {
	debug_msg("Adding I/O event hook for %s\n", _fd.str().c_str());
	if (eventloop.add_ioevent_cb(_fd, IOT_READ,
			callback(this, &RemoteSocket::data_io_cb)) == false) {
	    XLOG_ERROR("FAILED TO ADD I/O CALLBACK %s", _fd.str().c_str());
	}
    } else {
	debug_msg("Removing I/O event hook for %s\n", _fd.str().c_str());
	eventloop.remove_ioevent_cb(_fd);
    }
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::data_io_cb(XorpFd fd, IoEventType)
{
    XLOG_ASSERT(fd == _fd);
    debug_msg("data_io_cb %s\n", fd.str().c_str());
    //
    // Create command.  We use buffer associated with this command type
    // to read data into (to avoid a copy).
    //
    SocketUserSendRecvEvent<A>* cmd =
	new SocketUserSendRecvEvent<A>(owner().tgt_name(), sockid());

    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);

    // XXX buffer is overprovisioned for normal case and size hard-coded...
    // It get's resized a little later on to amount of data read.
    cmd->data().resize(64000);
    ssize_t rsz = recvfrom(fd, XORP_BUF_CAST(&cmd->data()[0]),
			   cmd->data().size(), 0,
			   reinterpret_cast<struct sockaddr *>(&ss), &ss_len);

    if (rsz < 0) {
	delete cmd;
	ref_ptr<XrlSocketCommandBase> ecmd = new
	    SocketUserSendErrorEvent<A>(owner().tgt_name(),
					sockid(), strerror(errno), false);
	owner().enqueue(ecmd);
	return;
    }

    cmd->data().resize(rsz);

    // Establish socket type
    int so_type = comm_sock_get_type(fd);

    switch (so_type) {
    case SOCK_DGRAM:
    {
	XLOG_ASSERT(ss.ss_family == A::af());
	typename A::SockAddrType *sa =
	    reinterpret_cast<typename A::SockAddrType *>(&ss);
	socklen_t sa_len = sizeof(*sa);
	cmd->set_source(*sa, sa_len);
	break;
    }
    case SOCK_STREAM:
    {
	if (rsz == 0) {
	    // socket closed, need to call deletion of self
	    delete cmd;
	    ref_ptr<XrlSocketCommandBase> cmd =
		new SocketUserSendCloseEvent<A>(
		    owner().tgt_name(),
		    sockid(), "Remote host closed the connection.");
	    owner().enqueue(cmd);
	    _ss.destroy_socket(sockid(), _addr);
	    return;
	}

	struct sockaddr_storage newss;
	socklen_t newss_len = sizeof(newss);

	int error = getpeername(fd,
				reinterpret_cast<struct sockaddr *>(&newss),
				&newss_len);
	if (error != 0) {
	    ref_ptr<XrlSocketCommandBase> ecmd =
		new SocketUserSendErrorEvent<A>(owner().tgt_name(), sockid(),
						comm_get_last_error_str(),
						false);
		owner().enqueue(ecmd);
		return;
	}
	XLOG_ASSERT(newss.ss_family == A::af());
	typename A::SockAddrType *sa =
	    reinterpret_cast<typename A::SockAddrType *>(&newss);
	socklen_t sa_len = sizeof(*sa);
	cmd->set_source(*sa, sa_len);
	break;
    }
    default:
    {
	delete cmd;
	string error = "Invalid socket type %d", so_type;
	ref_ptr<XrlSocketCommandBase> ecmd =
	    new SocketUserSendErrorEvent<A>(owner().tgt_name(),
					    sockid(), error, false);
	owner().enqueue(ecmd);
	return;
    }
    }

    debug_msg("Command %s\n", cmd->str().c_str());
    owner().enqueue(cmd);
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::set_connect_recv_enable(bool en)
{
    EventLoop& eventloop = _ss.eventloop();
    if (en) {
	eventloop.add_ioevent_cb(_fd, IOT_ACCEPT,
				 callback(this, &RemoteSocket::accept_io_cb));
    } else {
	eventloop.remove_ioevent_cb(_fd);
    }
}

template <typename A>
void
XrlSocketServer::RemoteSocket<A>::accept_io_cb(XorpFd fd, IoEventType)
{
    XLOG_ASSERT(fd == _fd);

    XorpFd afd = comm_sock_accept(_fd);
    if (!afd.is_valid()) {
	ref_ptr<XrlSocketCommandBase> ecmd =
	    new SocketUserSendErrorEvent<A>(owner().tgt_name(), sockid(),
					    comm_get_last_error_str(), false);
	owner().enqueue(ecmd);
	return;
    }

    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);

    int error = getpeername(afd, (struct sockaddr *)&ss, &ss_len);
    if (error != 0) {
	ref_ptr<XrlSocketCommandBase> ecmd =
	    new SocketUserSendErrorEvent<A>(owner().tgt_name(), sockid(),
					    comm_get_last_error_str(), false);
	owner().enqueue(ecmd);
	return;
    }

    XLOG_ASSERT(ss.ss_family == A::af());
    sockaddr* sa = reinterpret_cast<sockaddr*>(&ss);
    A host(*sa);

    RemoteSocket<A>* remote_socket = new RemoteSocket<A>(_ss, owner(), afd,
							 host);
    _ss.push_socket(remote_socket);

    ref_ptr<XrlSocketCommandBase> cmd =
	new SocketUserSendConnectEvent<A>(&_ss, _owner.tgt_name(),
					  _sockid, host,
					  sockaddr_ip_port<A>(*sa),
					  remote_socket->sockid());
    owner().enqueue(cmd);
}

void
XrlSocketServer::accept_connection(const string& sockid)
{
    for (IPv4Sockets::iterator i4 = _ipv4sockets.begin();
	 i4 != _ipv4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    rs->set_data_recv_enable(true);
	    return;
	}
    }
    for (IPv6Sockets::iterator i6 = _ipv6sockets.begin();
	 i6 != _ipv6sockets.end(); ++i6) {
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
    for (IPv4Sockets::iterator i4 = _ipv4sockets.begin();
	 i4 != _ipv4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    _ipv4sockets.erase(i4);
	    return;
	}
    }
    for (IPv6Sockets::iterator i6 = _ipv6sockets.begin();
	 i6 != _ipv6sockets.end(); ++i6) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->sockid() == sockid) {
	    _ipv6sockets.erase(i6);
	    return;
	}
    }
}

void
XrlSocketServer::push_socket(const ref_ptr<RemoteSocket<IPv4> >& rs)
{
    _ipv4sockets.push_back(rs);
}

void
XrlSocketServer::push_socket(const ref_ptr<RemoteSocket<IPv6> >& rs)
{
    _ipv6sockets.push_back(rs);
}

// ----------------------------------------------------------------------------
// Helpers

template <typename A>
struct has_sockid {
    typedef XrlSocketServer::RemoteSocket<A> RemoteSocketType;

    has_sockid(const string* s) : _sockid(s) {}

    bool operator() (const ref_ptr<RemoteSocketType>& rps) const {
	return rps->sockid() == *_sockid;
    }
    bool operator() (ref_ptr<RemoteSocketType>& rps) {
	return rps->sockid() == *_sockid;
    }

private:
    const string* _sockid;
};


// ----------------------------------------------------------------------------
//

XrlSocketServer::XrlSocketServer(EventLoop&		eventloop,
				 AddressTableBase&	atable,
				 const IPv4&		finder_host,
				 uint16_t		finder_port)
    : ServiceBase("Socket Server"), _eventloop(eventloop), _atable(atable)
{
    const char* class_name = "socket_server";
    _r = new ChattyXrlStdRouter(eventloop, class_name, finder_host,
				finder_port, this);
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

bool
XrlSocketServer::startup()
{
    _r->finalize();
    set_status(SERVICE_STARTING);

    return true;
}

bool
XrlSocketServer::shutdown()
{
    set_status(SERVICE_SHUTDOWN);

    return true;
}

bool
XrlSocketServer::is_running() const
{
    if ((status() == SERVICE_READY)
	|| (status() == SERVICE_SHUTDOWN)
	|| (status() == SERVICE_FAILED)) {
	return (false);
    }

    return (true);
}

uint32_t
XrlSocketServer::socket_owner_count() const
{
    return _socket_owners.size();
}

uint32_t
XrlSocketServer::ipv4_socket_count() const
{
    return _ipv4sockets.size();
}

uint32_t
XrlSocketServer::ipv6_socket_count() const
{
    return _ipv6sockets.size();
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

    UNUSED(clsname);
    UNUSED(instance);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::finder_event_observer_0_1_xrl_target_death(
    const string& clsname,
    const string& instance
    )
{
    debug_msg("death event %s/%s\n", clsname.c_str(), instance.c_str());

    UNUSED(clsname);

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

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_open(const string&	creator,
				      const bool&	is_blocking,
				      string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    XorpFd fd = comm_open_tcp(AF_INET, is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd));
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open(const string&	creator,
				      const bool&	is_blocking,
				      string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    XorpFd fd = comm_open_udp(AF_INET, is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd));
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_bind(const string&	    creator,
				  const string&	    sockid,
				  const IPv4&	    local_addr,
				  const uint32_t&   local_port)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);
    
    UNUSED(creator);	// May use it in future using lookups via owner

    in_addr ia;
    local_addr.copy_out(ia);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int x = comm_sock_bind4(rs->fd(), &ia, htons(local_port));
	    // rs->set_connect_recv_enable(true);
	    if (x == XORP_OK) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_open_and_bind(const string&	creator,
					       const IPv4&	local_addr,
					       const uint32_t&	local_port,
					       const bool&	is_blocking,
					       string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (local_addr != IPv4::ANY() &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);
    XorpFd fd = comm_bind_tcp4(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_and_bind(const string&	creator,
					       const IPv4&	local_addr,
					       const uint32_t&	local_port,
					       const bool&	is_blocking,
					       string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (IPv4::ANY() != local_addr &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);
    XorpFd fd = comm_bind_udp4(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _ipv4sockets.back()->set_data_recv_enable(true);
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_bind_join(const string&	creator,
						const IPv4&	local_addr,
						const uint32_t&	local_port,
						const IPv4&	mcast_addr,
						const uint32_t&	ttl,
						const bool&	reuse,
						const bool&	is_blocking,
						string&		sockid)
{
    debug_msg("udp_open_bind_join(%s, %s, %u, %s, ttl=%u, reuse %d)\n",
	      creator.c_str(), local_addr.str().c_str(),
	      XORP_UINT_CAST(local_port),
	      mcast_addr.str().c_str(),
	      XORP_UINT_CAST(ttl),
	      reuse);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (local_addr != IPv4::ANY() &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    in_addr grp;
    mcast_addr.copy_out(grp);

    XorpFd fd = comm_bind_join_udp4(&grp, &ia, htons(local_port), reuse,
				    is_blocking);
    if (fd <= 0) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }
    debug_msg("fd = %s\n", fd.str().c_str());
    if (local_addr != IPv4::ANY() && comm_set_iface4(fd, &ia) != XORP_OK) {
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
    if (comm_set_reuseaddr(fd, true) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting reuse addr failed.");
    }
    if (comm_set_reuseport(fd, reuse) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting reuse addr failed.");
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner.");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _ipv4sockets.back()->set_data_recv_enable(true);
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_open_bind_connect(
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    const bool&		is_blocking,
    string&		sockid
    )
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    XorpFd fd = comm_bind_tcp4(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    in_addr ra;
    remote_addr.copy_out(ra);
    int in_progress = 0;
    if (comm_sock_connect4(fd, &ra, htons(remote_port), is_blocking,
			   &in_progress)
	!= XORP_OK) {
	if (is_blocking || (in_progress == 0)) {
	    comm_close(fd);
	    return XrlCmdError::COMMAND_FAILED("Connect failed.");
	}
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _ipv4sockets.back()->set_data_recv_enable(true);
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_open_bind_connect(
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t& 	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    const bool&		is_blocking,
    string&		sockid
    )
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia;
    local_addr.copy_out(ia);

    in_addr ra;
    remote_addr.copy_out(ra);

    int in_progress = 0;
    XorpFd fd = comm_bind_connect_udp4(&ia, htons(local_port),
				       &ra, htons(remote_port),
				       is_blocking, &in_progress);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv4sockets.push_back(new RemoteSocket<IPv4>(*this, *rso, fd, local_addr));
    _ipv4sockets.back()->set_data_recv_enable(true);
    sockid = _ipv4sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_join_group(const string&	sockid,
					    const IPv4&		group,
					    const IPv4&		if_addr)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci = find_if(_ipv4sockets.begin(),
					     _ipv4sockets.end(),
					     has_sockid<IPv4>(&sockid));
    if (ci == _ipv4sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, /* fake port */1, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia_group;
    group.copy_out(ia_group);

    in_addr ia_if_addr;
    if_addr.copy_out(ia_if_addr);

    const RemoteSocket<IPv4>* 	rs = ci->get();
    int				fd = rs->fd();

    if (comm_sock_join4(fd, &ia_group, &ia_if_addr) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_udp_leave_group(const string&	sockid,
					    const IPv4&		group,
					    const IPv4&		if_addr)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci = find_if(_ipv4sockets.begin(),
					     _ipv4sockets.end(),
					     has_sockid<IPv4>(&sockid));
    if (ci == _ipv4sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, /* fake port */1, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in_addr ia_group;
    group.copy_out(ia_group);

    in_addr ia_if_addr;
    if_addr.copy_out(ia_if_addr);

    const RemoteSocket<IPv4>* 	rs = ci->get();
    int				fd = rs->fd();

    if (comm_sock_leave4(fd, &ia_group, &ia_if_addr) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_close(const string& sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::iterator i4;
    for (i4 = _ipv4sockets.begin(); i4 != _ipv4sockets.end(); ++i4) {
	RemoteSocket<IPv4>* rs = i4->get();
	if (rs->sockid() == sockid) {
	    _ipv4sockets.erase(i4);
	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket4_0_1_tcp_listen(const string&	sockid,
					const uint32_t&	backlog)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int x = listen(rs->fd(), backlog);
	    rs->set_connect_recv_enable(true);
	    if (x == 0) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket4_0_1_send(
    const string&		sockid,
    const vector<uint8_t>&	data
    )
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int out = send(rs->fd(),
			   reinterpret_cast<char *>(const_cast<uint8_t *>(&data[0])),
			   data.size(), 0);
	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
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
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() != sockid)
	    continue;

	int flags = 0;

	if (out_of_band) {
#ifdef MSG_OOB
	    flags |= MSG_OOB;
#else
	    XLOG_WARNING("sendto with out_of_band, "
			 "but platform has no MSG_OOB");
#endif
	}

	if (end_of_record) {
#ifdef MSG_EOR
	    flags |= MSG_EOR;
#else
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR");
#endif
	}

	if (end_of_file) {
#ifdef MSG_EOF
	    flags |= MSG_EOF;
#else
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF");
#endif
	}

	int out = send(rs->fd(),
		       reinterpret_cast<char *>(const_cast<uint8_t *>(&data[0])),
			data.size(), flags);
	if (out == static_cast<int>(data.size())) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket4_0_1_send_to(const string&		sockid,
				     const IPv4&		remote_addr,
				     const uint32_t&		remote_port,
				     const vector<uint8_t>&	data)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
	RemoteSocket<IPv4>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    sockaddr_in sai;
	    remote_addr.copy_out(sai);
	    if (remote_port > 0xffff) {
		return XrlCmdError::COMMAND_FAILED("Port out of range.");
	    }
	    sai.sin_port = htons(remote_port);
	    int out = sendto(rs->fd(),
			     reinterpret_cast<char *>(const_cast<uint8_t *>(&data[0])),
			     data.size(), 0,
			     reinterpret_cast<const sockaddr*>(&sai),
			     sizeof(sai));

	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
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
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv4Sockets::const_iterator ci;
    for (ci = _ipv4sockets.begin(); ci != _ipv4sockets.end(); ++ci) {
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

	if (out_of_band) {
#ifdef MSG_OOB
	    flags |= MSG_OOB;
#else
	    XLOG_WARNING("sendto with out_of_band, "
			 "but platform has no MSG_OOB");
#endif
	}

	if (end_of_record) {
#ifdef MSG_EOR
	    flags |= MSG_EOR;
#else
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR");
#endif
	}

	if (end_of_file) {
#ifdef MSG_EOF
	    flags |= MSG_EOF;
#else
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF");
#endif
	}

	int out = sendto(rs->fd(), _FEA_BUF_CONST_CAST(&data[0]),
			 data.size(), flags,
			 reinterpret_cast<const sockaddr*>(&sai), sizeof(sai));
	if (out == static_cast<int>(data.size())) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket4_0_1_send_from_multicast_if(
					const string&		sockid,
					const IPv4&		group_addr,
					const uint32_t&		group_port,
					const IPv4&		if_addr,
					const vector<uint8_t>& 	data
					)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, group_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    IPv4Sockets::const_iterator ci = find_if(_ipv4sockets.begin(),
					     _ipv4sockets.end(),
					     has_sockid<IPv4>(&sockid));

    if (ci == _ipv4sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv4>* 	rs = ci->get();
    int				fd = rs->fd();

    // Save old multicast interface so it can be restored after send.
    uint32_t	oa;
    socklen_t	oa_len = sizeof(oa);
    if (0 != getsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
			XORP_SOCKOPT_CAST(&oa), &oa_len)) {
	XLOG_WARNING("Failed to get multicast interface.");
    }

    uint32_t a  = if_addr.addr();
    if (0 != setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
			XORP_SOCKOPT_CAST(&a), sizeof(a))) {
	return XrlCmdError::COMMAND_FAILED(
		"Could not set default multicast interface to " +
		if_addr.str() );
    }

    XrlCmdError r = socket4_0_1_send_to(sockid, group_addr, group_port, data);

    // Restore old multicast interface
    setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, XORP_SOCKOPT_CAST(&oa),
	       sizeof(oa));
    return r;
}

XrlCmdError
XrlSocketServer::socket4_0_1_set_socket_option(const string&	sockid,
					       const string&	optname,
					       const uint32_t&	optval)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    const char* o_cstr = optname.c_str();

    IPv4Sockets::const_iterator ci = find_if(_ipv4sockets.begin(),
					     _ipv4sockets.end(),
					     has_sockid<IPv4>(&sockid));

    if (ci == _ipv4sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv4>* 	rs = ci->get();
    int				fd = rs->fd();

    if (strcasecmp(o_cstr, "multicast_loopback") == 0) {
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,
		       const_cast<char*>(reinterpret_cast<const char*>(&optval)),
		       sizeof(optval)) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else if (strcasecmp(o_cstr, "multicast_ttl") == 0) {
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
			_FEA_BUF_CONST_CAST(&optval),
			sizeof(optval)) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket4_0_1_get_socket_option(const string&	sockid,
					       const string&	optname,
					       uint32_t&	optval)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    const char* o_cstr = optname.c_str();

    IPv4Sockets::const_iterator ci = find_if(_ipv4sockets.begin(),
					     _ipv4sockets.end(),
					     has_sockid<IPv4>(&sockid));

    if (ci == _ipv4sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv4>* 	rs = ci->get();
    int				fd = rs->fd();
    socklen_t			optlen = sizeof(optval);
    if (strcasecmp(o_cstr, "multicast_loopback") == 0) {
	if (getsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,
			_FEA_BUF_CONST_CAST(&optval), &optlen) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else if (strcasecmp(o_cstr, "multicast_ttl") == 0) {
	if (getsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		       XORP_SOCKOPT_CAST(&optval), &optlen) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// socket6/0.1 implementation

XrlCmdError
XrlSocketServer::socket6_0_1_tcp_open(const string&	creator,
				      const bool&	is_blocking,
				      string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    XorpFd fd = comm_open_tcp(AF_INET, is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd));
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_open(const string&	creator,
				      const bool&	is_blocking,
				      string&		sockid)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    XorpFd fd = comm_open_udp(AF_INET6, is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd));
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_bind(const string&	    creator,
				  const string&	    sockid,
				  const IPv6&	    local_addr,
				  const uint32_t&   local_port)
{
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);
    
    UNUSED(creator);	// May use it in future using lookups via owner

    in6_addr ia;
    local_addr.copy_out(ia);
    
    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int x = comm_sock_bind6(rs->fd(), &ia, htons(local_port));
	    // rs->set_connect_recv_enable(true);
	    if (x == XORP_OK) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_tcp_open_and_bind(const string&	creator,
					       const IPv6&	local_addr,
					       const uint32_t&	local_port,
					       const bool&	is_blocking,
					       string&		sockid)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (local_addr != IPv6::ANY() &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia;
    local_addr.copy_out(ia);
    XorpFd fd = comm_bind_tcp6(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd, local_addr));
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_open_and_bind(const string&	creator,
					       const IPv6&	local_addr,
					       const uint32_t&	local_port,
					       const bool&	is_blocking,
					       string&		sockid)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (local_addr != IPv6::ANY() &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia;
    local_addr.copy_out(ia);
    XorpFd fd = comm_bind_udp6(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd, local_addr));
    _ipv6sockets.back()->set_data_recv_enable(true);
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_open_bind_join(const string&	creator,
						const IPv6&	local_addr,
						const uint32_t&	local_port,
						const IPv6&	mcast_addr,
						const uint32_t&	ttl,
						const bool&	reuse,
						const bool&	is_blocking,
						string&		sockid)
{
    debug_msg("udp_open_bind_join(%s, %s, %u, %s, ttl=%u, reuse %d)\n",
	      creator.c_str(), local_addr.str().c_str(), local_port,
	      mcast_addr.str().c_str(), ttl, reuse);

    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (local_addr != IPv6::ANY() &&
	valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    uint32_t pif_index = _atable.address_pif_index(local_addr);
    if (pif_index == 0) {
	return XrlCmdError::COMMAND_FAILED(NO_PIF_MSG);
    }

    in6_addr grp;
    mcast_addr.copy_out(grp);

    XorpFd fd = comm_bind_join_udp6(&grp, pif_index, htons(local_port), reuse,
				    is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }
    debug_msg("fd = %s\n", fd.str().c_str());
    if (local_addr != IPv6::ANY() &&
	comm_set_iface6(fd, pif_index) != XORP_OK) {
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
    if (comm_set_reuseaddr(fd, true) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting reuse addr failed.");
    }
    if (comm_set_reuseport(fd, reuse) != XORP_OK) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Setting reuse addr failed.");
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner.");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd, local_addr));
    _ipv6sockets.back()->set_data_recv_enable(true);
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_tcp_open_bind_connect(
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    const IPv6&		remote_addr,
    const uint32_t&	remote_port,
    const bool&		is_blocking,
    string&		sockid
    )
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia;
    local_addr.copy_out(ia);

    XorpFd fd = comm_bind_tcp6(&ia, htons(local_port), is_blocking);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    in6_addr ra;
    remote_addr.copy_out(ra);
    int in_progress = 0;
    if (comm_sock_connect6(fd, &ra, htons(remote_port), is_blocking,
			   &in_progress)
	!= XORP_OK) {
	if (is_blocking || (in_progress == 0)) {
	    comm_close(fd);
	    return XrlCmdError::COMMAND_FAILED("Connect failed.");
	}
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd, local_addr));
    _ipv6sockets.back()->set_data_recv_enable(true);
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_open_bind_connect(
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t& 	local_port,
    const IPv6&		remote_addr,
    const uint32_t&	remote_port,
    const bool&		is_blocking,
    string&		sockid
    )
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, local_addr, local_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia;
    local_addr.copy_out(ia);

    in6_addr ra;
    remote_addr.copy_out(ra);

    int in_progress = 0;
    XorpFd fd = comm_bind_connect_udp6(&ia, htons(local_port),
				       &ra, htons(remote_port),
				       is_blocking, &in_progress);
    if (!fd.is_valid()) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    RemoteSocketOwner* rso = find_or_create_owner(creator);
    if (rso == 0) {
	comm_close(fd);
	return XrlCmdError::COMMAND_FAILED("Could not create owner");
    }
    _ipv6sockets.push_back(new RemoteSocket<IPv6>(*this, *rso, fd, local_addr));
    _ipv6sockets.back()->set_data_recv_enable(true);
    sockid = _ipv6sockets.back()->sockid();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_join_group(const string&	sockid,
					    const IPv6&		group,
					    const IPv6&		if_addr)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci = find_if(_ipv6sockets.begin(),
					     _ipv6sockets.end(),
					     has_sockid<IPv6>(&sockid));
    if (ci == _ipv6sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, /* fake port */1, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia_group;
    group.copy_out(ia_group);

    uint32_t pif_index = _atable.address_pif_index(if_addr);
    if (pif_index == 0) {
	return XrlCmdError::COMMAND_FAILED(NO_PIF_MSG);
    }

    const RemoteSocket<IPv6>* 	rs = ci->get();
    int				fd = rs->fd();

    if (comm_sock_join6(fd, &ia_group, pif_index) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_udp_leave_group(const string&	sockid,
					     const IPv6&	group,
					     const IPv6&	if_addr)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci = find_if(_ipv6sockets.begin(),
					     _ipv6sockets.end(),
					     has_sockid<IPv6>(&sockid));
    if (ci == _ipv6sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, /* fake port */1, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    in6_addr ia_group;
    group.copy_out(ia_group);

    uint32_t pif_index = _atable.address_pif_index(if_addr);
    if (pif_index == 0) {
	return XrlCmdError::COMMAND_FAILED(NO_PIF_MSG);
    }

    const RemoteSocket<IPv6>* 	rs = ci->get();
    int				fd = rs->fd();

    if (comm_sock_leave6(fd, &ia_group, pif_index) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(last_comm_error());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlSocketServer::socket6_0_1_close(const string& sockid)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::iterator i6;
    for (i6 = _ipv6sockets.begin(); i6 != _ipv6sockets.end(); ++i6) {
	RemoteSocket<IPv6>* rs = i6->get();
	if (rs->sockid() == sockid) {
	    _ipv6sockets.erase(i6);
	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_tcp_listen(const string&	sockid,
					const uint32_t&	backlog)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    if (comm_listen(rs->fd(), backlog) == XORP_OK) {
		rs->set_connect_recv_enable(true);
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_send(
    const string&		sockid,
    const vector<uint8_t>&	data
    )
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    int out = send(rs->fd(), _FEA_BUF_CONST_CAST(&data[0]),
			   data.size(), 0);
	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_send_with_flags(
    const string&		sockid,
    const vector<uint8_t>& 	data,
    const bool&			out_of_band,
    const bool&			end_of_record,
    const bool&			end_of_file
    )
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() != sockid)
	    continue;

	int flags = 0;

	if (out_of_band) {
#ifdef MSG_OOB
	    flags |= MSG_OOB;
#else
	    XLOG_WARNING("sendto with out_of_band, "
			 "but platform has no MSG_OOB");
#endif
	}

	if (end_of_record) {
#ifdef MSG_EOR
	    flags |= MSG_EOR;
#else
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR");
#endif
	}

	if (end_of_file) {
#ifdef MSG_EOF
	    flags |= MSG_EOF;
#else
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF");
#endif
	}

	int out = send(rs->fd(), _FEA_BUF_CONST_CAST(&data[0]),
		       data.size(), flags);
	if (out == static_cast<int>(data.size())) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_send_to(const string&		sockid,
				     const IPv6&		remote_addr,
				     const uint32_t&		remote_port,
				     const vector<uint8_t>&	data)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() == sockid) {
	    sockaddr_in6 sai;
	    remote_addr.copy_out(sai);
	    if (remote_port > 0xffff) {
		return XrlCmdError::COMMAND_FAILED("Port out of range.");
	    }
	    sai.sin6_port = htons(remote_port);
	    int out = sendto(rs->fd(), _FEA_BUF_CONST_CAST(&data[0]),
			     data.size(), 0,
			     reinterpret_cast<const sockaddr*>(&sai),
			     sizeof(sai));

	    if (out == (int)data.size()) {
		return XrlCmdError::OKAY();
	    }
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_send_to_with_flags(const string&	sockid,
						const IPv6&	remote_addr,
						const uint32_t&	remote_port,
						const vector<uint8_t>&	data,
						const bool&	out_of_band,
						const bool&	end_of_record,
						const bool&	end_of_file)
{
    if (comm_ipv6_present() != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);

    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    IPv6Sockets::const_iterator ci;
    for (ci = _ipv6sockets.begin(); ci != _ipv6sockets.end(); ++ci) {
	RemoteSocket<IPv6>* rs = ci->get();
	if (rs->sockid() != sockid)
	    continue;

	sockaddr_in6 sai;
	remote_addr.copy_out(sai);
	if (remote_port > 0xffff) {
	    return XrlCmdError::COMMAND_FAILED("Port out of range.");
	}
	sai.sin6_port = htons(static_cast<uint16_t>(remote_port));

	int flags = 0;

	if (out_of_band) {
#ifdef MSG_OOB
	    flags |= MSG_OOB;
#else
	    XLOG_WARNING("sendto with out_of_band, "
			 "but platform has no MSG_OOB");
#endif
	}

	if (end_of_record) {
#ifdef MSG_EOR
	    flags |= MSG_EOR;
#else
	    XLOG_WARNING("sendto with end_of_record, "
			 "but platform has no MSG_EOR");
#endif
	}

	if (end_of_file) {
#ifdef MSG_EOF
	    flags |= MSG_EOF;
#else
	    XLOG_WARNING("sendto with end_of_file, "
			 "but platform has no MSG_EOF");
#endif
	}

	int out = sendto(rs->fd(), _FEA_BUF_CONST_CAST(&data[0]),
			 data.size(), flags,
			 reinterpret_cast<const sockaddr*>(&sai), sizeof(sai));
	if (out == static_cast<int>(data.size())) {
	    return XrlCmdError::OKAY();
	}
	return XrlCmdError::COMMAND_FAILED(strerror(errno));
    }
    return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);
}

XrlCmdError
XrlSocketServer::socket6_0_1_send_from_multicast_if(
					const string&		sockid,
					const IPv6&		group_addr,
					const uint32_t&		group_port,
					const IPv6&		if_addr,
					const vector<uint8_t>& 	data
					)
{
#ifdef HAVE_IPV6
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    string err;
    if (valid_addr_port(_atable, if_addr, group_port, err) == false) {
	return XrlCmdError::COMMAND_FAILED(err);
    }

    uint32_t pi = _atable.address_pif_index(if_addr);
    if (pi == 0) {
	return XrlCmdError::COMMAND_FAILED(NO_PIF_MSG);
    }

    IPv6Sockets::const_iterator ci = find_if(_ipv6sockets.begin(),
					     _ipv6sockets.end(),
					     has_sockid<IPv6>(&sockid));

    if (ci == _ipv6sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv6>* 	rs = ci->get();
    int				fd = rs->fd();

    uint32_t  	old_pi;
    socklen_t 	old_pi_len = sizeof(old_pi);
    if (0 != getsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &old_pi,
			&old_pi_len)) {
	XLOG_WARNING("Failed to get multicast interface.");
    }

    if (0 != setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			XORP_SOCKOPT_CAST(&pi), sizeof(pi))) {
	return XrlCmdError::COMMAND_FAILED(
		"Could not set default multicast interface to " +
			 if_addr.str() );
    }

    XrlCmdError r = socket6_0_1_send_to(sockid, group_addr, group_port, data);

    // Restore old multicast interface
    setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
	       XORP_SOCKOPT_CAST(&old_pi), sizeof(old_pi));
    return r;
#else // ! HAVE_IPV6
    UNUSED(sockid);
    UNUSED(group_addr);
    UNUSED(group_port);
    UNUSED(if_addr);
    UNUSED(data);
    return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);
#endif // ! HAVE_IPV6
}

XrlCmdError
XrlSocketServer::socket6_0_1_set_socket_option(const string&	sockid,
					       const string&	optname,
					       const uint32_t&	optval)
{
#ifdef HAVE_IPV6
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    const char* o_cstr = optname.c_str();

    IPv6Sockets::const_iterator ci = find_if(_ipv6sockets.begin(),
					     _ipv6sockets.end(),
					     has_sockid<IPv6>(&sockid));

    if (ci == _ipv6sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv6>* 	rs = ci->get();
    int				fd = rs->fd();

    if (strcasecmp(o_cstr, "multicast_loopback") == 0) {
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       XORP_SOCKOPT_CAST(&optval), sizeof(optval)) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else if (strcasecmp(o_cstr, "multicast_hops") == 0) {
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       XORP_SOCKOPT_CAST(&optval), sizeof(optval)) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    return XrlCmdError::OKAY();
#else // ! HAVE_IPV6
    UNUSED(sockid);
    UNUSED(optname);
    UNUSED(optval);
    return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);
#endif // ! HAVE_IPV6
}

XrlCmdError
XrlSocketServer::socket6_0_1_get_socket_option(const string&	sockid,
					       const string&	optname,
					       uint32_t&	optval)
{
#ifdef HAVE_IPV6
    if (status() != SERVICE_RUNNING)
	return XrlCmdError::COMMAND_FAILED(NOT_RUNNING_MSG);

    const char* o_cstr = optname.c_str();

    IPv6Sockets::const_iterator ci = find_if(_ipv6sockets.begin(),
					     _ipv6sockets.end(),
					     has_sockid<IPv6>(&sockid));

    if (ci == _ipv6sockets.end())
	return XrlCmdError::COMMAND_FAILED(NOT_FOUND_MSG);

    const RemoteSocket<IPv6>* 	rs = ci->get();
    int				fd = rs->fd();
    socklen_t			optlen = sizeof(optval);
    if (strcasecmp(o_cstr, "multicast_loopback") == 0) {
	if (getsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       XORP_SOCKOPT_CAST(&optval), &optlen) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else if (strcasecmp(o_cstr, "multicast_hops") == 0) {
	if (getsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       XORP_SOCKOPT_CAST(&optval), &optlen) != 0) {
	    return XrlCmdError::COMMAND_FAILED(strerror(errno));
	}
    } else {
	return XrlCmdError::COMMAND_FAILED("Not implemented");
    }

    return XrlCmdError::OKAY();
#else // ! HAVE_IPV6
    UNUSED(sockid);
    UNUSED(optname);
    UNUSED(optval);
    return XrlCmdError::COMMAND_FAILED(NO_IPV6_MSG);
#endif // ! HAVE_IPV6
}


// ----------------------------------------------------------------------------
// XrlSocketServer AddressTableEventObserver methods

void
XrlSocketServer::invalidate_address(const IPv4& addr, const string& why)
{
    // This code is copied to:
    // 			invalidate_address(const IPv6&, const string&)
    IPv4Sockets::iterator i = _ipv4sockets.begin();
    while (i != _ipv4sockets.end()) {
	ref_ptr<RemoteSocket<IPv4> > rp4 = *i;
	if (rp4->addr_is(addr)) {
	    RemoteSocketOwner& o = rp4->owner();
	    o.enqueue(new
		      SocketUserSendCloseEvent<IPv4>(o.tgt_name().c_str(),
						     rp4->sockid(),
						     why));
	    _ipv4sockets.erase(i++);
	    continue;
	}
	++i;
    }
}

void
XrlSocketServer::invalidate_address(const IPv6& addr, const string& why)
{
    // This code is copied from:
    // 		invalidate_address(const IPv4&, const string&)
    IPv6Sockets::iterator i = _ipv6sockets.begin();
    while (i != _ipv6sockets.end()) {
	ref_ptr<RemoteSocket<IPv6> > rp6 = *i;
	if (rp6->addr_is(addr)) {
	    RemoteSocketOwner& o = rp6->owner();
	    o.enqueue(new
		      SocketUserSendCloseEvent<IPv6>(o.tgt_name().c_str(),
						     rp6->sockid(),
						     why));
	    _ipv6sockets.erase(i++);
	    continue;
	}
	++i;
    }
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
	set_status(SERVICE_RUNNING);
}

