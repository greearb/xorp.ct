// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



//
// Multicast source record information used by IGMPv3 (RFC 3376) and
// MLDv2 (RFC 3810).
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mld6igmp_source_record.hh"
#include "mld6igmp_group_record.hh"
#include "mld6igmp_vif.hh"


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
 * Mld6igmpSourceRecord::Mld6igmpSourceRecord:
 * @group_record: The group record this entry belongs to.
 * @source: The entry source address.
 * 
 * Return value: 
 **/
Mld6igmpSourceRecord::Mld6igmpSourceRecord(Mld6igmpGroupRecord& group_record,
					   const IPvX& source)
    : _group_record(group_record),
      _source(source),
      _query_retransmission_count(0)
{
    
}

/**
 * Mld6igmpSourceRecord::~Mld6igmpSourceRecord:
 * @: 
 * 
 * Mld6igmpSourceRecord destructor.
 **/
Mld6igmpSourceRecord::~Mld6igmpSourceRecord()
{

}

/**
 * Set the source timer.
 *
 * @param timeval the timeout interval of the source timer.
 */
void
Mld6igmpSourceRecord::set_source_timer(const TimeVal& timeval)
{
    EventLoop& eventloop = _group_record.eventloop();

    _source_timer = eventloop.new_oneoff_after(
	timeval,
	callback(this, &Mld6igmpSourceRecord::source_timer_timeout));
}

/**
 * Cancel the source timer.
 */
void
Mld6igmpSourceRecord::cancel_source_timer()
{
    _source_timer.unschedule();
}

/**
 * Lower the source timer.
 *
 * @param timeval the timeout interval the source timer should be
 * lowered to.
 */
void
Mld6igmpSourceRecord::lower_source_timer(const TimeVal& timeval)
{
    EventLoop& eventloop = _group_record.eventloop();
    TimeVal timeval_remaining;

    //
    // Lower the source timer
    //
    _source_timer.time_remaining(timeval_remaining);
    if (timeval < timeval_remaining) {
	_source_timer = eventloop.new_oneoff_after(
	    timeval,
	    callback(this, &Mld6igmpSourceRecord::source_timer_timeout));
    }
}

/**
 * Timeout: the source timer has expired.
 */
void
Mld6igmpSourceRecord::source_timer_timeout()
{
    _group_record.source_expired(this);
}

/**
 * Get the number of seconds until the source timer expires.
 * 
 * @return the number of seconds until the source timer expires.
 */
uint32_t
Mld6igmpSourceRecord::timeout_sec() const
{
    TimeVal tv;
    
    _source_timer.time_remaining(tv);
    
    return (tv.sec());
}

/**
 * Constructor for a given group record.
 *
 * @param group_record the group record this set belongs to.
 */
Mld6igmpSourceSet::Mld6igmpSourceSet(Mld6igmpGroupRecord& group_record)
    : _group_record(group_record)
{

}

/**
 * Destructor.
 */
Mld6igmpSourceSet::~Mld6igmpSourceSet()
{
    // XXX: don't delete the payload, because it might be used elsewhere
}

/**
 * Find a source record.
 *
 * @param source the source address.
 * @return the corresponding source record (@ref Mld6igmpSourceRecord)
 * if found, otherwise NULL.
 */
Mld6igmpSourceRecord*
Mld6igmpSourceSet::find_source_record(const IPvX& source)
{
    Mld6igmpSourceSet::iterator iter = this->find(source);

    if (iter != this->end())
	return (iter->second);

    return (NULL);
}

/**
 * Delete the payload of the set, and clear the set itself.
 */
void
Mld6igmpSourceSet::delete_payload_and_clear()
{
    Mld6igmpSourceSet::iterator iter;

    //
    // Delete the payload of the set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	Mld6igmpSourceRecord* source_record = iter->second;
	delete source_record;
    }

    //
    // Clear the set itself
    //
    this->clear();
}

Mld6igmpSourceSet::Mld6igmpSourceSet(const Mld6igmpSourceSet& other) :
	map<IPvX, Mld6igmpSourceRecord *>(other), _group_record(other._group_record) {
}

/**
 * Assignment operator for sets.
 *
 * @param other the right-hand operand.
 * @return the assigned set.
 */
Mld6igmpSourceSet&
Mld6igmpSourceSet::operator=(const Mld6igmpSourceSet& other)
{
    Mld6igmpSourceSet::const_iterator iter;

    XLOG_ASSERT(&_group_record == &(other._group_record));

    this->clear();

    //
    // Copy the payload of the set
    //
    for (iter = other.begin(); iter != other.end(); ++iter) {
	insert(make_pair(iter->first, iter->second));
    }

    return (*this);
}

/**
 * UNION operator for sets.
 *
 * @param other the right-hand operand.
 * @return the union of two sets. Note that if an element is in
 * both sets, we use the value from the first set.
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator+(const Mld6igmpSourceSet& other)
{
    Mld6igmpSourceSet result(*this);	// XXX: all elements from the first set
    Mld6igmpSourceSet::const_iterator iter;

    //
    // Insert all elements from the second set that are not in the first set
    //
    for (iter = other.begin(); iter != other.end(); ++iter) {
	const IPvX& ipvx = iter->first;
	if (result.find(ipvx) == result.end())
	    result.insert(make_pair(iter->first, iter->second));
    }

    return (result);
}

/**
 * UNION operator for sets when the second operand is a set of IPvX
 * addresses.
 *
 * @param other the right-hand operand.
 * @return the union of two sets. Note that if an element is not in the
 * first set, then it is created (see @ref Mld6igmpSourceRecord).
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator+(const set<IPvX>& other)
{
    Mld6igmpSourceSet result(*this);	// XXX: all elements from the first set
    set<IPvX>::const_iterator iter;
    Mld6igmpSourceRecord* source_record;

    //
    // Insert all elements from the second set that are not in the first set
    //
    for (iter = other.begin(); iter != other.end(); ++iter) {
	const IPvX& ipvx = *iter;
	if (result.find(ipvx) == result.end()) {
	    source_record = new Mld6igmpSourceRecord(_group_record, ipvx);
	    result.insert(make_pair(ipvx, source_record));
	}
    }

    return (result);
}

/**
 * INTERSECTION operator for sets.
 *
 * @param other the right-hand operand.
 * @return the intersection of two sets. Note that we use the values
 * from the first set.
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator*(const Mld6igmpSourceSet& other)
{
    Mld6igmpSourceSet result(_group_record);
    Mld6igmpSourceSet::const_iterator iter;

    //
    // Insert all elements from the first set that are also in the second set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	const IPvX& ipvx = iter->first;
	if (other.find(ipvx) != other.end())
	    result.insert(make_pair(iter->first, iter->second));
    }

    return (result);
}

/**
 * INTERSECTION operator for sets when the second operand is a set of IPvX
 * addresses.
 *
 * @param other the right-hand operand.
 * @return the intersection of two sets. Note that we use the values
 * from the first set.
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator*(const set<IPvX>& other)
{
    Mld6igmpSourceSet result(_group_record);
    Mld6igmpSourceSet::const_iterator iter;

    //
    // Insert all elements from the first set that are also in the second set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	const IPvX& ipvx = iter->first;
	if (other.find(ipvx) != other.end())
	    result.insert(make_pair(iter->first, iter->second));
    }

    return (result);
}

/**
 * REMOVAL operator for sets.
 *
 * @param other the right-hand operand.
 * @return the elements from the first set (after the elements from
 * the right-hand set have been removed).
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator-(const Mld6igmpSourceSet& other)
{
    Mld6igmpSourceSet result(_group_record);
    Mld6igmpSourceSet::const_iterator iter;

    //
    // Insert all elements from the first set that are not in the second set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	const IPvX& ipvx = iter->first;
	if (other.find(ipvx) == other.end())
	    result.insert(make_pair(iter->first, iter->second));
    }

    return (result);
}

/**
 * REMOVAL operator for sets when the second operand is a set of IPvX
 * addresses.
 *
 * @param other the right-hand operand.
 * @return the elements from the first set (after the elements from
 * the right-hand set have been removed).
 */
Mld6igmpSourceSet
Mld6igmpSourceSet::operator-(const set<IPvX>& other)
{
    Mld6igmpSourceSet result(_group_record);
    Mld6igmpSourceSet::const_iterator iter;

    //
    // Insert all elements from the first set that are not in the second set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	const IPvX& ipvx = iter->first;
	if (other.find(ipvx) == other.end())
	    result.insert(make_pair(iter->first, iter->second));
    }

    return (result);
}

/**
 * Set the source timer for a set of source addresses.
 *
 * @param sources the set of source addresses whose source timer will
 * be set.
 * @param timeval the timeout interval of the source timer.
 */
void
Mld6igmpSourceSet::set_source_timer(const set<IPvX>& sources,
				    const TimeVal& timeval)
{
    set<IPvX>::const_iterator iter;
    Mld6igmpSourceSet::iterator record_iter;

    for (iter = sources.begin(); iter != sources.end(); ++iter) {
	const IPvX& ipvx = *iter;
	record_iter = this->find(ipvx);
	if (record_iter != this->end()) {
	    Mld6igmpSourceRecord* source_record = record_iter->second;
	    source_record->set_source_timer(timeval);
	}
    }
}

/**
 * Set the source timer for all source addresses.
 *
 * @param timeval the timeout interval of the source timer.
 */
void
Mld6igmpSourceSet::set_source_timer(const TimeVal& timeval)
{
    Mld6igmpSourceSet::iterator iter;

    for (iter = this->begin(); iter != this->end(); ++iter) {
	Mld6igmpSourceRecord* source_record = iter->second;
	source_record->set_source_timer(timeval);
    }
}

/**
 * Cancel the source timer for a set of source addresses.
 *
 * @param sources the set of source addresses whose source timer will
 * be canceled.
 */
void
Mld6igmpSourceSet::cancel_source_timer(const set<IPvX>& sources)
{
    set<IPvX>::const_iterator iter;
    Mld6igmpSourceSet::iterator record_iter;

    for (iter = sources.begin(); iter != sources.end(); ++iter) {
	const IPvX& ipvx = *iter;
	record_iter = this->find(ipvx);
	if (record_iter != this->end()) {
	    Mld6igmpSourceRecord* source_record = record_iter->second;
	    source_record->cancel_source_timer();
	}
    }
}

/**
 * Cancel the source timer for all source addresses.
 */
void
Mld6igmpSourceSet::cancel_source_timer()
{
    Mld6igmpSourceSet::iterator iter;

    for (iter = this->begin(); iter != this->end(); ++iter) {
	Mld6igmpSourceRecord* source_record = iter->second;
	source_record->cancel_source_timer();
    }
}

/**
 * Lower the source timer for a set of sources.
 *
 * @param sources the source addresses.
 * @param timeval the timeout interval the source timer should be
 * lowered to.
 */
void
Mld6igmpSourceSet::lower_source_timer(const set<IPvX>& sources,
				      const TimeVal& timeval)
{
    set<IPvX>::const_iterator iter;
    Mld6igmpSourceSet::iterator record_iter;

    for (iter = sources.begin(); iter != sources.end(); ++iter) {
	const IPvX& ipvx = *iter;
	record_iter = this->find(ipvx);
	if (record_iter != this->end()) {
	    Mld6igmpSourceRecord* source_record = record_iter->second;
	    source_record->lower_source_timer(timeval);
	}
    }
}

/**
 * Extract the set of source addresses.
 *
 * @return the set with the source addresses.
 */
set<IPvX>
Mld6igmpSourceSet::extract_source_addresses() const
{
    set<IPvX> sources;
    Mld6igmpSourceSet::const_iterator record_iter;

    for (record_iter = this->begin();
	 record_iter != this->end();
	 ++record_iter) {
	const Mld6igmpSourceRecord* source_record = record_iter->second;
	const IPvX& ipvx = source_record->source();
	sources.insert(ipvx);
    }

    return (sources);
}
