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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_OUTPUT_UPDATES_HH__
#define __RIP_OUTPUT_UPDATES_HH__

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "output.hh"
#include "route_db.hh"
#include "update_queue.hh"

/**
 * @short Triggered Updates Output class.
 *
 * The OutputUpdate class produces an asynchronous sequence of
 * triggered update packets.
 *
 * Specialized implementations exist for IPv4 and IPv6.
 */
template <typename A>
class OutputUpdates : public OutputBase<A>
{
public:
    OutputUpdates(EventLoop&		e,
		  Port<A>&		port,
		  PacketQueue<A>&	pkt_queue,
		  RouteDB<A>&		rdb);
    ~OutputUpdates();

    /**
     * Fast forward iterator doing triggered up reading.
     *
     * Triggered updates do not run during periodic route table dumps.  This
     * method should be called immediately before halting for periodic update
     * as it will effectively stop the output of updates that are already
     * covered in table dump.
     */
    void ffwd();

protected:
    void output_packet();

private:
    OutputUpdates(const OutputUpdates<A>& o);		    // Not implemented
    OutputUpdates<A>& operator=(const OutputUpdates<A>& o); // Not implemented

private:
    UpdateQueue<A>&		 	  _uq;
    typename UpdateQueue<A>::ReadIterator _uq_iter;
};

#endif // __RIP_OUTPUT_UPDATES_HH__
