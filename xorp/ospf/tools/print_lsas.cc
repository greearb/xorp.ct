// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



// Get LSAs (in raw binary) from OSPF and print them.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf/ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxorp/tlv.hh"

#include <set>
#include <list>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/ospfv2_xif.hh"
#include "xrl/interfaces/ospfv3_xif.hh"

#include "ospf/ospf.hh"
#include "ospf/test_common.hh"


/**
 * return OS name.
 */
string
host_os()
{
#ifdef	HAVE_UNAME
    struct utsname name;

    if (0 != uname(&name)) {
	return HOST_OS_NAME;
    }

    return name.sysname;
#endif

    return HOST_OS_NAME;
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
	switch(_version) {
	case OspfTypes::V2: {
	    XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	    ospfv2.send_get_area_list(xrl_target(_version),
				      callback(this, &GetAreaList::response));
	}
	    break;
	case OspfTypes::V3: {
	    XrlOspfv3V0p1Client ospfv3(&_xrl_router);
	    ospfv3.send_get_area_list(xrl_target(_version),
				      callback(this, &GetAreaList::response));
	}
	    break;
	}
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
	switch(_version) {
	case OspfTypes::V2: {
	    XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	    ospfv2.send_get_lsa(xrl_target(_version), _area, _index,
				callback(this, &FetchDB::response));
	}
	    break;
	case OspfTypes::V3: {
	    XrlOspfv3V0p1Client ospfv3(&_xrl_router);
	    ospfv3.send_get_lsa(xrl_target(_version), _area, _index,
				callback(this, &FetchDB::response));
	}
	    break;
	}
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
	switch(_version) {
	case OspfTypes::V2:
	    printf(" Type       ID               Adv "
		   "Rtr           Seq      Age  Opt  Cksum  Len\n");
	    break;
	case OspfTypes::V3:
	    printf(" Type       ID               Adv "
		   "Rtr           Seq         Age  Cksum  Len\n");
	    break;
	}

	return true;
    }

    bool print(Lsa::LsaRef lsar) {
	switch(_version) {
	case OspfTypes::V2:
	    printf("%-8s", lsar->name());
	    break;
	case OspfTypes::V3:
	    printf("%-11s", lsar->name());
	    break;
	}
	printf("%c", lsar->get_self_originating() ? '*' : ' ');
	Lsa_header& header = lsar->get_header();
	printf("%-17s", pr_id(header.get_link_state_id()).c_str());
	printf("%-17s", pr_id(header.get_advertising_router()).c_str());
	printf("%-#12x", header.get_ls_sequence_number());
	printf("%4d", header.get_ls_age());
	switch(_version) {
	case OspfTypes::V2:
	    printf("  %-#5x", header.get_options());
	    break;
	case OspfTypes::V3:
	    printf("  ");
	    break;
	}
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

class Save : public Output {
public:
    Save(OspfTypes::Version version, FilterLSA& filter, string& fname)
	: Output(version, filter), _fname(fname)
    {}

    bool begin() {
	if (!_tlv.open(_fname, false /* write */)) {
	    XLOG_ERROR("Unable to open %s", _fname.c_str());
	    return false;
	}

	// 1) Version
	vector<uint8_t> data;
	data.resize(sizeof(uint32_t));
	_tlv.put32(data, 0, TLV_VERSION);
	if (!_tlv.write(TLV_CURRENT_VERSION, data))
	    return false;

	// 2) System info
	string host = host_os();
	uint32_t len = host.size();
	data.resize(len);
	memcpy(&data[0], host.c_str(), len);
	if (!_tlv.write(TLV_SYSTEM_INFO, data))
	    return false;

	// 3) OSPF Version
	data.resize(sizeof(uint32_t));
	_tlv.put32(data, 0, _version);
	if (!_tlv.write(TLV_OSPF_VERSION, data))
	    return false;

	return true;
    }

    bool begin_area(string area) {
	vector<uint8_t> data;
	data.resize(sizeof(uint32_t));
	_tlv.put32(data, 0, ntohl(IPv4(area.c_str()).addr()));
	return _tlv.write(TLV_AREA, data);
    }

    bool print(Lsa::LsaRef lsar) {
	size_t len;
	uint8_t *ptr = lsar->lsa(len);
	vector<uint8_t> data;
	data.resize(len);
	memcpy(&data[0], ptr, len);
	return _tlv.write(TLV_LSA, data);
    }

    bool end() {
	return _tlv.close();
    }

private:
    string _fname;
    Tlv _tlv;
};

enum Pstyle {
    BRIEF,
    DETAIL,
    SUMMARY,
    SAVE
};

int
usage(const char *myname)
{
    fprintf(stderr,
	    "usage: %s [-a area] [-f type] [-s] [-b] [-d] [-S fname]\n",
	    myname);

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
    string fname;

    int c;
    while ((c = getopt(argc, argv, "23a:f:sbdS:")) != -1) {
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
	case 'f': {
	    char *endptr;
	    uint32_t number = strtoul(optarg, &endptr, 0);
	    if (0 != *endptr) {
		XLOG_ERROR("<%s> is not a number", optarg);
		return -1;
	    }
	    filter.add(number);
	}
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
	case 'S':
	    pstyle = SAVE;
	    fname = optarg;
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
    case SAVE:
	output = new Save(version, filter, fname);
	break;
    }

    try {
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
		XLOG_ERROR("Failed to get area list");
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
		XLOG_ERROR("Failed to fetch area %s", i->str().c_str());
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

	delete output;

    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return 0;
}

