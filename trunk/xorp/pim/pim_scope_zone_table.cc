// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_scope_zone_table.cc,v 1.5 2003/03/03 02:07:00 pavlin Exp $"


//
// PIM scope zone table implementation.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_scope_zone_table.hh"


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
 * PimScopeZoneId::PimScopeZoneId:
 * @scope_zone_prefix: The scope zone address prefix.
 * @is_scope_zone: If true, this is administratively scoped zone, otherwise
 * this is the global zone.
 * 
 * PimScopeZoneId constructor.
 **/
PimScopeZoneId::PimScopeZoneId(const IPvXNet& scope_zone_prefix,
			       bool is_scope_zone)
    : _scope_zone_prefix(scope_zone_prefix),
      _is_scope_zone(is_scope_zone)
{
    
}

/**
 * Equality Operator
 * 
 * @param other the right-hand operand to compare against.
 * @return true if the left-hand operand is numerically same as the
 * right-hand operand.
 */
bool
PimScopeZoneId::operator==(const PimScopeZoneId& other) const
{
    return ((scope_zone_prefix() == other.scope_zone_prefix())
	    && (is_scope_zone() == other.is_scope_zone()));
}

/**
 * PimScopeZoneId::is_overlap:
 * 
 * Tehst whether a scope zone overlaps with another zone.
 * 
 * @param other the other scope zone ID to compare against.
 * @return true if both zones are scoped and the the scope zone prefixes
 * for this zone and @other do overlap.
 */
bool
PimScopeZoneId::is_overlap(const PimScopeZoneId& other) const
{
    return (is_scope_zone()
	    && other.is_scope_zone()
	    && scope_zone_prefix().is_overlap(other.scope_zone_prefix()));
}

/**
 * PimScopeZoneId::contains:
 * @ipvxnet: The subnet address to compare whether is contained within this
 * scope zone.
 * 
 * Test whether a scope zone contains a subnet address.
 * 
 * Return value: true if this scope zone contains @ipvxnet, otherwise false.
 **/
bool
PimScopeZoneId::contains(const IPvXNet& ipvxnet) const
{
    return (scope_zone_prefix().contains(ipvxnet));
}

/**
 * PimScopeZoneId::contains:
 * @ipvx: The address to compare whether is contained within this
 * scope zone.
 * 
 * Test whether a scope zone contains an address.
 * 
 * Return value: true if this scope zone contains @ipvx, otherwise false.
 **/
bool
PimScopeZoneId::contains(const IPvX& ipvx) const
{
    return (scope_zone_prefix().contains(ipvx));
}

/**
 * PimScopeZoneId::str:
 * 
 * Convert this scope zone ID from binary for to presentation format.
 * 
 * Return value: C++ string with the human-readable ASCII representation
 * of the scope zone ID.
 **/
string
PimScopeZoneId::str() const
{
    return (c_format("%s(%s)", _scope_zone_prefix.str().c_str(),
		     _is_scope_zone? "scoped" : "non-scoped"));
}

/**
 * PimScopeZoneTable::PimScopeZoneTable:
 * @pim_node: The associated PIM node.
 * 
 * PimScopeZoneTable constructor.
 **/
PimScopeZoneTable::PimScopeZoneTable(PimNode& pim_node)
    : _pim_node(pim_node)
{
    
}

/**
 * PimScopeZoneTable::~PimScopeZoneTable:
 * @void: 
 * 
 * PimScopeZoneTable destructor.
 * 
 **/
PimScopeZoneTable::~PimScopeZoneTable(void)
{
    
}

/**
 * PimScopeZoneTable::add_scope_zone:
 * @scope_zone_prefix: The prefix address of the zone to add.
 * @vif_index: The vif index of the interface to add as zone boundary.
 * 
 * Add a scope zone.
 **/
void
PimScopeZoneTable::add_scope_zone(const IPvXNet& scope_zone_prefix,
				  uint16_t vif_index)
{
    // Test first if we have that scope zone already
    list<PimScopeZone>::iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_same_scope_zone(scope_zone_prefix)) {
	    // We already have entry for this scope zone
	    pim_scope_zone.set_scoped_vif(vif_index, true);
	    return;
	}
    }
    
    // Add the new scope
    Mifset scoped_vifs;
    scoped_vifs.set(vif_index);
    PimScopeZone pim_scope_zone(scope_zone_prefix, scoped_vifs);
    _pim_scope_zone_list.push_back(pim_scope_zone);
}

/**
 * PimScopeZoneTable::delete_scope_zone:
 * @scope_zone_prefix: The prefix address of the zone to delete.
 * @vif_index: The vif index of the interface to delete as zone boundary.
 * 
 * Delete a scope zone.
 **/
void
PimScopeZoneTable::delete_scope_zone(const IPvXNet& scope_zone_prefix,
				     uint16_t vif_index)
{
    // Find the scope zone and delete it
    list<PimScopeZone>::iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_same_scope_zone(scope_zone_prefix)) {
	    // Found
	    pim_scope_zone.set_scoped_vif(vif_index, false);
	    // If last scope zone boundary, remove the entry
	    if (pim_scope_zone.is_empty())
		_pim_scope_zone_list.erase(iter);
	    return;
	}
    }
}

/**
 * PimScopeZoneTable::is_scoped:
 * @addr: The address to test whether it is scoped.
 * @vif_index: The vif index of the interface to test.
 * 
 * Test if address @addr is scoped on interface with vif index of @vif_index.
 * Note that we test against all scope zones until a scoping one is found.
 * 
 * Return value: True if @addr is scoped on @vif_index, otherwise false.
 **/
bool
PimScopeZoneTable::is_scoped(const IPvX& addr, uint16_t vif_index) const
{
    list<PimScopeZone>::const_iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	const PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_scoped(addr, vif_index))
	    return (true);
    }
    
    return (false);
}

/**
 * PimScopeZoneTable::is_scoped:
 * @zone_id: The zone ID to test whether it is scoped.
 * @vif_index: The vif index of the interface to test.
 * 
 * Test if zone with zone ID of @zone_id is scoped on interface with
 * vif index of @vif_index.
 * Note that we test against all scope zones until a scoping one is found.
 * 
 * Return value: True if @zone_id is scoped on @vif_index, otherwise false.
 **/
bool
PimScopeZoneTable::is_scoped(const PimScopeZoneId& zone_id,
			     uint16_t vif_index) const
{
    if (! zone_id.is_scope_zone())
	return (false);
    
    list<PimScopeZone>::const_iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	const PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_scoped(zone_id, vif_index))
	    return (true);
    }
    
    return (false);
}

/**
 * PimScopeZoneTable::is_zone_border_router:
 * @group_prefix: The group prefix to test.
 * 
 * Test if the router is a Zone Border Router (ZBR) for @group_prefix.
 * 
 * Return value: True if the router is a ZBR for @group_prefix, otherwise
 * false.
 **/
bool
PimScopeZoneTable::is_zone_border_router(const IPvXNet& group_prefix) const
{
    list<PimScopeZone>::const_iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	const PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.scope_zone_prefix().contains(group_prefix))
	    return (true);
    }
    
    return (false);
}

/**
 * PimScopeZone::PimScopeZone:
 * @scope_zone_prefix: The scope zone address prefix.
 * @scoped_vifs: The bitmask with the vifs that are zone boundary.
 * 
 * PimScopeZone constructor.
 **/
PimScopeZone::PimScopeZone(const IPvXNet& scope_zone_prefix,
			   const Mifset& scoped_vifs)
    : _scope_zone_prefix(scope_zone_prefix),
      _scoped_vifs(scoped_vifs)
{
    
}

/**
 * PimScopeZone::~PimScopeZone:
 * @: 
 * 
 * PimScopeZone destructor.
 **/
PimScopeZone::~PimScopeZone()
{
    
}

/**
 * PimScopeZone::set_scoped_vif:
 * @vif_index: The vif index of the interface to set or reset.
 * @v: If true, set the interface as a boundary for this scope zone,
 * otherwise reset it.
 * 
 * Set or reset an interface as a boundary for this scope zone.
 **/
void
PimScopeZone::set_scoped_vif(uint16_t vif_index, bool v)
{
    if (vif_index < _scoped_vifs.size()) {
	if (v)
	    _scoped_vifs.set(vif_index);
	else
	    _scoped_vifs.reset(vif_index);
    }
}

/**
 * PimScopeZone::is_scoped:
 * @addr: The address to test whether it is scoped.
 * @vif_index: The vif index of the interface to test.
 * 
 * Test if address @addr is scoped on interface with vif index of @vif_index.
 * 
 * Return value: True if @addr is scoped on @vif_index, otherwise false.
 **/
bool
PimScopeZone::is_scoped(const IPvX& addr, uint16_t vif_index) const
{
    if (! _scope_zone_prefix.contains(addr))
	return (false);
    
    return (is_set(vif_index));
}

/**
 * PimScopeZone::is_scoped:
 * @zone_id: The zone ID to test whether it is scoped.
 * @vif_index: The vif index of the interface to test.
 * 
 * Test if zone with zone ID of @zone_id is scoped on interface with
 * vif index of @vif_index.
 * 
 * Return value: True if @zone_id is scoped on @vif_index, otherwise false.
 **/
bool
PimScopeZone::is_scoped(const PimScopeZoneId& zone_id,
			uint16_t vif_index) const
{
    if (! zone_id.is_scope_zone())
	return (false);
    
    // XXX: scoped zones don't nest, hence if the scope zone prefixes
    // do overlap, then there is scoping.
    if (! _scope_zone_prefix.is_overlap(zone_id.scope_zone_prefix()))
	return (false);
    
    return (is_set(vif_index));
}

/**
 * PimScopeZone::is_set:
 * @vif_index: The vif index of the interface to test.
 * 
 * Test if an interface is a boundary for this scope zone.
 * 
 * Return value: True if the tested interface is a boundary for this scope
 * zone, otherwise false.
 **/
bool
PimScopeZone::is_set(uint16_t vif_index) const
{
    if (vif_index < _scoped_vifs.size())
	return (_scoped_vifs.test(vif_index));
    return (false);
}

/**
 * PimScopeZone::is_same_scope_zone:
 * @scope_zone_prefix: The prefix address to test if it is the prefix
 * address of this scope zone.
 * 
 * Test if this scope zone prefix address is same as @scope_zone_prefix.
 * 
 * Return value: True if @scope_zone_prefix is same prefix address
 * as the prefix address of this scope zone, otherwise false.
 **/
bool
PimScopeZone::is_same_scope_zone(const IPvXNet& scope_zone_prefix) const
{
    return (_scope_zone_prefix == scope_zone_prefix);
}
