// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/tools/print_lsas.cc,v 1.4 2005/10/13 16:49:05 atanu Exp $"

// Get LSAs from OSPF and print them.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <set>
#include <list>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ospf/ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "ospf/ospf.hh"

#include "xrl/interfaces/ospfv2_xif.hh"

class GetAreaList {
public:
    GetAreaList(XrlStdRouter& xrl_router)
	: _xrl_router(xrl_router), _done(false), _fail(false)
    {
    }

    void add(IPv4 area) {
	_areas.push_back(area);
    }

    void start() {
	XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	ospfv2.send_get_area_list("ospfv2",
				  callback(this, &GetAreaList::response));
    }

    bool busy() {
	return !_done;
    }

    bool fail() {
	return _fail;
    }

    list<IPv4>& get() {
	return _areas;
    }

private:
    void response(const XrlError& error, const XrlAtomList *atomlist) {
	_done = true;
	if (XrlError::OKAY() != error) {
	    XLOG_WARNING("Attempt to get lsa failed");
	    _fail = true;
	    return;
	}
	const size_t size = atomlist->size();
	for (size_t i = 0; i < size; i++)
	    _areas.push_back(IPv4(htonl(atomlist->get(i).uint32())));
	
    }
private:
    XrlStdRouter &_xrl_router;
    bool _done;
    bool _fail;

    list<IPv4> _areas;
};

class FetchDB {
public:
    FetchDB(XrlStdRouter& xrl_router, OspfTypes::Version version, IPv4 area)
	: _xrl_router(xrl_router), _lsa_decoder(version),
	  _done(false), _fail(false), _area(area), _index(0)
    {
	initialise_lsa_decoder(version, _lsa_decoder);
    }

    void start() {
	XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	ospfv2.send_get_lsa("ospfv2", _area, _index,
			    callback(this, &FetchDB::response));
    }

    bool busy() {
	return !_done;
    }

    bool fail() {
	return _fail;
    }

    list<Lsa::LsaRef>& get_lsas() {
	return _lsas;
    }

private:
    void response(const XrlError& error,
		  const bool *valid,
		  const bool *toohigh,
		  const bool *self,
		  const vector<uint8_t>* lsa) {
	if (XrlError::OKAY() != error) {
	    XLOG_WARNING("Attempt to get lsa failed");
	    _done = true;
	    _fail = true;
	    return;
	}
	if (*valid) {
	    size_t len = lsa->size();
	    Lsa::LsaRef lsar = _lsa_decoder.
		decode(const_cast<uint8_t *>(&(*lsa)[0]), len);
	    lsar->set_self_originating(*self);
	    _lsas.push_back(lsar);
	}
	if (*toohigh) {
	    _done = true;
	    return;
	}
	_index++;
	start();
    }
private:
    XrlStdRouter &_xrl_router;
    LsaDecoder _lsa_decoder;
    bool _done;
    bool _fail;
    const IPv4 _area;
    uint32_t _index;

    list<Lsa::LsaRef> _lsas;
};

class FilterLSA {
public:
    void add(uint16_t type) {
	_filter.push_back(type);
    }

    bool accept(uint16_t type) {
	if (_filter.empty())
	    return true;

	if (_filter.end() != find(_filter.begin(), _filter.end(), type))
	    return true;
	
	return false;
    }
private:
    list<uint16_t> _filter;
};

void
print_brief(Lsa::LsaRef lsar)
{
    printf("%-8s", lsar->name());
    printf("%c", lsar->get_self_originating() ? '*' : ' ');
    Lsa_header& header = lsar->get_header();
    printf("%-17s", pr_id(header.get_link_state_id()).c_str());
    printf("%-17s", pr_id(header.get_advertising_router()).c_str());
    printf("%-#12x", header.get_ls_sequence_number());
    printf("%4d", header.get_ls_age());
    printf("  %-#5x", header.get_options());
    printf("%-#7x", header.get_ls_checksum());
    printf("%3d", header.get_length());
    printf("\n");
}

void
print_detail(Lsa::LsaRef lsar)
{
    printf("%s\n", lsar->str().c_str());
}

enum Pstyle {
    BRIEF,
    DETAIL
};

void
print_lsa_database(string area, list<Lsa::LsaRef>& lsas, Pstyle pstyle,
		   FilterLSA& filter)
{
    printf("   OSPF link state database, Area %s\n", area.c_str());

    switch (pstyle) {
    case BRIEF:
	printf(" Type       ID               Adv "
	       "Rtr           Seq      Age  Opt  Cksum  Len\n");
	break;
    case DETAIL:
	break;
    }

    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	if (!filter.accept((*i)->get_ls_type()))
	    continue;
	switch (pstyle) {
	case BRIEF:
	    print_brief(*i);
	    break;
	case DETAIL:
	    print_detail(*i);
	    break;
	}
    }
}

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s [-a area] [-f type] [-b] [-d]\n", myname);
    return -1;
}

int 
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    Pstyle pstyle = BRIEF;
    FilterLSA filter;
    string area;

    int c;
    while ((c = getopt(argc, argv, "a:f:bd")) != -1) {
	switch (c) {
	case 'a':
	    area = optarg;
	    break;
	case 'f':
	    filter.add(atoi(optarg));
	    break;
	case 'b':
	    pstyle = BRIEF;
	    break;
	case 'd':
	    pstyle = DETAIL;
	    break;
	default:
	    return usage(argv[0]);
	}
    }

    try {
	//
	// Init stuff
	//
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "print_lsas");

	debug_msg("Waiting for router");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	debug_msg("\n");

	GetAreaList get_area_list(xrl_router);
	if (area.empty()) {
	    get_area_list.start();
	    while(get_area_list.busy())
		eventloop.run();

	    if (get_area_list.fail()) {
		fprintf(stderr, "Failed to get area list\n");
		return -1;
	    }
	} else {
	    get_area_list.add(IPv4(area.c_str()));
	}

	list<IPv4>& area_list = get_area_list.get();
	list<IPv4>::const_iterator i;
	for (i = area_list.begin(); i != area_list.end(); i++) {

	    FetchDB fetchdb(xrl_router, OspfTypes::V2, *i);

	    fetchdb.start();
	    while(fetchdb.busy())
		eventloop.run();

	    if (fetchdb.fail()) {
		fprintf(stderr, "Failed to fetch area %s\n", area.c_str());
		return -1;
	    }
	    print_lsa_database(i->str(), fetchdb.get_lsas(), pstyle, filter);
	}

    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    // 
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

