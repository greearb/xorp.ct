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

// $XORP: xorp/bgp/route_table_policy_im.hh,v 1.1 2004/09/17 13:50:55 abittau Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_IM_HH__
#define __BGP_ROUTE_TABLE_POLICY_IM_HH__

#include "route_table_policy.hh"

/**
 * @short Import policy tables also deal with propagating policy route dumps.
 *
 * Import tables will detect policy route dumps [NULL in dump_peer]. It will
 * then transport the dump to an add/delete/request based on the outcome of the
 * filter.
 */
template <class A>
class PolicyTableImport : public PolicyTable<A> {
public:
    /**
     * @param tablename name of the table.
     * @param safi the safe.
     * @param parent parent table.
     * @param pfs a reference to the global policy filters.
     */
    PolicyTableImport(const string& tablename, 
		      const Safi& safi,
		      BGPRouteTable<A>* parent,
		      PolicyFilters& pfs);

    /**
     * If dump_peer is null, then it is a policy route dump and we need to deal
     * with it.
     *
     * @param rtmsg route being dumped.
     * @param caller table that called this method.
     * @param dump_peer peer we are dumping to. If policy dump,
     * it will be NULL.
     * @return ADD_FILTERED if route is rejected. XORP_OK otherwise.
     */
    int route_dump(const InternalMessage<A> &rtmsg,
                   BGPRouteTable<A> *caller,
                   const PeerHandler *dump_peer);
};

#endif // __BGP_ROUTE_TABLE_POLICY_IM_HH__
