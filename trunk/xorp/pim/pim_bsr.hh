// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/pim/pim_bsr.hh,v 1.4 2003/02/27 03:11:05 pavlin Exp $


#ifndef __PIM_PIM_BSR_HH__
#define __PIM_PIM_BSR_HH__


//
// PIM Bootstrap Router (BSR) mechanism definitions
//


#include <list>

#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"
#include "libproto/proto_unit.hh"
#include "mrt/timer.hh"
#include "pim_scope_zone_table.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class PimNode;
class PimVif;
class BsrGroupPrefix;
class BsrZone;
class BsrRp;

// The PIM BSR class
class PimBsr : public ProtoUnit {
public:
    PimBsr(PimNode& pim_node);
    virtual ~PimBsr();
    
    int		start();
    int		stop();
    PimNode&	pim_node()		{ return (_pim_node); }
    
    int		unicast_pim_bootstrap(PimVif *pim_vif,
				      const IPvX& nbr_addr) const;
    
    list<BsrZone *>& config_bsr_zone_list() { return (_config_bsr_zone_list); }
    list<BsrZone *>& active_bsr_zone_list() { return (_active_bsr_zone_list); }
    list<BsrZone *>& expire_bsr_zone_list() { return (_expire_bsr_zone_list); }
    
    BsrZone	*add_config_bsr_zone(const BsrZone& bsr_zone,
				     string& error_msg);
    BsrZone	*add_active_bsr_zone(const BsrZone& bsr_zone,
				       string& error_msg);
    BsrZone	*add_expire_bsr_zone(const BsrZone& bsr_zone);
    void	delete_config_bsr_zone(BsrZone *old_bsr_zone);
    void	delete_active_bsr_zone(BsrZone *old_bsr_zone);
    void	delete_expire_bsr_zone(BsrZone *old_bsr_zone);
    void	delete_all_expire_bsr_zone_by_zone_id(const PimScopeZoneId& zone_id);
    void	delete_expire_bsr_zone_prefix(const IPvXNet& group_prefix,
					      bool is_scope_zone);
    
    BsrZone	*find_config_bsr_zone(const PimScopeZoneId& zone_id) const;    
    BsrZone	*find_active_bsr_zone(const PimScopeZoneId& zone_id) const;
    BsrZone	*find_config_bsr_zone_by_prefix(const IPvXNet& group_prefix,
						bool is_scope_zone) const;
    BsrZone	*find_active_bsr_zone_by_prefix(const IPvXNet& group_prefix,
						bool is_scope_zone) const;
    BsrRp	*find_rp(const IPvXNet& group_prefix, bool is_scope_zone,
			 const IPvX& rp_addr) const;
    void	add_rps_to_rp_table();
    void	schedule_rp_table_apply_rp_changes();
    void	clean_expire_bsr_zones();
    void	schedule_clean_expire_bsr_zones();
    
    //
    // Test-related methods
    //
    BsrZone	*add_test_bsr_zone(const PimScopeZoneId& zone_id,
				   const IPvX& bsr_addr,
				   uint8_t bsr_priority,
				   uint8_t hash_masklen,
				   uint16_t fragment_tag);
    BsrZone	*find_test_bsr_zone(const PimScopeZoneId& zone_id) const;
    BsrGroupPrefix *add_test_bsr_group_prefix(const PimScopeZoneId& zone_id,
					      const IPvXNet& group_prefix,
					      bool is_scope_zone,
					      uint8_t expected_rp_count);
    BsrRp	*add_test_bsr_rp(const PimScopeZoneId& zone_id,
				 const IPvXNet& group_prefix,
				 const IPvX& rp_addr,
				 uint8_t rp_priority,
				 uint16_t rp_holdtime);
    int		send_test_bootstrap(const string& vif_name);
    int		send_test_bootstrap_by_dest(const string& vif_name,
					    const IPvX& dest_addr);
    int		send_test_cand_rp_adv();
    
private:
    BsrZone	*find_bsr_zone_by_prefix_from_list(
	const list<BsrZone *>& zone_list, const IPvXNet& group_prefix,
	bool is_scope_zone) const;
    
    bool	can_add_config_bsr_zone(const BsrZone& bsr_zone,
					string& error_msg) const;
    bool	can_add_active_bsr_zone(const BsrZone& bsr_zone,
					string& error_msg) const;
    PimNode&	_pim_node;			// The associated PIM node
    list<BsrZone *> _config_bsr_zone_list;	// List of configured Cand-BSR
						// zones and/or Cand-RP info
    list<BsrZone *> _active_bsr_zone_list;	// List of active BSR zones
    list<BsrZone *> _expire_bsr_zone_list;	// List of expiring BSR zones
    list<BsrZone *> _test_bsr_zone_list;	// List of test BSR zones
    Timer	_rp_table_apply_rp_changes_timer; // Timer to apply RP changes
						  // to the RpTable
    Timer	_clean_expire_bsr_zones_timer;	// Timer to cleanup expiring
						// BSR zones
};

class BsrZone {
public:
    BsrZone(PimBsr& pim_bsr, const BsrZone& bsr_zone);
    BsrZone(PimBsr& pim_bsr, const PimScopeZoneId& zone_id);
    BsrZone(PimBsr& pim_bsr, const IPvX& bsr_addr, uint8_t bsr_priority,
	    uint8_t hash_masklen, uint16_t fragment_tag);
    ~BsrZone();
    
    PimBsr&	pim_bsr()		{ return (_pim_bsr);		}
    
    //
    // BsrZone type
    //
    bool	is_config_bsr_zone() const { return (_is_config_bsr_zone); }
    bool	is_active_bsr_zone() const { return (_is_active_bsr_zone); }
    bool	is_expire_bsr_zone() const { return (_is_expire_bsr_zone); }
    bool	is_test_bsr_zone() const { return (_is_test_bsr_zone); }
    void	set_config_bsr_zone(bool v);
    void	set_active_bsr_zone(bool v);
    void	set_expire_bsr_zone(bool v);
    void	set_test_bsr_zone(bool v);
    
    const IPvX&	bsr_addr() const	{ return (_bsr_addr);		}
    uint8_t	bsr_priority() const	{ return (_bsr_priority);	}
    uint8_t	hash_masklen() const	{ return (_hash_masklen);	}
    uint16_t	fragment_tag() const	{ return (_fragment_tag);	}
    uint16_t	new_fragment_tag()	{ return (++_fragment_tag);	}
    bool	is_accepted_message() const { return (_is_accepted_message); }
    void	set_is_accepted_message(bool v) { _is_accepted_message = v; }
    bool	is_unicast_message() const { return (_is_unicast_message); }
    const IPvX&	unicast_message_src() const { return (_unicast_message_src); }
    void	set_is_unicast_message(bool v, const IPvX& src) {
	_is_unicast_message = v;
	_unicast_message_src = src;
    }
    bool	is_cancel() const	{ return (_is_cancel);		}
    void	set_is_cancel(bool v) 	{ _is_cancel = v;		}
    const PimScopeZoneId& zone_id() const { return (_zone_id);		}
    void	set_zone_id(const PimScopeZoneId& zone_id) { _zone_id = zone_id; }
    
    // The states used per scope zone
    enum bsr_zone_state_t {
	STATE_INIT,			// The state after initialization
	// States if I am a Candidate BSR
	STATE_CANDIDATE_BSR,
	STATE_PENDING_BSR,
	STATE_ELECTED_BSR,
	// States if I am not a Candidate BSR
	STATE_NO_INFO,
	STATE_ACCEPT_ANY,
	STATE_ACCEPT_PREFERRED
    };
    bsr_zone_state_t bsr_zone_state() const	{ return (_bsr_zone_state); }
    void set_bsr_zone_state(bsr_zone_state_t v) { _bsr_zone_state = v;	}
    
    bool	is_consistent(string& error_msg) const;
    bool	can_merge_rp_set(const BsrZone& bsr_zone,
				 string& error_msg) const;
    void	merge_rp_set(const BsrZone& bsr_zone);
    void	store_rp_set(const BsrZone& bsr_zone);
    
    Timer&	bsr_timer()			{ return (_bsr_timer);	}
    const Timer& const_bsr_timer() const	{ return (_bsr_timer);	}
    void	timeout_bsr_timer();
    Timer&	scope_zone_expiry_timer() {
	return (_scope_zone_expiry_timer);
    }
    const Timer& const_scope_zone_expiry_timer() const {
	return (_scope_zone_expiry_timer);
    }
    
    const list<BsrGroupPrefix *>& bsr_group_prefix_list() const {
	return (_bsr_group_prefix_list);
    }
    BsrGroupPrefix *add_bsr_group_prefix(const IPvXNet& group_prefix_init,
					 bool is_scope_zone_init,
					 uint8_t expected_rp_count);
    void	delete_bsr_group_prefix(BsrGroupPrefix *bsr_group_prefix);
    BsrGroupPrefix *find_bsr_group_prefix(const IPvXNet& group_prefix) const;
    
    bool	process_candidate_bsr(const BsrZone& bsr_zone);
    bool	i_am_bsr() const;
    bool	is_new_bsr_preferred(const BsrZone& bsr_zone) const;
    bool	is_new_bsr_same_priority(const BsrZone& bsr_zone) const;
    void	randomized_override_interval(const IPvX& my_addr,
					     uint8_t my_priority,
					     struct timeval *result_timeval) const;

    bool	is_bsm_forward() const { return (_is_bsm_forward); }
    void	set_bsm_forward(bool v) { _is_bsm_forward = v; }
    bool	is_bsm_originate() const { return (_is_bsm_originate); }
    void	set_bsm_originate(bool v) { _is_bsm_originate = v; }
    bool	i_am_candidate_bsr() const { return (_i_am_candidate_bsr); }
    void	set_i_am_candidate_bsr(bool i_am_candidate_bsr,
				       const IPvX& my_bsr_addr,
				       uint8_t my_bsr_priority);
    
    const IPvX&	my_bsr_addr() const	{ return (_my_bsr_addr);	}
    uint8_t	my_bsr_priority() const	{ return (_my_bsr_priority);	}
    
    //
    // Cand-RP related methods
    //
    BsrRp	*find_rp(const IPvXNet& group_prefix,
			 const IPvX& rp_addr) const;
    BsrRp	*add_rp(const IPvXNet& group_prefix,
			bool is_scope_zone_init,
			const IPvX& rp_addr,
			uint8_t rp_priority,
			uint16_t rp_holdtime,
			string& error_msg);
    Timer&	candidate_rp_advertise_timer() {
	return (_candidate_rp_advertise_timer);
    }
    const Timer& const_candidate_rp_advertise_timer() const {
	return (_candidate_rp_advertise_timer);
    }
    void	start_candidate_rp_advertise_timer();
    void	expire_candidate_rp_advertise_timer();
    
private:
    PimBsr&	_pim_bsr;		// The PimBsr for this BsrZone
    
    // BsrZone type
    bool	_is_config_bsr_zone;	// True if config BSR zone
    bool	_is_active_bsr_zone;	// True if active BSR zone
    bool	_is_expire_bsr_zone;	// True if expire BSR zone
    bool	_is_test_bsr_zone;	// True if test BSR zone
    
    // State at all routers
    IPvX	_bsr_addr;		// The address of the Bootstrap router
    uint8_t	_bsr_priority;		// The BSR priority (larger is better)
    uint8_t	_hash_masklen;		// The hash mask length
    uint16_t	_fragment_tag;		// The fragment tag
    bool	_is_accepted_message;	// True if info accepted already
    bool	_is_unicast_message;	// True if info received by unicast
    IPvX	_unicast_message_src;	// The source address of the info
					// (if received by unicast)
    
    PimScopeZoneId _zone_id;		// The (prefix-based) scope zone ID.
					// If non-scoped zone, the ID is
					// IPvXNet:ip_multicast_base_prefix()
    
    Timer	_bsr_timer;		// The Bootstrap Timer
    list<BsrGroupPrefix *> _bsr_group_prefix_list; // The list of group
					// prefixes for this zone.
					// XXX: if a scope zone, and if there
					// is a group-RP prefix for the whole
					// zone, this prefix must be in front.
    bsr_zone_state_t _bsr_zone_state;	// Scope zone state
    Timer	_scope_zone_expiry_timer; // The Scope-Zone Expiry Timer
    
    // State at a Candidate BSR
    bool	_i_am_candidate_bsr;	// True if I am Cand-BSR for this zone
    IPvX	_my_bsr_addr;		// My address if a Cand-BSR
    uint8_t	_my_bsr_priority;	// My BSR priority if a Cand-BSR
    
    // State at a Candidate RP
    Timer	_candidate_rp_advertise_timer; // The C-RP Adv. Timer
    
    // Misc. state
    bool	_is_bsm_forward;	// Temp. state: if true, forward BSM
    bool	_is_bsm_originate;	// Temp. state: if true, originate BSM
    bool	_is_cancel;		// True if we are canceling this zone
};

class BsrGroupPrefix {
public:
    BsrGroupPrefix(BsrZone& bsr_zone, const IPvXNet& group_prefix,
		   bool is_scope_zone, uint8_t expected_rp_count);
    BsrGroupPrefix(BsrZone& bsr_zone, const BsrGroupPrefix& bsr_group_prefix);
    ~BsrGroupPrefix();
    
    BsrZone&	bsr_zone()		{ return (_bsr_zone);		}
    bool	is_scope_zone() const { return (_is_scope_zone); }
    const IPvXNet& group_prefix() const	{ return (_group_prefix);	}
    uint8_t	expected_rp_count() const { return (_expected_rp_count); }
    uint8_t	received_rp_count() const { return (_received_rp_count); }
    void	set_received_rp_count(uint8_t v) { _received_rp_count = v; }
    void	set_expected_rp_count(uint8_t v) { _expected_rp_count = v; }
    const list<BsrRp *>& rp_list()	const	{ return (_rp_list);	}
    BsrRp	*add_rp(const IPvX& rp_addr, uint8_t rp_priority,
			uint16_t rp_holdtime);
    void	delete_rp(BsrRp *bsr_rp);
    BsrRp	*find_rp(const IPvX& rp_addr) const;
    bool is_completed() const { return (_expected_rp_count == _received_rp_count); }
    
    // Removal related methods
    void	schedule_bsr_group_prefix_remove();
    const Timer& const_bsr_group_prefix_remove_timer() const {
	return (_bsr_group_prefix_remove_timer);
    }
    
    
private:
    BsrZone&	_bsr_zone;		// The BSR zone I belong to
    
    // Group prefix state
    IPvXNet	_group_prefix;		// The group address prefix
    bool	_is_scope_zone;		// True if prefix for admin. scope zone
    uint8_t	_expected_rp_count;	// Expected number of RPs
    uint8_t	_received_rp_count;	// Received number of RPs so far
    list<BsrRp *> _rp_list;		// The list of received RPs
    
    // Misc. state
    Timer	_bsr_group_prefix_remove_timer;	// Timer to remove empty prefix
};

class BsrRp {
public:
    BsrRp(BsrGroupPrefix& bsr_group_prefix, const IPvX& rp_addr,
	  uint8_t rp_priority, uint16_t rp_holdtime);
    BsrRp(BsrGroupPrefix& bsr_group_prefix, const BsrRp& bsr_rp);
    
    BsrGroupPrefix& bsr_group_prefix() { return (_bsr_group_prefix); }
    const IPvX&	rp_addr() const	{ return (_rp_addr);			}
    uint8_t	rp_priority() const { return (_rp_priority);		}
    uint16_t	rp_holdtime() const { return (_rp_holdtime);		}
    void	set_rp_priority(uint8_t v)  { _rp_priority = v;		}
    void	set_rp_holdtime(uint16_t v) { _rp_holdtime = v;		}
    const Timer& const_candidate_rp_expiry_timer() const {
	return (_candidate_rp_expiry_timer);
    }
    void	start_candidate_rp_expiry_timer();
    
private:
    BsrGroupPrefix& _bsr_group_prefix;	// The BSR prefix I belong to
    
    // RP state
    IPvX	_rp_addr;		// The address of the RP
    uint8_t	_rp_priority;		// RP priority (smaller is better)
    uint16_t	_rp_holdtime;		// RP holdtime (in seconds)
    Timer	_candidate_rp_expiry_timer; // The C-RP Expiry Timer
};

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __PIM_PIM_BSR_HH__
