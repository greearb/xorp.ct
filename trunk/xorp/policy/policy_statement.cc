// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/policy/policy_statement.cc,v 1.12 2007/02/16 22:46:54 pavlin Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"

#include "policy/common/policy_utils.hh"

#include "policy_statement.hh"   


using namespace policy_utils;

PolicyStatement::PolicyStatement(const string& name, SetMap& smap) : 
    _name(name), _smap(smap)
{
}

PolicyStatement::~PolicyStatement()
{
    del_dependancies();
    policy_utils::clear_map_container(_terms);

    list<pair<ConfigNodeId, Term*> >::iterator iter;
    for (iter = _out_of_order_terms.begin();
	 iter != _out_of_order_terms.end();
	 ++iter) {
	delete iter->second;
    }
}
   
void 
PolicyStatement::add_term(const ConfigNodeId& order, Term* term)
{
    if((_terms.find(order) != _terms.end())
       || (find_out_of_order_term(order) != _out_of_order_terms.end())) {
	xorp_throw(PolicyException,
		   "Term already present in position: " + order.str());
    }

    pair<TermContainer::iterator, bool> res;
    res = _terms.insert(order, term);
    if (res.second != true) {
	//
	// Failed to add the entry, probably because it was received out of
	// order. Add it to the list of entries that need to be added later.
	//
	_out_of_order_terms.push_back(make_pair(order, term));
	return;
    }

    //
    // Try to add any entries that are out of order.
    // Note that we need to keep trying traversing the list until
    // no entry is added.
    //
    while (true) {
	bool entry_added = false;
	list<pair<ConfigNodeId, Term*> >::iterator iter;
	for (iter = _out_of_order_terms.begin();
	     iter != _out_of_order_terms.end();
	     ++iter) {
	    res = _terms.insert(iter->first, iter->second);
	    if (res.second == true) {
		// Entry added successfully
		entry_added = true;
		_out_of_order_terms.erase(iter);
		break;
	    }
	}

	if (! entry_added)
	    break;
    }
}

PolicyStatement::TermContainer::iterator 
PolicyStatement::get_term_iter(const string& name)
{
    TermContainer::iterator i;

    for(i = _terms.begin();
	i != _terms.end(); ++i) {

        if( (i->second)->name() == name) {
	    return i;
	}
    }    

    return i;
}

PolicyStatement::TermContainer::const_iterator 
PolicyStatement::get_term_iter(const string& name) const 
{
    TermContainer::const_iterator i;

    for(i = _terms.begin();
	i != _terms.end(); ++i) {

        if( (i->second)->name() == name) {
	    return i;
	}
    }    

    return i;
}

Term& 
PolicyStatement::find_term(const string& name) const 
{
    TermContainer::const_iterator i = get_term_iter(name);
    if(i == _terms.end()) {
	list<pair<ConfigNodeId, Term*> >::const_iterator list_iter;
	list_iter = find_out_of_order_term(name);
	if (list_iter != _out_of_order_terms.end()) {
	    Term* t = list_iter->second;
	    return *t;
	}

	xorp_throw(PolicyStatementErr,
		   "Term " + name + " not found in policy " + _name);
    }

    Term* t = i->second;
    return *t;    
}

bool 
PolicyStatement::delete_term(const string& name)
{
    TermContainer::iterator i = get_term_iter(name);

    if(i == _terms.end()) {
	list<pair<ConfigNodeId, Term*> >::iterator list_iter;
	list_iter = find_out_of_order_term(name);
	if (list_iter != _out_of_order_terms.end()) {
	    Term* t = list_iter->second;
	    _out_of_order_terms.erase(list_iter);
	    delete t;
	    return true;
	}
	return false;
    }

    Term* t = i->second;

    _terms.erase(i);

    delete t;
    return true;
}

void
PolicyStatement::set_policy_end()
{
    TermContainer::iterator i;

    for (i = _terms.begin(); i != _terms.end(); ++i) {
	Term* term = i->second;
	term->set_term_end();
    }

    //
    // XXX: The multi-value term nodes should not have holes, hence
    // print a warning if there are remaining out of order terms.
    //
    if (! _out_of_order_terms.empty()) {
	// Create a list with the term names
	string term_names;
	list<pair<ConfigNodeId, Term*> >::iterator list_iter;
	for (list_iter = _out_of_order_terms.begin();
	     list_iter != _out_of_order_terms.end();
	     ++list_iter) {
	    Term* term = (*list_iter).second;
	    if (list_iter != _out_of_order_terms.begin())
		term_names += ", ";
	    term_names += term->name();
	}
	XLOG_ERROR("Found out-of-order term(s) inside policy %s: %s. "
		   "The term(s) will be excluded!",
		   name().c_str(), term_names.c_str());
    }
}

const string& 
PolicyStatement::name() const 
{
    return _name; 
}

bool 
PolicyStatement::accept(Visitor& v) 
{
    return v.visit(*this);
}

PolicyStatement::TermContainer& 
PolicyStatement::terms()
{ 
    return _terms; 
}

void 
PolicyStatement::set_dependancy(const set<string>& sets)
{
    // replace all dependancies

    // delete them all
    del_dependancies();

    // replace
    _sets = sets;

    // re-insert dependancies
    for(set<string>::iterator i = _sets.begin(); i != _sets.end(); ++i)
	_smap.add_dependancy(*i,_name);
}

void 
PolicyStatement::del_dependancies() {
    // remove all dependancies
    for(set<string>::iterator i = _sets.begin(); i != _sets.end(); ++i)
	_smap.del_dependancy(*i,_name);

    _sets.clear();    
}

bool
PolicyStatement::term_exists(const string& name) const 
{
    if((get_term_iter(name)  == _terms.end())
       && (find_out_of_order_term(name) == _out_of_order_terms.end())) {
	return false;
    }

    return true;
}

list<pair<ConfigNodeId, Term*> >::iterator
PolicyStatement::find_out_of_order_term(const ConfigNodeId& order)
{
    list<pair<ConfigNodeId, Term*> >::iterator iter;

    for (iter = _out_of_order_terms.begin();
	 iter != _out_of_order_terms.end();
	 ++iter) {
	const ConfigNodeId& list_order = iter->first;
	if (list_order.unique_node_id() == order.unique_node_id())
	    return (iter);
    }

    return (_out_of_order_terms.end());
}

list<pair<ConfigNodeId, Term*> >::iterator
PolicyStatement::find_out_of_order_term(const string& name)
{
    list<pair<ConfigNodeId, Term*> >::iterator iter;

    for (iter = _out_of_order_terms.begin();
	 iter != _out_of_order_terms.end();
	 ++iter) {
	const Term* term = iter->second;
	if (term->name() == name)
	    return (iter);
    }

    return (_out_of_order_terms.end());
}

list<pair<ConfigNodeId, Term*> >::const_iterator
PolicyStatement::find_out_of_order_term(const string& name) const
{
    list<pair<ConfigNodeId, Term*> >::const_iterator iter;

    for (iter = _out_of_order_terms.begin();
	 iter != _out_of_order_terms.end();
	 ++iter) {
	const Term* term = iter->second;
	if (term->name() == name)
	    return (iter);
    }

    return (_out_of_order_terms.end());
}
