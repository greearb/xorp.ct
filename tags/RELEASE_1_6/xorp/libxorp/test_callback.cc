// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libxorp/test_callback.cc,v 1.12 2008/10/02 21:57:33 bms Exp $"

//
// Callback test program
//

#include <stdlib.h>
#include <stdio.h>

#include "xorp.h"
#include "callback.hh"

typedef XorpCallback1<void, int>::RefPtr TestCallback;

class Widget {
public:
    Widget() {}
    ~Widget() {}
    void notify(int event) { printf("Widget event %d\n", event); }

    void funky_notify(int event, TestCallback cb) {
	printf("Funky, ");
	cb->dispatch(event);
    }
private:
};

class SafeWidget : public CallbackSafeObject {
public:
    SafeWidget(int* counter) : _counter(counter) {}
    void notify(int event) {
	printf("SafeWidget event %d\n", event);
	*_counter = *_counter + 1;
    }
private:
    int* _counter;
};

int
main()
{
    Widget w;

    TestCallback cbm;

    // The callback should be empty
    if (!cbm.is_empty()) {
	printf("ERROR: callback is not empty\n");
	return -1;
    }

    // Set the callback
    cbm = callback(&w, &Widget::notify);

    // Test that the callback is not empty
    if (cbm.is_empty()) {
	printf("ERROR: callback is empty\n");
	return -1;
    }

    // Call the callback
    cbm->dispatch(3);	// call w.notify(3);

    cbm = callback(&w, &Widget::funky_notify,
		   callback(&w, &Widget::funky_notify,
			    callback(&w, &Widget::notify)
			    )
		   );
    cbm->dispatch(4);

    cbm = callback(&w, &Widget::notify);
    for (int i = 0; i < 8; i++) {
	cbm = callback(&w, &Widget::funky_notify, cbm);
    }
    cbm->dispatch(5);

    // Show safe callback - only dispatches if object referred to in
    // callback() method exists.  ie object deletion prevents further
    // dispatches upon object
    int counter = 0;
    const int stop_point = 5;
    SafeWidget* sw = new SafeWidget(&counter);
    cbm = callback(sw, &SafeWidget::notify);
    for (int i = 0; i < 10; ++i) {
	if (i == stop_point)
	    delete sw;
	cbm->dispatch(i);
    }

    if (counter != stop_point) {
	printf("ERROR: safe callback executed after object deletion\n");
	return -1;
    }

    // Test destructor of callback correctly interacts with
    // CallbackSafeObject correctly.
    {
	int counter2 = 0;
	SafeWidget* sw = new SafeWidget(&counter2);
	{
	    TestCallback cbset[10];
	    for (int i = 0; i < 10; i++) {
		cbset[i] = callback(sw, &SafeWidget::notify);
	    }
	    cbset[0] = 0;
	}
	delete sw;
    }

    return (0);
}
