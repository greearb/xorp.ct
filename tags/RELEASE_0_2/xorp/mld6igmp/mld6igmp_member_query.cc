// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_member_query.cc,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $"

//
// Multicast group membership information used by
// MLDv1 (RFC 2710), IGMPv1 and IGMPv2 (RFC 2236).
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_member_query.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


/**
 * MemberQuery::MemberQuery:
 * @mld6igmp_vif: The vif interface this entry belongs to.
 * @source: The entry source address. It could be NULL for (*,G) entries.
 * @group: The entry group address.
 * 
 * Create a (S,G) or (*,G) entry used by IGMP or MLD6 to query host members.
 * 
 * Return value: 
 **/
MemberQuery::MemberQuery(Mld6igmpVif& mld6igmp_vif, const IPvX& source,
			 const IPvX& group)
    : _mld6igmp_vif(mld6igmp_vif),
      _source(source),
      _group(group)
{
    
}

/**
 * MemberQuery::~MemberQuery:
 * @: 
 * 
 * MemberQuery destrictor.
 **/
MemberQuery::~MemberQuery()
{
    // TODO: Notify routing (-)
    // TODO: ??? Maybe not the right place, or this should
    // be the only place to use ACTION_PRUNE notification??
    // join_prune_notify_routing(source(), group(), ACTION_PRUNE);
}

