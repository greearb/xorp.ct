// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/var_map.hh,v 1.12 2008/10/02 21:58:01 bms Exp $

#ifndef __POLICY_VAR_MAP_HH__
#define __POLICY_VAR_MAP_HH__

#include <string>
#include <map>

#include <boost/noncopyable.hpp>

#include "policy/common/policy_exception.hh"
#include "policy/common/varrw.hh"

#include "process_watch.hh"

/**
 * @short A VarMap contains all information for legal protocol variables.
 *
 * It contains all the possible protocols.
 * All the possible variables those protocols support and whether the variables
 * are read-only or read-write.
 *
 * This is crutial for semantic checking.
 */
class VarMap :
    public boost::noncopyable
{
public:
    /**
     * @short Exception thrown on VarMap errors such as on unknown variables.
     */
    class VarMapErr : public PolicyException {
    public:
        VarMapErr(const char* file, size_t line, const string& init_why = "")   
            : PolicyException("VarMapErr", file, line, init_why) {} 
    };

    /**
     * A variable may be READ [readonly] or READ_WRITE [read/write].
     */
    enum Access {
	READ,
	READ_WRITE,
	WRITE
    };

    /**
     * A variable has Access control, it has a name, and a type.
     */
    struct Variable {
	Access access;
	string name;
	string type;
	VarRW::Id id;

	Variable(const string& n, const string& t, Access a,
		 VarRW::Id i) :
		    access(a), name(n), type(t), id(i)
	{
	}

	Variable(const Variable& v)
	{
	    access = v.access;
	    name = v.name;
	    type = v.type;
	    id = v.id;
	}

	bool writable() const 
	{ 
	    return access == READ_WRITE || access == WRITE; 
	}
	
	bool operator==(const Variable& other) const {
	    return ((access == other.access)
		    && (name == other.name)
		    && (type == other.type)
		    && (id == other.id));
	}
    };

    typedef map<VarRW::Id,Variable*> VariableMap;
    typedef map<string,VariableMap*> ProtoMap;

    /**
     * Return Variable information for a variable of a specific protocol.
     *
     * Throws an exception if no information is found.
     *
     * Caller must not delete the map.
     *
     * @return access and type information for the requested variable.
     * @param protocol protocol of variable interested in.
     * @param varname name of variable interested in.
     */
    const Variable& variable(const string& protocol, 
			     const VarRW::Id& varname) const;

    VarRW::Id var2id(const string& protocol, const string& varname) const;

    /**
     * As the VarMap learns about new protocols, it will register interest with
     * the process watcher for that protocol to monitor the status of the
     * protocol process.
     *
     * @param pw processWatch to use.
     */
    VarMap(ProcessWatchBase& pw);
    ~VarMap();

    /**
     * @return True if the protocol is known to the VarMap, false otherwise.
     * @param protocol protocol caller wish to knows existance of.
     */
    bool protocol_known(const string& protocol);

    /**
     * Add a variable to a protocol.
     *
     * @param protocol protocol for which variable should be added.
     * @param var the variable to add.  Do not delete.
     */
    void add_protocol_variable(const string& protocol, Variable* var);

    
    /**
     * String representation of varmap.  Use only for debugging.
     *
     * @return string representation of varmap.
     */
    string str(); 

private:
    /**
     * Use this if you want a variable to be present for all protocols.
     *
     * @param var the variable to add.  Watch out for clashes and don't delete.
     */
    void add_metavariable(Variable *var);

    /**
     * Add a variable to a specific protocol.
     *
     * @param vm VariableMap where variable should be added.
     * @param var the variable to add.  Do not delete.
     */
    void add_variable(VariableMap& vm, Variable* var);

    /**
     * A VariableMap relates a variable name to its Variable information [access
     * and type].
     *
     * Throws an exception if no map is found.
     *
     * @return variable map for requested protocol.
     * @param protocol protocol name for which variable map is requested.
     */
    const VariableMap& variablemap(const string& protocol) const;

    ProtoMap _protocols;
    ProcessWatchBase& _process_watch;

    typedef VariableMap MetaVarContainer;
    MetaVarContainer _metavars;
};

#endif // __POLICY_VAR_MAP_HH__
