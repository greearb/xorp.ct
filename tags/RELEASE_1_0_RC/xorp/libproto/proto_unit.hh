// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libproto/proto_unit.hh,v 1.7 2004/02/17 22:51:35 pavlin Exp $


#ifndef __LIBPROTO_PROTO_UNIT_HH__
#define __LIBPROTO_PROTO_UNIT_HH__


#include "libxorp/xorp.h"

#include "proto_state.hh"


//
// Protocol unit generic functionality
//


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short The unique module IDs (one per protocol or module).
 * 
 * Note: the module IDs must be consistent with the _xorp_module_name
 * definition in file proto_unit.cc (TODO: a temporary solution).
 */
enum xorp_module_id {
    XORP_MODULE_MIN		= 0,
    XORP_MODULE_NULL		= 0,
    XORP_MODULE_FEA		= 1,
    XORP_MODULE_MFEA		= 2,
    XORP_MODULE_MLD6IGMP	= 3,
    XORP_MODULE_PIMSM		= 4,
    XORP_MODULE_PIMDM		= 5,
    XORP_MODULE_BGMP		= 6,
    XORP_MODULE_BGP		= 7,
    XORP_MODULE_OSPF		= 8,
    XORP_MODULE_RIP		= 9,
    XORP_MODULE_CLI		= 10,
    XORP_MODULE_RIB		= 11,
    XORP_MODULE_RTRMGR		= 12,
    XORP_MODULE_STATIC_ROUTES	= 13,
    XORP_MODULE_FIB2MRIB	= 14,
    XORP_MODULE_MAX
};

/**
 * @short Base class for each protocol unit (node, vif, etc).
 */
class ProtoUnit : public ProtoState {
public:
    /**
     * Constructor for a given address family and module ID.
     * 
     * @param init_family the address family.
     * @param init_module_id the module ID (@ref xorp_module_id).
     */
    ProtoUnit(int init_family, xorp_module_id init_module_id);
    
    /**
     * Destructor
     */
    virtual ~ProtoUnit();
    
    /**
     * Get the address family.
     * 
     * @return the address family (e.g., AF_INET or AF_INET6 for IPv4 and
     * IPv6 respectively).
     */
    int		family()	const	{ return (_family);		}
    
    /**
     * Get the module ID.
     * 
     * @return the module ID (@ref xorp_module_id).
     */
    xorp_module_id	module_id()	const	{ return (_module_id);	}
    
    /**
     * Get the current protocol version.
     * 
     * @return the current protocol version.
     */
    int		proto_version()	const	{ return (_proto_version);	}
    
    /**
     * Set the current protocol version.
     * 
     * @param v the protocol version.
     */
    void	set_proto_version(int v) { _proto_version = v;		}
    
    /**
     * Get the default protocol version.
     * 
     * @return the default protocol version.
     */
    int		proto_version_default() const { return (_proto_version_default); }
    
    /**
     * Set the default protocol version.
     * 
     * @param v the default protocol version.
     */
    void	set_proto_version_default(int v) { _proto_version_default = v;}
    
    /**
     * Test if the address family of the unit is IPv4.
     * 
     * @return true if the address family of the unit is IPv4.
     */
    bool	is_ipv4()	const	{ return (_family == AF_INET);  }

    /**
     * Test if the address family of the unit is IPv6.
     * 
     * @return true if the address family of the unit is IPv6.
     */
    bool	is_ipv6()	const	{ return (_family == AF_INET6); }
    
    /**
     * Get the module name.
     * 
     * TODO: temporary, all names are listed in "_xorp_module_name[][]"
     * in proto_unit.cc.
     * 
     * @return C-style string with the module name.
     */
    const char	*module_name()	const	{ return (_module_name.c_str()); }
    
    /**
     * Get the communication handler for this unit.
     * 
     * Note: currently, the purpose of the communication handler is undefined.
     * 
     * @return the communication handler for this unit.
     */
    int		comm_handler()	const	{ return (_comm_handler);	}
    
    /**
     * Set the communication handler for this unit.
     * 
     * @param v the communication handler to set for this unit.
     */
    void	set_comm_handler(int v) { _comm_handler = v;		}
    
    //
    // Protocol tests
    //
    
    /**
     * Test if the protocol is MLD6 or IGMP.
     * 
     * @return true if the protocol is MLD6 or IGMP.
     */
    bool	proto_is_mld6igmp() const { return (_module_id == XORP_MODULE_MLD6IGMP); }
    
    /**
     * Test if the protocol is IGMP.
     * 
     * @return true if the protocol is IGMP.
     */
    bool	proto_is_igmp() const { return (proto_is_mld6igmp() && is_ipv4()); }
    
    /**
     * Test if the protocol is MLD6.
     * 
     * @return true if the protocol is MLD6.
     */
    bool	proto_is_mld6() const { return (proto_is_mld6igmp() && is_ipv6()); }

    /**
     * Test if the protocol is PIM-SM.
     * 
     * @return true if the protocol is PIM-SM.
     */
    bool	proto_is_pimsm() const { return (_module_id == XORP_MODULE_PIMSM); }
    
    /**
     * Test if the protocol is PIM-DM.
     * 
     * @return true if the protocol is PIM-DM.
     */
    bool	proto_is_pimdm() const { return (_module_id == XORP_MODULE_PIMDM); }

    /**
     * Test if the protocol is BGMP.
     * 
     * @return true if the protocol is BGMP.
     */
    bool	proto_is_bgmp() const { return (_module_id == XORP_MODULE_BGMP); }

    /**
     * Test if the protocol is BGP.
     * 
     * @return true if the protocol is BGP.
     */
    bool	proto_is_bgp() const { return (_module_id == XORP_MODULE_BGP); }

    /**
     * Test if the protocol is OSPF.
     * 
     * @return true if the protocol is OSPF.
     */
    bool	proto_is_ospf() const { return (_module_id == XORP_MODULE_OSPF); }
    
    /**
     * Test if the protocol is RIP.
     * 
     * @return true if the protocol is RIP.
     */
    bool	proto_is_rip() const { return (_module_id == XORP_MODULE_RIP); }

    /**
     * Test if the protocol is StaticRoutes.
     * 
     * @return true if the protocol is StaticRoutes
     */
    bool	proto_is_static_routes() const { return (_module_id == XORP_MODULE_STATIC_ROUTES); }

    /**
     * Test if the protocol is Fib2mrib.
     * 
     * @return true if the protocol is Fib2mrib
     */
    bool	proto_is_fib2mrib() const { return (_module_id == XORP_MODULE_FIB2MRIB); }
    
private:
    int		_family;		// The address family.
    xorp_module_id _module_id;		// The module ID (XORP_MODULE_*).
    int		_comm_handler;		// The communication handler.
    int		_proto_version;		// Protocol version (proto. specific).
    int		_proto_version_default;	// Default protocol version.
    string	_module_name;		// The module name.
};

//
// Global variables
//

//
// Global functions prototypes
//
/**
 * Get the module name for a given address family and module ID.
 * 
 * TODO: temporary, all names are listed in "_xorp_module_name[][]"
 * in proto_unit.cc.
 * 
 * @param family the address family (e.g., AF_INET or AF_INET6 for
 * IPv4 and IPv6 respectively).
 * @param module_id the module ID (@ref xorp_module_id).
 * @return C-style string with the module name.
 */
const char	*xorp_module_name(int family, xorp_module_id module_id);

/**
 * Convert from module name to module ID.
 * 
 * The module name must be a valid name returned by @ref xorp_module_name().
 * 
 * @param module_name the module name.
 * @return the module ID (@ref xorp_module_id) if @ref module_name is valid,
 * otherwise @ref XORP_MODULE_NULL.
 */
xorp_module_id	xorp_module_name2id(const char *module_name);

/**
 * Test if a module ID is valid.
 * 
 * A valid module ID is defined as valid if it is in the interval
 * [@ref XORP_MODULE_MIN, @ref XORP_MODULE_MAX).
 * 
 * @param module_id the module ID to test (@ref xorp_module_id).
 * @return true if @ref module_id is valid, otherwise false.
 */
bool		is_valid_module_id(xorp_module_id module_id);

#endif // __LIBPROTO_PROTO_UNIT_HH__
