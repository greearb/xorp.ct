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

// $XORP: xorp/pim/pim_scope_zone_table.hh,v 1.6 2003/03/03 04:27:41 pavlin Exp $


#ifndef __PIM_PIM_SCOPE_ZONE_TABLE_HH__
#define __PIM_PIM_SCOPE_ZONE_TABLE_HH__


//
// PIM scope zone table definition.
//


#include <list>

#include "libxorp/ipvxnet.hh"
#include "mrt/mifset.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class PimNode;
class PimScopeZone;

// The PIM Scope Zone ID
class PimScopeZoneId {
public:
    PimScopeZoneId(const IPvXNet& scope_zone_prefix, bool is_scope_zone);
    
    const IPvXNet& scope_zone_prefix() const { return (_scope_zone_prefix); }
    bool is_scope_zone() const { return (_is_scope_zone); }
    bool operator==(const PimScopeZoneId& other) const;
    bool is_overlap(const PimScopeZoneId& other) const;
    bool contains(const IPvXNet& ipvxnet) const;
    bool contains(const IPvX& ipvx) const;
    string str() const;
    
private:
    IPvXNet	_scope_zone_prefix;	// The scope zone address prefix
    bool	_is_scope_zone;		// If true, this is admin. scoped zone
};


// PIM-specific scope zone table
class PimScopeZoneTable {
public:
    PimScopeZoneTable(PimNode& pim_node);
    virtual ~PimScopeZoneTable();
    
    list<PimScopeZone>& pim_scope_zone_list() { return (_pim_scope_zone_list); }
    void add_scope_zone(const IPvXNet& scope_zone_prefix, uint16_t vif_index);
    void delete_scope_zone(const IPvXNet& scope_zone_prefix,
			   uint16_t vif_index);
    
    bool is_scoped(const IPvX& addr, uint16_t vif_index) const;
    bool is_scoped(const PimScopeZoneId& zone_id, uint16_t vif_index) const;
    bool is_zone_border_router(const IPvXNet& group_prefix) const;
    
    PimNode&	pim_node() const	{ return (_pim_node);		}
    
private:
    // Private functions
    
    // Private state
    PimNode&	_pim_node;			// The PIM node I belong to
    list<PimScopeZone> _pim_scope_zone_list;	// The list with scoped zones
};

class PimScopeZone {
public:
    PimScopeZone(const IPvXNet& scope_zone_prefix, const Mifset& scoped_vifs);
    virtual ~PimScopeZone();
    
    const IPvXNet& scope_zone_prefix() const { return (_scope_zone_prefix); }
    void set_scoped_vif(uint16_t vif_index, bool v);
    
    bool is_empty() const { return (! _scoped_vifs.any()); }
    bool is_set(uint16_t vif_index) const;
    bool is_scoped(const IPvX& addr, uint16_t vif_index) const;
    bool is_scoped(const PimScopeZoneId& zone_id, uint16_t vif_index) const;
    bool is_same_scope_zone(const IPvXNet& scope_zone_prefix) const;
    
private:
    IPvXNet	_scope_zone_prefix;		// The scoped zone prefix
    Mifset	_scoped_vifs;			// The set of scoped vifs
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_SCOPE_ZONE_TABLE_HH__
