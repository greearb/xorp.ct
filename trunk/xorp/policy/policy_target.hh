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

#ifndef __POLICY_POLICY_TARGET_HH__
#define __POLICY_POLICY_TARGET_HH__

#include "libxipc/xrl_std_router.hh"
#include "process_watch.hh"
#include "configuration.hh"
#include "filter_manager.hh"

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
     * @param term name of term to create.
     */
    void create_term(const string& policy, const string& term);

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
     * Updates the source block of a term.
     *
     * Exception is thrown on error.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param source un-parsed source configuration block of term.
     */
    void update_term_source(const string& policy,
			    const string& term,
			    const string& source);
    
    /**
     * Updates the source block of a term.
     *
     * Exception is thrown on error.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param dest un-parsed dest configuration block of term.
     */
    void update_term_dest(const string& policy,
			  const string& term,
			  const string& dest);
    
    /**
     * Updates the source block of a term.
     *
     * Exception is thrown on error.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param action un-parsed action configuration block of term.
     */
    void update_term_action(const string& policy,
			    const string& term,
			    const string& action);

    
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
     * @param name name of set to update.
     * @param elements the elements of a set comma separated.
     */
    void update_set(const string& name, const string& elements);

    /**
     * Attempts to delete a set.
     *
     * Exception is thrown on error.
     *
     * @param name name of set to create.
     */
    void delete_set(const string& name);

    
    /**
     * Updates the import policy list for a protocol and triggers a delayed
     * commit.
     *
     * @param protocol protocol for which to update imports.
     * @param policies comma separated policy list.
     */
    void update_import(const string& protocol, const string& policies);
    
    /**
     * Updates the export policy list for a protocol and triggers a delayed
     * commit.
     *
     * @param protocol protocol for which to update imports.
     * @param policies comma separated policy list.
     */
    void update_export(const string& protocol, const string& policies);

    /**
     * Commit all configuration changes, but trigger a delayed update to the
     * actual policy filters.
     *
     * @param msec milliseconds after which policy filters should be updated.
     */
    void commit(uint32_t msec);

    /**
     * @return string representation of configuration.
     */
    string get_conf(); 

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
     * Configure the variable map used for semantic checking.
     * This should be initialized only once at startup.
     *
     * Dynamic configuration may easily be implemented by invalidiating policies
     * in the configuration class.
     *
     * Dynamic addition of variables should be safe. It is the removal and
     * update which needs to trigger a policy to be flagged as modified.
     *
     * @param conf un-parsed configuration of variable map.
     */
    void configure_varmap(const string& conf);

private:
    bool _running;
    uint32_t _commit_delay;

    ProcessWatch    _process_watch;
    Configuration   _conf;
    FilterManager   _filter_manager;

};

#endif // __POLICY_POLICY_TARGET_HH__
