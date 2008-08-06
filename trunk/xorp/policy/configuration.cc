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

#ident "$XORP: xorp/policy/configuration.cc,v 1.29 2008/08/06 08:27:10 abittau Exp $"

#include "libxorp/xorp.h"
#include "policy_module.h"
#include "configuration.hh"
#include "visitor_dep.hh"
#include "policy/common/policy_utils.hh"
#include "visitor_test.hh"
#include "visitor_printer.hh"

using namespace policy_utils;

Configuration::Configuration(ProcessWatchBase& pw) : 
    _currtag(0), _varmap(pw), _filter_manager(NULL)
{
}

Configuration::~Configuration()
{
    _imports.clear();
    _exports.clear();

    clear_map(_import_filters);
    clear_map(_sourcematch_filters);
    clear_map(_export_filters);

    clear_map(_tagmap);

    //
    // XXX: Clear the _policies before the _sets goes out of scope.
    // Otherwise, the _policies destructor might try to use a reference
    // to _sets after _sets has been destroyed.
    //
    _policies.clear();
}
  
Term& 
Configuration::find_term(const string& policy, const string& term)
{
    const PolicyStatement& ps = _policies.find(policy);
    return ps.find_term(term);
}

void 
Configuration::delete_term(const string& policy, const string& term)
{
    PolicyStatement& ps = _policies.find(policy);

    if (ps.delete_term(term)) {
	// policy needs to be re-compiled [will do so on commit]
	policy_modified(policy);

        return;
    }   

    xorp_throw(ConfError, "TERM NOT FOUND " + policy + " " + term);
}
   
void 
Configuration::update_term_block(const string& policy,
                                 const string& term,
	                         const uint32_t& block,
				 const ConfigNodeId& order,
		                 const string& statement)
{
    Term& t = find_term(policy,term);
    try {
	t.set_block(block, order, statement);
	policy_modified(policy);
    } catch (const Term::term_syntax_error& e) {
        string err = "In policy " + policy + ": " + e.why();
        xorp_throw(ConfError, err);
    }
} 

void 
Configuration::create_term(const string& policy, const ConfigNodeId& order,
			   const string& term)
{
    PolicyStatement& ps = _policies.find(policy);

    if (ps.term_exists(term)) {
	xorp_throw(ConfError,
		   "Term " + term + " exists already in policy " + policy);
    }

    Term* t = new Term(term);

    ps.add_term(order, t);
    policy_modified(policy);
}

void
Configuration::create_policy(const string&   policy)
{
    _policies.create(policy,_sets);
    _modified_policies.insert(policy);
}

void 
Configuration::delete_policy(const string&   policy)
{
    _policies.delete_policy(policy);
    // if we manage to delete a policy, it means it is not in use... so we do
    // not need to send updates to filters.
    _modified_policies.erase(policy);
}

void 
Configuration::create_set(const string& set)
{
    _sets.create(set);
}  

void 
Configuration::update_set(const string& type, const string& set, 
			  const string& elements)
{
    // policies affected will be marked as modified.
    _sets.update_set(type, set, elements, _modified_policies);
}  

void 
Configuration::delete_set(const string& set)
{
    // if we manage to delete a set, it is not in use, so no updates are
    // necessary to filters / configuration.
    _sets.delete_set(set);
}  

void
Configuration::add_to_set(const string& type, const string& set,
			  const string& element)
{
    // policies affected will be marked as modified.
    _sets.add_to_set(type, set, element, _modified_policies);
}

void
Configuration::delete_from_set(const string& type, const string& set,
			       const string& element)
{
    // policies affected will be marked as modified.
    _sets.delete_from_set(type, set, element, _modified_policies);
}

void 
Configuration::update_imports(const string& protocol, const POLICIES& imports,
			      const string& mod)
{
    // check if protocol exists
    if (!_varmap.protocol_known(protocol))
	xorp_throw(ConfError, "imports: Protocol " + protocol + " unknown");

    update_ie(protocol, imports, _imports, PolicyList::IMPORT, mod);
    _modified_targets.insert(Code::Target(protocol, filter::IMPORT));
}

void 
Configuration::update_exports(const string& protocol, 
			      const POLICIES& exports,
			      const string& mod)
{
    // check if protocol exists
    if(!_varmap.protocol_known(protocol))
	xorp_throw(ConfError, "exports: Protocol " + protocol + " unknown");

    // XXX: if conf fails we lost tagmap
    TagMap::iterator i = _tagmap.find(protocol);
    if(i != _tagmap.end()) {
        TagSet* ts = (*i).second;

        delete ts;

	_tagmap.erase(i);
    }

    update_ie(protocol, exports, _exports, PolicyList::EXPORT, mod);

    // other modified targets [such as sourcematch] will be added as compilation
    // proceeds.
    _modified_targets.insert(Code::Target(protocol,filter::EXPORT));
}

void
Configuration::clear_imports(const string& protocol)
{
    // check if protocol exists
    if (!_varmap.protocol_known(protocol))
	xorp_throw(ConfError, "imports: Protocol " + protocol + " unknown");

    _imports.clear(_modified_targets);
    _modified_targets.insert(Code::Target(protocol, filter::IMPORT));
}

void
Configuration::clear_exports(const string& protocol)
{
    // check if protocol exists
    if (!_varmap.protocol_known(protocol))
	xorp_throw(ConfError, "imports: Protocol " + protocol + " unknown");

    _exports.clear(_modified_targets);
    _modified_targets.insert(Code::Target(protocol, filter::EXPORT));
}

string 
Configuration::str() 
{
    ostringstream conf;
/*
for(PolicyMap::iterator i = _policies.begin();
    i != _policies.end(); ++i) {

    conf += ((*i).second)->str();
}    

for(SetMap::iterator i = _sets.begin();
    i != _sets.end(); ++i) {

    conf += "set " + (*i).first + " {\n";
    conf += ((*i).second)->str();
    conf += "\n}\n";
}

for(IEMap::iterator i = _imports.begin();
    i != _imports.end(); ++i) {

    conf += "import " + (*i).first;

    conf += "\n";
}
return conf;
*/
    conf << "IMPORTS:\n";
    conf << codemap_str(_import_filters);

    conf << "SOURCE MATCH:\n";
    conf << codemap_str(_sourcematch_filters);

    conf << "EXPORTS:\n";
    conf << codemap_str(_export_filters);

    conf << "TAGS:\n";
    for(TagMap::iterator i = _tagmap.begin(); i != _tagmap.end(); ++i) {
        const string& protocol = (*i).first;
        const TagSet& tagset = *((*i).second);

        conf << protocol << ":";

        for(TagSet::const_iterator j = tagset.begin(); j != tagset.end(); ++j) {
	   conf << " " << *j;
	}

        conf << "\n";
    }

    conf << "CURRTAG: " << _currtag << endl;

    return conf.str();
}

void 
Configuration::update_dependencies(PolicyStatement& policy)
{
    // check if used sets & policies exist, and mark dependencies.
    VisitorDep dep(_sets, _policies);

    policy.accept(dep);
}

void 
Configuration::compile_policy(const string& name)
{
    PolicyStatement& policy = _policies.find(name);

    // Mark the end of the policy
    policy.set_policy_end();

    // probably is a fresh / modified policy, so update dependencies with sets.
    update_dependencies(policy);

    // save old tag to check for integer overflow
    tag_t old_currtag = _currtag; 

    // go through all the import statements
    _imports.compile(policy, _modified_targets, _currtag);

    // go through all export statements
    _exports.compile(policy, _modified_targets, _currtag);

    // integer overflow
    if (_currtag < old_currtag)
	// FIXME
	XLOG_FATAL("The un-avoidable occurred: We ran out of policy tags");
}

void 
Configuration::compile_policies()
{
    // integer overflow check
    tag_t old_currtag = _currtag; 

    // compile all modified policies
    for (PolicySet::iterator i = _modified_policies.begin();
	i != _modified_policies.end(); ++i) {

        compile_policy(*i);
    }
    _modified_policies.clear();

    // compile any import policies that have not yet been compiled.
    // This is a case if a policy is not modified, but just added to a policy
    // list.
    _imports.compile(_modified_targets, _currtag);

    // same for exports.
    _exports.compile(_modified_targets, _currtag);

    // integer overflow.
    if (_currtag < old_currtag) {
	// FIXME
	XLOG_FATAL("The un-avoidable occurred: We ran out of policy tags");
	abort();
    }
}

void 
Configuration::link_sourcematch_code(const Code::Target& target)
{
    // create empty code but only with target set.
    // This will allow the += operator of Code to behave properly. [i.e. link
    // only stuff we really are interested in].
    Code* code = new Code();
    code->set_target(target);

    // only export statements have source match code.
    // go through all of them and link.
    _exports.link_code(*code);

    // kill previous
    CodeMap::iterator i = _sourcematch_filters.find(target.protocol());
    if(i != _sourcematch_filters.end()) {
	delete (*i).second;
	_sourcematch_filters.erase(i);
    }


    // if there is nothing, keep it deleted and empty.
    if(code->code() == "") 
        delete code;
    else {
        _sourcematch_filters[target.protocol()] = code;
    }	
}

void 
Configuration::update_tagmap(const string& protocol)
{
    // delete previous tags if present
    TagMap::iterator tmi = _tagmap.find(protocol);
    if (tmi != _tagmap.end()) {
	delete (*tmi).second;
	_tagmap.erase(tmi);
    }

    // Get the redist policytags for the protocol
    TagSet* tagset = new TagSet();

    _exports.get_redist_tags(protocol, *tagset);

    if (tagset->size())
	_tagmap[protocol] = tagset;

    // if empty, just don't keep anything [no entry at all].
    else
	delete tagset;
}

void 
Configuration::link_code()
{
    // go through all modified targets and relink them.
    for(Code::TargetSet::iterator i = _modified_targets.begin();
	i != _modified_targets.end(); ++i) {

        const Code::Target& t = *i;

        switch(t.filter()) {
	    case filter::IMPORT:
		link_code(t,_imports,_import_filters);
		break;
	
	    case filter::EXPORT_SOURCEMATCH:
		link_sourcematch_code(t);
		break;
	
	    case filter::EXPORT:
		link_code(t,_exports,_export_filters);
		// export policies produce tags, update them.
		update_tagmap(t.protocol());
		break;
	}

	// we need a filter manager, and need to inform it modified targets
	// [which reflect policy filters in protocols].
	XLOG_ASSERT(_filter_manager);
	_filter_manager->update_filter(t);

    }
    _modified_targets.clear();

}

void 
Configuration::commit(uint32_t msec)
{
    // recompile and link
    compile_policies();
    link_code();

    XLOG_ASSERT(_filter_manager);

    // flush changes after the delay. [usful when receiving a lot of small
    // changes... such as boot-up].
    _filter_manager->flush_updates(msec);
}

void 
Configuration::add_varmap(const string& protocol, const string& variable,
			  const string& type, const string& access,
			  const VarRW::Id& id) 
{
    // figure out access...
    VarMap::Access acc = VarMap::READ;

    if (access == "rw")
	acc = VarMap::READ_WRITE;
    else if (access == "r")
	acc = VarMap::READ;
    else if (access == "w")
	acc = VarMap::WRITE;
    else
	xorp_throw(PolicyException,
		   "Unknown access (" + access + ") for protocol: " 
		   + protocol + " variable: " + variable);

    _varmap.add_protocol_variable(protocol, 
		  new VarMap::Variable(variable, type, acc, id)); 
}

void
Configuration::set_filter_manager(FilterManagerBase& fm)
{ 
    // cannot reassign
    XLOG_ASSERT(!_filter_manager);

    _filter_manager = &fm;
}

void 
Configuration::update_ie(const string& protocol, 
			 const POLICIES& policies, 
			 IEMap& iemap, 
			 PolicyList::PolicyType pt,
			 const string& mod)
{
    // create a new policy list
    PolicyList* pl = new PolicyList(protocol, pt, _policies, _sets, _varmap,
				    mod);

    // add the policy names to the policy list
    for (POLICIES::const_iterator i = policies.begin();
	i != policies.end(); ++i) {

        pl->push_back(*i);
    }	    

    // if there were policies, get their targets [no longer have policies]
    iemap.get_targets(protocol, mod, _modified_targets);

    // replace policy list
    iemap.insert(protocol, mod, pl);
}

void 
Configuration::link_code(const Code::Target& target, 
			 IEMap& iemap, 
			 CodeMap& codemap)
{
    // create new code and set target, so code may be linked properly
    Code* code = new Code();
    code->set_target(target);

    // link the code
    iemap.link_code(target.protocol(), *code);

    // erase previous code
    CodeMap::iterator iter = codemap.find(target.protocol());
    if(iter != codemap.end()) {
	delete (*iter).second;
	codemap.erase(iter);
    }	   

    // if code is empty [no-op filter] just erase it, and keep no entry.
    if(code->code() == "") 
        delete code;
    else 
	codemap[target.protocol()] = code;
}
    
string 
Configuration::codemap_str(CodeMap& cm)
{
    string ret = "";
    for(CodeMap::iterator i = cm.begin();
	i != cm.end(); ++i) {

    
        Code* c= (*i).second;

        ret += "PROTO: " + (*i).first + "\n";
        ret += "CODE: " + c->str() + "\n";
    }
    return ret;
}

string
Configuration::dump_state(uint32_t id)
{
    switch(id) {
	// dump policies
	case 0:
	    return _policies.str();
	    break;

	// dump varmap
	case 1:
	    return _varmap.str();
	    break;

	// dump sets
	case 2:
	    return _sets.str();
	    break;

	default:
	    xorp_throw(PolicyException, "Unknown state id: " + to_str(id));
    }
}

void
Configuration::policy_modified(const string& policy)
{
    _modified_policies.insert(policy);

    _policies.policy_deps(policy, _modified_policies);
}

bool
Configuration::test_policy(const string& policy, const RATTR& attr, RATTR& mods)
{
    PolicyStatement& ps = _policies.find(policy);

    VisitorTest test(_sets, _policies, _varmap, attr, mods);

    ps.accept(test);

    return test.accepted();
}

void
Configuration::show(const string& type, const string& name, RESOURCES& res)
{
    if (type == "policy-statement")
	show_policies(name, res);
    else
	show_sets(type, name, res);
}

void
Configuration::show_policies(const string& name, RESOURCES& res)
{
    PolicyMap::KEYS p;

    _policies.policies(p);

    for (PolicyMap::KEYS::iterator i = p.begin(); i != p.end(); ++i) {
	string n = *i;

	if (!name.empty() && name.compare(n) != 0)
	    continue;

	PolicyStatement& ps = _policies.find(n);

	ostringstream oss;
	VisitorPrinter printer(oss);

	ps.accept(printer);

	res[n] = oss.str();
    }
}

void
Configuration::show_sets(const string& type, const string& name, RESOURCES& res)
{
    SETS s;

    _sets.sets_by_type(s, type);

    for (SETS::iterator i = s.begin(); i != s.end(); ++i) {
	string n = *i;

	if (!name.empty() && name.compare(n) != 0)
	    continue;

	const Element& e = _sets.getSet(n);

	res[n] = e.str();
    }
}

IEMap::IEMap()
{
}

IEMap::~IEMap()
{
    clear();
}

IEMap::POLICY*
IEMap::find_policy(const string& protocol)
{
    PROTOCOL::iterator i = _protocols.find(protocol);

    if (i == _protocols.end())
	return NULL;

    return i->second;
}

PolicyList*
IEMap::find(const string& protocol, const string& mod)
{
    POLICY* p = find_policy(protocol);

    if (!p)
	return NULL;

    POLICY::iterator i = p->find(mod);
    if (i == p->end())
	return NULL;

    return i->second;
}

void
IEMap::insert(const string& protocol, const string& mod, PolicyList* pl)
{
    POLICY* p = find_policy(protocol);

    if (!p) {
	p = new POLICY;

	_protocols[protocol] = p;
    }

    // delete old if there
    PolicyList* pol = find(protocol, mod);
    if (pol)
	delete pol;

    (*p)[mod] = pl;
}

void
IEMap::clear()
{
    for (PROTOCOL::iterator i = _protocols.begin();
         i != _protocols.end(); ++i) {

	POLICY* p = i->second;

	clear(p);
	delete p;
    }

    _protocols.clear();
}

void
IEMap::clear(POLICY* p)
{
    for (POLICY::iterator i = p->begin(); i != p->end(); ++i)
	delete i->second;

    p->clear();
}

void
IEMap::get_targets(const string& proto, const string& mod, TARGETSET& ts)
{
    PolicyList* pl = find(proto, mod);

    if (!pl)
	return;

    pl->get_targets(ts);
}

void
IEMap::compile(PolicyStatement& ps, TARGETSET& ts, tag_t& tag)
{
    FOR_ALL_POLICIES(j) {
	PolicyList* p = j->second;

	p->compile_policy(ps, ts, tag);
    }
}

void
IEMap::compile(TARGETSET& ts, tag_t& tag)
{
    FOR_ALL_POLICIES(j) {
	PolicyList* p = j->second;

	p->compile(ts, tag);
    }
}

void
IEMap::link_code(Code& code)
{
    FOR_ALL_POLICIES(j) {
	PolicyList* p = j->second;

	p->link_code(code);
    }
}

void
IEMap::link_code(const string& proto, Code& code)
{
    POLICY* p = find_policy(proto);
    XLOG_ASSERT(p);

    for (POLICY::reverse_iterator i = p->rbegin(); i != p->rend(); ++i) {
	PolicyList* pl = i->second;

	pl->link_code(code);
    }
}

void
IEMap::get_redist_tags(const string& proto, TagSet& ts)
{
    POLICY* p = find_policy(proto);

    if (!p)
	return;

    for (POLICY::iterator i = p->begin(); i != p->end(); i++) {
	PolicyList* pl = i->second;

	pl->get_redist_tags(proto, ts);
    }
}

void
IEMap::clear(TARGETSET& ts)
{
    FOR_ALL_POLICIES(j) {
	PolicyList* p = j->second;

	p->get_targets(ts);
    }

    clear();
}
