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

// $XORP: xorp/fea/click_elements/forward2.hh,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $

#ifndef __FEA_CLICK_ELEMENTS_FORWARD2_HH__
#define __FEA_CLICK_ELEMENTS_FORWARD2_HH__

#include <click/element.hh>

#include <fea/fti.hh>
#include "rtable2.hh"

class Forward2 : public Element {
public:
    Forward2();
    ~Forward2();

    const char *class_name() const	{ return "Forward2"; }
    const char *processing() const	{ return AGNOSTIC; }
    Forward2 *clone() const	{ return new Forward2; }

    int configure(const Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);

    void add_handlers();

    static int write_handler_start_transaction(const String &, Element *, 
					       void *, ErrorHandler *);
    static String read_handler_start_transaction(Element *, void *thunk);

    static int write_handler_commit_transaction(const String &, Element *, 
					       void *, ErrorHandler *);

    static int write_handler_abort_transaction(const String &, Element *, 
					       void *, ErrorHandler *);

    static int write_handler_break_transaction(const String &, Element *, 
					       void *, ErrorHandler *);
    static int write_handler_delete_all_entries(const String &, Element *, 
					       void *, ErrorHandler *);
    static int write_handler_delete_entry(const String &, Element *, 
					       void *, ErrorHandler *);

    static int write_handler_add_entry(const String &, Element *, 
					       void *, ErrorHandler *);

    static String read_handler_table(Element *e, void *thunk);

    static int write_handler_lookup_route(const String &, Element *, void *, 
			     ErrorHandler *);
    static String read_handler_lookup_route(Element *e, void *thunk);
    
    static int write_handler_lookup_entry(const String &, Element *,
						void *, ErrorHandler *);
    static String read_handler_lookup_entry(Element *e, void *thunk);

    void push(int port, Packet *p);
    
private:
    Rtable2 _route[2];
    Rtable2 *_rt_current;
    Rtable2 *_rt_shadow;

    bool _intransaction;	/* Are we in a transaction */
    int _transactionID;

    void route_swap() {
	static int route_index = 0;

	_rt_current = &_route[route_index];
	route_index = 0 == route_index ? 1 : 0;
	_rt_shadow = &_route[route_index];
    };

    void route_sync() {
	_rt_shadow->copy(_rt_current);
    };

    String lookup_result;
};

#endif // __FEA_CLICK_ELEMENTS_FORWARD2_HH__
