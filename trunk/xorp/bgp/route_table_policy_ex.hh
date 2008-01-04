// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_policy_ex.hh,v 1.4 2007/02/16 22:45:18 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_EX_HH__
#define __BGP_ROUTE_TABLE_POLICY_EX_HH__

#include "route_table_policy.hh"

/**
 * @short Export policy tables match neighbor in a different way.
 *
 * Export tables will supply a specialized varrw which will match neighbor
 * against the peer which the advertisement is about to be sent out, rather than
 * the originating peer.
 */
template <class A>
class PolicyTableExport : public PolicyTable<A> {
public:
    /**
     * @param tablename name of the table.
     * @param safi the safe.
     * @param parent parent table.
     * @param pfs a reference to the global policy filters.
     * @param neighbor the id of the peer on this output branch.
     */
    PolicyTableExport(const string& tablename, 
		      const Safi& safi,
		      BGPRouteTable<A>* parent,
		      PolicyFilters& pfs,
		      const string& neighbor);
		      
protected:
    void init_varrw();

private:
    const string _neighbor;
};

#endif // __BGP_ROUTE_TABLE_POLICY_EX_HH__
