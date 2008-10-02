// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/tools/print_databases.cc,v 1.2 2008/07/23 05:09:55 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr/olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxorp/tlv.hh"

#include <set>
#include <list>

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/olsr4_xif.hh"

#include "olsr/olsr_types.hh"
#include "olsr/olsr.hh"
#include "olsr/test_common.hh"

// Sorting will break the 1:1 mapping between id and Info
// structures after retrieval via XRL. Bear this in mind if we need to
// further extend this tool.
#ifndef SORT_OUTPUT
#define SORT_OUTPUT
#endif

/**
 * Humanize a bool.
 *
 * @param value the boolean value to express as a human readable string.
 * @param short_form true if a single character should be printed.
 * @return "yes"/"*" if value is true, otherwise "no"/" ".
 */
inline const char*
humanize_bool(const bool value, const bool short_form = false)
{
    if (value) {
    	if (short_form)
		return ("*");
	else
		return ("yes");
    } else {
    	if (short_form)
		return (" ");
	else
		return ("no");
    }
}

//
// OLSRv1/IPv4 operational mode command to get database status.
//
// Note: TLVs are in olsr/olsr_types.hh.
//

//
// Base class.
//

class Getter {
public:
    Getter(XrlRouter& xr, const string& tgt)
      : _xr(xr), _tgt(tgt), _fail(false), _done(false)
    {}

    virtual ~Getter() {}

    // parse command line arguments and fire off the XRLs
    // to get the particular state we want.
    virtual void get(int argc, char *argv[]) = 0;

    // true if the command failed
    virtual bool fail() { return _fail; }

    // print the appropriate output
    virtual void output() = 0;

    // TODO: save to a TLV file
    //virtual void save() = 0;

    // poll for completion; return true if we should keep
    // running the event loop.
    inline bool busy() { return ! _done; }

protected:
    XrlRouter&	_xr;
    string	_tgt;
    bool	_fail;
    bool	_done;
};


//
// External route information.
//

struct ExternalInfo {
    IPv4Net	_destination;
    IPv4	_lasthop;
    uint32_t	_distance;
    uint32_t	_hold_time;
};

class GetExternals : public Getter {
public:
    GetExternals(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetExternals()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void external_cb(const XrlError& e,
		     const IPv4Net* destination,
		     const IPv4* lasthop,
		     const uint32_t* distance,
		     const uint32_t* hold_time);

    void fetch_next();

private:
    list<OlsrTypes::ExternalID>			_external_ids;
    list<OlsrTypes::ExternalID>::iterator	_index;

    list<ExternalInfo>				_infos;
};

void
GetExternals::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_hna_entry_list(this->_tgt.c_str(),
			           callback(this, &GetExternals::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get external route list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetExternals::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get external route list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_external_ids.push_back((atoms->get(i).uint32()));
    }

    _index = _external_ids.begin();

    fetch_next();
}

void
GetExternals::fetch_next()
{
    if (_external_ids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_hna_entry(this->_tgt.c_str(),
			      *_index,
			      callback(this,
				       &GetExternals::external_cb));

    if (! success)
	XLOG_WARNING("Failed to get external route info.");
}

void
GetExternals::external_cb(const XrlError& e,
			  const IPv4Net* destination,
			  const IPv4* lasthop,
			  const uint32_t* distance,
			  const uint32_t* hold_time)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Attempt to get external route info failed");
	_done = true;
	_fail = true;
	return;
    }
    ExternalInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(destination);
    copy_info(lasthop);
    copy_info(distance);
    copy_info(hold_time);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetExternals::output()
{
    if (_infos.empty()) {
	printf("No external routes learned.\n");
	return;
    }

    printf("%-18s %-15s %8s %5s\n", "Destination", "Lasthop",
	   "Distance", "Hold");

    list<ExternalInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const ExternalInfo& info = (*ii);
	printf("%-18s %-15s %8u %5u\n",
		cstring(info._destination),
		cstring(info._lasthop),
		XORP_UINT_CAST(info._distance),
		XORP_UINT_CAST(info._hold_time));
    }
}

//
// Interface information.
//

struct InterfaceInfo {
    string	_ifname;
    string	_vifname;
    IPv4	_local_addr;
    uint16_t	_local_port;
    IPv4	_all_nodes_addr;
    uint16_t	_all_nodes_port;
};

class GetInterfaces : public Getter {
public:
    GetInterfaces(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetInterfaces()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void interface_cb(const XrlError& e,
		      const string* ifname,
		      const string* vifname,
		      const IPv4* local_addr,
		      const uint32_t* local_port,
		      const IPv4* all_nodes_addr,
		      const uint32_t* all_nodes_port);

    void fetch_next();

private:
    list<OlsrTypes::FaceID>		_face_ids;
    list<OlsrTypes::FaceID>::iterator	_index;

    list<InterfaceInfo>			_infos;
};

void
GetInterfaces::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_interface_list(this->_tgt.c_str(),
			           callback(this, &GetInterfaces::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get interface list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetInterfaces::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get interface list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_face_ids.push_back((atoms->get(i).uint32()));
    }

    _index = _face_ids.begin();

    fetch_next();
}

void
GetInterfaces::fetch_next()
{
    if (_face_ids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_interface_info(this->_tgt.c_str(),
				   *_index,
			           callback(this,
					    &GetInterfaces::interface_cb));

    if (! success)
	XLOG_WARNING("Failed to get interface info.");
}

void
GetInterfaces::interface_cb(
    const XrlError& e,
    const string* ifname,
    const string* vifname,
    const IPv4* local_addr,
    const uint32_t* local_port,
    const IPv4* all_nodes_addr,
    const uint32_t* all_nodes_port)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Attempt to get interface info failed");
	_done = true;
	_fail = true;
	return;
    }
    InterfaceInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(ifname);
    copy_info(vifname);
    copy_info(local_addr);
    copy_info(local_port);
    copy_info(all_nodes_addr);
    copy_info(all_nodes_port);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetInterfaces::output()
{
    if (_infos.empty()) {
	printf("No interfaces configured.\n");
	return;
    }

    printf("%-10s %-20s %-20s\n", "Interface", "LocalAddr", "AllNodesAddr");

    list<InterfaceInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const InterfaceInfo& info = (*ii);
	printf("%-4s/%-4s %15s:%-5u %15s:%-5u\n",
	       info._ifname.c_str(),
	       info._vifname.c_str(),
	       cstring(info._local_addr),
	       XORP_UINT_CAST(info._local_port),
	       cstring(info._all_nodes_addr),
	       XORP_UINT_CAST(info._all_nodes_port));
    }
}

struct InterfaceStatsInfo {
    uint32_t	_bad_packets;
    uint32_t	_bad_messages;
    uint32_t	_messages_from_self;
    uint32_t	_unknown_messages;
    uint32_t	_duplicates;
    uint32_t	_forwarded;
};

// TODO: Interface statistics.

//
// Link information.
//

struct LinkInfo {
    IPv4	_local_addr;
    IPv4	_remote_addr;
    IPv4	_main_addr;
    uint32_t	_link_type;
    uint32_t	_sym_time;
    uint32_t	_asym_time;
    uint32_t	_hold_time;
};

class GetLinks : public Getter {
public:
    GetLinks(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetLinks()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void link_cb(const XrlError& e,
		 const IPv4*   local_addr,
		 const IPv4*   remote_addr,
		 const IPv4*   main_addr,
		 const uint32_t* link_type,
		 const uint32_t* sym_time,
		 const uint32_t* asym_time,
		 const uint32_t* hold_time);

    void fetch_next();

private:
    list<OlsrTypes::LogicalLinkID>		_link_ids;
    list<OlsrTypes::LogicalLinkID>::iterator	_index;

    list<LinkInfo>				_infos;
};

void
GetLinks::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_link_list(this->_tgt.c_str(),
			      callback(this, &GetLinks::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get link list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetLinks::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get link list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_link_ids.push_back((atoms->get(i).uint32()));
    }

    _index = _link_ids.begin();

    fetch_next();
}

void
GetLinks::fetch_next()
{
    if (_link_ids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_link_info(this->_tgt.c_str(),
			      *_index,
			      callback(this,
				       &GetLinks::link_cb));

    if (! success)
	XLOG_WARNING("Failed to get link info.");
}

void
GetLinks::link_cb(const XrlError& e,
		  const IPv4*   local_addr,
		  const IPv4*   remote_addr,
		  const IPv4*   main_addr,
		  const uint32_t* link_type,
		  const uint32_t* sym_time,
		  const uint32_t* asym_time,
		  const uint32_t* hold_time)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get link info.");
	_done = true;
	_fail = true;
	return;
    }
    LinkInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(local_addr);
    copy_info(remote_addr);
    copy_info(main_addr);
    copy_info(link_type);
    copy_info(sym_time);
    copy_info(asym_time);
    copy_info(hold_time);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

struct LinkSortOrderPred {
    bool operator()(const LinkInfo& lhs, const LinkInfo& rhs) const
    {
	return lhs._remote_addr < rhs._remote_addr;
    }
};

void
GetLinks::output()
{
    if (_infos.empty()) {
	printf("No links learned.\n");
	return;
    }

//LocalAddr       RemoteAddr      Neighbor        Type   ASYM    SYM  Hold
//%-15s           %-15s           %-15s           %-4s    %5u    %5u   %5u
    printf("%-15s %-15s %-15s %-4s %5s %5s %5s\n",
	   "LocalAddr", "RemoteAddr", "Neighbor", "Type", "ASYM",
	   "SYM", "Hold");

    // TODO: Turn LinkType into a symbolic string.

#ifdef SORT_OUTPUT
    _infos.sort(LinkSortOrderPred());
#endif

    list<LinkInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const LinkInfo& info = (*ii);
	printf("%-15s %-15s %-15s %-4u %5u %5u %5u\n",
		cstring(info._local_addr),
		cstring(info._remote_addr),
		cstring(info._main_addr),
		XORP_UINT_CAST(info._link_type),	    // XXX
		XORP_UINT_CAST(info._sym_time),
		XORP_UINT_CAST(info._asym_time),
		XORP_UINT_CAST(info._hold_time));
    }
}

//
// MID entries.
//

struct MidInfo {
    IPv4	_main_addr;
    IPv4	_iface_addr;
    uint32_t	_distance;
    uint32_t	_hold_time;
};

class GetMids : public Getter {
public:
    GetMids(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetMids()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void mid_cb(const XrlError& e,
		const IPv4*   main_addr,
		const IPv4*   iface_addr,
		const uint32_t* distance,
		const uint32_t* hold_time);

    void fetch_next();

private:
    list<OlsrTypes::MidEntryID>			_mid_ids;
    list<OlsrTypes::MidEntryID>::iterator	_index;

    list<MidInfo>				_infos;
};

void
GetMids::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_mid_entry_list(this->_tgt.c_str(),
				   callback(this, &GetMids::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get MID list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetMids::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get MID list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_mid_ids.push_back((atoms->get(i).uint32()));
    }

    _index = _mid_ids.begin();

    fetch_next();
}

void
GetMids::fetch_next()
{
    if (_mid_ids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_mid_entry(this->_tgt.c_str(),
			      *_index,
			      callback(this,
				       &GetMids::mid_cb));

    if (! success)
	XLOG_WARNING("Failed to get MID info.");
}

void
GetMids::mid_cb(const XrlError& e,
		const IPv4*   main_addr,
		const IPv4*   iface_addr,
		const uint32_t* distance,
		const uint32_t* hold_time)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get MID info.");
	_done = true;
	_fail = true;
	return;
    }
    MidInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(main_addr);
    copy_info(iface_addr);
    copy_info(distance);
    copy_info(hold_time);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetMids::output()
{
    if (_infos.empty()) {
	printf("No MID entries learned.\n");
	return;
    }

    printf("%-15s %-15s %9s %5s\n", "MainAddr", "RemoteAddr",
	   "Distance", "Hold");

    list<MidInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const MidInfo& info = (*ii);
	printf("%-15s %-15s %9u %5u\n",
		cstring(info._main_addr),
		cstring(info._iface_addr),
		XORP_UINT_CAST(info._distance),
		XORP_UINT_CAST(info._hold_time));
    }
}

//
// Neighbor information.
//

struct NeighborInfo {
    IPv4	_main_addr;
    uint32_t	_willingness;
    uint32_t	_degree;
    uint32_t	_link_count;
    uint32_t	_twohop_link_count;
    bool	_is_advertised;
    bool	_is_sym;
    bool	_is_mpr;
    bool	_is_mpr_selector;
};

class GetNeighbors : public Getter {
public:
    GetNeighbors(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetNeighbors()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void neighbor_cb(const XrlError& e,
		     const IPv4*	_main_addr,
		     const uint32_t*	_willingness,
		     const uint32_t*	_degree,
		     const uint32_t*	_link_count,
		     const uint32_t*	_twohop_link_count,
		     const bool*	_is_advertised,
		     const bool*	_is_sym,
		     const bool*	_is_mpr,
		     const bool*	_is_mpr_selector);

    void fetch_next();

private:
    list<OlsrTypes::NeighborID>			_nids;
    list<OlsrTypes::NeighborID>::iterator	_index;

    list<NeighborInfo>				_infos;
};

void
GetNeighbors::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_neighbor_list(this->_tgt.c_str(),
				  callback(this, &GetNeighbors::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get neighbor list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetNeighbors::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get neighbor list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_nids.push_back((atoms->get(i).uint32()));
    }

    _index = _nids.begin();

    fetch_next();
}

void
GetNeighbors::fetch_next()
{
    if (_nids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_neighbor_info(this->_tgt.c_str(),
				  *_index,
				  callback(this, &GetNeighbors::neighbor_cb));

    if (! success)
	XLOG_WARNING("Failed to get neighbor info.");
}

void
GetNeighbors::neighbor_cb(const XrlError& e,
			 const IPv4*	 main_addr,
			 const uint32_t* willingness,
			 const uint32_t* degree,
			 const uint32_t* link_count,
			 const uint32_t* twohop_link_count,
			 const bool*	 is_advertised,
			 const bool*	 is_sym,
			 const bool*	 is_mpr,
			 const bool*	 is_mpr_selector)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get neighbor info.");
	_done = true;
	_fail = true;
	return;
    }
    NeighborInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(main_addr);
    copy_info(willingness);
    copy_info(degree);
    copy_info(link_count);
    copy_info(twohop_link_count);
    copy_info(is_advertised);
    copy_info(is_sym);
    copy_info(is_mpr);
    copy_info(is_mpr_selector);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

struct NeighborSortOrderPred {
    bool operator()(const NeighborInfo& lhs, const NeighborInfo& rhs) const
    {
	return lhs._main_addr < rhs._main_addr;
    }
};

void
GetNeighbors::output()
{
    if (_infos.empty()) {
	printf("No neighbors learned.\n");
	return;
    }

    printf("%-15s %4s %6s %5s %6s %3s %3s %3s %3s\n",
	   "MainAddr", "Will", "Degree", "Links", "2links",
	   "ADV", "SYM", "MPR", "MPRS");

#ifdef SORT_OUTPUT
    _infos.sort(NeighborSortOrderPred());
#endif

    list<NeighborInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const NeighborInfo& info = (*ii);
	printf("%-15s %4u %6u %5u %6u %3s %3s %3s %3s\n",
		cstring(info._main_addr),
		XORP_UINT_CAST(info._willingness),
		XORP_UINT_CAST(info._degree),
		XORP_UINT_CAST(info._link_count),
		XORP_UINT_CAST(info._twohop_link_count),
		humanize_bool(info._is_advertised, true),
		humanize_bool(info._is_sym, true),
		humanize_bool(info._is_mpr, true),
		humanize_bool(info._is_mpr_selector, true));
    }
}

//
// Topology information.
//

struct TopologyInfo {
    IPv4	_destination;
    IPv4	_lasthop;
    uint32_t	_distance;
    uint32_t	_seqno;
    uint32_t	_hold_time;
};

class GetTopology : public Getter {
public:
    GetTopology(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetTopology()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void topology_cb(const XrlError& e,
		     const IPv4*   destination,
		     const IPv4*   lasthop,
		     const uint32_t* distance,
		     const uint32_t* seqno,
		     const uint32_t* hold_time);

    void fetch_next();

private:
    list<OlsrTypes::TopologyID>			_tc_ids;
    list<OlsrTypes::TopologyID>::iterator	_index;

    list<TopologyInfo>				_infos;
};

void
GetTopology::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_tc_entry_list(this->_tgt.c_str(),
				  callback(this, &GetTopology::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get topology list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetTopology::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get topology list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_tc_ids.push_back((atoms->get(i).uint32()));
    }

    _index = _tc_ids.begin();

    fetch_next();
}

void
GetTopology::fetch_next()
{
    if (_tc_ids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_tc_entry(this->_tgt.c_str(),
			     *_index,
			     callback(this,
				      &GetTopology::topology_cb));

    if (! success)
	XLOG_WARNING("Failed to get topology info.");
}

void
GetTopology::topology_cb(const XrlError& e,
			 const IPv4*   destination,
			 const IPv4*   lasthop,
			 const uint32_t* distance,
			 const uint32_t* seqno,
			 const uint32_t* hold_time)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get topology info.");
	_done = true;
	_fail = true;
	return;
    }
    TopologyInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(destination);
    copy_info(lasthop);
    copy_info(distance);
    copy_info(seqno);
    copy_info(hold_time);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetTopology::output()
{
    if (_infos.empty()) {
	printf("No TC entries learned.\n");
	return;
    }

    printf("%-15s %-15s %8s %5s %5s\n",
	   "Destination", "Lasthop", "Distance", "SeqNo", "Hold");

    list<TopologyInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const TopologyInfo& info = (*ii);
	printf("%-15s %-15s %8u %5u %5u\n",
		cstring(info._destination),
		cstring(info._lasthop),
		XORP_UINT_CAST(info._distance),
		XORP_UINT_CAST(info._seqno),
		XORP_UINT_CAST(info._hold_time));
    }
}

//
// Two-hop link information.
//

struct TwohopLinkInfo {
    uint32_t	_last_face_id;
    IPv4	_nexthop_addr;
    IPv4	_dest_addr;
    uint32_t	_hold_time;
};

class GetTwohopLinks : public Getter {
public:
    GetTwohopLinks(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetTwohopLinks()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void twohop_link_cb(const XrlError& e,
			const uint32_t*	_last_face_id,
			const IPv4*	_nexthop_addr,
			const IPv4*	_dest_addr,
			const uint32_t*	_hold_time);

    void fetch_next();

private:
    list<OlsrTypes::TwoHopLinkID>			_tlids;
    list<OlsrTypes::TwoHopLinkID>::iterator		_index;

    list<TwohopLinkInfo>				_infos;
};

void
GetTwohopLinks::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_twohop_link_list(this->_tgt.c_str(),
				     callback(this, &GetTwohopLinks::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get neighbor list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetTwohopLinks::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get neighbor list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_tlids.push_back((atoms->get(i).uint32()));
    }

    _index = _tlids.begin();

    fetch_next();
}

void
GetTwohopLinks::fetch_next()
{
    if (_tlids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_twohop_link_info(this->_tgt.c_str(),
				     *_index,
				     callback(this,
					      &GetTwohopLinks::twohop_link_cb));

    if (! success)
	XLOG_WARNING("Failed to get two-hop link info.");
}

void
GetTwohopLinks::twohop_link_cb(const XrlError&	e,
			      const uint32_t*	last_face_id,
			      const IPv4*	nexthop_addr,
			      const IPv4*	dest_addr,
			      const uint32_t*	hold_time)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get two-hop link info.");
	_done = true;
	_fail = true;
	return;
    }
    TwohopLinkInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(last_face_id);
    copy_info(nexthop_addr);
    copy_info(dest_addr);
    copy_info(hold_time);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetTwohopLinks::output()
{
    if (_infos.empty()) {
	printf("No two-hop links learned.\n");
	return;
    }

    printf("%-15s %-15s %5s\n",
	   "Destination", "Nexthop", "Hold");

    list<TwohopLinkInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const TwohopLinkInfo& info = (*ii);
	printf( "%-15s %-15s %5u\n",
		cstring(info._dest_addr),
		cstring(info._nexthop_addr),
		XORP_UINT_CAST(info._hold_time));
    }
}

//
// Two-hop neighbor information.
//

struct TwohopNeighborInfo {
    IPv4	_main_addr;
    bool	_is_strict;
    uint32_t	_link_count;
    uint32_t	_reachability;
    uint32_t	_coverage;
};

class GetTwohopNeighbors : public Getter {
public:
    GetTwohopNeighbors(XrlRouter& xr, const string& tgt)
     : Getter(xr, tgt)
    {}

    ~GetTwohopNeighbors()
    {}

    void get(int argc, char *argv[]);

    // print the appropriate output
    void output();

private:
    void list_cb(const XrlError& e, const XrlAtomList *atoms);
    void twohop_neighbor_cb(const XrlError& e,
			    const IPv4*	main_addr,
			    const bool*	is_strict,
			    const uint32_t*	link_count,
			    const uint32_t*	reachability,
			    const uint32_t*	coverage);

    void fetch_next();

private:
    list<OlsrTypes::TwoHopNodeID>			_tnids;
    list<OlsrTypes::TwoHopNodeID>::iterator		_index;

    list<TwohopNeighborInfo>				_infos;
};

void
GetTwohopNeighbors::get(int argc, char *argv[])
{
    XrlOlsr4V0p1Client cl(&this->_xr);

    bool success =
	cl.send_get_twohop_neighbor_list(
	    this->_tgt.c_str(),
	    callback(this, &GetTwohopNeighbors::list_cb));

    if (! success)
	XLOG_WARNING("Failed to get two-hop neighbor list.");

    UNUSED(argc);
    UNUSED(argv);
}

void
GetTwohopNeighbors::list_cb(const XrlError& e, const XrlAtomList *atoms)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get two-hop neighbor list.");
	_done = true;
	_fail = true;
	return;
    }

    const size_t size = atoms->size();
    for (size_t i = 0; i < size; i++) {
	_tnids.push_back((atoms->get(i).uint32()));
    }

    _index = _tnids.begin();

    fetch_next();
}

void
GetTwohopNeighbors::fetch_next()
{
    if (_tnids.end() == _index) {
	_done = true;
	return;
    }

    XrlOlsr4V0p1Client cl(&this->_xr);
    bool success =
	cl.send_get_twohop_neighbor_info(
	    this->_tgt.c_str(),
	    *_index,
	    callback(this, &GetTwohopNeighbors::twohop_neighbor_cb));

    if (! success)
	XLOG_WARNING("Failed to get two-hop neighbor info.");
}

void
GetTwohopNeighbors::twohop_neighbor_cb(const XrlError& e,
				      const IPv4*	main_addr,
				      const bool*	is_strict,
				      const uint32_t*	link_count,
				      const uint32_t*	reachability,
				      const uint32_t*	coverage)
{
    if (XrlError::OKAY() != e) {
	XLOG_WARNING("Failed to get two-hop neighbor info.");
	_done = true;
	_fail = true;
	return;
    }
    TwohopNeighborInfo info;
#define copy_info(var) info._ ## var = *var
    copy_info(main_addr);
    copy_info(is_strict);
    copy_info(link_count);
    copy_info(reachability);
    copy_info(coverage);
#undef copy_info
    _infos.push_back(info);
    _index++;
    fetch_next();
}

void
GetTwohopNeighbors::output()
{
    if (_infos.empty()) {
	printf("No two-hop neighbors learned.\n");
	return;
    }

    printf("%-15s %3s %8s %12s\n",
	   "MainAddr", "N1", "Coverage", "Reachability");

    list<TwohopNeighborInfo>::iterator ii;
    for (ii = _infos.begin(); ii != _infos.end(); ii++) {
	const TwohopNeighborInfo& info = (*ii);
	printf( "%-15s %3s %8u %12u\n",
		cstring(info._main_addr),
		humanize_bool(! info._is_strict, true),
		XORP_UINT_CAST(info._link_count),
		XORP_UINT_CAST(info._coverage));
    }
}

typedef map<string, Getter*> GetterMap;
GetterMap   _getters;

void
register_getter(const string& name, Getter* getter)
{
    _getters[name] = getter;
}

void
unregister_all_getters()
{
    while (! _getters.empty()) {
	GetterMap::iterator ii = _getters.begin();
	delete (*ii).second;
	_getters.erase(ii);
    }
}

void
initialize_getters(XrlRouter& xr, const string& tgt)
{
    register_getter("external", new GetExternals(xr, tgt));
    register_getter("interface", new GetInterfaces(xr, tgt));
    register_getter("link", new GetLinks(xr, tgt));
    register_getter("mid", new GetMids(xr, tgt));
    register_getter("neighbor", new GetNeighbors(xr, tgt));
    register_getter("topology", new GetTopology(xr, tgt));
    register_getter("twohop-link", new GetTwohopLinks(xr, tgt));
    register_getter("twohop-neighbor", new GetTwohopNeighbors(xr, tgt));
}

Getter*
get_getter(const string& get_type)
{
    GetterMap::iterator ii = _getters.find(get_type);

    if (ii == _getters.end())
	return 0;

    return (*ii).second;
}

int
usage(const char *myname)
{
    fprintf(stderr,
"usage: %s [external|interface|link|mid|neighbor|topology|\n"
"           twohop-link|twohop-neighbor]\n",
	    myname);
    return -1;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    char* progname = argv[0];

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    // TODO: getopt() here; use options for brief vs terse, ipv4 vs ipv6.

    if (argc < 2) {
	usage(progname);
	exit(1);
    }

    string olsr_name("olsr4");

    argc--; ++argv;
    string getter_name(argv[0]);
    argc--; ++argv;

    try {
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "print_databases");

	//debug_msg("Waiting for router\n");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	//debug_msg("\n");

	initialize_getters(xrl_router, olsr_name);

	Getter* getter = get_getter(getter_name);
	if (getter == 0) {
	    fprintf(stderr, "No such getter %s\n", getter_name.c_str());
	    unregister_all_getters();
	    exit(1);
	}

	getter->get(argc, argv);

	while (getter->busy())
	    eventloop.run();

	if (! getter->fail()) {
	    getter->output();
	}

    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    unregister_all_getters();

    xlog_stop();
    xlog_exit();

    return 0;
}
