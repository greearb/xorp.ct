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

// $XORP: xorp/mld6igmp/mld6igmp_member_query.hh,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $

#ifndef __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__
#define __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__


//
// IGMP and MLD6 membership definitions.
//


#include "libxorp/ipvx.hh"
#include "mrt/timer.hh"


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
     * Constructor for a given vif, source and group address.
     * 
     * @param mld6igmp_vif the interface this entry belongs to.
     * @param source the source address (used for MLDv2 or IGMPv3).
     * @param group the multicast group address.
     */
    MemberQuery(Mld6igmpVif& mld6igmp_vif, const IPvX& source,
		const IPvX& group);
    
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
     * Get the source address (used for MLDv2 or IGMPv3).
     * 
     * @return the source address.
     */
    const IPvX&	source()	const	{ return (_source);		}
    
    /**
     * Get the multicast group address.
     * 
     * @return the multicast group address.
     */
    const IPvX&	group() 	const	{ return (_group);		}
    
    /**
     * Get the number of seconds until time to query for host members.
     * 
     * @return the number of seconds until time to query for host members.
     */
    uint32_t	timeout_sec()	const	{ return (_member_query_timer.left_sec()); }
    
    /**
     * Get the address of the host who last reported as member.
     * 
     * @return the address of the host who last reported as member.
     */
    const IPvX& last_reported_host() const { return (_last_reported_host); }
    
private:
    friend class Mld6igmpVif;
    
    Mld6igmpVif& _mld6igmp_vif;		// The interface this entry belongs to
    IPvX	_source;		// The source address (MLDv2 or IGMPv3)
    IPvX	_group;			// The multicast group address
    IPvX	_last_reported_host;	// The host who last reported as member
    Timer	_member_query_timer;	// Timer to query for hosts members
    Timer	_last_member_query_timer;   // Timer to expire this entry
    Timer	_igmpv1_host_present_timer; // XXX: does not apply to MLD6
    // TODO: come-up with a better solution for those friend functions
    friend void	igmp_last_member_query_timeout(void *data_pointer);
    friend void	igmp_member_query_timeout(void *data_pointer);
    friend void	mld6_last_member_query_timeout(void *data_pointer);
    friend void	mld6_member_query_timeout(void *data_pointer);
};


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_MEMBER_QUERY_HH__
