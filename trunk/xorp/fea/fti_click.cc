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

#ident "$XORP: xorp/fea/fti_click.cc,v 1.6 2004/08/03 03:51:46 pavlin Exp $"

#include <unistd.h>
#include <cstdio>
#include <list>

#include "fea_module.h"
#include "config.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "fticonfig.hh"
#include "fti_click.hh"
#include "click_glue.hh"

FtiClick::FtiClick() throw (FtiConfigError)
{
    _click_proc = "/click/rt";
#if 0
    if (-1 == access(_click_proc, W_OK)) {
	static char buf[1024];
	snprintf(buf, 1024,
		"FtiClick::FtiClick: accessing forwarding element %s: %s",
		_click_proc,
		strerror(errno));
	throw FtiConfigError(buf);
    }
#else
    if (!Click::is_loaded()) {
	xorp_throw(FtiConfigError, "Click is not loaded");
    }
#endif
}

bool
FtiClick::start_configuration()
{
    if (!write("start_transaction", ""))
	return false;

    string buf;

    if (!read("start_transaction", buf))
	return false;

    debug_msg("TID %s\n", buf.c_str());

    _tid = buf;

    return true;
}

bool
FtiClick::end_configuration();
{
    return write("commit_transaction", _tid);
}

bool
FtiClick::delete_all_entries4()
{
    return write("delete_all_entries", _tid);
}

bool
FtiClick::delete_entry4(const Fte4& fte)
{
    string request = _tid + string("\n") + fte.net().str() +string("\n");

    return write("delete_entry", request);
}

bool FtiClick::add_entry4(const Fte4& fte)
{
    int port;

    if (-1 == (port = ifname_to_port(fte.vifname())))
	return false;

    static char sport[1024];
    snprintf(sport, 1024, "%d", port);

    string request = _tid + string("\n") + fte.net().str() +
	string(" ") + fte.nexthop().str() +
	string(" ") + string(sport) + string("\n");

    return write("add_entry", request);
}

/*
** XXX
** For the time being allow only one reader.
*/
bool
FtiClick::start_reading4()
{
    if (_reading)
	return false;

    _routing_table.clear();
    if (!make_routing_list(_routing_table))
	return false;

    _reading = true;

    debug_msg("1\n");
    _current_entry = _routing_table.begin();

    return true;
}

bool
FtiClick::read_entry4(Fte4& fte)
{
    if (!_reading)
	return false;

    if (!(_routing_table.end() != _current_entry)) {
	_reading = false;
	return false;
    }

    fte = *_current_entry++;

    return true;
}

bool
FtiClick::lookup_route_by_dest4(IPv4 addr, Fte4& fte)
{
    string request = addr.str() + string("\n");

    if (!write("lookup_route", request))
	return false;

    FILE *fp;
    if (!open(&fp, "lookup_route", "r"))
	return false;

    /*
    ** Lines of the form:
    **
    ** 128.16.64.16    0.0.0.0/0       18.26.4.1       1
    */
    static char dst[10240], netmask[10240], nexthop[10240];
    int port;
    fscanf(fp, "%s %s %s %d", dst, netmask, nexthop, &port);

    const char* ptr = strchr(netmask, '/');
    assert(ptr != 0);

    fte = Fte4(IPv4Net(netmask), IPv4(nexthop), port_to_ifname(port));

    if (!close(&fp, "lookup_route"))
	return false;

    return true;
}

bool
FtiClick::lookup_route_by_network4(const IPv4Net& /* dst*/, Fte4& /*fte*/)
{
    XLOG_ERROR("Not implemented yet");

    return false;
}

bool
FtiClick::add_entry6(const Fte6& )
{
    return false;
}

bool
FtiClick::delete_entry6(const Fte6& )
{
    return false;
}

bool
FtiClick::delete_all_entries6()
{
    return false;
}

bool
FtiClick::start_reading6()
{
    return false;
}

bool
FtiClick::read_entry6(Fte6&)
{
    return false;
}

bool
FtiClick::lookup_route_by_dest6(const IPv6&, Fte6& )
{
    return false;
}

bool
FtiClick::lookup_route_by_network6(const IPv6Net&, Fte6& )
{
    return false;
}

bool
FtiClick::make_routing_list(list<Fte4>& rt)
{
    FILE *fp;

    if (!open(&fp, "table", "r"))
	return false;

    char line[1024];

    while(NULL != fgets(line, sizeof(line), fp)) {
	/*
	** Lines of the form:
	**
	** 10.0.4.255/32   255.255.255.255 192.150.187.1  0
	*/
	char dst[10240], netmask[10240], nexthop[10240];
	int port;
	sscanf(line, "%s %s %s %d", dst, netmask, nexthop, &port);

	rt.push_back(
	    Fte4(IPv4Net(netmask), IPv4(nexthop), port_to_ifname(port))
	    );
    }

    Fte4 fte;
    for (_current_entry = _routing_table.begin();
	_current_entry != _routing_table.end(); fte = *_current_entry++) {
	debug_msg("%s\n", fte.str().c_str());
    }

    if (!close(&fp, "table"))
	return false;

    return true;
}

bool
FtiClick::open(FILE **fp, string file, const char *mode)
{
    string transaction = string(_click_proc) + string("/") + file;

    if (NULL == (*fp = fopen(transaction.c_str(), mode))) {
	XLOG_ERROR("fopen %s failed: %s", transaction.c_str(),
		   strerror(errno));
	return false;
    }

    return true;
}

bool
FtiClick::close(FILE **fp, string file)
{

    if (0 != fclose(*fp)) {
	string transaction = string(_click_proc) + string("/") + file;
	XLOG_ERROR("fclose %s failed: %s", transaction.c_str(),
		   strerror(errno));
	return false;
    }

    return true;
}

bool
FtiClick::write(const string& file, string value)
{
    FILE *fp;
    string transaction = string(_click_proc) + file;

    if (!open(&fp, file, "w"))
	return false;

    fprintf(fp, "%s", value.c_str());

    if (!close(&fp, file))
	return false;

    return true;
}

bool
FtiClick::read(const string& file, string& value)
{
    FILE *fp;
    string transaction = string(_click_proc) + file;

    if (!open(&fp, file, "r"))
	return false;

    static char buf[1024];
    fscanf(fp, "%s", buf);
    value = string(buf);

    if (!close(&fp, file))
	return false;

    return true;
}

int
FtiClick::ifname_to_port(const string& ifname)
{
    return _cpm.ifname_to_port(ifname);
}

string
FtiClick::port_to_ifname(int port)
{
    return _cpm.port_to_ifname(port);
}

