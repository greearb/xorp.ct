// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/bgp/route_table_policy_sm.hh,v 1.1 2004/09/17 13:50:55 abittau Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_SM_HH__
#define __BGP_ROUTE_TABLE_POLICY_SM_HH__

#include "route_table_policy.hh"

/**
 * @short SourceMatch table has the aditional ability to perform route dumps.
 *
 * The SourceMatch policy table will request the route dumps from the ribin.
 * These dumps will eventually meet an import policy table and be translated to
 * an add/replace or delete.
 */
template <class A>
class PolicyTableSourceMatch : public PolicyTable<A> {
public:
    /**
     * @param tablename the name of the table.
     * @param safi the safi.
     * @param parent the parent table.
     * @param pfs a reference to the global policyfilters.
     */
    PolicyTableSourceMatch(const string& tablename,
			   const Safi& safi,
			   BGPRouteTable<A>* parent,
			   PolicyFilters& pfs);
    ~PolicyTableSourceMatch();

    /**
     * Push routes of all these peers.
     *
     * @param peer_list peers for which routes whould be dumped.
     */
    void push_routes(list<const PeerTableInfo<A>*>& peer_list);

    /**
     * Dump the next route.
     */
    void do_next_route_dump();

    /**
     * Stop dumping routes.
     */
    void end_route_dump();


private:
    bool		_pushing_routes;
    DumpIterator<A>*	_dump_iter;
};

#endif // __BGP_ROUTE_TABLE_POLICY_SM_HH__
