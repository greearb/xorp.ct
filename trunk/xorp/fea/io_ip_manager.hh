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

// $XORP: xorp/fea/io_ip_manager.hh,v 1.2 2007/05/26 02:10:26 pavlin Exp $

#ifndef __FEA_IO_IP_MANAGER_HH__
#define __FEA_IO_IP_MANAGER_HH__

#include <list>
#include <vector>
#include <set>
#include <map>

#include "libxorp/ipvx.hh"

#include "forwarding_plane/io/io_ip_socket.hh"

class IoIpManager;


/**
 * Simple structure used to cache commonly passed IPv4 and IPv6 header
 * information which comes from socket control message headers.
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
class IoIpComm : public IoIpSocket {
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
	virtual ~JoinedMulticastGroup() {}

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
     * @param eventloop the event loop to use.
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param ip_protocol the IP protocol number.
     * @param iftree the interface tree to use (@see IfTree).
     */
    IoIpComm(EventLoop& eventloop, int family, uint8_t ip_protocol,
	     const IfTree& iftree);
    virtual ~IoIpComm();

    /**
     * Add a filter to list of input filters.  The IoIpComm class
     * assumes that the callee will be responsible for managing the memory
     * associated with the filter and will call remove_filter() if the
     * filter is deleted or goes out of scope.
     */
    bool add_filter(InputFilter* filter);

    /**
     * Remove filter from list of input filters.
     */
    bool remove_filter(InputFilter* filter);

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
    int		proto_socket_write(const string&	if_name,
				   const string&	vif_name,
				   const IPvX&		src_address,
				   const IPvX&		dst_address,
				   int32_t		ip_ttl,
				   int32_t		ip_tos,
				   bool			ip_router_alert,
				   bool			ip_internet_control,
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
     * @param ip_protocol the IP protocol number.
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
    void process_recv_data(const string&	if_name,
			   const string&	vif_name,
			   const IPvX&		src_address,
			   const IPvX&		dst_address,
			   int32_t		ip_ttl,
			   int32_t		ip_tos,
			   bool			ip_router_alert,
			   bool			ip_internet_control,
			   const vector<uint8_t>& ext_headers_type,
			   const vector<vector<uint8_t> >& ext_headers_payload,
			   const vector<uint8_t>& payload);

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

private:
    IoIpComm(const IoIpComm&);			// Not implemented.
    IoIpComm& operator=(const IoIpComm&);	// Not implemented.

    list<InputFilter*>		_input_filters;
    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

/**
 * @short A class that manages raw IP I/O.
 *
 * The IoIpManager has two containers: a container for IP protocol handlers
 * (@see IoIpComm) indexed by the protocol associated with the handler, and
 * a container for the filters associated with each receiver_name.  When
 * a receiver registers for interest in a particular type of raw
 * packet a handler (@see IoIpComm) is created if necessary, then the
 * relevent filter is created and associated with the IoIpComm.
 */
class IoIpManager {
public:
    /**
     * Constructor for IoIpManager.
     */
    IoIpManager(EventLoop& eventloop, const IfTree& iftree);

    virtual ~IoIpManager();

    /**
     * @short Class that implements the API for sending IP packet to a
     * receiver.
     */
    class SendToReceiverBase {
    public:
	virtual ~SendToReceiverBase() {}
	/**
	 * Send a raw IP packet to a receiver.
	 *
	 * @param receiver_name the name of the receiver to send the
	 * IP packet to.
	 * @param header the IP header information.
	 * @param payload the payload, everything after the IP header
	 * and options.
	 */
	virtual void send_to_receiver(const string&		receiver_name,
				      const struct IPvXHeaderInfo& header,
				      const vector<uint8_t>&	payload) = 0;
    };

    /**
     * Send an IP packet on a raw socket.
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
     */
    int leave_multicast_group(const string&	receiver_name,
			      const string&	if_name,
			      const string&	vif_name,
			      uint8_t		ip_protocol,
			      const IPvX&	group_address,
			      string&		error_msg);

    /**
     * Send a raw IP packet to a receiver.
     *
     * @param receiver_name the name of the receiver to send the IP packet to.
     * @param header the IP header information.
     * @param payload the payload, everything after the IP header and options.
     */
    void send_to_receiver(const string&			receiver_name,
			  const struct IPvXHeaderInfo&	header,
			  const vector<uint8_t>&	payload);

    /**
     * Set the instance that is responsible for sending IP packets
     * to a receiver.
     */
    void set_send_to_receiver_base(SendToReceiverBase* v) {
	_send_to_receiver_base = v;
    }

    /**
     * Erase filters for a given receiver name.
     *
     * @param receiver_name the name of the receiver.
     * @param family the address family.
     */
    void erase_filters_by_name(const string& receiver_name, int family);

    /**
     * Get a reference to the interface tree.
     *
     * @return a reference to the interface tree (@see IfTree).
     */
    const IfTree&	iftree() const { return _iftree; }

private:
    typedef map<uint8_t, IoIpComm*> CommTable;
    typedef multimap<string, IoIpComm::InputFilter*> FilterBag;

    CommTable& comm_table_by_family(int family);
    FilterBag& filters_by_family(int family);

    void erase_filters(CommTable& comm_table, FilterBag& filters,
		       const FilterBag::iterator& begin,
		       const FilterBag::iterator& end);

    EventLoop&		_eventloop;
    const IfTree&	_iftree;

    // Collection of IP communication handlers keyed by protocol.
    CommTable		_comm_table4;
    CommTable		_comm_table6;

    // Collection of input filters created by IoIpManager
    FilterBag		_filters4;
    FilterBag		_filters6;

    SendToReceiverBase*	_send_to_receiver_base;
};

#endif // __FEA_IO_IP_MANAGER_HH__
