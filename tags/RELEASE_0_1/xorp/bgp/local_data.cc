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

#ident "$XORP: xorp/bgp/local_data.cc,v 1.28 2002/12/09 18:28:42 hodson Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "packet.hh"

LocalData::LocalData()
    // XXX it's not nice to give a spurious value for _as_num or _bgp_id
    : _as_num(), _bgp_id()
{
    _num_parameters = 0;
    _param_length = 0;
}

LocalData::~LocalData()
{
    list <const BGPParameter*>::iterator iter;
    iter = _parameters.begin();
    while (iter != _parameters.end()) {
	delete *iter;
	++iter;
    }
}

const AsNum&
LocalData::get_as_num() const
{
	return _as_num;
}

void
LocalData::set_as_num(const AsNum& a)
{
	_as_num = a;
}

const IPv4&
LocalData::get_id() const
{
	return _bgp_id;
}

void
LocalData::set_id(const IPv4& i)
{
	_bgp_id = i;
}

void
LocalData::add_parameter(const BGPParameter* p)
{
    _parameters.push_back(p);
    // p->dump_contents();
    _num_parameters++;
    // TODO add bounds checking
    _param_length = _param_length+p->get_paramlength();
    debug_msg("LocalData::add_parameter, num: %d, length %d,"
	      " total length: %d\n",
	      _num_parameters, p->get_paramlength(), _param_length);
}

void
LocalData::remove_parameter(const BGPParameter* p)
{
    list <const BGPParameter*>::iterator iter;
    iter = _parameters.begin();
    while (iter != _parameters.end()) {
	if (*iter == p) {
	    _parameters.erase(iter);
	    break;
	}
	++iter;
    }

}

uint8_t*
LocalData::get_optparams() const
{
    uint8_t* data = new uint8_t[_param_length];
    uint8_t pos = 0;

    list <const BGPParameter*>::const_iterator iter;
    iter = _parameters.begin();
    while (iter != _parameters.end()) {
	memcpy(data+pos, (*iter)->get_data(),(*iter)->get_paramlength());
	pos = pos + (*iter)->get_paramlength();
	debug_msg("Parameter total size %d", (*iter)->get_paramlength());
	++iter;
    }
    return data;
}
uint8_t
LocalData::get_paramlength() const
{
    return _param_length;
}


const BGPParameter*
LocalData::get_parameter() const
{
    return 0;
}

void
LocalData::dump_data() const
{
}
