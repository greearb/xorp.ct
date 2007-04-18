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


#ifndef __FEA_XRL_FEA_IO_HH__
#define __FEA_XRL_FEA_IO_HH__


//
// FEA (Forwarding Engine Abstraction) XRL-based I/O implementation.
//

#include "fea_io.hh"

class EventLoop;

/**
 * @short FEA (Forwarding Engine Abstraction) XRL-based I/O class.
 */
class XrlFeaIO : public FeaIO {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     */
    XrlFeaIO(EventLoop& eventloop);

    /**
     * Destructor
     */
    virtual	~XrlFeaIO();

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

private:
};

#endif // __FEA_XRL_FEA_IO_HH__
