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

// $XORP$


#ifndef __FEA_FEA_IO_HH__
#define __FEA_FEA_IO_HH__


//
// FEA (Forwarding Engine Abstraction) I/O interface.
//

class EventLoop;

/**
 * @short FEA (Forwarding Engine Abstraction) I/O class.
 */
class FeaIO {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     */
    FeaIO(EventLoop& eventloop);

    /**
     * Destructor
     */
    virtual	~FeaIO();

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
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop&	eventloop() { return (_eventloop); }

private:
    EventLoop&	_eventloop;	// The event loop to use
    bool	_is_running;	// True if the service is running
};

#endif // __FEA_FEA_IO_HH__
