// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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



#ifndef __FEA_XRL_FEA_IO_HH__
#define __FEA_XRL_FEA_IO_HH__


//
// FEA (Forwarding Engine Abstraction) XRL-based I/O implementation.
//

#include "fea_io.hh"

class EventLoop;
class XrlFeaNode;
class XrlRouter;

/**
 * @short FEA (Forwarding Engine Abstraction) XRL-based I/O class.
 */
class XrlFeaIo : public FeaIo {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     * @param xrl_router the XRL transmission and reception point.
     * @param xrl_finder_targetname the XRL targetname of the Finder.
     */
    XrlFeaIo(EventLoop& eventloop, XrlRouter& xrl_router,
	     const string& xrl_finder_targetname);

    /**
     * Destructor
     */
    virtual	~XrlFeaIo();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Register interest in events relating to a particular instance.
     *
     * @param instance_name name of target instance to receive event
     * notifications for.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_instance_event_interest(const string& instance_name,
					 string& error_msg);

    /**
     * Deregister interest in events relating to a particular instance.
     *
     * @param instance_name name of target instance to stop event
     * notifications for.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int deregister_instance_event_interest(const string& instance_name,
					   string& error_msg);

private:
    void register_instance_event_interest_cb(const XrlError& xrl_error,
					     string instance_name);
    void deregister_instance_event_interest_cb(const XrlError& xrl_error,
					       string instance_name);

    XrlRouter&		_xrl_router;	// The standard XRL send/recv point
    const string	_xrl_finder_targetname;	// The Finder target name
};

#endif // __FEA_XRL_FEA_IO_HH__
