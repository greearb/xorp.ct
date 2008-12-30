// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_click.hh,v 1.8 2008/07/23 05:10:19 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_CLICK_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_CLICK_HH__

#include <map>

#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/time_slice.hh"
#include "libxorp/timer.hh"

#include "fea/fibconfig_entry_set.hh"
#include "fea/nexthop_port_mapper.hh"
#include "fea/data_plane/control_socket/click_socket.hh"


class FibConfigEntrySetClick : public FibConfigEntrySet,
			       public ClickSocket,
			       public NexthopPortMapperObserver {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigEntrySetClick(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigEntrySetClick();

    /**
     * Start operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);

    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Add a single IPv4 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_entry4(const Fte4& fte);

    /**
     * Delete a single IPv4 forwarding entry.
     *
     * Must be with a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_entry4(const Fte4& fte);

    /**
     * Add a single IPv6 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_entry6(const Fte6& fte);

    /**
     * Delete a single IPv6 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_entry6(const Fte6& fte);

    /**
     * Obtain a reference to the table with the IPv4 forwarding entries.
     *
     * @return a reference to the table with the IPv4 forwarding entries.
     */
    const map<IPv4Net, Fte4>& fte_table4() const { return _fte_table4; }

    /**
     * Obtain a reference to the table with the IPv6 forwarding entries.
     *
     * @return a reference to the table with the IPv6 forwarding entries.
     */
    const map<IPv6Net, Fte6>& fte_table6() const { return _fte_table6; }

private:
    virtual void nexthop_port_mapper_event(bool is_mapping_changed);

    int add_entry(const FteX& fte);
    int delete_entry(const FteX& fte);

    // Methods related to reinstalling all IPv4 and IPv6 forwarding entries
    void start_task_reinstall_all_entries();
    void run_task_reinstall_all_entries();
    /**
     * @return true if there are more entries to install, otherwise false.
     */
    bool reinstall_all_entries4();
    /**
     * @return true if there are more entries to install, otherwise false.
     */
    bool reinstall_all_entries6();

    ClickSocketReader _cs_reader;

    map<IPv4Net, Fte4>	_fte_table4;
    map<IPv6Net, Fte6>	_fte_table6;

    // State related to reinstalling all IPv4 and IPv6 forwarding entries
    XorpTimer _reinstall_all_entries_timer;
    TimeSlice _reinstall_all_entries_time_slice;
    bool _start_reinstalling_fte_table4;
    bool _is_reinstalling_fte_table4;
    bool _start_reinstalling_fte_table6;
    bool _is_reinstalling_fte_table6;
    IPv4Net _reinstalling_ipv4net;
    IPv6Net _reinstalling_ipv6net;
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_CLICK_HH__
