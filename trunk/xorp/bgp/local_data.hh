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

// $XORP: xorp/bgp/local_data.hh,v 1.3 2003/01/28 03:21:52 rizzo Exp $

#ifndef __BGP_LOCAL_DATA_HH__
#define __BGP_LOCAL_DATA_HH__

#include <sys/types.h>
#include <string.h>
#include <list>

#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/asnum.hh"
#include "libxorp/eventloop.hh"

#include "parameter.hh"

class LocalData {
public:
    LocalData() : _as(AsNum::AS_INVALID)	{}

    LocalData(const AsNum& as, const IPv4& id)
	    : _as(as), _id(id)			{
	_num_parameters = 0;
	_param_length = 0;
    }

    ~LocalData()				{
	list <const BGPParameter*>::const_iterator iter;

	for (iter = _parameters.begin(); iter != _parameters.end(); ++iter)
	    delete *iter;
    }


    const AsNum& as() const			{ return _as;		}

    void set_as(const AsNum& a)			{ _as = a;		}

    const IPv4& id() const			{ return _id;		}

    void set_id(const IPv4& i)			{ _id = i;		}


    uint8_t get_paramlength() const		{ return _param_length;	}
    const BGPParameter* get_parameter() const	{ return 0;		}

    void dump_data() const			{}

    void add_parameter(const BGPParameter *p);

    void remove_parameter(const BGPParameter *p) {
	list <const BGPParameter*>::iterator iter;

	for (iter = _parameters.begin(); iter != _parameters.end(); ++iter)
	    if (*iter == p) {
		_parameters.erase(iter);
		break;
	    }
    }

    uint8_t* get_optparams() const;

protected:
private:
    AsNum	_as;
    IPv4	_id;

    list <const BGPParameter*> _parameters;
    uint8_t	_num_parameters;
    uint8_t	_param_length;
};

#endif // __BGP_LOCAL_DATA_HH__
