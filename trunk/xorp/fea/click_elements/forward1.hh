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

// $XORP: xorp/fea/click_elements/forward1.hh,v 1.3 2002/12/09 18:29:01 hodson Exp $

#ifndef __FEA_CLICK_ELEMENTS_FORWARD1_HH__
#define __FEA_CLICK_ELEMENTS_FORWARD1_HH__

// Copyright (c) 2001,2002 International Computer Science Institute
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
//
// $Id$

#include <click/element.hh>
#include "rtable1.hh"

class Forward1 : public Element {
public:
    Forward1();
    ~Forward1();

    const char *class_name() const	{ return "Forward1"; }
    const char *processing() const	{ return AGNOSTIC; }
    Forward1 *clone() const	{ return new Forward1; }

    int configure(const Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    
    void push(int port, Packet *p);
    
private:
    Rtable1 _rt;
};

#endif // __FEA_CLICK_ELEMENTS_FORWARD1_HH__
