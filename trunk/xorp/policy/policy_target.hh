// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/policy_target.hh,v 1.15 2008/08/06 08:17:07 abittau Exp $

#ifndef __POLICY_POLICY_TARGET_HH__
#define __POLICY_POLICY_TARGET_HH__

#include "libxipc/xrl_std_router.hh"
#include "process_watch.hh"
#include "configuration.hh"
#include "filter_manager.hh"
#include "policy/common/varrw.hh"
#include <string>

/**
 * @short The XORP Policy target.
 *
 * This is the class that will be called to perform operation from the xrl
 * target.
 */
class PolicyTarget {
public:
    static string policy_target_name;

    /**
     * @param rtr Xrl router used by this XORP process.
     */
    PolicyTarget(XrlStdRouter& rtr);

    /**
     * @return true if process is running.
     */
    bool running();
    
    /**
     * Shutdown the process.
     */
    void shutdown();

    /**
     * Attempts to create a term.
     * Terms are appended in existing policies [currently no way of inserting a
     * term in a specific position].
     *
     * Exception is thrown on error.
     *
     * @param policy policy in which term should be created.
     * @param order node ID with position of term.
     * @param term name of term to create.
     */
    void create_term(const string& policy, const ConfigNodeId& order,
		     const string& term);

    /**
     * Attempts to delete a term.
     *
     * Exception is thrown on error.
     *
     * @param policy policy in which term should be deleted.
     * @param term name of the term.
     */
    void delete_term(const string& policy, const string& term);

    /**
     * Update the source/dest/action block of a term in a policy.
     *
     * Exception is thrown on error
     *
     * @param policy the name of the policy.
     * @param term the name of the term.
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order node ID with position of term.
     * @param statement the statement to insert.
     */
    void update_term_block(const string& policy,
			   const string& term,
			   const uint32_t& block,
			   const ConfigNodeId& order,
			   const string& statement);
    
    /**
     * Attempts to create a policy.
     *
     * Exception is thrown on error.
     *
     * @param policy name of policy to create.
     */
    void create_policy(const string& policy);


    /**
     * Attempts to delete a policy.
     *
     * Exception is thrown on error.
     *
     * @param policy name of policy to delete.
     */
    void delete_policy(const string& policy);

    /**
     * Attempts to create a policy.
     *
     * Exception is thrown on error.
     *
     * @param name name of set to create.
     */
    void create_set(const string& name);
    
    /**
     * Attempts to update set elements.
     *
     * Exception is thrown on error.
     *
     * @param type the type of the set.
     * @param name name of set to update.
     * @param elements the elements of a set comma separated.
     */
    void update_set(const string& type, const string& name, 
		    const string& elements);

    /**
     * Attempts to delete a set.
     *
     * Exception is thrown on error.
     *
     * @param name name of set to create.
     */
    void delete_set(const string& name);

    /**
     * Add an element to a set.
     *
     * Exception is thrown on error.
     *
     * @param type the type of the set.
     * @param name name of the set.
     * @param element the element to add.
     */
    void add_to_set(const string& type, const string& name, 
		    const string& element);

    /**
     * Delete an element from a set.
     *
     * Exception is thrown on error.
     *
     * @param type the type of the set.
     * @param name name of the set.
     * @param element the element to delete.
     */
    void delete_from_set(const string& type, const string& name, 
			 const string& element);

    /**
     * Updates the import policy list for a protocol and triggers a delayed
     * commit.
     *
     * @param protocol protocol for which to update imports.
     * @param policies comma separated policy list.
     */
    void update_import(const string& protocol, const string& policies,
		       const string& modifier);

    /**
     * Updates the export policy list for a protocol and triggers a delayed
     * commit.
     *
     * @param protocol protocol for which to update imports.
     * @param policies comma separated policy list.
     */
    void update_export(const string& protocol, const string& policies,
		       const string& modifier);

    /* 
     * Configure the variable map used for semantic checking.
     * This should be initialized only once at startup.
     *
     * Dynamic configuration may easily be implemented by invalidiating policies
     * in the configuration class.
     *
     * Dynamic addition of variables should be safe. It is the removal and
     * update which needs to trigger a policy to be flagged as modified.
     *
     * @param protocol the protocol for which the variable is available.
     * @param variable the name of the variable.
     * @param type the type of the variable.
     * @param access the permissions on the variable (r/rw).
     * @param id the varrw interface id.
     */
    void add_varmap(const string& protocol, const string& variable,
		    const string& type, const string& access,
		    const VarRW::Id& id);

    /**
     * Commit all configuration changes, but trigger a delayed update to the
     * actual policy filters.
     *
     * @param msec milliseconds after which policy filters should be updated.
     */
    void commit(uint32_t msec);

    /**
     * Dump internal state.  Use only for debugging.
     *
     * @param id which part of the state to dump.
     * @return string representation of internal state.
     */
    string dump_state(uint32_t id); 

    /**
     * Announce birth of a XORP process.
     *
     * @param tclass target class.
     * @param tinstance target instance of class.
     */
    void birth(const string& tclass, const string& tinstance);

    /**
     * Announce death of a XORP process.
     *
     * @param tclass target class.
     * @param tinstance target instance of class.
     */
    void death(const string& tclass, const string& tinstance);

    /**
     * Update the protocol -> XRL target map.
     *
     * @param protocol the protocol.
     * @param target the XRL target.
     */
    void set_proto_target(const string& protocol, const string& target);

private:
    bool	    _running;
    uint32_t	    _commit_delay;
    ProtocolMap	    _pmap;
    ProcessWatch    _process_watch;
    Configuration   _conf;
    FilterManager   _filter_manager;
};

#endif // __POLICY_POLICY_TARGET_HH__
