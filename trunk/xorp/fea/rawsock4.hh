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

// $XORP: xorp/fea/rawsock4.hh,v 1.20 2007/05/22 22:57:19 pavlin Exp $

#ifndef __FEA_RAWSOCK4_HH__
#define __FEA_RAWSOCK4_HH__

#include <list>
#include <vector>
#include <set>
#include <map>

#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/eventloop.hh"

#include "rawsock.hh"

class RawSocket4Manager;


/**
 * Base class for raw IPv4 sockets.
 */
class RawSocket4 : public RawSocket {
public:
    RawSocket4(EventLoop& eventloop, uint8_t ip_protocol,
	       const IfTree& iftree);

    virtual ~RawSocket4();

private:
    RawSocket4(const RawSocket4&);		// Not implemented.
    RawSocket4& operator=(const RawSocket4&);	// Not implemented.
};

/**
 * Simple structure used to cache commonly passed IPv4 header information
 * which comes from socket control message headers. This is used when
 * reading from the kernel, so we use C types, rather than XORP C++ Class
 * Library types.
 */
struct IPv4HeaderInfo {
    string	if_name;
    string	vif_name;
    IPv4	src_address;
    IPv4	dst_address;
    uint8_t	ip_protocol;
    int32_t	ip_ttl;
    int32_t	ip_tos;
    bool	ip_router_alert;
    bool	ip_internet_control;
};

/**
 * A RawSocketClass that allows arbitrary filters to receive the data
 * associated with a raw socket.
 */
class FilterRawSocket4 : public RawSocket4 {
public:
    /**
     * Filter class.
     */
    class InputFilter {
    public:
	InputFilter(RawSocket4Manager&	raw_socket_manager,
		    const string&	receiver_name,
		    uint8_t		ip_protocol)
	    : _raw_socket_manager(raw_socket_manager),
	      _receiver_name(receiver_name),
	      _ip_protocol(ip_protocol)
	{}
	virtual ~InputFilter() {}

	/**
	 * Get a reference to the raw socket manager.
	 *
	 * @return a reference to the raw socket manager.
	 */
	RawSocket4Manager& raw_socket_manager() { return (_raw_socket_manager); }

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
	 * Method invoked when data arrives on associated FilterRawSocket4
	 * instance.
	 */
	virtual void recv(const struct IPv4HeaderInfo& header,
			  const vector<uint8_t>& payload) = 0;

	/**
	 * Method invoked by the destructor of the associated
	 * FilterRawSocket4 instance.  This method provides the
	 * InputFilter with the opportunity to delete itself or update
	 * it's state.  The input filter does not need to call
	 * FilterRawSocket4::remove_filter() since filter removal is
	 * automatically conducted.
	 */
	virtual void bye() = 0;

    private:
	RawSocket4Manager&	_raw_socket_manager;
	string			_receiver_name;
	uint8_t			_ip_protocol;
    };

    /**
     * Joined multicast group class.
     */
    class JoinedMulticastGroup {
    public:
	JoinedMulticastGroup(const string& if_name, const string& vif_name,
			     const IPv4& group_address)
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
	IPv4		_group_address;
	set<string>	_receivers;
    };

public:
    FilterRawSocket4(EventLoop& eventloop, uint8_t ip_protocol,
		     const IfTree& iftree);
    virtual ~FilterRawSocket4();

    /**
     * Add a filter to list of input filters.  The FilterRawSocket4 class
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
     * Send an IPv4 packet on a raw socket.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * the TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4).
     * If it has a negative value, the TOS will be
     * set internally before transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_socket_write(const string&	if_name,
				   const string&	vif_name,
				   const IPv4&		src_address,
				   const IPv4&		dst_address,
				   int32_t		ip_ttl,
				   int32_t		ip_tos,
				   bool			ip_router_alert,
				   bool			ip_internet_control,
				   const vector<uint8_t>& payload,
				   string&		error_msg);

    /**
     * Join an IPv4 multicast group.
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
				     const IPv4&	group_address,
				     const string&	receiver_name,
				     string&		error_msg);

    /**
     * Leave an IPv4 multicast group.
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
				      const IPv4&	group_address,
				      const string&	receiver_name,
				      string&		error_msg);
    
protected:
    void	process_recv_data(const string&		if_name,
				  const string&		vif_name,
				  const IPvX&		src_address,
				  const IPvX&		dst_address,
				  int32_t		ip_ttl,
				  int32_t		ip_tos,
				  bool			ip_router_alert,
				  bool			ip_internet_control,
				  const vector<uint8_t>& ext_headers_type,
				  const vector<vector<uint8_t> >& ext_headers_payload,
				  const vector<uint8_t>& payload);

private:
    FilterRawSocket4(const FilterRawSocket4&);		 // Not implemented.
    FilterRawSocket4& operator=(const FilterRawSocket4&); // Not implemented.

protected:
    list<InputFilter*>		_input_filters;
    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

/**
 * @short A class that manages IPv4 raw sockets.
 *
 * The RawSocket4Manager has two containers: a container for raw
 * sockets indexed by the protocol associated with the raw socket, and
 * a container for the filters associated with each receiver_name.  When
 * a receiver registers for interest in a particular type of raw
 * packet a raw socket (FilterRawSocket4) is created if necessary,
 * then the relevent filter is created and associated with the
 * RawSocket.
 */
class RawSocket4Manager {
public:
    /**
     * Constructor for RawSocket4Manager instances.
     */
    RawSocket4Manager(EventLoop& eventloop, const IfTree& iftree);

    virtual ~RawSocket4Manager();

    /**
     * Send an IPv4 packet on a raw socket.
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
     * @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, the TOS will be set internally before
     * transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const string&	if_name,
	     const string&	vif_name,
	     const IPv4&	src_address,
	     const IPv4&	dst_address,
	     uint8_t		ip_protocol,
	     int32_t		ip_ttl,
	     int32_t		ip_tos,
	     bool		ip_router_alert,
	     bool		ip_internet_control,
	     const vector<uint8_t>&	payload,
	     string&		error_msg);

    /**
     * Register to receive IPv4 packets.
     *
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
    int register_receiver(const string&	receiver_name,
			  const string&	if_name,
			  const string&	vif_name,
			  uint8_t	ip_protocol,
			  bool		enable_multicast_loopback,
			  string&	error_msg);

    /**
     * Unregister to receive IPv4 packets.
     *
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
    int unregister_receiver(const string&	receiver_name,
			    const string&	if_name,
			    const string&	vif_name,
			    uint8_t		ip_protocol,
			    string&		error_msg);

    /**
     * Join an IPv4 multicast group.
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
			     const IPv4&	group_address,
			     string&		error_msg);

    /**
     * Leave an IPv4 multicast group.
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
			      const IPv4&	group_address,
			      string&		error_msg);

    /**
     * Received an IPv4 packet on a raw socket and forward it to the
     * recipient.
     *
     * @param receiver_name the name of the receiver to send the IP packet to.
     * @param header the IPv4 header information.
     * @param payload the payload, everything after the IP header and options.
     */
    virtual void recv_and_forward(const string&			receiver_name,
				  const struct IPv4HeaderInfo&	header,
				  const vector<uint8_t>&	payload) = 0;

    /**
     * Erase filters for a given receiver name.
     *
     * @param receiver_name the name of the receiver.
     */
    void erase_filters_by_name(const string& receiver_name);

    /**
     * Get a reference to the interface tree.
     *
     * @return a reference to the interface tree (@see IfTree).
     */
    const IfTree&	iftree() const { return _iftree; }

protected:
    EventLoop&		_eventloop;
    const IfTree&	_iftree;

    // Collection of IPv4 raw sockets keyed by protocol
    typedef map<uint8_t, FilterRawSocket4*> SocketTable4;
    SocketTable4	_sockets;

    // Collection of input filters created by RawSocketManager
    typedef multimap<string, FilterRawSocket4::InputFilter*> FilterBag4;
    FilterBag4		_filters;

protected:
    void erase_filters(const FilterBag4::iterator& begin,
		       const FilterBag4::iterator& end);
};

#endif // __FEA_RAWSOCK4_HH__
