// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/policy_list.cc,v 1.11 2007/06/29 02:03:20 pavlin Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"

#include "policy_list.hh"
#include "visitor_semantic.hh"
#include "export_code_generator.hh"
#include "source_match_code_generator.hh"

PolicyList::PolicyList(const string& p, PolicyType pt, 
		       PolicyMap& pmap, SetMap& smap, VarMap& vmap) :
	
	_protocol(p), _type(pt), _pmap(pmap), _smap(smap), _varmap(vmap) 
{
}
    
PolicyList::~PolicyList()
{
    for(PolicyCodeList::iterator i = _policies.begin(); 
	i != _policies.end(); ++i) {

	PolicyCode& pc = *i;

	_pmap.del_dependancy(pc.first,_protocol);

	delete (*i).second;
    }
}

void 
PolicyList::push_back(const string& policyname)
{
    _policies.push_back(PolicyCode(policyname,NULL));
    _pmap.add_dependancy(policyname,_protocol);
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
    for(PolicyCodeList::iterator i = _policies.begin();
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

    VisitorSemantic sem_check(varrw, _varmap, _smap,_protocol,type);

    // exception will be thrown if all goes wrong.
    ps.accept(sem_check);
}

void 
PolicyList::compile_import(PolicyCodeList::iterator& iter, 
			   PolicyStatement& ps,
			   Code::TargetSet& modified_targets)
{
    // check the policy
    semantic_check(ps,VisitorSemantic::IMPORT);

    // generate the code
    CodeGenerator cg(_protocol, _varmap);
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
    // make sure policy makes sense
    semantic_check(ps, VisitorSemantic::EXPORT);

    // generate source match code
    SourceMatchCodeGenerator smcg(tagstart, _varmap);
    ps.accept(smcg);

    // generate Export code
    ExportCodeGenerator ecg(_protocol, smcg.tags(), _varmap);
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
