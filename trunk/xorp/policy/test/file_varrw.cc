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

#ident "$XORP$"

#include "config.h"

#include "policy/common/policy_utils.hh"

#include "file_varrw.hh"

FileVarRW::FileVarRW(const string& fname) {
    FILE* f = fopen(fname.c_str(),"r");
    if(!f) {
	string err = "Can't open file " + fname;
	err += ": ";
	err += strerror(errno);

	throw Error(err);
    }	

    char buff[1024];

    int lineno = 1;
    while(fgets(buff,sizeof(buff)-1,f)) {
	if(doLine(buff))
	    continue;
	
	// parse error
	fclose(f);
	ostringstream oss;

	oss << "Parse error at line: " << lineno;
	throw Error(oss.str());
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
   
    if(iss.eof())
	return false;
    iss >> varname;



    if(iss.eof())
	return false;
    iss >> type;


    while(iss.eof())
	return false;
    
    
    unsigned len = str.length();
    unsigned pos = len - iss.rdbuf()->in_avail();
    pos++;

    if(pos >= len)
	return false;

    value = str.substr(pos,len-pos-1);

    Element* e = _ef.create(type,value.c_str());

    _trash.insert(e);

    cout << "FileVarRW adding variable " << varname 
	 << " of type " << type << ": " << e->str() << endl;

    _map[varname] = e;

    return true;
}


const Element&
FileVarRW::read(const string& id) {
    Map::iterator i = _map.find(id);

    if(i == _map.end())
	throw Error("Cannot read variable: " + id);

    const Element* e = (*i).second;

    cout << "FileVarRW READ " << id << ": " 
	 << e->str() << endl;

    return *e;
}

void
FileVarRW::write(const string& id, const Element& e) {
    cout << "FileVarRW WRITE " << id << ": " 
	 << e.str() << endl;

    _map[id] = &e;
}

void
FileVarRW::sync() {
    cout << "FileVarRW SYNC" << endl;
    for(Map::iterator i = _map.begin(); i != _map.end(); ++i)
	cout << (*i).first << ": " << ((*i).second)->str() << endl;

    clear_trash();
}


void
FileVarRW::clear_trash() {
    policy_utils::clear_container(_trash);
}
