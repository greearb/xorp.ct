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

#ident "$XORP: xorp/libproto/proto_register.cc,v 1.4 2003/05/08 21:56:22 pavlin Exp $"


//
// Protocol registration geneeric functionality implementation
//


#include "libproto_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "proto_register.hh"


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
 * ProtoRegister::add_protocol:
 * @module_instance_name: The module instance name of the protocol to
 * add/register.
 * @module_id: The #xorp_module_id of the protocol to add/register.
 * 
 * Add/register a protocol instance.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoRegister::add_protocol(const string& module_instance_name,
			    xorp_module_id module_id)
{
    if (! is_valid_module_id(module_id)) {
	XLOG_ERROR("Cannot add protocol instance %s: "
		   "invalid module_id = %d",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);
    }
    
    RegisteredProtocol& registered_protocol = _registered_protocols[module_id];
    
    if (registered_protocol.add_protocol_instance(module_instance_name) < 0)
	return (XORP_ERROR);
    
    _all_module_instance_name_list.push_back(pair<string, xorp_module_id>(module_instance_name, module_id));
    
    return (XORP_OK);
}

/**
 * ProtoRegister::delete_protocol:
 * @module_instance_name: The module instance name of the protocol to
 * delete/deregister.
 * @module_id: The #xorp_module_id of the protocol to delete/deregister.
 * 
 * Delete/deregister a protocol instance.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoRegister::delete_protocol(const string& module_instance_name,
			       xorp_module_id module_id)
{
    if (! is_valid_module_id(module_id)) {
	XLOG_ERROR("Cannot delete protocol instance %s: "
		   "invalid module_id = %d",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);
    }
    
    RegisteredProtocol& registered_protocol = _registered_protocols[module_id];
    
    if (registered_protocol.delete_protocol_instance(module_instance_name) < 0)
	return (XORP_ERROR);
    
    list<pair<string, xorp_module_id> >::iterator iter;
    
    iter = find(_all_module_instance_name_list.begin(),
		_all_module_instance_name_list.end(),
		pair<string, xorp_module_id>(module_instance_name, module_id));
    
    if (iter == _all_module_instance_name_list.end())
	return (XORP_ERROR);		// Not on the list
    
    _all_module_instance_name_list.erase(iter);
    
    return (XORP_OK);
}

/**
 * ProtoRegister::is_registered:
 * @module_instance_name: The module instance name of the protocol to test.
 * @module_id: The #xorp_module_id of the protocol to test.
 * 
 * Test if a protocol module instance is registered.
 * 
 * Return value: True if the module is registered, otherwise false.
 **/
bool
ProtoRegister::is_registered(const string& module_instance_name,
			     xorp_module_id module_id) const
{
    if (! is_valid_module_id(module_id)) {
	XLOG_ERROR("Cannot test if registered protocol instance %s: "
		   "invalid module_id = %d",
		   module_instance_name.c_str(), module_id);
	return (XORP_ERROR);
    }
    
    const RegisteredProtocol& registered_protocol = _registered_protocols[module_id];
    
    return (registered_protocol.is_registered(module_instance_name));
}

/**
 * ProtoRegister::RegisteredProtocol::add_protocol_instance:
 * @module_instance_name: The module instance name of the protocol to add.
 * 
 * Add a protocol instance name to the list of registered protocols.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoRegister::RegisteredProtocol::add_protocol_instance(
    const string& module_instance_name)
{
    if (is_registered(module_instance_name)) {
	return (XORP_ERROR);		// Already added
    }
    
    _module_instance_name_list.push_back(module_instance_name);
    _is_registered = true;
    
    return (XORP_OK);
}

/**
 * ProtoRegister::RegisteredProtocol::delete_protocol_instance:
 * @module_instance_name: The module instance name of the protocol to delete.
 * 
 * Delete a protocol instance name to the list of registered protocols.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoRegister::RegisteredProtocol::delete_protocol_instance(
    const string& module_instance_name)
{
    list<string>::iterator iter;
    
    iter = find(_module_instance_name_list.begin(),
		_module_instance_name_list.end(),
		module_instance_name);
    
    if (iter == _module_instance_name_list.end())
	return (XORP_ERROR);		// Not on the list
    
    _module_instance_name_list.erase(iter);
    
    if (_module_instance_name_list.empty())
	_is_registered = false;		// The last one to delete
    
    return (XORP_OK);
}

/**
 * ProtoRegister::RegisteredProtocol::is_registered:
 * @module_instance_name: The module instance name of the protocol to test.
 * 
 * Test if a protocol module instance is registered.
 * 
 * Return value: True if the module is registered, otherwise false.
 **/
bool
ProtoRegister::RegisteredProtocol::is_registered(
    const string& module_instance_name) const
{
    if (find(_module_instance_name_list.begin(),
	     _module_instance_name_list.end(),
	     module_instance_name)
	== _module_instance_name_list.end()) {
	return (false);
    }
    
    return (true);
}
