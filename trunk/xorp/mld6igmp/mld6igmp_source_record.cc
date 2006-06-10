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

#ident "$XORP: xorp/mld6igmp/mld6igmp_source_record.cc,v 1.1 2006/06/07 05:36:34 pavlin Exp $"

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
      _source(source)
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

Mld6igmpSourceSet::Mld6igmpSourceSet(Mld6igmpGroupRecord& group_record)
    : _group_record(group_record)
{

}

Mld6igmpSourceSet::~Mld6igmpSourceSet()
{
    // XXX: don't delete the payload, because it might be used elsewhere
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
