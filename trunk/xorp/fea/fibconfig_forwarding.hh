// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/fibconfig_forwarding.hh,v 1.3 2008/01/03 22:59:33 pavlin Exp $

#ifndef __FEA_FIBCONFIG_FORWARDING_HH__
#define __FEA_FIBCONFIG_FORWARDING_HH__

#include "fea_data_plane_manager.hh"

class FibConfig;


class FibConfigForwarding {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigForwarding(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigForwarding();

    /**
     * Get the @ref FibConfig instance.
     *
     * @return the @ref FibConfig instance.
     */
    FibConfig& fibconfig() { return _fibconfig; }

    /**
     * Get the @ref FeaDataPlaneManager instance.
     *
     * @return the @ref FeaDataPlaneManager instance.
     */
    FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

    /**
     * Get the const @ref FeaDataPlaneManager instance.
     *
     * @return the const @ref FeaDataPlaneManager instance.
     */
    const FeaDataPlaneManager& fea_data_plane_manager() const { return _fea_data_plane_manager; }

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
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Test whether the IPv4 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv4 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int unicast_forwarding_enabled4(bool& ret_value, string& error_msg) const = 0;

    /**
     * Test whether the IPv6 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv6 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int unicast_forwarding_enabled6(bool& ret_value, string& error_msg) const = 0;

    /**
     * Test whether the acceptance of IPv6 Router Advertisement messages is
     * enabled or disabled.
     * 
     * @param ret_value if true on return, then the acceptance of IPv6 Router
     * Advertisement messages is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int accept_rtadv_enabled6(bool& ret_value, string& error_msg) const = 0;

    /**
     * Set the IPv4 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv4 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_unicast_forwarding_enabled4(bool v, string& error_msg) = 0;

    /**
     * Set the IPv6 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv6 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_unicast_forwarding_enabled6(bool v, string& error_msg) = 0;

    /**
     * Enable or disable the acceptance of IPv6 Router Advertisement messages
     * from other routers. It should be enabled for hosts, and disabled for
     * routers.
     * 
     * @param v if true, then enable the acceptance of IPv6 Router
     * Advertisement messages, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_accept_rtadv_enabled6(bool v, string& error_msg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    FibConfig&		_fibconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;

    //
    // Original state from the underlying system before the plugin was started
    //
    bool	_orig_unicast_forwarding_enabled4;
    bool	_orig_unicast_forwarding_enabled6;
    bool	_orig_accept_rtadv_enabled6;

    bool	_first_start;	// True if started for first time
};

#endif // __FEA_FIBCONFIG_FORWARDING_HH__
