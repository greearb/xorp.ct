// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

// $XORP: xorp/fea/io_link.hh,v 1.5 2008/10/02 21:56:48 bms Exp $


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

    /**
     * Receved an Ethernet packet.
     *
     * @param packet the packet.
     * @param packet_size the size of the packet.
     */
    void recv_ethernet_packet(const uint8_t* packet, size_t packet_size);

    /**
     * Prepare an Ethernet packet for transmission.
     *
     * @param src_address the MAC source address.
     * @param dst_address the MAC destination address.
     * @param ether_type the EtherType protocol number.
     * @param payload the payload, everything after the MAC header.
     * @param packet the return-by-reference packet prepared for transmission.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int prepare_ethernet_packet(const Mac& src_address,
				const Mac& dst_address,
				uint16_t ether_type,
				const vector<uint8_t>& payload,
				vector<uint8_t>& packet,
				string& error_msg);

    // Misc other state
    bool	_is_running;
    uint8_t*	_databuf;	// Data buffer for sending and receiving

    // Misc. constants
    static const size_t MAX_PACKET_SIZE = (64*1024);	// Max. packet size
    static const uint16_t ETHERNET_HEADER_SIZE		= 14;
    static const uint16_t ETHERNET_LENGTH_TYPE_THRESHOLD = 1536;
    static const uint16_t ETHERNET_MIN_FRAME_SIZE	= 60;	// Excl. CRC

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
