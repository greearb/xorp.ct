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

#ident "$XORP: xorp/policy/configuration.cc,v 1.22 2008/01/04 03:17:09 pavlin Exp $"

#include "libxorp/xorp.h"

#include "policy_module.h"
#include "configuration.hh"
#include "visitor_setdep.hh"
#include "policy/common/policy_utils.hh"

using namespace policy_utils;

Configuration::Configuration(ProcessWatchBase& pw) : 
    _currtag(0), _varmap(pw), _filter_manager(NULL)
{
}

Configuration::~Configuration()
{
    clear_map(_imports);
    clear_map(_exports);

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

    if(ps.delete_term(term)) {
	// policy needs to be re-compiled [will do so on commit]
	_modified_policies.insert(policy);
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
        _modified_policies.insert(policy); // recompile on commit.
    } catch(const Term::term_syntax_error& e) {
        string err = "In policy " + policy + ": " + e.why();
        xorp_throw(ConfError, err);
    }
} 

void 
Configuration::create_term(const string& policy, const ConfigNodeId& order,
			   const string& term)
{
    PolicyStatement& ps = _policies.find(policy);

    if(ps.term_exists(term)) {
	xorp_throw(ConfError,
		   "Term " + term + " exists already in policy " + policy);
    }

    Term* t = new Term(term);

    ps.add_term(order, t);
    _modified_policies.insert(policy);
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
Configuration::update_imports(const string& protocol, 
			      const list<string>& imports)
{
   
    // check if protocol exists
    if(!_varmap.protocol_known(protocol))
	xorp_throw(ConfError, "imports: Protocol " + protocol + " unknown");

    update_ie(protocol,imports,_imports,PolicyList::IMPORT);
    _modified_targets.insert(Code::Target(protocol,filter::IMPORT));
}

void 
Configuration::update_exports(const string& protocol, 
			      const list<string>& exports)
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

    update_ie(protocol,exports,_exports,PolicyList::EXPORT);

    // other modified targets [such as sourcematch] will be added as compilation
    // proceeds.
    _modified_targets.insert(Code::Target(protocol,filter::EXPORT));
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
Configuration::update_set_dependancy(PolicyStatement& policy)
{
    // check if sets exist and remeber which ones are used
    VisitorSetDep setdep(_sets);

    policy.accept(setdep);

    const set<string>& sets = setdep.sets();

    // make the policy depend on these sets.
    policy.set_dependancy(sets);
}

void 
Configuration::compile_policy(const string& name)
{
    PolicyStatement& policy = _policies.find(name);

    // Mark the end of the policy
    policy.set_policy_end();

    // probably is a fresh / modified policy, so update dependancies with sets.
    update_set_dependancy(policy);

    // save old tag to check for integer overflow
    uint32_t old_currtag = _currtag; 


    // go through all the import statements
    for(IEMap::iterator i = _imports.begin();
	i != _imports.end(); ++i) {
    
        PolicyList* pl = (*i).second;

	// compile the policy if it is used in this list.
	pl->compile_policy(policy,_modified_targets,_currtag);
    }    

    // go through all export statements
    for(IEMap::iterator i = _exports.begin();
	i != _exports.end(); ++i) {
    
        PolicyList* pl = (*i).second;

	// compile policy if it is used in this list.
        pl->compile_policy(policy,_modified_targets,_currtag);
    }

    // integer overflow
    if(_currtag < old_currtag) {
	// FIXME
	XLOG_FATAL("The un-avoidable occurred: We ran out of policy tags");
	abort(); // unreach [compile warning?]
    }
}

void 
Configuration::compile_policies()
{
    // integer overflow check
    uint32_t old_currtag = _currtag; 

    // compile all modified policies
    for(PolicySet::iterator i = _modified_policies.begin();
	i != _modified_policies.end(); ++i) {

        compile_policy(*i);
    }
    _modified_policies.clear();



    // compile any import policies that have not yet been compiled.
    // This is a case if a policy is not modified, but just added to a policy
    // list.
    for(IEMap::iterator i = _imports.begin();
        i != _imports.end(); ++i) {
        
        PolicyList* pl = (*i).second;
        pl->compile(_modified_targets,_currtag);
    }    

    // same for exports.
    for(IEMap::iterator i = _exports.begin();
        i != _exports.end(); ++i) {
    
        PolicyList* pl = (*i).second;
        pl->compile(_modified_targets,_currtag);
    }    
   
    // integer overflow.
    if(_currtag < old_currtag) {
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
    for(IEMap::iterator i = _exports.begin(); i != _exports.end(); ++i) {
        PolicyList* pl = (*i).second;

        pl->link_code(*code);
    }

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
    if(tmi != _tagmap.end()) {
	delete (*tmi).second;
	_tagmap.erase(tmi);
    }

    // only exports produce tags...
    IEMap::iterator i = _exports.find(protocol);
    // XXX: this shouldn't really happen... i think.
    if(i == _exports.end()) {
	return;
    }

    PolicyList* pl = (*i).second;

    // Get the redist policytags for the protocol
    TagSet* tagset = new TagSet();
    pl->get_redist_tags(protocol, *tagset);

    if(tagset->size()) {
	_tagmap[protocol] = tagset;
    }
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
			 const list<string>& policies, 
			 IEMap& iemap, 
			 PolicyList::PolicyType pt)
{
    // create a new policy list
    PolicyList* pl = new PolicyList(protocol,pt,_policies,_sets,_varmap);

    // add the policy names to the policy list
    for(list<string>::const_iterator i = policies.begin();
        i != policies.end(); ++i) {

        pl->push_back(*i);
    }	    

    // delete old policy list
    IEMap::iterator i = iemap.find(protocol);
    if(i != iemap.end()) {
        PolicyList* oldpl = (*i).second;

	// targets are modified [they no longer have these policies]
        oldpl->get_targets(_modified_targets);
        delete oldpl;
    }
    
    // replace policy list
    iemap[protocol] = pl;

}	
    
void 
Configuration::link_code(const Code::Target& target, 
			 IEMap& iemap, 
			 CodeMap& codemap)
{
    // find the policy list for the target
    IEMap::iterator i = iemap.find(target.protocol());

    XLOG_ASSERT(i != iemap.end());

    PolicyList* pl = (*i).second;

    // create new code and set target, so code may be linked properly
    Code* code = new Code();
    code->set_target(target);

    // link the code
    pl->link_code(*code);


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
