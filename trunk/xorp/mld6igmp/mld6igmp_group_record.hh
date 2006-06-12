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

// $XORP: xorp/mld6igmp/mld6igmp_group_record.hh,v 1.5 2006/06/10 05:46:01 pavlin Exp $

#ifndef __MLD6IGMP_MLD6IGMP_GROUP_RECORD_HH__
#define __MLD6IGMP_MLD6IGMP_GROUP_RECORD_HH__


//
// IGMP and MLD group record.
//


#include <map>
#include <set>

#include "libxorp/ipvx.hh"
#include "libxorp/timer.hh"

#include "mld6igmp_source_record.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class Mld6igmpVif;

/**
 * @short A class to store information about multicast group membership.
 */
class Mld6igmpGroupRecord {
public:
    /**
     * Constructor for a given vif and group address.
     * 
     * @param mld6igmp_vif the interface this entry belongs to.
     * @param group the multicast group address.
     */
    Mld6igmpGroupRecord(Mld6igmpVif& mld6igmp_vif, const IPvX& group);
    
    /**
     * Destructor.
     */
    ~Mld6igmpGroupRecord();

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
     * Get the corresponding event loop.
     *
     * @return the corresponding event loop.
     */
    EventLoop& eventloop();

    /**
     * Test whether the filter mode is INCLUDE.
     *
     * @return true if the filter mode is INCLUDE.
     */
    bool is_include_mode() const	{ return (_is_include_mode); }

    /**
     * Test whether the filter mode is EXCLUDE.
     *
     * @return true if the filter mode is EXCLUDE.
     */
    bool is_exclude_mode() const	{ return (! _is_include_mode); }

    /**
     * Set the filter mode to INCLUDE.
     */
    void set_include_mode()		{ _is_include_mode = true; }

    /**
     * Set the filter mode to EXCLUDE.
     */
    void set_exclude_mode()		{ _is_include_mode = false; }

    /**
     * Process MODE_IS_INCLUDE report.
     *
     * @param sources the source addresses.
     */
    void process_mode_is_include(const set<IPvX>& sources);

    /**
     * Process MODE_IS_EXCLUDE report.
     *
     * @param sources the source addresses.
     */
    void process_mode_is_exclude(const set<IPvX>& sources);

    /**
     * Process CHANGE_TO_INCLUDE_MODE report.
     *
     * @param sources the source addresses.
     */
    void process_change_to_include_mode(const set<IPvX>& sources);

    /**
     * Process CHANGE_TO_EXCLUDE_MODE report.
     *
     * @param sources the source addresses.
     */
    void process_change_to_exclude_mode(const set<IPvX>& sources);

    /**
     * Process ALLOW_NEW_SOURCES report.
     *
     * @param sources the source addresses.
     */
    void process_allow_new_sources(const set<IPvX>& sources);

    /**
     * Process BLOCK_OLD_SOURCES report.
     *
     * @param sources the source addresses.
     */
    void process_block_old_sources(const set<IPvX>& sources);

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
    /**
     * Timeout: the group timer has expired.
     */
    void group_timer_timeout();

    Mld6igmpVif& _mld6igmp_vif;		// The interface this entry belongs to
    IPvX	_group;			// The multicast group address
    bool	_is_include_mode;	// Flag for INCLUDE/EXCLUDE filter mode
    Mld6igmpSourceSet _do_forward_sources;	// Sources to forward
    Mld6igmpSourceSet _dont_forward_sources;	// Sources not to forward

    IPvX	_last_reported_host;	// The host that last reported as member
    XorpTimer	_member_query_timer;	// Timer to query for host members
    XorpTimer	_last_member_query_timer;   // Timer to expire this entry
    XorpTimer	_igmpv1_host_present_timer; // XXX: does not apply to MLD
    XorpTimer	_group_timer;		// Group timer for filter mode switch
};

/**
 * @short A class to store information about a set of multicast groups.
 */
class Mld6igmpGroupSet : public map<IPvX, Mld6igmpGroupRecord *> {
public:
    /**
     * Constructor for a given vif.
     * 
     * @param mld6igmp_vif the interface this set belongs to.
     */
    Mld6igmpGroupSet(Mld6igmpVif& mld6igmp_vif);
    
    /**
     * Destructor.
     */
    ~Mld6igmpGroupSet();

    /**
     * Delete the payload of the set, and clear the set itself.
     */
    void delete_payload_and_clear();

    /**
     * Process MODE_IS_INCLUDE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_mode_is_include(const IPvX& group, const set<IPvX>& sources);

    /**
     * Process MODE_IS_EXCLUDE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_mode_is_exclude(const IPvX& group, const set<IPvX>& sources);

    /**
     * Process CHANGE_TO_INCLUDE_MODE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_change_to_include_mode(const IPvX& group,
					const set<IPvX>& sources);

    /**
     * Process CHANGE_TO_EXCLUDE_MODE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_change_to_exclude_mode(const IPvX& group,
					const set<IPvX>& sources);

    /**
     * Process ALLOW_NEW_SOURCES report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_allow_new_sources(const IPvX& group,
				   const set<IPvX>& sources);

    /**
     * Process BLOCK_OLD_SOURCES report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     */
    void process_block_old_sources(const IPvX& group,
				   const set<IPvX>& sources);

private:
    Mld6igmpVif& _mld6igmp_vif;		// The interface this set belongs to
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_GROUP_RECORD_HH__
