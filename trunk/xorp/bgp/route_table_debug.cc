// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/route_table_debug.cc,v 1.17 2008/07/23 05:09:36 pavlin Exp $"

//#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_debug.hh"

template<class A>
DebugTable<A>::DebugTable(string table_name,  
			  BGPRouteTable<A> *parent_table) 
    : BGPRouteTable<A>("DebugTable-" + table_name, SAFI_UNICAST)
{
    this->_parent = parent_table;
    _print_tablename = false;
    _msgs = 0;
    _ofile = NULL;
    _get_on_wakeup = false;
}

template<class A>
DebugTable<A>::~DebugTable() {
    if (_ofile != NULL && _close_on_delete)
	fclose(_ofile);
}

template<class A>
int
DebugTable<A>::add_route(const InternalMessage<A> &rtmsg, 
			 BGPRouteTable<A> *caller) {
    assert(caller == this->_parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", this->tablename().c_str()); 
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
    assert(caller == this->_parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", this->tablename().c_str()); 
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
    assert(caller == this->_parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", this->tablename().c_str()); 
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
    assert(caller == this->_parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", this->tablename().c_str()); 
    fprintf(_ofile, "[PUSH]\n");
    return 0;
}

template<class A>
int
DebugTable<A>::route_dump(const InternalMessage<A> &rtmsg, 
			  BGPRouteTable<A> *caller,
			  const PeerHandler */*peer*/) {
    assert(caller == this->_parent);
    if (_print_tablename)
	fprintf(_ofile, "[** TABLE %s **]\n", this->tablename().c_str()); 
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
DebugTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid) const {
    return this->_parent->lookup_route(net, genid);
}

template<class A>
void
DebugTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use){
    this->_parent->route_used(rt, in_use);
}

template<class A>
void
DebugTable<A>::wakeup() {
    if (_get_on_wakeup)
	while (this->_parent->get_next_message(this)) {}
}

template<class A>
string
DebugTable<A>::str() const {
    string s = "DebugTable<A>" + this->tablename();
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
    XLOG_ASSERT(this->_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}

template<class A>
void
DebugTable<A>::peering_down_complete(const PeerHandler *peer, 
				     uint32_t genid,
				     BGPRouteTable<A> *caller) {
    XLOG_ASSERT(this->_parent == caller);
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
    _close_on_delete = true;
    return true;
}

template<class A>
void
DebugTable<A>::set_output_file(FILE *file) {
    _close_on_delete = false;
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
