// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/asnum.cc,v 1.11 2002/12/09 18:29:10 hodson Exp $"

#include "xorp.h"
#include "asnum.hh"
#include "c_format.hh"
#include "debug.h"

//
// default constructor generates an AsNum that will be invalid
//
AsNum::AsNum()
    : _asnum(0xffffffffU),
      _is_extended(false)
{
    
}

AsNum::AsNum(uint16_t value)
{
    debug_msg("Constructor AsNum(uint16_t %d) called\n", value);
    set_asnum(value);
}

AsNum::AsNum(uint32_t value)
{
    debug_msg("Constructor AsNum(uint32_t %d) called\n", value);
    set_asnum(value);
}

void
AsNum::set_asnum(uint16_t value)
{
    _asnum = (uint16_t)value;
    _is_extended = false;
}

void
AsNum::set_asnum(uint32_t value)
{
    _asnum = value;
    _is_extended = true;
}

bool
AsNum::operator==(const AsNum& other) const
{
    debug_msg("AsNum::operator==(const AsNum& other) const called\n");
    
    if (_is_extended != other._is_extended) {
	debug_msg("== on extended and non-extended AS nums\n");
	return false;
    }
    
    //
    // Both AS numbers are either extended or non-extended
    //
    if (_is_extended) {
	debug_msg("== on extended AS nums\n"); 
	if (_asnum == other._asnum)
	    return true;
	else
	    return false;
    } else {
	debug_msg("== on non-extended AS nums\n");
	if (((uint16_t)_asnum) == ((uint16_t)other._asnum))
	    return true;
	else
	    return false;
    }
}

bool
AsNum::operator<(const AsNum& other) const
{
    return _asnum < other._asnum;
}

string
AsNum::str() const
{
    return c_format("AS/%d %s", _asnum, _is_extended ? "extended" : "");
}
