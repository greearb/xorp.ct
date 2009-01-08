// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libproto/proto_unit.cc,v 1.16 2008/10/02 21:57:18 bms Exp $"


//
// Protocol unit generic functionality implementation
//


#include "libproto_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "proto_unit.hh"


//
// Exported variables
//

//
// Local constants definitions
//

// XXX: must be consistent with xorp_module_id definition
// (TODO: a temp. solution)
// TODO: the _4/6 suffix is a temp. solution
static const char *_xorp_module_name[][2] = {
    { "XORP_MODULE_NULL",	"XORP_MODULE_NULL" },	// 0
    { "fea",			"fea"		},	// 1
    { "MFEA_4",			"MFEA_6"	},	// 2
    { "IGMP",			"MLD"		},	// 3
    { "PIMSM_4",		"PIMSM_6"	},	// 4
    { "PIMDM_4",		"PIMDM_6"	},	// 5
    { "BGMP_4",			"BGMP_6"	},	// 6
    { "BGP_4",			"BGP_6"		},	// 7
    { "OSPF_4",			"OSPF_6"	},	// 8
    { "RIP_4",			"RIP_6"		},	// 9
    { "CLI",			"CLI"		},	// 10
    { "rib",			"rib"		},	// 11
    { "RTRMGR",			"RTRMGR"	},	// 12
    { "static_routes",		"static_routes"	},	// 13
    { "fib2mrib",		"fib2mrib"	},	// 14
    { "XORP_MODULE_UNKNOWN",	"XORP_MODULE_UNKNOWN" }
};

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
 * xorp_module_name:
 * @family: The address family.
 * @module_id: The #xorp_module_id module ID to search for.
 * 
 * Return the pre-defined module name for a given address family and module ID.
 * 
 * Return value: C string with the module name.
 **/
const char *
xorp_module_name(int family, xorp_module_id module_id)
{
    if (! is_valid_module_id(module_id)) {
	XLOG_ERROR("Invalid module_id = %d", module_id);
	return (NULL);
    }
    
    if (family == AF_INET)
	return (_xorp_module_name[module_id][0]);
    
#ifdef HAVE_IPV6
    if (family == AF_INET6)
	return (_xorp_module_name[module_id][1]);
#endif // HAVE_IPV6
    
    XLOG_ERROR("Invalid address family = %d", family);
    return (NULL);
}

/**
 * xorp_module_name2id:
 * @module_name: The module name to searh for.
 * 
 * Return the pre-defined module ID for a given module name.
 * 
 * Return value: The module ID if @module_name is a valid module name,
 * otherwise %XORP_MODULE_NULL.
 **/
xorp_module_id
xorp_module_name2id(const char *module_name)
{
    for (int i = XORP_MODULE_MIN; i < XORP_MODULE_MAX; i++) {
	if ((strcmp(module_name, _xorp_module_name[i][0]) == 0)
	    || (strcmp(module_name, _xorp_module_name[i][1]) == 0))
	    return (static_cast<xorp_module_id>(i));
    }
    
    return (XORP_MODULE_NULL);
}

/**
 * is_valid_module_id:
 * @module_id: The module ID to test.
 * 
 * Test if a module ID is valid.
 * 
 * Return value: true if @module_id is valid, otherwise false.
 **/
bool
is_valid_module_id(xorp_module_id module_id)
{
    if ((XORP_MODULE_MIN <= module_id) && (module_id < XORP_MODULE_MAX))
	return (true);
    
    return (false);
}

/**
 * ProtoUnit::ProtoUnit:
 * @init_family: The address family (%AF_INET or %AF_INET6
 * for IPv4 and IPv6 respectively).
 * @init_module_id: The module ID (XORP_MODULE_*).
 * 
 * Proto unit constructor.
 **/
ProtoUnit::ProtoUnit(int init_family, xorp_module_id init_module_id)
    : _family(init_family),
      _module_id(init_module_id)
{
    if (! is_valid_module_id(init_module_id)) {
	XLOG_FATAL("Invalid module_id = %d", init_module_id);
    }
    
    _comm_handler	= -1;
    _proto_version	= 0;
    _proto_version_default = 0;
    _module_name	= xorp_module_name(init_family, init_module_id);
}

ProtoUnit::~ProtoUnit()
{
    
}
