// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_socket_server.hh,v 1.5 2004/04/22 01:11:50 pavlin Exp $

#ifndef __FEA_XRL_SOCKET_SERVER_HH__
#define __FEA_XRL_SOCKET_SERVER_HH__

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/service.hh"
#include "xrl/targets/socket_server_base.hh"

#include "fea/addr_table.hh"
#include "fea/xrl_socket_cmds.hh"

class EventLoop;
class XrlRouter;

class XrlSocketServer
    : public ServiceBase,
      public XrlSocketServerTargetBase,
      public AddressTableEventObserver
{
public:
    XrlSocketServer(EventLoop&		e,
		    AddressTableBase&	addr_table,
		    const IPv4&		finder_host,
		    uint16_t		finder_port);

    ~XrlSocketServer();

    /**
     * Start XrlSocketServer.
     *
     * Causes instance to register Xrls with the Finder subsequently
     * become operational.
     *
     * @return true on success, false on failure.
     */
    bool startup();

    /**
     * Shutdown XrlSocketServer.
     *
     * @return true on success, false on failure.
     */
    bool shutdown();

    /**
     * Get count of number of socket owners.  These are XrlTargets that own
     * one or more sockets.
     */
    uint32_t socket_owner_count() const;

    /**
     * Get count of number of IPv4 sockets open.
     */
    uint32_t ipv4_socket_count() const;

    /**
     * Get count of number of IPv6 sockets open.
     */
    uint32_t ipv6_socket_count() const;

    XrlCmdError common_0_1_get_target_name(string& name);

    XrlCmdError common_0_1_get_version(string& version);

    XrlCmdError common_0_1_get_status(uint32_t& status_code,
				      string&	reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError
    finder_event_observer_0_1_xrl_target_birth(const string& clsname,
					       const string& instance);

    XrlCmdError
    finder_event_observer_0_1_xrl_target_death(const string& clsname,
					       const string& instance);

    XrlCmdError socket4_0_1_tcp_open_and_bind(const string&	creator,
					      const IPv4&	local_addr,
					      const uint32_t&	local_port,
					      string&		sockid);

    XrlCmdError socket4_0_1_udp_open_and_bind(const string&	creator,
					      const IPv4&	local_addr,
					      const uint32_t&	local_port,
					      string&		sockid);

    XrlCmdError socket4_0_1_udp_open_bind_join(const string&	creator,
					       const IPv4&	local_addr,
					       const uint32_t&	local_port,
					       const IPv4&	mcast_addr,
					       const uint32_t&	ttl,
					       const bool&	reuse,
					       string&		sockid);

    XrlCmdError socket4_0_1_tcp_open_bind_connect(const string&	creator,
						  const IPv4&	local_addr,
						  const uint32_t& local_port,
						  const IPv4&	remote_addr,
						  const uint32_t& remote_port,
						  string&	sockid);

    XrlCmdError socket4_0_1_udp_open_bind_connect(const string&	creator,
						  const IPv4&	local_addr,
						  const uint32_t& local_port,
						  const IPv4&	remote_addr,
						  const uint32_t& remote_port,
						  string&	sockid);

    XrlCmdError socket4_0_1_udp_join_group(const string&	sockid,
					   const IPv4&		group,
					   const IPv4&		if_addr);

    XrlCmdError socket4_0_1_udp_leave_group(const string&	sockid,
					    const IPv4&	 	group,
					    const IPv4&	 	if_addr);

    XrlCmdError socket4_0_1_close(const string& sockid);

    XrlCmdError socket4_0_1_tcp_listen(const string&	sockid,
				       const uint32_t&	backlog);

    XrlCmdError socket4_0_1_send(const string&		sockid,
				 const vector<uint8_t>&	data);

    XrlCmdError socket4_0_1_send_with_flags(const string&	sockid,
					    const vector<uint8_t>& data,
					    const bool&		out_of_band,
					    const bool&		end_of_record,
					    const bool&		end_of_file);

    XrlCmdError socket4_0_1_send_to(const string&		sockid,
				    const IPv4&			remote_addr,
				    const uint32_t&		remote_port,
				    const vector<uint8_t>&	data);

    XrlCmdError socket4_0_1_send_to_with_flags(const string&	sockid,
					       const IPv4&	remote_addr,
					       const uint32_t&	remote_port,
					       const vector<uint8_t>&	data,
					       const bool&	out_of_band,
					       const bool&	end_of_record,
					       const bool&	end_of_file);

    XrlCmdError socket4_0_1_send_from_multicast_if(
					const string&	sockid,
					const IPv4&	group_addr,
					const uint32_t&	group_port,
					const IPv4&	if_addr,
					const vector<uint8_t>& data
					);

    XrlCmdError socket4_0_1_set_socket_option(const string&	sockid,
					      const string&	optname,
					      const uint32_t&	optval);

    XrlCmdError socket4_0_1_get_socket_option(const string&	sockid,
					      const string&	optname,
					      uint32_t&		optval);

    XrlCmdError socket6_0_1_tcp_open_and_bind(const string&	creator,
					      const IPv6&	local_addr,
					      const uint32_t&	local_port,
					      string&		sockid);

    XrlCmdError socket6_0_1_udp_open_and_bind(const string&	creator,
					      const IPv6&	local_addr,
					      const uint32_t&	local_port,
					      string&		sockid);

    XrlCmdError socket6_0_1_udp_open_bind_join(const string&	creator,
					       const IPv6&	local_addr,
					       const uint32_t&	local_port,
					       const IPv6&	mcast_addr,
					       const uint32_t&	ttl,
					       const bool&	reuse,
					       string&		sockid);

    XrlCmdError socket6_0_1_tcp_open_bind_connect(const string&	creator,
						  const IPv6&	local_addr,
						  const uint32_t& local_port,
						  const IPv6&	remote_addr,
						  const uint32_t& remote_port,
						  string&	sockid);

    XrlCmdError socket6_0_1_udp_open_bind_connect(const string&	creator,
						  const IPv6&	local_addr,
						  const uint32_t& local_port,
						  const IPv6&	remote_addr,
						  const uint32_t& remote_port,
						  string&	sockid);

    XrlCmdError socket6_0_1_udp_join_group(const string&	sockid,
					   const IPv6&		group,
					   const IPv6&		if_addr);

    XrlCmdError socket6_0_1_udp_leave_group(const string&	sockid,
					    const IPv6&	 	group,
					    const IPv6&	 	if_addr);

    XrlCmdError socket6_0_1_close(const string& sockid);

    XrlCmdError socket6_0_1_tcp_listen(const string&	sockid,
				       const uint32_t&	backlog);

    XrlCmdError socket6_0_1_send(const string&		sockid,
				 const vector<uint8_t>&	data);

    XrlCmdError socket6_0_1_send_with_flags(const string&	sockid,
					    const vector<uint8_t>& data,
					    const bool&		out_of_band,
					    const bool&		end_of_record,
					    const bool&		end_of_file);

    XrlCmdError socket6_0_1_send_to(const string&		sockid,
				    const IPv6&			remote_addr,
				    const uint32_t&		remote_port,
				    const vector<uint8_t>&	data);

    XrlCmdError socket6_0_1_send_to_with_flags(const string&	sockid,
					       const IPv6&	remote_addr,
					       const uint32_t&	remote_port,
					       const vector<uint8_t>&	data,
					       const bool&	out_of_band,
					       const bool&	end_of_record,
					       const bool&	end_of_file);

    XrlCmdError socket6_0_1_send_from_multicast_if(
					const string&		sockid,
					const IPv6&		group_addr,
					const uint32_t&		group_port,
					const IPv6&		if_addr,
					const vector<uint8_t>&	data
					);

    XrlCmdError socket6_0_1_set_socket_option(const string&	sockid,
					      const string&	optname,
					      const uint32_t&	optval);

    XrlCmdError socket6_0_1_get_socket_option(const string&	sockid,
					      const string&	optname,
					      uint32_t&		optval);

    const string& instance_name() const;

    void xrl_router_ready(const string& tgtname);

    void reject_connection(const string& sockid);
    void accept_connection(const string& sockid);

    inline EventLoop& eventloop()			{ return _e; }
    inline const EventLoop& eventloop()	const		{ return _e; }

    inline const AddressTableBase& address_table() const { return _atable; }

protected:
    void invalidate_address(const IPv4& addr, const string& why);
    void invalidate_address(const IPv6& addr, const string& why);

private:
    XrlSocketServer(const XrlSocketServer&);		// not implemented
    XrlSocketServer& operator=(const XrlSocketServer&);	// not implemented

protected:
    struct RemoteSocketOwner : public XrlSocketCommandDispatcher {
	RemoteSocketOwner(XrlSocketServer&	ss,
			  XrlSender&		xs,
			  const string&		target_name);
	~RemoteSocketOwner();
	inline const string& tgt_name() const		{ return _tname; }

	void incr_socket_count();
	void decr_socket_count();
	uint32_t socket_count() const;

	inline void set_watched(bool a)			{ _watched = a; }
	inline bool watched() const			{ return _watched; }

    protected:
	void xrl_cb(const XrlError& xe);

    private:
	XrlSocketServer& _ss;
	string		 _tname;
	uint32_t	 _sockets;
	bool		 _watched;
    };

public:
    RemoteSocketOwner* find_or_create_owner(const string& xrl_target_name);
    RemoteSocketOwner* find_owner(const string& xrl_target_name);
    void destroy_owner(const string& xrl_target);

    void add_owner_watch(const string& xrl_target_name);
    void add_owner_watch_cb(const XrlError& xe, string xrl_target_name);

    void remove_owner_watch(const string& xrl_target_name);
    void remove_owner_watch_cb(const XrlError& xe, string xrl_target_name);

    void remove_sockets_owned_by(const string& xrl_target_name);

public:
    template <typename A>
    struct RemoteSocket {
    public:
	RemoteSocket(XrlSocketServer&	ss,
		     RemoteSocketOwner& rso,
		     int		fd,
		     const A&		addr);
	~RemoteSocket();

	inline int  fd() const				{ return _fd;	      }
	inline const string& sockid() const		{ return _sockid;     }
	inline bool addr_is(const A& a) const		{ return a == _addr;  }
	inline const RemoteSocketOwner& owner() const	{ return _owner; }
	inline RemoteSocketOwner& owner()		{ return _owner; }

	void set_data_recv_enable(bool en);
	void data_sel_cb(int fd, SelectorMask);

	void set_connect_recv_enable(bool en);
	void connect_sel_cb(int fd, SelectorMask);

    protected:
	XrlSocketServer&   _ss;
	RemoteSocketOwner& _owner;
	int		   _fd;
	A		   _addr;
	string		   _sockid;

    private:
	RemoteSocket(const RemoteSocket& rs);
	RemoteSocket& operator=(const RemoteSocket& rs);
    };

    void push_socket(const ref_ptr<RemoteSocket<IPv4> >& s);
    void push_socket(const ref_ptr<RemoteSocket<IPv6> >& s);

public:
    typedef list<ref_ptr<RemoteSocket<IPv4> > > V4Sockets;
    typedef list<ref_ptr<RemoteSocket<IPv6> > > V6Sockets;

protected:
    EventLoop&			_e;
    XrlRouter*			_r;
    AddressTableBase&		_atable;

    map<string, RemoteSocketOwner> _socket_owners;
    V4Sockets			_v4sockets;
    V6Sockets			_v6sockets;
};

#endif // __FEA_XRL_SOCKET_SERVER_HH__
