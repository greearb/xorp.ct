// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/fea/click_glue.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_CLICK_GLUE_HH__
#define __FEA_CLICK_GLUE_HH__


#include "ifconfig.hh"
#include "ifconfig_click.hh"

/*
** The click forwarding table interface needs to access the mappings
** from click ports to interface names. Rather than allowing total
** access this is a small shim layer to expose only the necessary
** functionality.
*/
class ClickPortmap {
public:
    ClickPortmap() :_ifconfig_click(0)
    {}

    ClickPortmap(IfconfigClick *ifconfig_click) :
	_ifconfig_click(ifconfig_click)
    {}

    inline
    int ifname_to_port(const string& interface_name) {
	return _ifconfig_click->ifname_to_port(interface_name);
    }

    inline
    string port_to_ifname(int port) {
	return _ifconfig_click->port_to_ifname(port);
    }

private:
    IfconfigClick *_ifconfig_click;
};

#endif // __FEA_CLICK_GLUE_HH__
