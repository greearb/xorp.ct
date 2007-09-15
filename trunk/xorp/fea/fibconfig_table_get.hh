// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/fibconfig_table_get.hh,v 1.11 2007/07/16 23:54:05 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TABLE_GET_HH__
#define __FEA_FIBCONFIG_TABLE_GET_HH__

#include <list>

#include "fte.hh"
#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class FibConfig;


class FibConfigTableGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     */
    FibConfigTableGet(FeaDataPlaneManager& fea_data_plane_manager)
	: _is_running(false),
	  _fibconfig(fea_data_plane_manager.fibconfig()),
	  _fea_data_plane_manager(fea_data_plane_manager)
    {}

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigTableGet() {}

    /**
     * Get the @ref FibConfig instance.
     *
     * @return the @ref FibConfig instance.
     */
    FibConfig&	fibconfig() { return _fibconfig; }

    /**
     * Get the @ref FeaDataPlaneManager instance.
     *
     * @return the @ref FeaDataPlaneManager instance.
     */
    FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

    /**
     * Test whether this instance is running.
     *
     * @return true if the instance is running, otherwise false.
     */
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;
    
    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table4(list<Fte4>& fte_list) = 0;

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table6(list<Fte6>& fte_list) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    FibConfig&		_fibconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_FIBCONFIG_TABLE_GET_HH__
