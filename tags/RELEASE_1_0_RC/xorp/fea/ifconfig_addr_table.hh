// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_addr_table.hh,v 1.4 2004/04/10 07:55:19 pavlin Exp $

#ifndef __FEA_IFCONFIG_ADDR_TABLE_HH__
#define __FEA_IFCONFIG_ADDR_TABLE_HH__

#include "fea/addr_table.hh"
#include "fea/ifconfig.hh"

/**
 * Standard Xorp FEA AddressTable implementation.
 *
 * This class acts as a proxy between the interface configuration tree
 * the XrlSocketServer.
 */
class IfConfigAddressTable
    : public AddressTableBase, public IfConfigUpdateReporterBase
{
public:
    /**
     * Constructor.
     *
     * After construction, instance should be registered with
     * IfConfigUpdateReporter to receive updates in order to function
     * correctly.
     */
    IfConfigAddressTable(const IfTree& iftree);
    ~IfConfigAddressTable();

    /**
     * Query whether address is valid.
     *
     * @return true if address is enabled.
     */
    bool address_valid(const IPv4& addr) const;

    /**
     * Query whether address is valid.
     *
     * @return true if address is enabled.
     */
    bool address_valid(const IPv6& addr) const;

    /**
     * Get Unix kernel interface index number associated with address.
     *
     * @return non-zero value on success, zero on failure.
     */
    uint32_t address_pif_index(const IPv4& addr) const;

    /**
     * Get Unix kernel interface index number associated with address.
     *
     * @return non-zero value on success, zero on failure.
     */
    uint32_t address_pif_index(const IPv6& addr) const;

protected:
    void interface_update(const string& ifname,
			  const Update& update,
			  bool		system);

    void vif_update(const string&	ifname,
		    const string&	vifname,
		    const Update&	update,
		    bool		system);

    void vifaddr4_update(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr,
			 const Update&	update,
			 bool system);

    void vifaddr6_update(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr,
			 const Update&	update,
			 bool		system);

    void updates_completed(bool		system);

    /**
     * Walk interface configuration tree and find addresses that are
     * valid (ie not on a disabled or deleted branch).
     */
    void get_valid_addrs(set<IPv4>& v4s, set<IPv6>& v6s);

    /**
     * Set new valid interface addresses.  Triggers invalid_address
     * messages for addresses in list that are not present in instance
     * address list and then sets instance address set to be same as supplied.
     */
    void set_addrs(const set<IPv4>& new_v4s);

    /**
     * Set new valid interface addresses.  Triggers invalid_address
     * messages for addresses in list that are not present in instance
     * address list and then sets instance address set to be same as supplied.
     */
    void set_addrs(const set<IPv6>& new_v6s);

    /**
     * Covering method for update handling.  Scans valid addresses and
     * announces those that have disappeared (calls @ref
     * get_valid_addrs and then @ref set_addrs).
     */
    inline void update();

protected:
    const IfTree& _iftree;
    set<IPv4> _v4addrs;
    set<IPv6> _v6addrs;
};

#endif // __FEA_IFCONFIG_ADDR_TABLE_HH__
