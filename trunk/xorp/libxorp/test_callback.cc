// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_callback.cc,v 1.3 2003/12/20 00:26:58 hodson Exp $"

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
    int counter2 = 0;
    sw = new SafeWidget(&counter2);
    {
	cbm = callback(sw, &SafeWidget::notify);
	cbm->dispatch(0);
	TestCallback cbm2 = cbm;
    }
    delete sw;

    return (0);
}
