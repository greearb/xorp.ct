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

// $XORP: xorp/libproto/proto_register.hh,v 1.2 2003/03/10 23:20:20 hodson Exp $


#ifndef __LIBPROTO_PROTO_REGISTER_HH__
#define __LIBPROTO_PROTO_REGISTER_HH__


#include <list>
#include <string>

#include "libxorp/xorp.h"
#include "proto_unit.hh"


//
// Protocol registration generic functionality
//


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//


/**
 * @short Base class for protocol registration.
 */
class ProtoRegister {
public:
    /**
     * Add/register a protocol instance.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to add/register.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to add/register.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_protocol(const string& module_instance_name,
			     xorp_module_id module_id);

    /**
     * Delete/deregister a protocol instance.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to delete/deregister.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to delete/deregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_protocol(const string& module_instance_name,
				xorp_module_id module_id);

    /**
     * Test if any instance of a given protocol is registered.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to test.
     * @return true if any instance of the given protocol is registered,
     * otherwise false.
     */
    bool	is_registered(xorp_module_id module_id) const {
	return (_registered_protocols[module_id].is_registered());
    }

    /**
     * Test if a protocol module instance is registered.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to test.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to test.
     * @return true if the module is registered, otherwise false.
     */
    bool	is_registered(const string& module_instance_name,
			      xorp_module_id module_id) const;

    /**
     * Get the list of all registered protocol instances for a given protocol.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol.
     * @return the list of all registered protocol instances for the given
     * protocol.
     */
    const list<string>& module_instance_name_list(xorp_module_id module_id) const {
	return (_registered_protocols[module_id].module_instance_name_list());
    }
    
private:
    
    /**
     * The class to register instances of a single protocol.
     */
    class RegisteredProtocol {
    public:
	RegisteredProtocol() : _is_registered(false) {}
	
	int	add_protocol_instance(const string& module_instance_name);
	int	delete_protocol_instance(const string& module_instance_name);
	bool	is_registered() const { return (_is_registered); }
	bool	is_registered(const string& module_instance_name) const;
	const list<string>& module_instance_name_list() const {
	    return (_module_instance_name_list);
	}
	
    private:
	bool		_is_registered;       // True if protocol is registered
	list<string>	_module_instance_name_list; // The list of instances
    };
    
    RegisteredProtocol	_registered_protocols[XORP_MODULE_MAX];
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __LIBPROTO_PROTO_REGISTER_HH__
