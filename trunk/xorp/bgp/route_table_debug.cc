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

#ident "$XORP: xorp/bgp/route_table_debug.cc,v 1.2 2002/12/14 21:25:46 mjh Exp $"

//#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_debug.hh"

template<class A>
DebugTable<A>::DebugTable(string table_name,  
			  BGPRouteTable<A> *parent_table) 
    : BGPRouteTable<A>("DebugTable-" + table_name)
{
    _parent = parent_table;
    _print_tablename = false;
    _msgs = 0;
}

template<class A>
DebugTable<A>::~DebugTable() {
    if (_ofile != NULL)
	fclose(_ofile);
}

template<class A>
int
DebugTable<A>::add_route(const InternalMessage<A> &rtmsg, 
			 BGPRouteTable<A> *caller) {
    assert(caller == _parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", _tablename.c_str()); 
    fprintf(_ofile, "[ADD]\n");
    if (rtmsg.changed())
	fprintf(_ofile, "CHANGED flag is set\n");
    if (rtmsg.push())
	fprintf(_ofile, "PUSH flag is set\n");
    fprintf(_ofile, "%s\n", rtmsg.route()->str().c_str());
    fflush(_ofile);

    if (rtmsg.changed()) {
	rtmsg.inactivate();
    }
    return _canned_response;
}

template<class A>
int
DebugTable<A>::replace_route(const InternalMessage<A> &old_rtmsg, 
			     const InternalMessage<A> &new_rtmsg, 
			     BGPRouteTable<A> *caller) {
    assert(caller == _parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", _tablename.c_str()); 
    fprintf(_ofile, "[REPLACE]\n");
    if (old_rtmsg.changed())
	fprintf(_ofile, "CHANGED flag is set\n");
    if (old_rtmsg.push())
	fprintf(_ofile, "PUSH flag is set\n");
    fprintf(_ofile, "%s\n", old_rtmsg.route()->str().c_str());
    if (new_rtmsg.changed())
	fprintf(_ofile, "CHANGED flag is set\n");
    if (new_rtmsg.push())
	fprintf(_ofile, "PUSH flag is set\n");
    fprintf(_ofile, "%s\n", new_rtmsg.route()->str().c_str());
    fflush(_ofile);

    if (new_rtmsg.changed()) {
	new_rtmsg.inactivate();
    }
    if (old_rtmsg.changed()) {
	old_rtmsg.inactivate();
    }
    return _canned_response;
}

template<class A>
int
DebugTable<A>::delete_route(const InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller) {
    assert(caller == _parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", _tablename.c_str()); 
    fprintf(_ofile, "[DELETE]\n");
    if (rtmsg.changed())
	fprintf(_ofile, "CHANGED flag is set\n");
    if (rtmsg.push())
	fprintf(_ofile, "PUSH flag is set\n");
    fprintf(_ofile, "%s\n", rtmsg.route()->str().c_str());
    fflush(_ofile);

    if (rtmsg.changed()) {
	rtmsg.inactivate();
    }
    return 0;
}

template<class A>
int
DebugTable<A>::push(BGPRouteTable<A> *caller) {
    assert(caller == _parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", _tablename.c_str()); 
    fprintf(_ofile, "[PUSH]\n");
    return 0;
}

template<class A>
int
DebugTable<A>::route_dump(const InternalMessage<A> &rtmsg, 
			  BGPRouteTable<A> *caller,
			  const PeerHandler */*peer*/) {
    assert(caller == _parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", _tablename.c_str()); 
    fprintf(_ofile, "[DUMP]\n");
    fprintf(_ofile, "%s\n", rtmsg.route()->str().c_str());
    fflush(_ofile);
    if (rtmsg.changed()) {
	rtmsg.inactivate();
    }
    return 0;
}

template<class A>
const SubnetRoute<A>*
DebugTable<A>::lookup_route(const IPNet<A> &net) const {
    return _parent->lookup_route(net);
}

template<class A>
void
DebugTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use){
    _parent->route_used(rt, in_use);
}

template<class A>
string
DebugTable<A>::str() const {
    string s = "DebugTable<A>" + tablename();
    return s;
}

/* mechanisms to implement flow control in the output plumbing */
template<class A>
void 
DebugTable<A>::output_state(bool busy, BGPRouteTable<A> */*next_table*/) {
    if (busy)
	fprintf(_ofile, "[OUTPUT_STATE: BUSY]\n");
    else
	fprintf(_ofile, "[OUTPUT_STATE: IDLE]\n");
}

template<class A>
bool 
DebugTable<A>::get_next_message(BGPRouteTable<A> */*next_table*/) {
    fprintf(_ofile, "[GET_NEXT_MESSAGE]: ");
    if (_msgs > 0) {
	fprintf(_ofile, " RETURNING TRUE\n");
	_msgs--;
	return true;
    } else {
	fprintf(_ofile, " RETURNING FALSE\n");
	return false ;
    }
}

template<class A>
void
DebugTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				 BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}

template<class A>
void
DebugTable<A>::peering_down_complete(const PeerHandler *peer, 
				     uint32_t genid,
				     BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}



template<class A>
bool
DebugTable<A>::set_output_file(const string& filename) {
    _ofile = fopen(filename.c_str(), "w");
    if (_ofile == NULL) {
	XLOG_FATAL("Failed to open %s for writing %s", filename.c_str(),
		   strerror(errno));
	return false;
    }
    return true;
}

template<class A>
void
DebugTable<A>::set_output_file(FILE *file) {
    _ofile = file;
}

template<class A>
void
DebugTable<A>::write_separator() const {
    fprintf(_ofile, "[separator]-------------------------------------\n");
    fflush(_ofile);
}

template<class A>
void
DebugTable<A>::write_comment(const string& s) const {
    fprintf(_ofile, "[comment] %s\n", s.c_str());
    fflush(_ofile);
}

template class DebugTable<IPv4>;
