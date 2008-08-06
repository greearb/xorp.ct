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

#ident "$XORP: xorp/policy/test/file_varrw.cc,v 1.16 2008/08/06 08:11:01 abittau Exp $"

#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "policy/common/policy_utils.hh"
#include "file_varrw.hh"

FileVarRW::FileVarRW() : _trashc(0), _verbose(true)
{
    memset(_map, 0, sizeof(_map));
}

void
FileVarRW::load(const string& fname)
{
    FILE* f = fopen(fname.c_str(), "r");

    if (!f) {
	string err = "Can't open file " + fname;
	err += ": ";
	err += strerror(errno);

	xorp_throw(Error, err);
    }	

    char buff[1024];
    int  lineno = 1;

    while (fgets(buff, sizeof(buff)-1, f)) {
	if (doLine(buff))
	    continue;
	
	// parse error
	fclose(f);
	ostringstream oss;

	oss << "Parse error at line: " << lineno;
	xorp_throw(Error, oss.str());
    }

    fclose(f);
}

FileVarRW::~FileVarRW() {
    clear_trash();
}

bool
FileVarRW::doLine(const string& str) {
    istringstream iss(str);

    string varname;
    string type;
    string value;
   
    if (iss.eof())
	return false;
    iss >> varname;

    if (iss.eof())
	return false;
    iss >> type;

    if(iss.eof())
	return false;

    iss >> value;
    while (! iss.eof()) {
	string v;
	iss >> v;
	if (v.size())
	    value += " " + v;
    }

    Element* e = _ef.create(type, value.c_str());

    XLOG_ASSERT(_trashc < MAX_TRASH);
    _trash[_trashc++] = e;

    if (_verbose)
	cout << "FileVarRW adding variable " << varname 
	     << " of type " << type << ": " << e->str() << endl;

    char* err = 0;
    Id id = strtol(varname.c_str(), &err, 10);
    if (*err) {
	xorp_throw(Error, string("Varname must be ID [numeric]: ") + err);
    }

    _map[id] = e;

    return true;
}

const Element&
FileVarRW::read(const Id& id) {
    const Element* e = _map[id];

    if (!e) {
	ostringstream oss;

	oss << "Cannot read variable: " << id;
	xorp_throw(Error, oss.str());
    }

    if (_verbose)
	cout << "FileVarRW READ " << id << ": " 
	     << e->str() << endl;

    return *e;
}

void
FileVarRW::write(const Id& id, const Element& e) {
    if (_verbose)
	cout << "FileVarRW WRITE " << id << ": " 
	     << e.str() << endl;

    _map[id] = &e;
}

void
FileVarRW::sync() {
    if (_verbose) {
	    cout << "FileVarRW SYNC" << endl;
	    for (unsigned i = 0; i < VAR_MAX; i++) {
		const Element* e = _map[i];

		if (e)
		    cout << i << ": " << e->str() << endl;
	    }
    }
}

void
FileVarRW::clear_trash() {
    while (_trashc--)
	delete _trash[_trashc];

    _trashc = 0;
}

void
FileVarRW::set_verbose(bool verb)
{
    _verbose = verb;
}
