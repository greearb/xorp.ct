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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.56 2007/09/25 23:00:28 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__

#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class IfConfig;
class IPvX;


class IfConfigSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     */
    IfConfigSet(FeaDataPlaneManager& fea_data_plane_manager)
	: _is_running(false),
	  _ifconfig(fea_data_plane_manager.ifconfig()),
	  _fea_data_plane_manager(fea_data_plane_manager),
	  _is_interface_flipped(false)
    {}

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigSet() {}

    /**
     * Get the @ref IfConfig instance.
     *
     * @return the @ref IfConfig instance.
     */
    IfConfig&	ifconfig() { return _ifconfig; }

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
     * Push the network interface configuration into the underlying system.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param iftree the interface tree configuration to push.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int push_config(IfTree& iftree);

protected:
    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const = 0;

    /**
     * Determine if the interface's underlying provider implements unreachable
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if unreachable semantics are emulated.
     */
    virtual bool is_unreachable_emulated(const IfTreeInterface& i) const = 0;

    /**
     * Start the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_begin(string& error_msg) = 0;

    /**
     * Complete the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_end(string& error_msg) = 0;

    /**
     * Begin the interface configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface_begin(const IfTreeInterface* pulled_ifp,
				       const IfTreeInterface& config_iface,
				       string& error_msg) = 0;

    /**
     * End the interface configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface_end(const IfTreeInterface* pulled_ifp,
				     const IfTreeInterface& config_iface,
				     string& error_msg) = 0;

    /**
     * Begin the vif configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vif_begin(const IfTreeInterface* pulled_ifp,
				 const IfTreeVif* pulled_vif,
				 const IfTreeInterface& config_iface,
				 const IfTreeVif& config_vif,
				 string& error_msg) = 0;

    /**
     * End the vif configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vif_end(const IfTreeInterface* pulled_ifp,
			       const IfTreeVif* pulled_vif,
			       const IfTreeInterface& config_iface,
			       const IfTreeVif& config_vif,
			       string& error_msg) = 0;

    /**
     * Configure IPv4 address information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_addr(const IfTreeInterface* pulled_ifp,
			    const IfTreeVif* pulled_vif,
			    const IfTreeAddr4* pulled_addr,
			    const IfTreeInterface& config_iface,
			    const IfTreeVif& config_vif,
			    const IfTreeAddr4& config_addr,
			    string& error_msg) = 0;

    /**
     * Configure IPv6 address information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_addr(const IfTreeInterface* pulled_ifp,
			    const IfTreeVif* pulled_vif,
			    const IfTreeAddr6* pulled_addr,
			    const IfTreeInterface& config_iface,
			    const IfTreeVif& config_vif,
			    const IfTreeAddr6& config_addr,
			    string& error_msg) = 0;

protected:
    // Test whether the status of an interface was flipped
    bool is_interface_flipped() const { return (_is_interface_flipped); }
    void set_interface_flipped(bool v) { _is_interface_flipped = v; }

    // Misc other state
    bool	_is_running;

private:
    void push_iftree_begin(IfTree& iftree);
    void push_iftree_end(IfTree& iftree);
    void push_interface_begin(IfTreeInterface& i);
    void push_interface_end(IfTreeInterface& i);
    void push_vif_begin(IfTreeInterface& i, IfTreeVif& v);
    void push_vif_end(IfTreeInterface& i, IfTreeVif& v);
    void push_vif_address(IfTreeInterface& i, IfTreeVif& v, IfTreeAddr4& a);
    void push_vif_address(IfTreeInterface& i, IfTreeVif& v, IfTreeAddr6& a);

    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;

    bool _is_interface_flipped;		// If true, an interface was flipped
};

#endif // __FEA_IFCONFIG_SET_HH__
