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

// $XORP: xorp/bgp/local_data.hh,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $

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
    /**
     * XXX see if it is possible to avoid calling the default
     * initializer, as the AsNum field would be invalid.
     */
    // LocalData();	// not implemented
    LocalData() : _as_num(AsNum::invalid_As) {}

    LocalData(const AsNum& as, const IPv4& id)
	    : _as_num(as), _bgp_id(id)		{
	_num_parameters = 0;
	_param_length = 0;
    }

    ~LocalData()				{
	list <const BGPParameter*>::iterator iter;

	for (iter = _parameters.begin(); iter != _parameters.end(); ++iter)
	    delete *iter;
    }


    const AsNum& get_as_num() const		{ return _as_num;	}

    void set_as_num(const AsNum& a)		{ _as_num = a;		}

    const IPv4& get_id() const			{ return _bgp_id;	}

    void set_id(const IPv4& i)			{ _bgp_id = i;		}


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
    AsNum _as_num;
    IPv4 _bgp_id;

    list <const BGPParameter*> _parameters;
    uint8_t _num_parameters;
    uint8_t _param_length;
};

#endif // __BGP_LOCAL_DATA_HH__
