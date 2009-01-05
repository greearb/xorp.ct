// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/route_table_debug.hh,v 1.16 2008/11/08 06:14:38 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DEBUG_HH__
#define __BGP_ROUTE_TABLE_DEBUG_HH__

#include "route_table_base.hh"
#include "libxorp/ref_trie.hh"

template<class A>
class DebugTable : public BGPRouteTable<A>  {
public:
    DebugTable(string tablename, BGPRouteTable<A> *parent);
    ~DebugTable();
    int add_route(InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
		      InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    int route_dump(InternalMessage<A> &rtmsg, 
		   BGPRouteTable<A> *caller,
		   const PeerHandler *peer);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;
    void wakeup();
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return DEBUG_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);

    void set_canned_response(int response) {
	_canned_response = response;
    }
    void set_next_messages(int msgs) {
	_msgs = msgs;
    }
    void set_get_on_wakeup(bool get_on_wakeup) {
	_get_on_wakeup = get_on_wakeup;
    }
    bool set_output_file(const string& filename);
    void set_output_file(FILE *file);
    FILE *output_file() const {return _ofile;}
    void write_separator() const;
    void write_comment(const string& s) const;
    void enable_tablename_printing() {_print_tablename = true;}
    void set_is_winner(bool value) {_set_is_winner = value;}
private:
    void print_route(const SubnetRoute<A>& route, FPAListRef palist) const;
    int _canned_response;
    int _msgs;
    FILE *_ofile;
    bool _close_on_delete;
    bool _print_tablename;
    bool _get_on_wakeup;
    bool _set_is_winner;
};

#endif // __BGP_ROUTE_TABLE_DEBUG_HH__
