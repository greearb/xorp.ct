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

#include "policy_module.h"
#include "config.h"

#include "code.hh"
#include "policy/common/policy_utils.hh"

bool 
Code::Target::operator<(const Target& rhs) const {
    //XXX: i think XOR will do the trick [filter number / protocol]

    string left,right;

    left = protocol + policy_utils::to_str(filter);
    right = rhs.protocol + policy_utils::to_str(rhs.filter);

    return left < right;
}

bool
Code::Target::operator==(const Target& rhs) const {
    if(protocol != rhs.protocol)
	return false;

    if(filter != rhs.filter)
	return false;

    return true;
}

bool
Code::Target::operator!=(const Target& rhs) const {
    return !(*this == rhs);
}

string
Code::Target::str() const {
    string ret = "Protocol: ";

    ret += protocol;
    ret += ", Filter: " + filter2str(filter);

    return ret;
    
}

string 
Code::str() {
    string ret = "TARGET proto: " + _target.protocol;

    ret += " FILTER: ";
    ret += filter2str(_target.filter);
    ret += "\nCODE:\n";
    ret += _code;

    ret += "SETS:";

    for(set<string>::iterator i = _sets.begin();
	i != _sets.end(); ++i) {

	ret += " " + *i;
    }    

    ret += "\n";

    return ret;
	
}



Code& 
Code::operator+=(const Code& rhs) {
    // may only add for same target
    if(_target != rhs._target)
	return *this;	// do nothing

    // add the code [link it]
    _code += rhs._code;

    // add any new sets.
    for(set<string>::iterator i = rhs._sets.begin();
        i != rhs._sets.end(); ++i)
	
	_sets.insert(*i);

    // add tags
    for(TagSet::iterator i = rhs._tags.begin();
	i != rhs._tags.end(); ++i)

	_tags.insert(*i);

    // add protos
    for(set<string>::iterator i = rhs._source_protos.begin();
	i != rhs._source_protos.end(); ++i)

	_source_protos.insert(*i);


    return *this;	
}
