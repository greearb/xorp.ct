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

#ifndef __POLICY_CONFIGURATION_HH__
#define __POLICY_CONFIGURATION_HH__

#include "policy/common/policy_exception.hh"

#include "process_watch_base.hh"
#include "set_map.hh"
#include "policy_map.hh"
#include "policy_list.hh"

#include "filter_manager_base.hh"
#include "var_map.hh"


/**
 * @short Class that contains all configuration and generated code state.
 *
 * This class contains all user policy configuration. It updates the relevant
 * configuration portions based on user changes. Also, it does some sanity
 * checking by (dis)allowing the user to do certain actions [such as delete sets
 * which are referenced in policies].
 */
class Configuration {
public:
    typedef map<string,Code*> CodeMap;
    typedef set<uint32_t> TagSet;
    typedef map<string,TagSet*> TagMap;

   
    /**
     * @short Exception thrown on configuration error
     */
    class ConfError : public PolicyException {
    public:
	ConfError(const string& e) : PolicyException(e) {}

    };


    /**
     * @param a process watcher used to initialize the VarMap.
     */
    Configuration(ProcessWatchBase& pw);
    ~Configuration();
   
    /**
     * Throws an exception on failure.
     * Checks for non-existant policy/term conditions.
     *
     * @param policy policy in which term should be deleted.
     * @param term term to delete.
     */
    void delete_term(const string& policy, const string& term);

    /**
     * Throws an exception on failure.
     * Checks for non-existant policy/term conditions. Also tries to parse the
     * configuration. No compilation / semantic check is performed now.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param source un-parsed user configuration of the source {} block.
     */
    void update_term_source(const string& policy, const string& term,
			    const string& source);

    /**
     * Throws an exception on failure.
     * Checks for non-existant policy/term conditions. Also tries to parse the
     * configuration. No compilation / semantic check is performed now.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param dest un-parsed user configuration of the dest {} block.
     */
    void update_term_dest(const string& policy, const string& term,
			  const string& dest);
  
    /**
     * Throws an exception on failure.
     * Checks for non-existant policy/term conditions. Also tries to parse the
     * configuration. No compilation / semantic check is performed now.
     *
     * @param policy policy in which term should be updated.
     * @param term term which should be updated.
     * @param action un-parsed user configuration of the action {} block.
     */
    void update_term_action(const string& policy, const string& term,
			    const string& action);
   
    /**
     * Append a term to a policy.
     *
     * Throws an exception on failure.
     * Checks if term already exists.
     *
     * @param policy policy in which term should be created.
     * @param term term name which should be created.
     */
    void create_term(const string& policy, const string& term);
  
    /**
     * Throws an exception on failure.
     * Checks if policy already exists.
     *
     * @param policy policy which should be created.
     */
    void create_policy(const string& policy);
    
    /**
     * Throws an exception on failure.
     * Checks if policy is in use [instantiated by an export/import directive.]
     *
     * @param policy policy which should be deleted.
     */
    void delete_policy(const string& policy);

    /**
     * Throws an exception on failure.
     * Checks if set already exists.
     *
     * @param set name of the set to be created.
     */
    void create_set(const string& set);

    /**
     * Throws an exception on failure.
     * Checks if set exists.
     *
     * @param set name of the set to be updated.
     * @param elements comma separated elements to be replaced in set.
     */
    void update_set(const string& set, const string& elements);

    /**
     * Throws an exception on failure.
     * Checks if set is in use.
     *
     * @param set name of set to delete.
     */
    void delete_set(const string& set);
  
    /**
     * Throws an exception on failure.
     * Checks if policies exist.
     *
     * @param protocol name of protocol which should have imports updated.
     * @param imports list of policy-names.
     */
    void update_imports(const string& protocol, const list<string>& imports);

    /**
     * Throws an exception on failure.
     * Checks if policies exist.
     *
     * @param protocol name of protocol which should have exports updated.
     * @param exports list of policy-names.
     */
    void update_exports(const string& protocol, const list<string>& exports);

    /**
     * @return string representation of configuration
     */
    string str();
   
    /**
     * Commit all configuration changes.
     * This will compile all needed policies and link them.
     * It will then commit changes to the actual policy filters.
     * Commits are optionally delayed in order to aggregate configuration
     * changes. For example, at boot-up many small changes are done in small
     * time intervals. It would be more efficient to configure the filters only
     * after all changes have been made. Thus delaying a commit will help.
     *
     * The delay will only be imposed on sending the configuration to the
     * filters -- all semantic checks and compile is done immediately.
     *
     * @param msec milliseconds after which code should be sent to filters.
     */
    void commit(uint32_t msec);

    /**
     * Initialize the VarMap needed for semantic checking.
     *
     * @param conf un-parsed user configuration of varmap.
     */
    void configure_varmap(const string& conf);

    /**
     * This method should be called once at initialization to set the
     * FilterManager.
     * It should not be deleted by the Configuration class -- it does not own
     * it.
     */
    void set_filter_manager(FilterManagerBase&);

    /**
     * A CodeMap is a map relating protocols to code. All the code for a
     * protocol will be found in its entry. The code however will normally be
     * for a specific filter.
     *
     * @return the CodeMap for import filters.
     */
    CodeMap& import_filters() { return _import_filters; }

    /**
     * @return the CodeMap for source match filters.
     */
    CodeMap& sourcematch_filters() { return _sourcematch_filters; }

    /**
     * @return the CodeMap for export filters.
     */
    CodeMap& export_filters() { return _export_filters; }

    /**
     * @return the SetMap relating set-name to the actual set.
     */
    SetMap&  sets() { return _sets; }

    /**
     * @return the policy tag map relating policytags to destination protocols.
     */
    TagMap&  tagmap() { return _tagmap; }

private:
    typedef map<string,PolicyList*> IEMap;


    /**
     * Throws an exception if no term is found.
     *
     * @return term being searched for.
     * @param policy policy name term should be found in.
     * @param term term being searched for.
     */
    Term& find_term(const string& policy, const string& term);

    /**
     * Scans policy and checks which sets it uses. It also binds the policy to
     * those sets, so sets may not be deleted.
     *
     * @param policy policy which should have set dependancies updated.
     */
    void update_set_dependancy(PolicyStatement& policy);

    /**
     * Generate code for a policy.
     * Throws an exception on failure.
     *
     * @param name name of policy to be compiled.
     */
    void compile_policy(const string& name);

    /**
     * Compile all modified and non previously compiled policies.
     * Throws an exception on failure.
     */
    void compile_policies();

    /**
     * Links all source match filter code for a specific target.
     * Code is internally kept fragmented [so deleting one policy will not
     * involve recompiling the whole policy list for a target, for example].
     *
     * @param target target for which code should be linked.
     */
    void link_sourcematch_code(const Code::Target& target);
    
    /**
     * Update the policytags used by a protocol.
     *
     * @param protocol protocol for which to update policytags.
     */
    void update_tagmap(const string& protocol);
  
    /**
     * Link code for updated targets.
     */
    void link_code();

    void update_ie(const string& protocol, const list<string>& policies, 
		   IEMap& iemap, PolicyList::PolicyType pt);

    void link_code(const Code::Target& target, IEMap& iemap, CodeMap& codemap);
    
    string codemap_str(CodeMap& cm);



    PolicyMap _policies;

    IEMap _imports;
    IEMap _exports;

    SetMap _sets;

    typedef set<string> PolicySet;
    PolicySet _modified_policies;

    Code::TargetSet _modified_targets;

    ElementFactory _ef;


    CodeMap _import_filters;
    CodeMap _sourcematch_filters;
    CodeMap _export_filters;

    uint32_t _currtag;


    TagMap _tagmap;

    VarMap _varmap;

    // do not delete
    FilterManagerBase* _filter_manager;


    // not impl
    Configuration(const Configuration&);
    Configuration& operator=(const Configuration&);
};

#endif // __POLICY_CONFIGURATION_HH__
