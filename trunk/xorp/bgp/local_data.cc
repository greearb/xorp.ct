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

#ident "$XORP: xorp/bgp/local_data.cc,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "packet.hh"

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

uint8_t*
LocalData::get_optparams() const
{
    uint8_t *data = new uint8_t[_param_length];
    uint8_t *dst = data; 
    list <const BGPParameter*>::const_iterator i;

    for (i = _parameters.begin(); i != _parameters.end(); ++i) {
	size_t l = (*i)->get_paramlength();

	memcpy(dst, (*i)->get_data(), l);
	dst += l;
	debug_msg("Parameter total size %d", l);
    }
    return data;
}
