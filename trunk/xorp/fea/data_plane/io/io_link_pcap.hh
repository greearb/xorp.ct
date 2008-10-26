// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/fea/data_plane/io/io_link_pcap.hh,v 1.14 2008/10/11 04:20:19 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__
#define __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__

//
// I/O link raw communication support.
//
// The mechanism is pcap(3).
//

#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"

#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif

#include "fea/io_link.hh"

#ifndef HAVE_PCAP_H
typedef void pcap_t;
#endif


/**
 * @short A base class for I/O link raw pcap(3)-based communication.
 *
 * Each protocol 'registers' for link raw I/O per interface and vif 
 * and gets assigned one object (per interface and vif) of this class.
 */
class IoLinkPcap : public IoLink {
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
     * received packets. The program uses tcpdump(1) style expression.
     */
    IoLinkPcap(FeaDataPlaneManager& fea_data_plane_manager,
	       const IfTree& iftree, const string& if_name,
	       const string& vif_name, uint16_t ether_type,
	       const string& filter_program);

    /**
     * Virtual destructor.
     */
    virtual ~IoLinkPcap();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start(string& error_msg);

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop(string& error_msg);

    /**
     * Join a multicast group on an interface.
     * 
     * @param group the multicast group to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(const Mac& group, string& error_msg);
    
    /**
     * Leave a multicast group on an interface.
     * 
     * @param group the multicast group to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(const Mac& group, string& error_msg);

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
    int		send_packet(const Mac&		src_address,
			    const Mac&		dst_address,
			    uint16_t		ether_type,
			    const vector<uint8_t>& payload,
			    string&		error_msg);

private:
    /**
     * Open the pcap access.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		open_pcap_access(string& error_msg);
    
    /**
     * Close the pcap access.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_pcap_access(string& error_msg);

    /**
     * Join or leave a multicast group on an interface.
     * 
     * @param is_join if true, then join the group, otherwise leave.
     * @param group the multicast group to join/leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_leave_multicast_group(bool is_join, const Mac& group,
					   string& error_msg);

    /**
     * Callback that is called when there data to read from the system.
     *
     * This is called as a IoEventCb callback.
     * @param fd file descriptor that with event caused this method to be
     * called.
     * @param type the event type.
     */
    void	ioevent_read_cb(XorpFd fd, IoEventType type);

    /**
     * Read data from the system, and then call the appropriate
     * module to process it.
     */
    void	recv_data();

    /**
     * Reopen the pcap access.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reopen_pcap_access(string& error_msg);

    // Private state
    XorpFd	_packet_fd;	// The file descriptor to send and recv packets
    pcap_t*	_pcap;		// The pcap descriptor to send and recv packets
    int		_datalink_type;	// The pcap-defined DLT_* data link type
    char*	_pcap_errbuf;	// The pcap buffer for error messages
    int		_multicast_sock; // The socket to join L2 multicast groups

    XorpTask	_recv_data_task; // Task for receiving pending data
};

#endif // __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__
