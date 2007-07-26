// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP$


#ifndef __FEA_DATA_PLANE_IO_IO_LINK_DUMMY_HH__
#define __FEA_DATA_PLANE_IO_IO_LINK_DUMMY_HH__


//
// I/O Dummy Link raw support.
//

#include <set>

#include "libxorp/xorp.h"

#include "fea/io_link.hh"
#include "fea/io_link_manager.hh"


/**
 * @short A base class for Dummy I/O link raw communication.
 *
 * Each protocol 'registers' for link raw I/O per interface and vif 
 * and gets assigned one object (per interface and vif) of this class.
 */
class IoLinkDummy : public IoLink {
public:
    /**
     * Constructor for link-level access for a given interface and vif.
     * 
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     * @param iftree the interface tree to use.
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number. If it is 0 then
     * it is unused.
     * @param filter_program the optional filter program to be applied on the
     * received packets. The program uses tcpdump(1) style expression.
     */
    IoLinkDummy(FeaDataPlaneManager& fea_data_plane_manager,
		const IfTree& iftree, const string& if_name,
		const string& vif_name, uint16_t ether_type,
		const string& filter_program);

    /**
     * Virtual destructor.
     */
    virtual ~IoLinkDummy();

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
    set<IoLinkComm::JoinedMulticastGroup> _joined_groups_table;
};

#endif // __FEA_DATA_PLANE_IO_IO_LINK_DUMMY_HH__
