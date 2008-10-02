// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/mfea_vif.hh,v 1.12 2008/07/23 05:10:11 pavlin Exp $


#ifndef __FEA_MFEA_VIF_HH__
#define __FEA_MFEA_VIF_HH__


//
// MFEA virtual interface definition.
//


#include <list>
#include <set>
#include <vector>
#include <utility>

#include "libxorp/vif.hh"
#include "libproto/proto_unit.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class MfeaNode;

/**
 * @short A class for MFEA-specific virtual interface.
 */
class MfeaVif : public ProtoUnit, public Vif {
public:
    /**
     * Constructor for a given MFEA node and a generic virtual interface.
     * 
     * @param mfea_node the @ref MfeaNode this interface belongs to.
     * @param vif the generic Vif interface that contains various information.
     */
    MfeaVif(MfeaNode& mfea_node, const Vif& vif);
    
    /**
     * Copy Constructor for a given MFEA node and MFEA-specific virtual
     * interface.
     * 
     * @param mfea_node the @ref MfeaNode this interface belongs to.
     * @param mfea_vif the origin @ref MfeaVif interface that contains
     * the initialization information.
     */
    MfeaVif(MfeaNode& mfea_node, const MfeaVif& mfea_vif);
    
    /**
     * Destructor
     */
    virtual ~MfeaVif();
    
    /**
     * Start MFEA on a single virtual interface.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start(string& error_msg);
    
    /**
     * Stop MFEA on a single virtual interface.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop(string& error_msg);

    /**
     * Enable MFEA on a single virtual interface.
     * 
     * If an unit is not enabled, it cannot be start, or pending-start.
     */
    void	enable();
    
    /**
     * Disable MFEA on a single virtual interface.
     * 
     * If an unit is disabled, it cannot be start or pending-start.
     * If the unit was runnning, it will be stop first.
     */
    void	disable();

    /**
     * Register a protocol on a single virtual interface.
     *
     * There could be only one registered protocol per interface/vif.
     *
     * @param module_instance_name the module instance name of the protocol
     * to register.
     * @param ip_protocol the IP protocol number. It must be between 1 and
     * 255.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_protocol(const string&	module_instance_name,
			  uint8_t	ip_protocol,
			  string&	error_msg);

    /**
     * Unregister a protocol on a single virtual interface.
     *
     * @param module_instance_name the module instance name of the protocol
     * to unregister.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_protocol(const string&	module_instance_name,
			    string&		error_msg);

    /**
     * Get the minimum TTL a multicast packet must have to be forwarded
     * on this virtual interface.
     * 
     * @return the minimum TTL a multicast packet must have
     * to be forwarded on this virtual interface.
     */
    uint8_t	min_ttl_threshold() const { return (_min_ttl_threshold); }
    
    /**
     * Set the minimum TTL a multicast packet must have to be forwarded
     * on this virtual interface.
     * 
     * @param v the value of the minimum TTL a multicast packet must have
     * to be forwarded on this virtual interface.
     */
    void	set_min_ttl_threshold(uint8_t v) { _min_ttl_threshold = v; }
    
    /**
     * Get the maximum multicast bandwidth rate allowed on this virtual
     * interface.
     * 
     * @return the maximum multicast bandwidth rate allowed
     *  on this virtual interface.
     */
    uint32_t	max_rate_limit() const { return (_max_rate_limit); }
    
    /**
     * Set the maximum multicast bandwidth rate allowed on this virtual
     * interface.
     * 
     * @param v the value of the maximum multicast bandwidth rate allowed
     * on this virtual interface.
     */
    void	set_max_rate_limit(uint32_t v) { _max_rate_limit = v; }

    /**
     * Get the registered module instance name.
     *
     * @return the registered module instance name.
     */
    const string& registered_module_instance_name() const {
	return (_registered_module_instance_name);
    }

    /**
     * Get the registered IP protocol.
     *
     * @return the registered IP protocol.
     */
    uint8_t registered_ip_protocol() const {
	return (_registered_ip_protocol);
    }

private:
    // Private functions
    MfeaNode&	mfea_node() const	{ return (_mfea_node);		}

    /**
     * Get the string with the flags about the vif status.
     * 
     * TODO: temporary here. Should go to the Vif class after the Vif
     * class starts using the Proto class.
     * 
     * @return the C++ style string with the flags about the vif status
     * (e.g., UP/DOWN/DISABLED, etc).
     */
    string	flags_string() const;
    
    // Private state
    MfeaNode&	_mfea_node;		// The MFEA node I belong to
    uint8_t	_min_ttl_threshold; // Min. TTL required to forward mcast pkts
    uint32_t	_max_rate_limit;    // Max. bw rate allowed to forward mcast

    //
    // State to keep information about the registered multicast routing
    // protocol for this vif.
    // There can be only one protocol instance registered which will be
    // responsible for the multicast routing on the vif.
    //
    string	_registered_module_instance_name;
    uint8_t	_registered_ip_protocol;
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __FEA_MFEA_VIF_HH__
