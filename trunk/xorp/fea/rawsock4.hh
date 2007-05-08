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

// $XORP: xorp/fea/rawsock4.hh,v 1.17 2007/02/16 22:45:49 pavlin Exp $

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

/**
 * Base class for raw IPv4 sockets.
 */
class RawSocket4 : public RawSocket {
public:
    RawSocket4(EventLoop& eventloop, uint32_t ip_protocol,
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
    uint32_t	ip_protocol;
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
	virtual ~InputFilter() {}
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
    FilterRawSocket4(EventLoop& eventloop, uint32_t protocol,
		     const IfTree& iftree);
    ~FilterRawSocket4();

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
    bool empty() const { return _filters.empty(); }

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
    list<InputFilter*>		_filters;
    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

#endif // __FEA_RAWSOCK4_HH__
