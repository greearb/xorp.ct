// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#ifndef __FEA_IO_IP_MANAGER_HH__
#define __FEA_IO_IP_MANAGER_HH__


#include "libxorp/callback.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/xorpfd.hh"

#include "fea_io.hh"
#include "io_ip.hh"

class FeaDataPlaneManager;
class FeaNode;
class IoIpManager;


/**
 * Structure used to store commonly passed IPv4 and IPv6 header information.
 */
struct IPvXHeaderInfo {
    string	if_name;
    string	vif_name;
    IPvX	src_address;
    IPvX	dst_address;
    uint8_t	ip_protocol;
    int32_t	ip_ttl;
    int32_t	ip_tos;
    bool	ip_router_alert;
    bool	ip_internet_control;
    vector<uint8_t> ext_headers_type;
    vector<vector<uint8_t> > ext_headers_payload;
};

/**
 * A class that handles raw IP I/O communication for a specific protocol.
 *
 * It also allows arbitrary filters to receive the raw IP data for that
 * protocol.
 */
class IoIpComm :
    public NONCOPYABLE,
    public IoIpReceiver
{
public:
    /**
     * Filter class.
     */
    class InputFilter {
    public:
	InputFilter(IoIpManager&	io_ip_manager,
		    const string&	receiver_name,
		    uint8_t		ip_protocol)
	    : _io_ip_manager(io_ip_manager),
	      _receiver_name(receiver_name),
	      _ip_protocol(ip_protocol)
	{}
	virtual ~InputFilter() {}

	/**
	 * Get a reference to the I/O IP manager.
	 *
	 * @return a reference to the I/O IP manager.
	 */
	IoIpManager& io_ip_manager() { return (_io_ip_manager); }

	/**
	 * Get a const reference to the I/O IP manager.
	 *
	 * @return a const reference to the I/O IP manager.
	 */
	const IoIpManager& io_ip_manager() const { return (_io_ip_manager); }

	/**
	 * Get the receiver name.
	 *
	 * @return the receiver name.
	 */
	const string& receiver_name() const { return (_receiver_name); }

	/**
	 * Get the IP protocol.
	 *
	 * @return the IP protocol.
	 */
	uint8_t ip_protocol() const { return (_ip_protocol); }

	/**
	 * Method invoked when data arrives on associated IoIpComm instance.
	 */
	virtual void recv(const struct IPvXHeaderInfo& header,
			  const vector<uint8_t>& payload) = 0;

	/**
	 * Method invoked when a multicast forwarding related upcall is
	 * received from the system.
	 */
	virtual void recv_system_multicast_upcall(const vector<uint8_t>& payload) = 0;

	/**
	 * Method invoked by the destructor of the associated IoIpComm
	 * instance. This method provides the InputFilter with the
	 * opportunity to delete itself or update its state.
	 * The input filter does not need to call IoIpComm::remove_filter()
	 * since filter removal is automatically conducted.
	 */
	virtual void bye() = 0;

    private:
	IoIpManager&	_io_ip_manager;
	string		_receiver_name;
	uint8_t		_ip_protocol;
    };

    /**
     * Joined multicast group class.
     */
    class JoinedMulticastGroup {
    public:
	JoinedMulticastGroup(const string& if_name, const string& vif_name,
			     const IPvX& group_address)
	    : _if_name(if_name),
	      _vif_name(vif_name),
	      _group_address(group_address)
	{}
#ifdef XORP_USE_USTL
	JoinedMulticastGroup() { }
#endif
	virtual ~JoinedMulticastGroup() {}

	const string& if_name() const { return _if_name; }
	const string& vif_name() const { return _vif_name; }
	const IPvX& group_address() const { return _group_address; }

	/**
	 * Less-Than Operator
	 *
	 * @param other the right-hand operand to compare against.
	 * @return true if the left-hand operand is numerically smaller
	 * than the right-hand operand.
	 */
	bool operator<(const JoinedMulticastGroup& other) const {
	    if (_if_name != other._if_name)
		return (_if_name < other._if_name);
	    if (_vif_name != other._vif_name)
		return (_vif_name < other._vif_name);
	    return (_group_address < other._group_address);
	}

	/**
	 * Equality Operator
	 *
	 * @param other the right-hand operand to compare against.
	 * @return true if the left-hand operand is numerically same as the
	 * right-hand operand.
	 */
	bool operator==(const JoinedMulticastGroup& other) const {
	    return ((_if_name == other._if_name) &&
		    (_vif_name == other._vif_name) &&
		    (_group_address == other._group_address));
	}

	/**
	 * Add a receiver.
	 *
	 * @param receiver_name the name of the receiver to add.
	 */
	void add_receiver(const string& receiver_name) {
	    _receivers.insert(receiver_name);
	}

	/**
	 * Delete a receiver.
	 *
	 * @param receiver_name the name of the receiver to delete.
	 */
	void delete_receiver(const string& receiver_name) {
	    _receivers.erase(receiver_name);
	}

	/**
	 * @return true if there are no receivers associated with this group.
	 */
	bool empty() const { return _receivers.empty(); }

	set<string>& get_receivers() { return _receivers; }

    private:
	string		_if_name;
	string		_vif_name;
	IPvX		_group_address;
	set<string>	_receivers;
    };

public:
    /**
     * Constructor for IoIpComm.
     *
     * @param io_ip_manager the corresponding I/O IP manager
     * (@ref IoIpManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param ip_protocol the IP protocol number (IPPROTO_*).
     */
    IoIpComm(IoIpManager& io_ip_manager, const IfTree& iftree, int family,
	     uint8_t ip_protocol);

    /**
     * Virtual destructor.
     */
    virtual ~IoIpComm();

    /**
     * Allocate the I/O IP plugins (one per data plane manager).
     */
    void allocate_io_ip_plugins();

    /**
     * Deallocate the I/O IP plugins (one per data plane manager).
     */
    void deallocate_io_ip_plugins();

    /**
     * Allocate an I/O IP plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void allocate_io_ip_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Deallocate the I/O IP plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void deallocate_io_ip_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Start all I/O IP plugins.
     */
    void start_io_ip_plugins();

    /**
     * Stop all I/O IP plugins.
     */
    void stop_io_ip_plugins();

    /**
     * Add a filter to list of input filters.
     *
     * The IoIpComm class assumes that the callee will be responsible for
     * managing the memory associated with the filter and will call
     * remove_filter() if the filter is deleted or goes out of scope.
     *
     * @param filter the filter to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_filter(InputFilter* filter);

    /**
     * Remove filter from list of input filters.
     *
     * @param filter the filter to remove.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_filter(InputFilter* filter);

    /**
     * @return true if there are no filters associated with this instance.
     */
    bool no_input_filters() const { return _input_filters.empty(); }

    /**
     * Send a raw IP packet.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * the TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (IP traffic class for IPv6).
     * If it has a negative value, the TOS will be
     * set internally before transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_packet(const string&	if_name,
			    const string&	vif_name,
			    const IPvX&		src_address,
			    const IPvX&		dst_address,
			    int32_t		ip_ttl,
			    int32_t		ip_tos,
			    bool		ip_router_alert,
			    bool		ip_internet_control,
			    const vector<uint8_t>& ext_headers_type,
			    const vector<vector<uint8_t> >& ext_headers_payload,
			    const vector<uint8_t>& payload,
			    string&		error_msg);

    /**
     * Received a raw IP packet.
     *
     * @param if_name the interface name the packet arrived on.
     * @param vif_name the vif name the packet arrived on.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * then the received value is unknown.
     * @param ip_tos The type of service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, then the received value is unknown.
     * @param ip_router_alert if true, the IP Router Alert option was
     * included in the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param packet the payload, everything after the IP header and
     * options.
     */
    virtual void recv_packet(const string&	if_name,
			     const string&	vif_name,
			     const IPvX&	src_address,
			     const IPvX&	dst_address,
			     int32_t		ip_ttl,
			     int32_t		ip_tos,
			     bool		ip_router_alert,
			     bool		ip_internet_control,
			     const vector<uint8_t>& ext_headers_type,
			     const vector<vector<uint8_t> >& ext_headers_payload,
			     const vector<uint8_t>& payload);

    /**
     * Received a multicast forwarding related upcall from the system.
     *
     * Examples of such upcalls are: "nocache", "wrongiif", "wholepkt",
     * "bw_upcall".
     *
     * @param payload the payload data for the upcall.
     */
    virtual void recv_system_multicast_upcall(const vector<uint8_t>& payload);

    /**
     * Create input socket.
     *
     * @param if_name the name of the interface to listen on
     * @param vif_name the name of the vif to listen on
     * @error error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR
     */
    void create_input_socket(const string& if_name, const string& vif_name);

    /**
     * Join an IP multicast group.
     *
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param group_address the multicast group address to join.
     * @param receiver_name the name of the receiver.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(const string&	if_name,
				     const string&	vif_name,
				     const IPvX&	group_address,
				     const string&	receiver_name,
				     string&		error_msg);

    /**
     * Leave an IP multicast group.
     *
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param group_address the multicast group address to leave.
     * @param receiver_name the name of the receiver.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(const string&	if_name,
				      const string&	vif_name,
				      const IPvX&	group_address,
				      const string&	receiver_name,
				      string&		error_msg);

    /**
     * Leave all IP multicast groups on this interface.
     *
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int leave_all_multicast_groups(const string& if_name,
			      const string& vif_name,
			      string& error_msg);

    /**
     * Get the IP protocol.
     *
     * @return the IP protocol.
     */
    uint8_t ip_protocol() const { return (_ip_protocol); }

    /**
     * Get the first valid file descriptor for receiving protocol messages.
     *
     * @return the first valid file descriptor for receiving protocol
     * messages.
     */
    XorpFd first_valid_mcast_protocol_fd_in(const string& local_dev);

private:
    IoIpComm(const IoIpComm&);			// Not implemented.
    IoIpComm& operator=(const IoIpComm&);	// Not implemented.

    IoIpManager&		_io_ip_manager;
    const IfTree&		_iftree;
    const int			_family;
    const uint8_t		_ip_protocol;

    typedef list<pair<FeaDataPlaneManager*, IoIp*> >IoIpPlugins;
    IoIpPlugins			_io_ip_plugins;

    list<InputFilter*>		_input_filters;
    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

/**
 * @short Class that implements the API for sending IP packet to a
 * receiver.
 */
class IoIpManagerReceiver {
public:
    /**
     * Virtual destructor.
     */
    virtual ~IoIpManagerReceiver() {}

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the
     * IP packet to.
     * @param header the IP header information.
     * @param payload the payload, everything after the IP header
     * and options.
     */
    virtual void recv_event(const string&			receiver_name,
			    const struct IPvXHeaderInfo&	header,
			    const vector<uint8_t>&		payload) = 0;
};

/**
 * @short A class that manages raw IP I/O.
 *
 * The IoIpManager has two containers: a container for IP protocol handlers
 * (@ref IoIpComm) indexed by the protocol associated with the handler, and
 * a container for the filters associated with each receiver_name.  When
 * a receiver registers for interest in a particular type of raw
 * packet a handler (@ref IoIpComm) is created if necessary, then the
 * relevent filter is created and associated with the IoIpComm.
 */
class IoIpManager : public IoIpManagerReceiver,
		    public InstanceWatcher {
public:
    typedef XorpCallback2<int, const uint8_t*, size_t>::RefPtr UpcallReceiverCb;

    /**
     * Constructor for IoIpManager.
     */
    IoIpManager(FeaNode& fea_node, const IfTree& iftree);

    /**
     * Virtual destructor.
     */
    virtual ~IoIpManager();

    /**
     * Send a raw IP packet.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_protocol the IP protocol number. It must be between 1 and
     * 255.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     * TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (IP traffic class for IPv6). If it
     * has a negative value, the TOS will be set internally before
     * transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const string&	if_name,
	     const string&	vif_name,
	     const IPvX&	src_address,
	     const IPvX&	dst_address,
	     uint8_t		ip_protocol,
	     int32_t		ip_ttl,
	     int32_t		ip_tos,
	     bool		ip_router_alert,
	     bool		ip_internet_control,
	     const vector<uint8_t>& ext_headers_type,
	     const vector<vector<uint8_t> >& ext_headers_payload,
	     const vector<uint8_t>&	payload,
	     string&		error_msg);

    /**
     * Register to receive IP packets.
     *
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param enable_multicast_loopback if true then enable delivering of
     * multicast datagrams back to this host (assuming the host is a member of
     * the same multicast group.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_receiver(int		family,
			  const string&	receiver_name,
			  const string&	if_name,
			  const string&	vif_name,
			  uint8_t	ip_protocol,
			  bool		enable_multicast_loopback,
			  string&	error_msg);

    /**
     * Unregister to receive IP packets.
     *
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP Protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_receiver(int			family,
			    const string&	receiver_name,
			    const string&	if_name,
			    const string&	vif_name,
			    uint8_t		ip_protocol,
			    string&		error_msg);

    /**
     * Join an IP multicast group.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param group_address the multicast group address to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int join_multicast_group(const string&	receiver_name,
			     const string&	if_name,
			     const string&	vif_name,
			     uint8_t		ip_protocol,
			     const IPvX&	group_address,
			     string&		error_msg);

    /**
     * Leave an IP multicast group.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     * @param group_address the multicast group address to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int leave_multicast_group(const string&	receiver_name,
			      const string&	if_name,
			      const string&	vif_name,
			      uint8_t		ip_protocol,
			      const IPvX&	group_address,
			      string&		error_msg);

    /** Leave all multicast groups on this vif */
    int leave_all_multicast_groups(const string& if_name,
				   const string& vif_name,
				   string& error_msg);

    /**
     * Register to receive multicast forwarding related upcalls from the
     * system.
     *
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param receiver_cb the receiver callback to be invoked when an
     * upcall is received.
     * @param receiver_fd the return-by-reference file descriptor for
     * the socket that receives the upcalls.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_system_multicast_upcall_receiver(int		family,
						  uint8_t	ip_protocol,
						  IoIpManager::UpcallReceiverCb receiver_cb,
						  XorpFd&	mcast_receiver_fd,
						  const string& local_dev,
						  string&	error_msg);

    /**
     * Unregister to receive multicast forwarding related upcalls from the
     * system.
     *
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param ip_protocol the IP Protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_system_multicast_upcall_receiver(int		family,
						    uint8_t	ip_protocol,
						    string&	error_msg);

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the IP packet to.
     * @param header the IP header information.
     * @param payload the payload, everything after the IP header and options.
     */
    void recv_event(const string&			receiver_name,
		    const struct IPvXHeaderInfo&	header,
		    const vector<uint8_t>&		payload);

    /**
     * Inform the watcher that a component instance is alive.
     *
     * @param instance_name the name of the instance that is alive.
     */
    void instance_birth(const string& instance_name);

    /**
     * Inform the watcher that a component instance is dead.
     *
     * @param instance_name the name of the instance that is dead.
     */
    void instance_death(const string& instance_name);

    /**
     * Set the instance that is responsible for sending IP packets
     * to a receiver.
     */
    void set_io_ip_manager_receiver(IoIpManagerReceiver* v) {
	_io_ip_manager_receiver = v;
    }

    /**
     * Get a reference to the interface tree.
     *
     * @return a reference to the interface tree (@ref IfTree).
     */
    const IfTree&	iftree() const { return _iftree; }

    /**
     * Register @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to register.
     * @param is_exclusive if true, the manager is registered as the
     * exclusive manager, otherwise is added to the list of managers.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
				    bool is_exclusive);

    /**
     * Unregister @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Get the list of registered data plane managers.
     *
     * @return the list of registered data plane managers.
     */
    list<FeaDataPlaneManager*>& fea_data_plane_managers() {
	return _fea_data_plane_managers;
    }

private:
    typedef map<uint8_t, IoIpComm*> CommTable;
    typedef multimap<string, IoIpComm::InputFilter*> FilterBag;

    /**
     * Get the CommTable for an address family.
     *
     * @param family the address family.
     * @return a reference to the CommTable for the address family.
     */
    CommTable& comm_table_by_family(int family);

    /**
     * Get the FilterBag for an address family.
     *
     * @param family the address family.
     * @return a reference to the FilterBag for the address family.
     */
    FilterBag& filters_by_family(int family);

    /**
     * Erase filters for a given receiver name.
     *
     * @param family the address family.
     * @param receiver_name the name of the receiver.
     */
    void erase_filters_by_receiver_name(int family,
					const string& receiver_name);

    /**
     * Test whether there is a filter for a given receiver name.
     *
     * @param receiver_name the name of the receiver.
     * @return true if there is a filter for the given receiver name,
     * otherwise false.
     */
    bool has_filter_by_receiver_name(const string& receiver_name) const;

    /**
     * Erase filters for a given CommTable and FilterBag.
     *
     * @param comm_table the associated CommTable.
     * @param filters the associated FilterBag.
     * @param begin the begin iterator to the FilterBag for the set of
     * filters to erase.
     * @param end the end iterator to the FilterBag for the set of filters
     * to erase.
     */
    void erase_filters(CommTable& comm_table, FilterBag& filters,
		       const FilterBag::iterator& begin,
		       const FilterBag::iterator& end);

    FeaNode&		_fea_node;
    EventLoop&		_eventloop;
    const IfTree&	_iftree;

    // Collection of IP communication handlers keyed by protocol.
    CommTable		_comm_table4;
    CommTable		_comm_table6;

    // Collection of input filters created by IoIpManager
    FilterBag		_filters4;
    FilterBag		_filters6;

    IoIpManagerReceiver* _io_ip_manager_receiver;

    list<FeaDataPlaneManager*> _fea_data_plane_managers;
};

#endif // __FEA_IO_IP_MANAGER_HH__
