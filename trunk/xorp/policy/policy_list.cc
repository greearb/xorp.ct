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

#ident "$XORP: xorp/policy/policy_list.cc,v 1.14 2008/08/06 08:17:06 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "policy_list.hh"
#include "visitor_semantic.hh"
#include "export_code_generator.hh"
#include "source_match_code_generator.hh"

PolicyList::PolicyList(const string& p, PolicyType pt, 
		       PolicyMap& pmap, SetMap& smap, VarMap& vmap,
		       string mod)
		       : _protocol(p), _type(pt), _pmap(pmap),
		         _smap(smap), _varmap(vmap), _mod(mod),
			 _mod_term(NULL), _mod_term_import(NULL),
			 _mod_term_export(NULL)
{
    if (!_mod.empty()) {
	    _mod_term_import = create_mod(Term::SOURCE);
	    _mod_term_export = create_mod(Term::DEST);
    }
}

Term*
PolicyList::create_mod(Term::BLOCKS block)
{
    // We add the modifier term at the beginning of each policy.  If it matches,
    // we continue executing the policy, else we go to the next one.
    Term* t  = new Term("__mod"); // XXX leak if exception is thrown below

    ConfigNodeId nid(0, 0);

    string statement = "not " + _mod;
    t->set_block(block, nid, statement);
    t->set_block_end(block);

    statement = "next policy;";
    t->set_block(Term::ACTION, nid, statement);
    t->set_block_end(Term::ACTION);

    return t;
}

PolicyList::~PolicyList()
{
    for(PolicyCodeList::iterator i = _policies.begin(); 
	i != _policies.end(); ++i) {

	PolicyCode& pc = *i;

	_pmap.del_dependency(pc.first,_protocol);

	delete (*i).second;
    }

    delete _mod_term_import;
    delete _mod_term_export;
}

void 
PolicyList::push_back(const string& policyname)
{
    _policies.push_back(PolicyCode(policyname,NULL));
    _pmap.add_dependency(policyname,_protocol);
}

void 
PolicyList::compile_policy(PolicyStatement& ps,Code::TargetSet& mod, 
			   uint32_t& tagstart)
{
    // go throw all the policies present in this list
    for(PolicyCodeList::iterator i = _policies.begin();
	i != _policies.end(); ++i) {

	// if the policy is present, then compile it
        if(ps.name() == (*i).first) {
	    switch(_type) {
		case IMPORT:
		    compile_import(i,ps,mod);
		    break;
		case EXPORT:
		    compile_export(i,ps,mod,tagstart);
		    break;
	    }
	}
    }    
}

void 
PolicyList::compile(Code::TargetSet& mod, uint32_t& tagstart)
{
    // go throw all policies in the list
    for(PolicyCodeList::iterator i = _policies.begin();
	i != _policies.end(); ++i) {

	PolicyCode& pc = *i;

	// deal only with non compiled policies [i.e. policies without
	// associated code].
	if(pc.second) 
	    continue;

	// find the policy statement and compile it.
	PolicyStatement& ps = _pmap.find(pc.first);
    
	switch(_type) {
	    case IMPORT:
		compile_import(i,ps,mod);
		break;
	
	    case EXPORT:
		compile_export(i,ps,mod,tagstart);
		break;
	}
    }    
}

string 
PolicyList::str()
{
    string ret = "Policy Type: ";
	
    switch(_type) {

	case IMPORT:
	    ret += "import";
	    break;
	
	case EXPORT:
	    ret += "export";
	    break;

    }
    ret += "\n";
    ret += "Protocol: " + _protocol + "\n";

    for(PolicyCodeList::iterator i = _policies.begin();
	i != _policies.end(); ++i) {

	ret += "PolicyName: " + (*i).first + "\n";

	ret += "Code:\n";

	CodeList* cl = (*i).second;
	if(cl)
	    ret += cl->str();
	else
	    ret += "NOT COMPILED\n";
    }

    return ret;
}

void 
PolicyList::link_code(Code& ret)
{
    // go through all the policies, and link the code
    for (PolicyCodeList::iterator i = _policies.begin();
	 i != _policies.end(); ++i) {

	CodeList* cl = (*i).second;

	// because of target set in ret, only relevant code will be linked.
	cl->link_code(ret);

    }
}

void 
PolicyList::get_targets(Code::TargetSet& targets)
{
    // go through all the policies in the list, and return the targets affected
    // by the code.
    for(PolicyCodeList::iterator i = _policies.begin(); i !=
	_policies.end(); ++i) {

	CodeList* cl = (*i).second;
	
	// get all the targets in this code list [a single policy may have more
	// than one targets -- such as source match filters].
	cl->get_targets(targets);
    }
}

void 
PolicyList::get_redist_tags(const string& protocol, Code::TagSet& ts)
{
    // go through all policies and return tags associated with the requested
    // protocol.
    for(PolicyCodeList::iterator i = _policies.begin(); i !=
	_policies.end(); ++i) {
    
	CodeList* cl = (*i).second;

	cl->get_redist_tags(protocol,ts);
    }    
}

void 
PolicyList::semantic_check(PolicyStatement& ps, 
			   VisitorSemantic::PolicyType type)
{
    // check if policy makes sense with this instantiation 
    // [i.e. protocol and import/export pair].
    SemanticVarRW varrw(_varmap);

    VisitorSemantic sem_check(varrw, _varmap, _smap, _pmap, _protocol, type);

    // exception will be thrown if all goes wrong.

    // check modifier [a bit of a hack]
    if (_mod_term)
	_mod_term->accept(sem_check);

    ps.accept(sem_check);
}

void 
PolicyList::compile_import(PolicyCodeList::iterator& iter, 
			   PolicyStatement& ps,
			   Code::TargetSet& modified_targets)
{
    _mod_term = _mod_term_import;

    // check the policy
    semantic_check(ps, VisitorSemantic::IMPORT);

    // generate the code
    CodeGenerator cg(_protocol, _varmap, _pmap);

    // check modifier [a bit of a hack]
    if (_mod_term)
	_mod_term->accept(cg);

    ps.accept(cg);

    // make a copy of the code
    Code* code = new Code(cg.code());

    // in this case, since it is import, there is only onle code fragment in the
    // list.
    CodeList* cl = new CodeList(ps.name());
    cl->push_back(code);

    // if any code was previously stored, delete it
    if((*iter).second) {
	delete (*iter).second;
    }

    // replace code
    (*iter).second = cl;

    // target was modified
    modified_targets.insert(code->target());
}

void 
PolicyList::compile_export(PolicyCodeList::iterator& iter, PolicyStatement& ps, 
			   Code::TargetSet& modified_targets, 
			   uint32_t& tagstart)
{
    _mod_term = _mod_term_export;

    // make sure policy makes sense
    semantic_check(ps, VisitorSemantic::EXPORT);

    // generate source match code
    SourceMatchCodeGenerator smcg(tagstart, _varmap, _pmap);
    
    // check modifier [a bit of a hack]
    if (_mod_term)
	_mod_term->accept(smcg);

    ps.accept(smcg);

    // generate Export code
    ExportCodeGenerator ecg(_protocol, smcg.tags(), _varmap, _pmap);

    // check modifier [a bit of a hack]
    if (_mod_term)
	_mod_term->accept(ecg);

    ps.accept(ecg);

    // update the global tag start
    tagstart = smcg.next_tag();

    // get the export code and add it to the new codelist.
    Code* code = new Code(ecg.code());
    CodeList* cl = new CodeList(ps.name());
    cl->push_back(code);

    //
    // If we had a codelist, then add the set of EXPORT_SOURCEMATCH
    // targets to the set of modified targets.
    //
    // This is needed to cover the case when we delete all policy terms
    // that export from a particular protocol.
    //
    if ((*iter).second) {
	CodeList* old_cl = (*iter).second;
	Code::TargetSet old_targets;
	old_cl->get_targets(old_targets, filter::EXPORT_SOURCEMATCH);

	Code::TargetSet::iterator i;
	for (i = old_targets.begin(); i != old_targets.end(); ++i) {
	    modified_targets.insert(*i);
	}
    }

    // if we had a codelist get rid of it
    if ((*iter).second) {
        delete (*iter).second;
    }
    // store new code list
    (*iter).second = cl;

    // export target modified
    modified_targets.insert(code->target());

    // we may get a lot of code fragments here:
    // consider a policy where each term has a different source protocol...
    vector<Code*>& codes = smcg.codes();

    // add the fragments to the code list
    for (vector<Code*>::iterator i = codes.begin();
         i != codes.end(); ++i) {

        Code* c = *i;
        cl->push_back(c);

        modified_targets.insert(c->target());

        // keep track of source protocols in export policy code.
        code->add_source_protocol(c->target().protocol());
    }
}
