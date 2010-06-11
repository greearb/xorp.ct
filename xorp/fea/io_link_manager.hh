// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2010 XORP, Inc and Others
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

// $XORP: xorp/fea/io_link_manager.hh,v 1.14 2008/10/10 01:23:53 pavlin Exp $

#ifndef __FEA_IO_LINK_MANAGER_HH__
#define __FEA_IO_LINK_MANAGER_HH__







#include "libxorp/callback.hh"
#include "libxorp/mac.hh"

#include "fea_io.hh"
#include "io_link.hh"

class FeaDataPlaneManager;
class FeaNode;
class IoLinkManager;


/**
 * Structure used to store commonly passed MAC header information.
 */
struct MacHeaderInfo {
    string	if_name;
    string	vif_name;
    Mac		src_address;
    Mac		dst_address;
    uint16_t	ether_type;
};

/**
 * A class that handles raw link I/O communication for a specific protocol.
 *
 * It also allows arbitrary filters to receive the raw link-level data for that
 * protocol.
 */
class IoLinkComm :
    public NONCOPYABLE,
    public IoLinkReceiver
{
public:
    /**
     * Filter class.
     */
    class InputFilter {
    public:
	InputFilter(IoLinkManager&	io_link_manager,
		    const string&	receiver_name,
		    const string&	if_name,
		    const string&	vif_name,
		    uint16_t		ether_type,
		    const string&	filter_program)
	    : _io_link_manager(io_link_manager),
	      _receiver_name(receiver_name),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ether_type(ether_type),
	      _filter_program(filter_program)
	{}
	virtual ~InputFilter() {}

	/**
	 * Get a reference to the I/O Link manager.
	 *
	 * @return a reference to the I/O Link manager.
	 */
	IoLinkManager& io_link_manager() { return (_io_link_manager); }

	/**
	 * Get a const reference to the I/O Link manager.
	 *
	 * @return a const reference to the I/O Link manager.
	 */
	const IoLinkManager& io_link_manager() const { return (_io_link_manager); }

	/**
	 * Get the receiver name.
	 *
	 * @return the receiver name.
	 */
	const string& receiver_name() const { return (_receiver_name); }

	/**
	 * Get the interface name.
	 *
	 * @return the interface name.
	 */
	const string& if_name() const { return (_if_name); }

	/**
	 * Get the vif name.
	 *
	 * @return the vif name.
	 */
	const string& vif_name() const { return (_vif_name); }

	/**
	 * Get the EtherType protocol number.
	 *
	 * @return the EtherType protocol number.
	 */
	uint16_t ether_type() const { return (_ether_type); }

	/**
	 * Get the filter program.
	 *
	 * @return the filter program.
	 */
	const string& filter_program() const { return (_filter_program); }

	/**
	 * Method invoked when data arrives on associated IoLinkComm instance.
	 */
	virtual void recv(const struct MacHeaderInfo& header,
			  const vector<uint8_t>& payload) = 0;

	/**
	 * Method invoked by the destructor of the associated IoLinkComm
	 * instance. This method provides the InputFilter with the
	 * opportunity to delete itself or update its state.
	 * The input filter does not need to call IoLinkComm::remove_filter()
	 * since filter removal is automatically conducted.
	 */
	virtual void bye() = 0;

    private:
	IoLinkManager&	_io_link_manager;
	string		_receiver_name;
	string		_if_name;
	string		_vif_name;
	uint16_t	_ether_type;
	string		_filter_program;
    };

    /**
     * Joined multicast group class.
     */
    class JoinedMulticastGroup {
    public:
	JoinedMulticastGroup(const Mac& group_address)
	    : _group_address(group_address)
	{}
	virtual ~JoinedMulticastGroup() {}

	const Mac& group_address() const { return _group_address; }

	/**
	 * Less-Than Operator
	 *
	 * @param other the right-hand operand to compare against.
	 * @return true if the left-hand operand is numerically smaller
	 * than the right-hand operand.
	 */
	bool operator<(const JoinedMulticastGroup& other) const {
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
	    return (_group_address == other._group_address);
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
	Mac		_group_address;
	set<string>	_receivers;
    };

public:
    /**
     * Constructor for IoLinkComm.
     *
     * @param io_link_manager the corresponding I/O Link manager
     * (@ref IoLinkManager).
     * @param iftree the interface tree to use.
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number.
     * @param filter_program the optional filter program to be applied on
     * the received packets.
     */
    IoLinkComm(IoLinkManager& io_link_manager, const IfTree& iftree,
	       const string& if_name, const string& vif_name,
	       uint16_t ether_type, const string& filter_program);

    /**
     * Virtual destructor.
     */
    virtual ~IoLinkComm();

    /**
     * Allocate the I/O Link plugins (one per data plane manager).
     */
    void allocate_io_link_plugins();

    /**
     * Deallocate the I/O Link plugins (one per data plane manager).
     */
    void deallocate_io_link_plugins();

    /**
     * Allocate an I/O Link plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void allocate_io_link_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Deallocate the I/O Link plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void deallocate_io_link_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Start all I/O Link plugins.
     */
    void start_io_link_plugins();

    /**
     * Stop all I/O Link plugins.
     */
    void stop_io_link_plugins();

    /**
     * Add a filter to list of input filters.
     *
     * The IoLinkComm class assumes that the callee will be responsible for
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
     * Send a raw link-level packet.
     *
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol number.
     * @param payload the payload, everything after the MAC header.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_packet(const Mac&		src_address,
			    const Mac&		dst_address,
			    uint16_t		ether_type,
			    const vector<uint8_t>& payload,
			    string&		error_msg);

    /**
     * Received a raw link-level packet.
     *
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol number.
     * @param packet the payload, everything after the MAC header.
     */
    virtual void recv_packet(const Mac&		src_address,
			     const Mac&		dst_address,
			     uint16_t		ether_type,
			     const vector<uint8_t>& payload);

    /**
     * Join a MAC multicast group.
     * 
     * @param group_address the multicast group address to join.
     * @param receiver_name the name of the receiver.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(const Mac&		group_address,
				     const string&	receiver_name,
				     string&		error_msg);

    /**
     * Leave a MAC multicast group.
     * 
     * @param group_address the multicast group address to leave.
     * @param receiver_name the name of the receiver.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(const Mac&	group_address,
				      const string&	receiver_name,
				      string&		error_msg);
    
    /**
     * Get the interface name.
     *
     * @return the interface name.
     */
    const string& if_name() const { return (_if_name); }

    /**
     * Get the vif name.
     *
     * @return the vif name.
     */
    const string& vif_name() const { return (_vif_name); }

    /**
     * Get the EtherType protocol number.
     *
     * @return the EtherType protocol number.
     */
    uint16_t ether_type() const { return (_ether_type); }

    /**
     * Get the filter program.
     *
     * @return the filter program.
     */
    const string& filter_program() const { return (_filter_program); }

private:
    IoLinkComm(const IoLinkComm&);		// Not implemented.
    IoLinkComm& operator=(const IoLinkComm&);	// Not implemented.

    IoLinkManager&		_io_link_manager;
    const IfTree&		_iftree;
    const string		_if_name;
    const string		_vif_name;
    const uint16_t		_ether_type;
    const string		_filter_program;

    typedef list<pair<FeaDataPlaneManager*, IoLink*> >IoLinkPlugins;
    IoLinkPlugins		_io_link_plugins;

    list<InputFilter*>		_input_filters;
    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

/**
 * @short Class that implements the API for sending raw link-level packet
 * to a receiver.
 */
class IoLinkManagerReceiver {
public:
    /**
     * Virtual destructor.
     */
    virtual ~IoLinkManagerReceiver() {}

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the
     * raw link-level packet to.
     * @param header the MAC header information.
     * @param payload the payload, everything after the MAC header.
     */
    virtual void recv_event(const string&		receiver_name,
			    const struct MacHeaderInfo& header,
			    const vector<uint8_t>&	payload) = 0;
};

/**
 * @short A class that manages raw link-level I/O.
 *
 * The IoLinkManager has two containers: a container for link-level handlers
 * (@ref IoLinkComm) indexed by the protocol associated with the handler, and
 * a container for the filters associated with each receiver_name.  When
 * a receiver registers for interest in a particular type of raw
 * packet a handler (@ref IoLinkComm) is created if necessary, then the
 * relevent filter is created and associated with the IoLinkComm.
 */
class IoLinkManager : public IoLinkManagerReceiver,
		      public InstanceWatcher {
public:
    typedef XorpCallback2<int, const uint8_t*, size_t>::RefPtr UpcallReceiverCb;

    /**
     * Constructor for IoLinkManager.
     */
    IoLinkManager(FeaNode& fea_node, const IfTree& iftree);

    /**
     * Virtual destructor.
     */
    virtual ~IoLinkManager();

    /**
     * Send a raw link-level packet on an interface.
     *
     * @param if_name the interface to send the packet on.
     * @param vif_name the vif to send the packet on.
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol type or the Destination SAP.
     * It must be between 1536 and 65535 to specify the EtherType, or between
     * 1 and 255 to specify the Destination SAP IEEE 802.2 LLC frames.
     * @param payload the payload, everything after the MAC header.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const string&	if_name,
	     const string&	vif_name,
	     const Mac&		src_address,
	     const Mac&		dst_address,
	     uint16_t		ether_type,
	     const vector<uint8_t>&	payload,
	     string&		error_msg);

    /**
     * Register to receive raw link-level packets.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ether_type the EtherType protocol number or the Destination SAP
     * that the receiver is interested in. It must be between 1536 and 65535
     * to specify the EtherType, or between 1 and 255 to specify the
     * Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     * used to specify all protocols.
     * @param filter_program the optional filter program to be applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @param enable_multicast_loopback if true then enable delivering of
     * multicast datagrams back to this host (assuming the host is a member of
     * the same multicast group.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_receiver(const string&	receiver_name,
			  const string&	if_name,
			  const string&	vif_name,
			  uint16_t	ether_type,
			  const string&	filter_program,
			  bool		enable_multicast_loopback,
			  string&	error_msg);

    /**
     * Unregister to receive raw link-level packets.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ether_type the EtherType protocol number or the Destination SAP
     * that the receiver is not interested in anymore. It must be between 1536
     * and 65535 to specify the EtherType, or between 1 and 255 to specify the
     * Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     * used to specify all protocols.
     * @param filter_program the filter program that was applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_receiver(const string&	receiver_name,
			    const string&	if_name,
			    const string&	vif_name,
			    uint16_t		ether_type,
			    const string&	filter_program,
			    string&		error_msg);

    /**
     * Join a MAC multicast group.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ether_type the EtherType protocol number or the Destination SAP
     * that the receiver is interested in. It must be between 1536 and 65535
     * to specify the EtherType, or between 1 and 255 to specify the
     * Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     * used to specify all protocols.
     * @param filter_program the optional filter program to be applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @param group_address the multicast group address to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int join_multicast_group(const string&	receiver_name,
			     const string&	if_name,
			     const string&	vif_name,
			     uint16_t		ether_type,
			     const string&	filter_program,
			     const Mac&		group_address,
			     string&		error_msg);

    /**
     * Leave a MAC multicast group.
     *
     * @param receiver_name the name of the receiver.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ether_type the EtherType protocol number or the Destination SAP
     * that the receiver is not interested in anymore. It must be between 1536
     * and 65535 to specify the EtherType, or between 1 and 255 to specify the
     * Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     * used to specify all protocols.
     * @param filter_program the filter program that was applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @param group_address the multicast group address to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int leave_multicast_group(const string&	receiver_name,
			      const string&	if_name,
			      const string&	vif_name,
			      uint16_t		ether_type,
			      const string&	filter_program,
			      const Mac&	group_address,
			      string&		error_msg);

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the raw link-level
     * packet to.
     * @param header the MAC header information.
     * @param payload the payload, everything after the MAC header.
     */
    void recv_event(const string&		receiver_name,
		    const struct MacHeaderInfo&	header,
		    const vector<uint8_t>&	payload);

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
     * Set the instance that is responsible for sending raw link-level packets
     * to a receiver.
     */
    void set_io_link_manager_receiver(IoLinkManagerReceiver* v) {
	_io_link_manager_receiver = v;
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

    /**
     * Add a multicast MAC address to an interface.
     *
     * @param if_name the interface name.
     * @param mac the MAC address to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_multicast_mac(const string& if_name, const Mac& mac,
			  string& error_msg);

    /**
     * Remove a multicast MAC address from an interface.
     *
     * @param if_name the interface name.
     * @param mac the MAC address to remove.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_multicast_mac(const string& if_name, const Mac& mac,
			     string& error_msg);

private:
    class CommTableKey {
    public:
	CommTableKey(const string& if_name, const string& vif_name,
		     uint16_t ether_type, const string& filter_program)
	    : _if_name(if_name),
	      _vif_name(vif_name),
	      _ether_type(ether_type),
	      _filter_program(filter_program)
	{}
	bool operator<(const CommTableKey& other) const {
	    if (_ether_type != other._ether_type)
		return (_ether_type < other._ether_type);
	    if (_if_name != other._if_name)
		return (_if_name < other._if_name);
	    if (_vif_name != other._vif_name)
		return (_vif_name < other._vif_name);
	    return (_filter_program < other._filter_program);
	}

    private:
	string		_if_name;
	string		_vif_name;
	uint16_t	_ether_type;
	string		_filter_program;
    };

    typedef map<CommTableKey, IoLinkComm*> CommTable;
    typedef multimap<string, IoLinkComm::InputFilter*> FilterBag;

    /**
     * Erase filters for a given receiver name.
     *
     * @param receiver_name the name of the receiver.
     */
    void erase_filters_by_receiver_name(const string& receiver_name);

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

    /**
     * Add a communication handler that can be used for raw link-level
     * transmission only.
     *
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number.
     * @return a reference to the transmission only communication handler.
     */
    IoLinkComm& add_iolink_comm_txonly(const string& if_name,
				       const string& vif_name,
				       uint16_t ether_type);

    /**
     * Find the communication handler for a given interface/vif name
     * and EtherType.
     *
     * Note that if the handler is not found, a new one is created and
     * returned.
     *
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number.
     * @return a reference to the communication handler.
     */
    IoLinkComm& find_iolink_comm(const string& if_name, const string& vif_name,
				 uint16_t ether_type);

    /**
     * Add/remove a multicast MAC address on an interface.
     *
     * @param add if true, then add the address, otherwise remove it.
     * @param if_name the interface name.
     * @param mac the MAC address to add/remove.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_remove_multicast_mac(bool add, const string& if_name,
				 const Mac& mac, string& error_msg);

    FeaNode&		_fea_node;
    EventLoop&		_eventloop;
    const IfTree&	_iftree;

    // Collection of raw link-level communication handlers keyed by protocol.
    CommTable		_comm_table;

    // Collection of input filters created by IoLinkManager
    FilterBag		_filters;

    IoLinkManagerReceiver* _io_link_manager_receiver;

    list<FeaDataPlaneManager*> _fea_data_plane_managers;
};

#endif // __FEA_IO_LINK_MANAGER_HH__
