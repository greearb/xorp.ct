// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __POLICY_VAR_MAP_HH__
#define __POLICY_VAR_MAP_HH__

#include "policy/common/policy_exception.hh"

#include "process_watch.hh"

#include <string>
#include <map>

/**
 * @short A VarMap contains all information for legal protocol variables.
 *
 * It contains all the possible protocols.
 * All the possible variables those protocols support and whether the variables
 * are read-only or read-write.
 *
 * This is crutial for semantic checking.
 */
class VarMap {
public:
    /**
     * @short Exception thrown on VarMap errors such as on unknown variables.
     */
    class VarMapErr : public PolicyException {
    public:
	VarMapErr(const string& err) : PolicyException(err) {}
    };

    /**
     * A variable may be READ [readonly] or READ_WRITE [read/write].
     */
    enum Access {
	READ,
	READ_WRITE
    };

    /**
     * A variable has Access control, it has a name, and a type.
     */
    struct Variable {
	Access access;
	string name;
	string type;

	Variable(const string& n, const string& t, Access a) : 
		    access(a), name(n), type(t) 
	{
	}
    };

    typedef map<string,Variable*> VariableMap;

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
			     const string& varname) const;

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
     * Add a variable to a specific protocol.
     *
     * @param vm VariableMap where variable should be added.
     * @param varname name of the variable.
     * @param type the type [id] of the variable/element.
     * @param acc the access of the variable.
     */
    void add_variable(VariableMap& vm, const string& varname, 
		      const string& type, Access acc);

    /**
     * Add a variable to a protocol.
     *
     * @param protocol protocol for which variable should be added.
     * @param varname name of variable.
     * @param type type [id] of the variable.
     * @param acc access [r/rw] of the variable.
     */
    void add_protocol_variable(const string& protocol, const string& varname, 
			       const string& type, Access acc);

    /**
     * Configure the VarMap from an unparsed configuration.
     *
     * @param conf un-parsed configuration file of the VarMap.
     */
    void configure(const string& conf);

private:
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

    // not impl
    VarMap(const VarMap&);
    VarMap& operator=(const VarMap&);
};

#endif // __POLICY_VAR_MAP_HH__
