// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/configuration.hh,v 1.20 2008/10/02 21:57:58 bms Exp $

#ifndef __POLICY_CONFIGURATION_HH__
#define __POLICY_CONFIGURATION_HH__



#include "policy/common/policy_exception.hh"

#include "process_watch_base.hh"
#include "set_map.hh"
#include "policy_map.hh"
#include "policy_list.hh"
#include "filter_manager_base.hh"
#include "var_map.hh"

typedef list<string>	    POLICIES;
typedef Code::TargetSet	    TARGETSET;
typedef uint32_t	    tag_t;
typedef set<tag_t>	    TagSet;
typedef map<string, string> RATTR;
typedef map<string, string> RESOURCES;

// XXX we go reverse in order to make peer specific policies override global
// ones.  Global is "" so it's always smallest (first).
#define FOR_ALL_POLICIES(n) \
    for (PROTOCOL::reverse_iterator i = _protocols.rbegin(); \
	 !(i == _protocols.rend()); ++i) \
	for (POLICY::reverse_iterator n = i->second->rbegin(); \
	    !(n == i->second->rend()); ++n)

class IEMap {
public:
    typedef map<string, PolicyList*>	POLICY; // modifier -> policy list
    typedef map<string, POLICY*>	PROTOCOL; // protocol -> modifiers

    IEMap();
    ~IEMap();

    PolicyList* find(const string& proto, const string& mod);
    void	insert(const string& proto, const string& mod, PolicyList* pl);
    void	clear();
    void	clear(TARGETSET& ts);
    void	get_targets(const string& proto, const string& mod,
			    TARGETSET& targets);
    void	compile(PolicyStatement& ps, TARGETSET& targets, tag_t& tag, map<string, set<uint32_t> >& ptags);
    void	compile(TARGETSET& targets, tag_t& tag, map<string, set<uint32_t> >& ptags);
    void	link_code(Code& code);
    void	link_code(const string& proto, Code& code);
    void	get_redist_tags(const string& proto, TagSet& ts);

private:
    POLICY*	find_policy(const string& proto);
    void	clear(POLICY* p);

    PROTOCOL	_protocols;
};

/**
 * @short Class that contains all configuration and generated code state.
 *
 * This class contains all user policy configuration. It updates the relevant
 * configuration portions based on user changes. Also, it does some sanity
 * checking by (dis)allowing the user to do certain actions [such as delete sets
 * which are referenced in policies].
 */
class Configuration :
    public NONCOPYABLE
{
public:
    typedef map<string,Code*> CodeMap;
    typedef map<string,TagSet*> TagMap;


    /**
     * @short Exception thrown on configuration error
     */
    class ConfError : public PolicyException {
    public:
        ConfError(const char* file, size_t line, const string& init_why = "")
            : PolicyException("ConfError", file, line, init_why) {}

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
     * Update the source/dest/action block of a term.
     *
     * Throws an exception on failure.
     * Checks for non-existent policy/term conditions. Also tries to parse the
     * configuration. No compilation / semantic check is performed now.
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
     * Append a term to a policy.
     *
     * Throws an exception on failure.
     * Checks if term already exists.
     *
     * @param policy policy in which term should be created.
     * @param order node ID with position of term.
     * @param term term name which should be created.
     */
    void create_term(const string& policy, const ConfigNodeId& order,
		     const string& term);

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
     * @param type the type of the set.
     * @param set name of the set to be updated.
     * @param elements comma separated elements to be replaced in set.
     */
    void update_set(const string& type, const string& set,
		    const string& elements);

    /**
     * Throws an exception on failure.
     * Checks if set is in use.
     *
     * @param set name of set to delete.
     */
    void delete_set(const string& set);

    /**
     * Add an element to a set.
     *
     * Throws an exception on failure.
     * Checks if set exists.
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
     * Throws an exception on failure.
     * Checks if set exists.
     *
     * @param type the type of the set.
     * @param name name of the set.
     * @param element the element to delete.
     */
    void delete_from_set(const string& type, const string& name,
			 const string& element);

    /**
     * Throws an exception on failure.
     * Checks if policies exist.
     *
     * @param protocol name of protocol which should have imports updated.
     * @param imports list of policy-names.
     */
    void update_imports(const string& protocol, const POLICIES& imports,
		        const string& mod);

    /**
     * Throws an exception on failure.
     * Checks if policies exist.
     *
     * @param protocol name of protocol which should have exports updated.
     * @param exports list of policy-names.
     */
    void update_exports(const string& protocol, const POLICIES& exports,
			const string& mod);

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
     * Add a variable to the VarMap, needed for semantic checking.
     *
     * @param protocol the protocol this variable is available to.
     * @param variable name of the variable.
     * @param type the type of the variable.
     * @param access the permissions on the variable (r/rw).
     * @param id the id used for VarRW interaction.
     */
    void add_varmap(const string& protocol, const string& name,
		    const string& type, const string& access,
		    const VarRW::Id& id);

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

    /**
     * Dump internal state.  Debugging only.
     *
     * @param id specifies which aspect of state to dump.
     * @return human readable state information.
     */
    string dump_state(uint32_t id);

    /**
     * Clear tags specified with ts from _protocol_tags
     *
     * @param ts tags to erase from _protocol_tags
     */
    void clear_protocol_tags(const TagSet& ts);

    void clear_imports(const string& protocol);
    void clear_exports(const string& protocol);
    bool test_policy(const string& policy, const RATTR& attrs, RATTR& mods);
    void show(const string& type, const string& name, RESOURCES& res);
    void show_sets(const string& type, const string& name, RESOURCES& res);
    void show_policies(const string& name, RESOURCES& res);

private:
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
     * @param policy policy which should have set dependencies updated.
     */
    void update_dependencies(PolicyStatement& policy);

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

    void update_ie(const string& protocol, const POLICIES& policies,
		   IEMap& iemap, PolicyList::PolicyType pt, const string& mod);

    void link_code(const Code::Target& target, IEMap& iemap, CodeMap& codemap);

    string codemap_str(CodeMap& cm);
    void   policy_modified(const string& policy);

    typedef set<string> PolicySet;

    PolicyMap		    _policies;
    IEMap		    _imports;
    IEMap		    _exports;
    SetMap		    _sets;
    PolicySet		    _modified_policies;
    TARGETSET		    _modified_targets;
    ElementFactory	    _ef;
    CodeMap		    _import_filters;
    CodeMap		    _sourcematch_filters;
    CodeMap		    _export_filters;
    tag_t		    _currtag;
    map<string, set<uint32_t> > _protocol_tags;
    TagMap		    _tagmap;
    VarMap		    _varmap;
    FilterManagerBase*	    _filter_manager; // do not delete
};

#endif // __POLICY_CONFIGURATION_HH__
