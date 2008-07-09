// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/data_plane/io/io_link_pcap.hh,v 1.8 2008/05/24 03:02:23 pavlin Exp $


#ifndef __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__
#define __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__


//
// I/O link raw pcap(3)-based support.
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

    // Private state
    XorpFd	_packet_fd;	// The file descriptor to send and recv packets
    pcap_t*	_pcap;		// The pcap descriptor to send and recv packets
    int		_datalink_type;	// The pcap-defined DLT_* data link type

    uint8_t*	_databuf;	// Data buffer for sending and receiving
    char*	_errbuf;	// Buffer for error messages
    int		_multicast_sock; // The socket to join L2 multicast groups

    XorpTask	_recv_data_task; // Task for receiving pending data

    // Misc. constants
    static const uint16_t ETHERNET_HEADER_SIZE		= 14;
    static const uint16_t ETHERNET_LENGTH_TYPE_THRESHOLD = 1536;
    static const uint16_t ETHERNET_MIN_FRAME_SIZE	= 60;	// Excl. CRC
};

#endif // __FEA_DATA_PLANE_IO_IO_LINK_PCAP_HH__
