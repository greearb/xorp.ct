// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/bgp/route_table_debug.hh,v 1.3 2003/02/11 22:06:25 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DEBUG_HH__
#define __BGP_ROUTE_TABLE_DEBUG_HH__

#include "route_table_base.hh"
#include "libxorp/trie.hh"

template<class A>
class DebugTable : public BGPRouteTable<A>  {
public:
    DebugTable(string tablename, BGPRouteTable<A> *parent);
    ~DebugTable();
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg, 
		   BGPRouteTable<A> *caller,
		   const PeerHandler *peer);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net) const;
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
    bool set_output_file(const string& filename);
    void set_output_file(FILE *file);
    FILE *output_file() const {return _ofile;}
    void write_separator() const;
    void write_comment(const string& s) const;
    void enable_tablename_printing() {_print_tablename = true;}
private:
    int _canned_response;
    int _msgs;
    FILE *_ofile;
    bool _close_on_delete;
    bool _print_tablename;
};

#endif // __BGP_ROUTE_TABLE_DEBUG_HH__
