// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/fea/fibconfig_table_get.hh,v 1.17 2008/10/02 21:56:46 bms Exp $

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
     * (@ref FeaDataPlaneManager).
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
     * Obtain the IPv4 unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the IPv4 unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table4(list<Fte4>& fte_list) = 0;

    /**
     * Obtain the IPv6 unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the IPv6 unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table6(list<Fte6>& fte_list) = 0;

    /** Routing table ID that we are interested in might have changed.  Maybe something
     * can filter on this for increased efficiency.
     */
    virtual int notify_table_id_change(uint32_t new_tbl) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    FibConfig&		_fibconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_FIBCONFIG_TABLE_GET_HH__
