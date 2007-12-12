// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_fea_io.hh,v 1.3 2007/10/11 07:12:56 pavlin Exp $


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
