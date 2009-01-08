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

// $XORP: xorp/pim/pim_scope_zone_table.hh,v 1.15 2008/10/02 21:57:55 bms Exp $


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
    void add_scope_zone(const IPvXNet& scope_zone_prefix, uint32_t vif_index);
    void delete_scope_zone(const IPvXNet& scope_zone_prefix,
			   uint32_t vif_index);
    
    bool is_scoped(const IPvX& addr, uint32_t vif_index) const;
    bool is_scoped(const PimScopeZoneId& zone_id, uint32_t vif_index) const;
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
    void set_scoped_vif(uint32_t vif_index, bool v);
    
    bool is_empty() const { return (! _scoped_vifs.any()); }
    bool is_set(uint32_t vif_index) const;
    bool is_scoped(const IPvX& addr, uint32_t vif_index) const;
    bool is_scoped(const PimScopeZoneId& zone_id, uint32_t vif_index) const;
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
