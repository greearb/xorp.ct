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

#ident "$XORP: xorp/pim/pim_scope_zone_table.cc,v 1.3 2002/12/09 18:29:31 hodson Exp $"


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
 * 
 * Add a scope zone.
 **/
void
PimScopeZoneTable::add_scope_zone(const IPvXNet& scope_zone_prefix)
{
    // Test first if we have that scope zone already
    list<PimScopeZone>::iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_same_scope_zone(scope_zone_prefix))
	    return;		// We already have this scope zone
    }
    
    // Add the new scope
    PimScopeZone pim_scope_zone(scope_zone_prefix);
    _pim_scope_zone_list.push_back(scope_zone_prefix);
}

/**
 * PimScopeZoneTable::delete_scope_zone:
 * @scope_zone_prefix: The prefix address of the zone to delete.
 * 
 * Delete a scope zone.
 **/
void
PimScopeZoneTable::delete_scope_zone(const IPvXNet& scope_zone_prefix)
{
    // Find the scope zone and delete it
    list<PimScopeZone>::iterator iter;
    for (iter = _pim_scope_zone_list.begin();
	 iter != _pim_scope_zone_list.end();
	 ++iter) {
	PimScopeZone& pim_scope_zone = *iter;
	if (pim_scope_zone.is_same_scope_zone(scope_zone_prefix)) {
	    // Found
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
 * PimScopeZone::PimScopeZone:
 * @scope_zone_prefix: The scope zone address prefix.
 * 
 * PimScopeZone constructor.
 **/
PimScopeZone::PimScopeZone(const IPvXNet& scope_zone_prefix)
    : _scope_zone_prefix(scope_zone_prefix)
{
    
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
 * @vif_index: The vif index of the interface to set.
 * 
 * Set an interface as a boundary for this scope zone.
 **/
void
PimScopeZone::set_scoped_vif(uint16_t vif_index)
{
    if (vif_index <= _scoped_vifs.size())
	_scoped_vifs.set(vif_index);
}

/**
 * PimScopeZone::reset_scoped_vif:
 * @vif_index: The vif index of the interface to reset.
 * 
 * Reset/remove an interface as a boundary for this scope zone.
 **/
void
PimScopeZone::reset_scoped_vif(uint16_t vif_index)
{
    if (vif_index <= _scoped_vifs.size())
	_scoped_vifs.reset(vif_index);
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
