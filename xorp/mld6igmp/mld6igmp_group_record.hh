// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __MLD6IGMP_MLD6IGMP_GROUP_RECORD_HH__
#define __MLD6IGMP_MLD6IGMP_GROUP_RECORD_HH__


//
// IGMP and MLD group record.
//

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
     * Test whether the entry is unused.
     *
     * @return true if the entry is unused, otherwise false.
     */
    bool is_unused() const;

    /**
     * Find a source that should be forwarded.
     *
     * @param source the source address.
     * @return the corresponding source record (@ref Mld6igmpSourceRecord)
     * if found, otherwise NULL.
     */
    Mld6igmpSourceRecord* find_do_forward_source(const IPvX& source);

    /**
     * Find a source that should not be forwarded.
     *
     * @param source the source address.
     * @return the corresponding source record (@ref Mld6igmpSourceRecord)
     * if found, otherwise NULL.
     */
    Mld6igmpSourceRecord* find_dont_forward_source(const IPvX& source);

    /**
     * Get a reference to the set of sources to forward.
     *
     * @return a reference to the set of sources to forward.
     */
    const Mld6igmpSourceSet& do_forward_sources() const { return (_do_forward_sources); }

    /**
     * Get a reference to the set of sources not to forward.
     *
     * @return a reference to the set of sources not to forward.
     */
    const Mld6igmpSourceSet& dont_forward_sources() const { return (_dont_forward_sources); }

    /**
     * Process MODE_IS_INCLUDE report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_mode_is_include(const set<IPvX>& sources,
				 const IPvX& last_reported_host);

    /**
     * Process MODE_IS_EXCLUDE report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_mode_is_exclude(const set<IPvX>& sources,
				 const IPvX& last_reported_host);

    /**
     * Process CHANGE_TO_INCLUDE_MODE report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_change_to_include_mode(const set<IPvX>& sources,
					const IPvX& last_reported_host);

    /**
     * Process CHANGE_TO_EXCLUDE_MODE report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_change_to_exclude_mode(const set<IPvX>& sources,
					const IPvX& last_reported_host);

    /**
     * Process ALLOW_NEW_SOURCES report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_allow_new_sources(const set<IPvX>& sources,
				   const IPvX& last_reported_host);

    /**
     * Process BLOCK_OLD_SOURCES report.
     *
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_block_old_sources(const set<IPvX>& sources,
				   const IPvX& last_reported_host);

    /**
     * Lower the group timer.
     *
     * @param timeval the timeout interval the timer should be lowered to.
     */
    void lower_group_timer(const TimeVal& timeval);

    /**
     * Lower the source timer for a set of sources.
     *
     * @param sources the source addresses.
     * @param timeval the timeout interval the timer should be lowered to.
     */
    void lower_source_timer(const set<IPvX>& sources, const TimeVal& timeval);

    /**
     * Take the appropriate actions for a source that has expired.
     *
     * @param source_record the source record that has expired.
     */
    void source_expired(Mld6igmpSourceRecord* source_record);

    /**
     * Get the number of seconds until the group timer expires.
     * 
     * @return the number of seconds until the group timer expires.
     */
    uint32_t	timeout_sec()	const;
    
    /**
     * Get the address of the host that last reported as member.
     * 
     * @return the address of the host that last reported as member.
     */
    const IPvX& last_reported_host() const { return (_last_reported_host); }

    /**
     * Get a refererence to the group timer.
     *
     * @return a reference to the group timer.
     */
    XorpTimer& group_timer() { return _group_timer; }

    /**
     * Schedule periodic Group-Specific and Group-and-Source-Specific Query
     * retransmission.
     *
     * If the sources list is empty, we schedule Group-Specific Query,
     * otherwise we schedule Group-and-Source-Specific Query.
     *
     * @param sources the source addresses.
     */
    void schedule_periodic_group_query(const set<IPvX>& sources);

    /**
     * Record that an older Membership report message has been received.
     *
     * @param message_version the corresponding protocol version of the
     * received message.
     */
    void received_older_membership_report(int message_version);

    /**
     * Test if the group is running in IGMPv1 mode.
     *
     * @return true if the group is running in IGMPv1 mode, otherwise false.
     */
    bool	is_igmpv1_mode() const;

    /**
     * Test if the group is running in IGMPv2 mode.
     *
     * @return true if the group is running in IGMPv2 mode, otherwise false.
     */
    bool	is_igmpv2_mode() const;

    /**
     * Test if the group is running in IGMPv3 mode.
     *
     * @return true if the group is running in IGMPv3 mode, otherwise false.
     */
    bool	is_igmpv3_mode() const;

    /**
     * Test if the group is running in MLDv1 mode.
     *
     * @return true if the group is running in MLDv1 mode, otherwise false.
     */
    bool	is_mldv1_mode() const;

    /**
     * Test if the group is running in MLDv2 mode.
     *
     * @return true if the group is running in MLDv2 mode, otherwise false.
     */
    bool	is_mldv2_mode() const;

    /**
     * Get the address family.
     *
     * @return the address family.
     */
    int		family() const { return _group.af(); }

private:
    /**
     * Calculate the forwarding changes and notify the interested parties.
     *
     * @param old_is_include mode if true, the old filter mode was INCLUDE,
     * otherwise was EXCLUDE.
     * @param old_do_forward_sources the old set of sources to forward.
     * @param old_dont_forward_sources the old set of sources not to forward.
     */
    void calculate_forwarding_changes(bool old_is_include_mode,
				      const set<IPvX>& old_do_forward_sources,
				      const set<IPvX>& old_dont_forward_sources) const;

    /**
     * Timeout: one of the older version host present timers has expired.
     */
    void	older_version_host_present_timer_timeout();

    /**
     * Timeout: the group timer has expired.
     */
    void	group_timer_timeout();

    /**
     * Periodic timeout: time to send the next Group-Specific and
     * Group-and-Source-Specific Queries.
     *
     * @return true if the timer should be scheduled again, otherwise false.
     */
    bool	group_query_periodic_timeout();

    /**
     * Set the address of the host that last reported as member.
     *
     * @param v the address of the host that last reported as member.
     */
    void set_last_reported_host(const IPvX& v) { _last_reported_host = v; }

    Mld6igmpVif& _mld6igmp_vif;		// The interface this entry belongs to
    IPvX	_group;			// The multicast group address
    bool	_is_include_mode;	// Flag for INCLUDE/EXCLUDE filter mode
    Mld6igmpSourceSet _do_forward_sources;	// Sources to forward
    Mld6igmpSourceSet _dont_forward_sources;	// Sources not to forward

    IPvX	_last_reported_host;	// The host that last reported as member

    // Timers indicating that hosts running older protocol version are present
    XorpTimer	_igmpv1_host_present_timer;
    XorpTimer	_igmpv2_mldv1_host_present_timer;

    XorpTimer	_group_timer;		// Group timer for filter mode switch
    XorpTimer	_group_query_timer;	// Timer for periodic Queries
    size_t	_query_retransmission_count; // Count for periodic Queries
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
     * Find a group record.
     *
     * @param group the group address.
     * @return the corresponding group record (@ref Mld6igmpGroupRecord)
     * if found, otherwise NULL.
     */
    Mld6igmpGroupRecord* find_group_record(const IPvX& group);

    /**
     * Delete the payload of the set, and clear the set itself.
     */
    void delete_payload_and_clear();

    /**
     * Process MODE_IS_INCLUDE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_mode_is_include(const IPvX& group, const set<IPvX>& sources,
				 const IPvX& last_reported_host);

    /**
     * Process MODE_IS_EXCLUDE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_mode_is_exclude(const IPvX& group, const set<IPvX>& sources,
				 const IPvX& last_reported_host);

    /**
     * Process CHANGE_TO_INCLUDE_MODE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_change_to_include_mode(const IPvX& group,
					const set<IPvX>& sources,
					const IPvX& last_reported_host);

    /**
     * Process CHANGE_TO_EXCLUDE_MODE report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_change_to_exclude_mode(const IPvX& group,
					const set<IPvX>& sources,
					const IPvX& last_reported_host);

    /**
     * Process ALLOW_NEW_SOURCES report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_allow_new_sources(const IPvX& group,
				   const set<IPvX>& sources,
				   const IPvX& last_reported_host);

    /**
     * Process BLOCK_OLD_SOURCES report.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param last_reported_host the address of the host that last reported
     * as member.
     */
    void process_block_old_sources(const IPvX& group,
				   const set<IPvX>& sources,
				   const IPvX& last_reported_host);

    /**
     * Lower the group timer.
     *
     * @param group the group address.
     * @param timeval the timeout interval the timer should be lowered to.
     */
    void lower_group_timer(const IPvX& group, const TimeVal& timeval);

    /**
     * Lower the source timer for a set of sources.
     *
     * @param group the group address.
     * @param sources the source addresses.
     * @param timeval the timeout interval the timer should be lowered to.
     */
    void lower_source_timer(const IPvX& group, const set<IPvX>& sources,
			    const TimeVal& timeval);

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
