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

// $XORP: xorp/bgp/local_data.hh,v 1.23 2002/12/09 18:28:42 hodson Exp $

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
    LocalData();
    ~LocalData();

    const AsNum& get_as_num() const;
    void set_as_num(const AsNum& a);

    const IPv4& get_id() const;
    void set_id(const IPv4& i);

    void add_parameter(const BGPParameter *p);
    const BGPParameter* get_parameter() const;

    void remove_parameter(const BGPParameter *p);

    uint8_t* get_optparams() const;
    uint8_t get_paramlength() const;

    void dump_data() const;
protected:
private:
    AsNum _as_num;
    IPv4 _bgp_id;

    list <const BGPParameter*> _parameters;
    uint8_t _num_parameters;
    uint8_t _param_length;
};

#endif // __BGP_LOCAL_DATA_HH__
