// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

// $XORP: xorp/fea/io_link.hh,v 1.3 2008/01/04 03:15:47 pavlin Exp $


#ifndef __FEA_IO_LINK_HH__
#define __FEA_IO_LINK_HH__

#include <vector>

#include "fea_data_plane_manager.hh"

//
// I/O link raw communication API.
//


class EventLoop;
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IoLinkManager;
class IoLinkReceiver;
class Mac;


/**
 * @short A base class for I/O link raw communication.
 *
 * Each protocol 'registers' for I/O per interface and vif and gets assigned
 * one object (per interface and vif) of this class.
 */
class IoLink {
public:
    /**
     * Constructor for link-level access for a given interface and vif.
     * 
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     * @param iftree the interface tree to use.
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number. If it is 0 then
     * it is unused.
     * @param filter_program the optional filter program to be applied on the
     * received packets.
     */
    IoLink(FeaDataPlaneManager& fea_data_plane_manager, const IfTree& iftree,
	   const string& if_name, const string& vif_name,
	   uint16_t ether_type, const string& filter_program);

    /**
     * Virtual destructor.
     */
    virtual ~IoLink();

    /**
     * Get the @ref IoLinkManager instance.
     *
     * @return the @ref IoLinkManager instance.
     */
    IoLinkManager& io_link_manager() { return _io_link_manager; }

    /**
     * Get the @ref FeaDataPlaneManager instance.
     *
     * @return the @ref FeaDataPlaneManager instance.
     */
    FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

    /**
     * Test whether this instance is running.
     *
     * @return true if the instance is running, otherwise false.
     */
    virtual bool is_running() const { return _is_running; }

    /**
     * Get the event loop.
     * 
     * @return the event loop.
     */
    EventLoop&	eventloop() { return (_eventloop); }

    /**
     * Get the interface tree.
     *
     * @return the interface tree.
     */
    const IfTree& iftree() const { return (_iftree); }

    /**
     * Get the interface name.
     * 
     * @return the interface name.
     */
    virtual const string& if_name() const { return (_if_name); }

    /**
     * Get the vif name.
     * 
     * @return the vif name.
     */
    virtual const string& vif_name() const { return (_vif_name); }

    /**
     * Get the EtherType protocol number.
     * 
     * @return the EtherType protocol number. If it is 0 then
     * it is unused.
     */
    virtual uint16_t ether_type() const { return (_ether_type); }

    /**
     * Get the optional filter program.
     * 
     * @return the optional filter program to be applied on the
     * received packets.
     */
    const string& filter_program() const { return (_filter_program); }

    /**
     * Get the registered receiver.
     *
     * @return the registered receiver.
     */
    IoLinkReceiver* io_link_receiver() { return (_io_link_receiver); }

    /**
     * Register the I/O Link raw packets receiver.
     *
     * @param io_link_receiver the receiver to register.
     */
    virtual void register_io_link_receiver(IoLinkReceiver* io_link_receiver);

    /**
     * Unregister the I/O Link raw packets receiver.
     */
    virtual void unregister_io_link_receiver();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	start(string& error_msg) = 0;

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;

    /**
     * Join a multicast group on an interface.
     * 
     * @param group the multicast group to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int join_multicast_group(const Mac& group, string& error_msg) = 0;
    
    /**
     * Leave a multicast group on an interface.
     * 
     * @param group the multicast group to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int leave_multicast_group(const Mac& group, string& error_msg) = 0;

    /**
     * Send a link-level packet.
     *
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol number.
     * @param payload the payload, everything after the MAC header.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send_packet(const Mac&		src_address,
			    const Mac&		dst_address,
			    uint16_t		ether_type,
			    const vector<uint8_t>& payload,
			    string&		error_msg) = 0;

    //
    // Debug-related methods
    //
    
    /**
     * Test if trace log is enabled.
     * 
     * This method is used to test whether to output trace log debug messges.
     * 
     * @return true if trace log is enabled, otherwise false.
     */
    bool	is_log_trace() const { return (_is_log_trace); }
    
    /**
     * Enable/disable trace log.
     * 
     * This method is used to enable/disable trace log debug messages output.
     * 
     * @param is_enabled if true, trace log is enabled, otherwise is disabled.
     */
    void	set_log_trace(bool is_enabled) { _is_log_trace = is_enabled; }

protected:
    /**
     * Received a link-level packet.
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

    // Misc other state
    bool	_is_running;

private:
    IoLinkManager&	_io_link_manager;	// The I/O Link manager
    FeaDataPlaneManager& _fea_data_plane_manager; // The data plane manager
    EventLoop&		_eventloop;		// The event loop to use
    const IfTree&	_iftree;		// The interface tree to use
    const string	_if_name;		// The interface name
    const string	_vif_name;		// The vif name
    const uint16_t	_ether_type;	// The EtherType protocol number
    const string	_filter_program;	// The filter program
    IoLinkReceiver*	_io_link_receiver;	// The registered receiver

    bool		_is_log_trace;		// True if trace log is enabled
};

/**
 * @short A base class for I/O Link raw packets receiver.
 *
 * The real receiver must inherit from this class and register with the
 * corresponding IoLink entity to receive the link raw packets.
 * @see IoLink.
 */
class IoLinkReceiver {
public:
    /**
     * Default constructor.
     */
    IoLinkReceiver() {}

    /**
     * Virtual destructor.
     */
    virtual ~IoLinkReceiver() {}

    /**
     * Received a link-level packet.
     *
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol number.
     * @param packet the payload, everything after the MAC header.
     */
    virtual void recv_packet(const Mac&		src_address,
			     const Mac&		dst_address,
			     uint16_t		ether_type,
			     const vector<uint8_t>& payload) = 0;
};

#endif // __FEA_IO_LINK_HH__
