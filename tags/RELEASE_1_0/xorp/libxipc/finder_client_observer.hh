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

// $XORP: xorp/libxipc/finder_client_observer.hh,v 1.1 2003/06/19 19:20:07 hodson Exp $

#ifndef __LIBXIPC_FINDER_CLIENT_OBSERVER_HH__
#define __LIBXIPC_FINDER_CLIENT_OBSERVER_HH__

/**
 * Base class for parties interested in receiving FinderClient event
 * notifications.
 */
class FinderClientObserver
{
public:
    virtual ~FinderClientObserver();

    /**
     * Finder connection established.
     *
     * Called by FinderClient when a connection to the Finder is
     * established.
     */
    virtual void finder_connect_event() = 0;

    /**
     * Finder connection terminated.
     *
     * Called by FinderClient when the connection to the Finder is lost.
     */
    virtual void finder_disconnect_event() = 0;

    /**
     * Finder registration of named target is complete and target is
     * able to send and receive Xrls requests.
     *
     * Called by FinderClient after Xrls have been registered and the
     * the target is enabled with the finder.
     *
     * @param target_name the name of the Xrl target transitioning to
     * ready state.
     */
    virtual void finder_ready_event(const string& target_name) = 0;
};

#endif // __LIBXIPC_FINDER_CLIENT_OBSERVER_HH__
