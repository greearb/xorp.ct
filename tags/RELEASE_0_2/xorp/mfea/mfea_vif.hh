// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/mfea/mfea_vif.hh,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $


#ifndef __MFEA_MFEA_VIF_HH__
#define __MFEA_MFEA_VIF_HH__


//
// MFEA virtual interface definition.
//


#include <list>
#include <set>
#include <vector>
#include <utility>

#include "libxorp/vif.hh"
#include "libproto/proto_register.hh"
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop MFEA on a single virtual interface.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Start a protocol on a single virtual interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to start on this vif.
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * to start on this vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_protocol(const string& module_instance_name,
			       x_module_id module_id);

    /**
     * Stop a protocol on a single virtual interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to stop on this vif.
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * to stop on this vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_protocol(const string& module_instance_name,
			      x_module_id module_id);
    
    /**
     * Leave all previously joined multicast groups on this interface.
     * 
     * @return the number of groups that were successfully left,
     * or XORP_ERROR if error.
     */
    int		leave_all_multicast_groups();
    
    /**
     * Leave all previously joined multicast groups on this interface
     * by a given module.
     * 
     * Leave all previously joined multicast groups on this interface
     * that were joined by a module with name @ref module_instance_name
     * and module ID @ref module_id.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that leaves all groups.
     * @param module_id the module ID of the protocol that leaves all groups
     * @return the number of groups that were successfully left,
     * or XORP_ERROR if error.
     */
    int		leave_all_multicast_groups(const string& module_instance_name,
					   x_module_id module_id);
    
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
     * Test if a protocol is registeres with this virtual interface.
     * 
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * to test.
     * @return true if protocol with module ID of @ref module_id is
     * registered with this virtual interface, otherwise false.
     */
    bool	proto_is_registered(x_module_id module_id) const {
	return (_proto_register.is_registered(module_id));
    }
    
    /**
     * Get the list of protocol instance names for a given protocol.
     * 
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * to get the list with instance names.
     * @return the list of protocol instance names for protocol
     * with module ID of @ref module_id.
     */
    const list<string>& proto_module_instance_name_list(x_module_id module_id) const {
	return (_proto_register.module_instance_name_list(module_id));
    }
    
    /**
     * Add a multicast group to the set of groups joined on this virtual
     * interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that has joined the group.
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * that has joined the group.
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	add_multicast_group(const string& module_instance_name,
			    x_module_id module_id,
			    const IPvX& group) {
	return (_joined_groups.add_multicast_group(module_instance_name,
						   module_id,
						   group));
    }

    /**
     * Delete a multicast group from the set of groups joined on this virtual
     * interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that has left the group.
     * @param module_id the module ID (@ref x_module_id) of the protocol
     * that has left the group.
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	delete_multicast_group(const string& module_instance_name,
			       x_module_id module_id,
			       const IPvX& group) {
	return (_joined_groups.delete_multicast_group(module_instance_name,
						      module_id,
						      group));
    }
    
    /**
     * Test if a multicast group is still joined on this virtual interface.
     * 
     * @param group the group address.
     * @return true if @ref group is still joined on this virtual address,
     * otherwise false.
     */
    bool	has_multicast_group(const IPvX& group) const {
	return (_joined_groups.has_multicast_group(group));
    }
    
private:
    // Private functions
    MfeaNode&	mfea_node() const	{ return (_mfea_node);		}
    
    // Private state
    MfeaNode&	_mfea_node;		// The MFEA node I belong to
    uint8_t	_min_ttl_threshold; // Min. TTL required to forward mcast pkts
    uint32_t	_max_rate_limit;    // Max. bw rate allowed to forward mcast
    
    // State to keep track of which protocols have started on this vif.
    ProtoRegister _proto_register;
    
    // Class to keep track of joined groups per vif
    class JoinedGroups {
    public:
	int	add_multicast_group(const string& module_instance_name,
				    x_module_id module_id,
				    const IPvX& group);
	int	delete_multicast_group(const string& module_instance_name,
				       x_module_id module_id,
				       const IPvX& group);
	bool	has_multicast_group(const IPvX& group) const;
	set<IPvX>& joined_multicast_groups() {
	    return (_joined_multicast_groups);
	}
	list<pair<pair<string, x_module_id>, IPvX> >& joined_state() {
	    return (_joined_state);
	}
	
    private:
	set<IPvX>	_joined_multicast_groups; // The joined mcast groups
	// The state about who joined which group.
	list<pair<pair<string, x_module_id>, IPvX> > _joined_state;
    };
    
    JoinedGroups _joined_groups;	// State with joined multicast groups
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MFEA_MFEA_VIF_HH__
