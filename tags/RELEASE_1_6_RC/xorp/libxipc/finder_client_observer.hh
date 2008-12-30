// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/finder_client_observer.hh,v 1.7 2008/07/23 05:10:41 pavlin Exp $

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
