// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/mld6igmp/mld6igmp_member_query.hh,v 1.8 2006/05/17 22:07:18 pavlin Exp $

#ifndef __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__
#define __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__


//
// IGMP and MLD membership definitions.
//


#include "libxorp/ipvx.hh"
#include "libxorp/timer.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class Mld6igmpVif;

/**
 * @short A class to store information about multicast membership.
 */
class MemberQuery {
public:
    /**
     * Constructor for a given vif and group address.
     * 
     * @param mld6igmp_vif the interface this entry belongs to.
     * @param group the multicast group address.
     */
    MemberQuery(Mld6igmpVif& mld6igmp_vif, const IPvX& group);
    
    /**
     * Destructor
     */
    ~MemberQuery();

    /**
     * Get the vif this entry belongs to.
     * 
     * @return a reference to the vif this entry belongs to.
     */
    Mld6igmpVif& mld6igmp_vif()	const	{ return (_mld6igmp_vif);	}
    
    /**
     * Get the multicast group address.
     * 
     * @return the multicast group address.
     */
    const IPvX&	group() const		{ return (_group); }

    /**
     * Get the number of seconds until time to query for host members.
     * 
     * @return the number of seconds until time to query for host members.
     */
    uint32_t	timeout_sec()	const;
    
    /**
     * Get the address of the host that last reported as member.
     * 
     * @return the address of the host that last reported as member.
     */
    const IPvX& last_reported_host() const { return (_last_reported_host); }

    /**
     * Set the address of the host that last reported as member.
     *
     * @param v the address of the host that last reported as member.
     */
    void set_last_reported_host(const IPvX& v) { _last_reported_host = v; }

    /**
     * Get a refererence to the timer to query for host members.
     *
     * @return a reference to the timer to query for host members.
     */
    XorpTimer& member_query_timer() { return _member_query_timer; }

    /**
     * Get a refererence to the Last Member Query timer.
     *
     * @return a reference to the Last Member Query timer.
     */
    XorpTimer& last_member_query_timer() { return _last_member_query_timer; }

    /**
     * Get a reference to the IGMP Version 1 Members Present timer.
     *
     * Note: this applies only for IGMP.
     *
     * @return a reference to the IGMP Version 1 Members Present timer.
     */
    XorpTimer& igmpv1_host_present_timer() { return _igmpv1_host_present_timer; }

    /**
     * Timeout: expire a multicast group entry.
     */
    void member_query_timer_timeout();
    
    /**
     * Timeout: the last group member has expired or has left the group.
     */
    void last_member_query_timer_timeout();

    /**
     * Get the address family.
     *
     * @return the address family.
     */
    int family() const { return _group.af(); }
    
private:
    Mld6igmpVif& _mld6igmp_vif;		// The interface this entry belongs to
    IPvX	_group;			// The multicast group address
    IPvX	_last_reported_host;	// The host that last reported as member
    XorpTimer	_member_query_timer;	// Timer to query for host members
    XorpTimer	_last_member_query_timer;   // Timer to expire this entry
    XorpTimer	_igmpv1_host_present_timer; // XXX: does not apply to MLD
};


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__
