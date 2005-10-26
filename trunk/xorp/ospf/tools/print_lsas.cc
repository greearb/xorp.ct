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

#ident "$XORP: xorp/ospf/tools/print_lsas.cc,v 1.6 2005/10/23 09:33:39 atanu Exp $"

// Get LSAs (in raw binary) from OSPF and print them.

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

/**
 * Get the XRL target name.
 */
inline
const char *
target(OspfTypes::Version version)
{
    switch (version) {
    case OspfTypes::V2:
	return TARGET_OSPFv2;
	break;
    case OspfTypes::V3:
	return TARGET_OSPFv3;
	break;
    }

    XLOG_UNREACHABLE();
}

/**
 * Get the list of configured areas
 */
class GetAreaList {
public:
    GetAreaList(XrlStdRouter& xrl_router, OspfTypes::Version version)
	: _xrl_router(xrl_router), _version(version),
	  _done(false), _fail(false)
    {
    }

    void add(IPv4 area) {
	_areas.push_back(area);
    }

    void start() {
	XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	ospfv2.send_get_area_list(target(_version),
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
    OspfTypes::Version _version;
    bool _done;
    bool _fail;

    list<IPv4> _areas;
};

/**
 * Fetch all the LSAs for one area.
 */
class FetchDB {
public:
    FetchDB(XrlStdRouter& xrl_router, OspfTypes::Version version, IPv4 area)
	: _xrl_router(xrl_router), _version(version), _lsa_decoder(version),
	  _done(false), _fail(false), _area(area), _index(0)
    {
	initialise_lsa_decoder(version, _lsa_decoder);
    }

    void start() {
	XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	ospfv2.send_get_lsa(target(_version), _area, _index,
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
    OspfTypes::Version _version;
    LsaDecoder _lsa_decoder;
    bool _done;
    bool _fail;
    const IPv4 _area;
    uint32_t _index;

    list<Lsa::LsaRef> _lsas;
};

/**
 * Filter LSAs. If the filter is empty everything is accepted, if the
 * filter contains types then only an LSA matching a type is passed.
 */
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

/**
 * The base class that needs to be implemented by all print routines.
 */ 
class Output {
public:
    Output(OspfTypes::Version version, FilterLSA& filter)
	: _version(version), _filter(filter)
    {}

    virtual ~Output()
    {}

    virtual bool begin() {
	return true;
    }

    void print_area(string area) {
	printf("   OSPF link state database, Area %s\n", area.c_str());
    }

    virtual bool begin_area(string area) {
	_area = area;

	return true;
    }

    virtual bool print(Lsa::LsaRef lsar) {
	printf("%s\n", lsar->str().c_str());

	return true;
    }

    virtual bool end_area(string /*area*/) {
	_area = "";

	return true;
    }

    virtual bool end() {
	return true;
    }

protected:
    OspfTypes::Version _version;
    FilterLSA& _filter;
    string _area;
};

class Brief : public Output {
public:
    Brief(OspfTypes::Version version, FilterLSA& filter)
	: Output(version, filter)
    {}

    bool begin_area(string area) {
	print_area(area);
	printf(" Type       ID               Adv "
	       "Rtr           Seq      Age  Opt  Cksum  Len\n");

	return true;
    }

    bool print(Lsa::LsaRef lsar) {
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

	return true;
    }
};

class Detail : public Output {
public:
    Detail(OspfTypes::Version version, FilterLSA& filter)
	: Output(version, filter)
    {}

    bool begin_area(string area) {
	print_area(area);
	
	return true;
    }
};

class Summary : public Output {
public:
    Summary(OspfTypes::Version version, FilterLSA& filter)
	: Output(version, filter), _externals(0)
    {}

    bool begin_area(string area) {
	printf("Area %s\n", area.c_str());

	_summary.clear();

	return true;
    }

    bool print(Lsa::LsaRef lsar) {
	uint16_t type = lsar->get_ls_type();
	if (0 == _summary.count(type)) {
	    _summary[type] = 1;
	} else {
	    _summary[type] = _summary[type] + 1;
	}
	
	return true;
    }

    bool end_area(string /*area*/) {
	LsaDecoder lsa_decoder(_version);
	initialise_lsa_decoder(_version, lsa_decoder);

	map<uint16_t, uint32_t>::const_iterator i;
	for (i = _summary.begin(); i != _summary.end(); i++) {
	    uint16_t type = (*i).first;
	    uint32_t count = (*i).second;
	    if (lsa_decoder.external(type)) {
		if (0 == _externals)
		    _externals = count;
	    } else {
		printf("%5d %s LSAs\n", count, lsa_decoder.name(type));
	    }
	}

	return true;
    }

    bool end() {
	if (_filter.accept(ASExternalLsa(_version).get_ls_type())) {
	    printf("Externals:\n");
	    if (0 != _externals)
		printf("%5d External LSAs\n", _externals);
	}

	return true;
    }

private:
    uint32_t _externals;	// Number of AS-External-LSAs.
    map<uint16_t, uint32_t> _summary;
};

enum Pstyle {
    BRIEF,
    DETAIL,
    SUMMARY
};

#if	0
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

uint32_t externals = 0;	// Number of AS-External-LSAs.

void
print_summary(OspfTypes::Version version, map<uint16_t, uint32_t>& summary)
{
    LsaDecoder lsa_decoder(version);
    initialise_lsa_decoder(version, lsa_decoder);

    map<uint16_t, uint32_t>::const_iterator i;
    for (i = summary.begin(); i != summary.end(); i++) {
	uint16_t type = (*i).first;
	uint32_t count = (*i).second;
	if (lsa_decoder.external(type)) {
	    if (0 == externals)
		externals = count;
	} else {
	    printf("%5d %s LSAs\n", count, lsa_decoder.name(type));
	}
    }
}

void
print_lsa_database(OspfTypes::Version version, string area,
		   list<Lsa::LsaRef>& lsas, Pstyle pstyle, FilterLSA& filter)
{
    switch (pstyle) {
    case BRIEF:
	printf("   OSPF link state database, Area %s\n", area.c_str());
	printf(" Type       ID               Adv "
	       "Rtr           Seq      Age  Opt  Cksum  Len\n");
	break;
    case DETAIL:
	printf("   OSPF link state database, Area %s\n", area.c_str());
	break;
    case SUMMARY:
	printf("Area %s\n", area.c_str());
	break;
    }

    // LSA Type, count
    map<uint16_t, uint32_t> summary;

    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	uint16_t type = (*i)->get_ls_type();
	if (!filter.accept(type))
	    continue;
	switch (pstyle) {
	case BRIEF:
	    print_brief(*i);
	    break;
	case DETAIL:
	    print_detail(*i);
	    break;
	case SUMMARY:
	    if (0 == summary.count(type)) {
		summary[type] = 1;
	    } else {
		summary[type] = summary[type] + 1;
	    }
	    break;
	}
    }

    switch (pstyle) {
    case BRIEF:
	break;
    case DETAIL:
	break;
    case SUMMARY:
	print_summary(version, summary);
	break;
    }
}
#endif

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s [-a area] [-f type] [-s] [-b] [-d]\n", myname);
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

    OspfTypes::Version version = OspfTypes::V2;
    Pstyle pstyle = BRIEF;
    FilterLSA filter;
    string area;

    int c;
    while ((c = getopt(argc, argv, "23a:f:sbd")) != -1) {
	switch (c) {
	case '2':
	    version = OspfTypes::V2;
	    break;
	case '3':
	    version = OspfTypes::V3;
	    break;
	case 'a':
	    area = optarg;
	    break;
	case 'f':
	    filter.add(atoi(optarg));
	    break;
	case 's':
	    pstyle = SUMMARY;
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

    Output *output = 0;
    switch (pstyle) {
    case BRIEF:
	output = new Brief(version, filter);
	break;
    case DETAIL:
	output = new Detail(version, filter);
	break;
    case SUMMARY:
	output = new Summary(version, filter);
	break;
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

	GetAreaList get_area_list(xrl_router, version);
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

	if (!output->begin())
	    return -1;
	list<IPv4>& area_list = get_area_list.get();
	list<IPv4>::const_iterator i;
	for (i = area_list.begin(); i != area_list.end(); i++) {

	    FetchDB fetchdb(xrl_router, version, *i);

	    fetchdb.start();
	    while(fetchdb.busy())
		eventloop.run();

	    if (fetchdb.fail()) {
		fprintf(stderr, "Failed to fetch area %s\n", i->str().c_str());
		return -1;
	    }

	    if (!output->begin_area(i->str()))
		return -1;
	    list<Lsa::LsaRef>& lsas = fetchdb.get_lsas();
	    list<Lsa::LsaRef>::const_iterator j;
	    for (j = lsas.begin(); j != lsas.end(); j++) {
		uint16_t type = (*j)->get_ls_type();
		if (!filter.accept(type))
		    continue;
		if (!output->print(*j))
		    return -1;
	    }
	    if (!output->end_area(i->str()))
		return -1;
	}
	if (!output->end())
	    return -1;

#if	0
	list<IPv4>& area_list = get_area_list.get();
	list<IPv4>::const_iterator i;
	for (i = area_list.begin(); i != area_list.end(); i++) {

	    FetchDB fetchdb(xrl_router, version, *i);

	    fetchdb.start();
	    while(fetchdb.busy())
		eventloop.run();

	    if (fetchdb.fail()) {
		fprintf(stderr, "Failed to fetch area %s\n", area.c_str());
		return -1;
	    }
	    print_lsa_database(version, i->str(), fetchdb.get_lsas(), pstyle,
			       filter);
	}

	switch (pstyle) {
	case BRIEF:
	    break;
	case DETAIL:
	    break;
	case SUMMARY: {
	    if (filter.accept(ASExternalLsa(version).get_ls_type())) {
		printf("Externals:\n");
		if (0 != externals)
		    printf("%5d External LSAs\n", externals);
	    }
	}
	    break;	    
	}
#endif	
	delete output;

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

