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

// $XORP: xorp/pim/pim_bsr.hh,v 1.23 2002/12/09 18:29:25 hodson Exp $


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


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class PimNode;
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
    BsrZone	*add_active_bsr_zone(BsrZone& bsr_zone,
				     string& error_msg);
    void	delete_active_bsr_zone(BsrZone *old_bsr_zone);
    BsrZone	*add_config_bsr_zone(const BsrZone& bsr_zone,
				     string& error_msg);
    void	delete_config_bsr_zone(BsrZone *old_bsr_zone);
    
    list<BsrZone *>& active_bsr_zone_list() { return (_active_bsr_zone_list); }
    list<BsrZone *>& config_bsr_zone_list() { return (_config_bsr_zone_list); }
    BsrZone	*find_active_bsr_zone_by_prefix(
	bool is_admin_scope_zone, const IPvXNet& group_prefix) const;
    BsrZone	*find_config_bsr_zone_by_prefix(
	bool is_admin_scope_zone, const IPvXNet& group_prefix) const;
    BsrZone	*find_bsr_zone_by_prefix_from_list(
	const list<BsrZone *>& zone_list,
	bool is_admin_scope_zone,
	const IPvXNet& group_prefix) const;
    
    BsrZone	*find_bsr_zone_from_list(
	const list<BsrZone *>& zone_list,
	bool is_admin_scope_zone,
	const IPvXNet& admin_scope_zone_id) const;
    
    bool	contains_bsr_zone_info(const list<BsrZone *>& zone_list,
				       const BsrZone& bsr_zone) const;
    
    void	schedule_rp_table_apply_rp_changes();
    
    void	add_rps_to_rp_table();
    
private:
    bool	can_add_bsr_zone_to_list(const list<BsrZone *>& zone_list,
					 const BsrZone& bsr_zone,
					 string& error_msg) const;
    
    PimNode&	_pim_node;		// The associated PIM node
    list<BsrZone *> _active_bsr_zone_list; // The list of active BSR zones
    list<BsrZone *> _config_bsr_zone_list; // The list of configured Cand-BSR
					   // zones and/or Cand-RP information
    Timer	_rp_table_apply_rp_changes_timer; // Timer to apply RP changes to the RpTable
};

class BsrZone {
public:
    BsrZone(PimBsr& pim_bsr, bool is_admin_scope_zone,
	    const IPvXNet& admin_scope_zone_id);
    BsrZone(PimBsr& pim_bsr, const IPvX& bsr_addr, uint8_t bsr_priority,
	    uint8_t hash_masklen, uint16_t fragment_tag);
    BsrZone(PimBsr& pim_bsr, const BsrZone& bsr_zone);
    ~BsrZone();

    BsrZone	*find_matching_active_bsr_zone() const;
    void	replace_bsr_group_prefix_list(const BsrZone& bsr_zone);
    void	merge_bsr_group_prefix_list(const BsrZone& bsr_zone);
    
    PimBsr&	pim_bsr()		{ return (_pim_bsr);		}    
    bool	is_consistent(string& error_msg) const;
    const IPvX&	bsr_addr() const	{ return (_bsr_addr);		}
    uint8_t	bsr_priority() const	{ return (_bsr_priority);	}
    uint8_t	hash_masklen() const	{ return (_hash_masklen);	}
    uint16_t	fragment_tag() const	{ return (_fragment_tag);	}
    uint16_t	new_fragment_tag()	{ return (++_fragment_tag); }
    
    bool	is_admin_scope_zone() const { return (_is_admin_scope_zone); }
    const IPvXNet& admin_scope_zone_id() const { return (_admin_scope_zone_id); }
    void	set_admin_scope_zone(bool is_admin_scope_zone,
				     const IPvXNet& admin_scope_zone_id);
    
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
    bsr_zone_state_t bsr_zone_state() const { return (_bsr_zone_state);	}
    void set_bsr_zone_state(bsr_zone_state_t v) { _bsr_zone_state = v;	}
    
    Timer&	bsr_timer()		{ return (_bsr_timer);		}
    void	timeout_bsr_timer();
    Timer&	scope_zone_expiry_timer(){ return (_scope_zone_expiry_timer); }
    const Timer& const_bsr_timer() const { return (_bsr_timer);		}
    const Timer& const_scope_zone_expiry_timer() const {
	return (_scope_zone_expiry_timer);
    }
    
    BsrGroupPrefix *add_bsr_group_prefix(bool is_admin_scope_zone_init,
					 const IPvXNet& group_prefix_init,
					 uint8_t expected_rp_count_init,
					 string& error_msg);
    void	delete_bsr_group_prefix(BsrGroupPrefix *bsr_group_prefix);
    BsrGroupPrefix *find_bsr_group_prefix(const IPvXNet& group_prefix) const;
    const list<BsrGroupPrefix *>& bsr_group_prefix_list() const {
	return (_bsr_group_prefix_list);
    }
    
    bool	process_candidate_bsr(BsrZone& bsr_zone);
    bool	is_new_bsr_preferred(const BsrZone& bsr_zone) const;
    bool	is_new_bsr_same_priority(const BsrZone& bsr_zone) const;
    void	randomized_override_interval(
	const IPvX& my_addr,
	uint8_t my_priority,
	struct timeval *result_timeval) const;
    void	set_bsm_forward(bool v) { _is_bsm_forward = v; }
    void	set_bsm_originate(bool v) { _is_bsm_originate = v; }
    void	set_accepted_previous_bsm(bool v) {
	_is_accepted_previous_bsm = v;
    }
    bool	is_bsm_forward() const { return (_is_bsm_forward); }
    bool	is_bsm_originate() const { return (_is_bsm_originate); }
    bool	is_accepted_previous_bsm() const {
	return (_is_accepted_previous_bsm);
    }
    
    bool i_am_candidate_bsr() const	{ return (_i_am_candidate_bsr); }
    void set_i_am_candidate_bsr(bool i_am_candidate_bsr,
				const IPvX& my_bsr_addr,
				uint8_t my_bsr_priority);
    
    const IPvX&	my_bsr_addr() const	{ return (_my_bsr_addr);	}
    uint8_t	my_bsr_priority() const	{ return (_my_bsr_priority);	}
    
    //
    // Cand-RP related methods
    //
    BsrRp	*add_rp(bool is_admin_scope_zone_init,
			const IPvXNet& group_prefix,
			const IPvX& rp_addr,
			uint8_t rp_priority,
			uint16_t rp_holdtime,
			string& error_msg);
    BsrRp	*find_rp(const IPvXNet& group_prefix,
			 const IPvX& rp_addr) const;
    void	start_candidate_rp_advertise_timer();
    void	timeout_candidate_rp_advertise_timer();
    Timer&	candidate_rp_advertise_timer() {
	return (_candidate_rp_advertise_timer);
    }
    const Timer& const_candidate_rp_advertise_timer() const {
	return (_candidate_rp_advertise_timer);
    }
    
private:
    PimBsr&	_pim_bsr;		// The PimBsr for this BsrZone
    
    // State at all routers
    IPvX	_bsr_addr;		// The address of the Bootstrap router
    uint8_t	_bsr_priority;		// The BSR priority (larger is better)
    uint8_t	_hash_masklen;		// The hash mask length
    uint16_t	_fragment_tag;		// The fragment tag
    
    bool	_is_admin_scope_zone;	// True if this is a scope zone
    IPvXNet	_admin_scope_zone_id;	// The scope zone (prefix) ID or
					// IPvXNet:ip_multicast_base_prefix()
					// if non-scoped zone
    
    Timer	_bsr_timer;		// The Bootstrap Timer
    list<BsrGroupPrefix *> _bsr_group_prefix_list; // The list of group
					// prefixes for this zone.
					// XXX: if a scope zone, and if there
					// is a group-RP prefix for the whole
					// zone, this prefix must be in front.
    bsr_zone_state_t _bsr_zone_state;	// Scope zone state
    
    // State at a Candidate BSR
    Timer	_scope_zone_expiry_timer; // The Scope-Zone Expiry Timer
    
    // Configuration state
    bool	_i_am_candidate_bsr;	// True if I am Cand-BSR for this zone
    IPvX	_my_bsr_addr;		// My address if a Cand-BSR
    uint8_t	_my_bsr_priority;	// My BSR priority if a Cand-BSR
    
    // State at a Candidate RP
    Timer	_candidate_rp_advertise_timer; // The C-RP Adv. Timer
    
    // Misc. state
    bool	_is_bsm_forward;	// Temp. state: if true, forward BSM
    bool	_is_bsm_originate;	// Temp. state: if true, originate BSM
    bool	_is_accepted_previous_bsm; // True if previously accepted a BSM
};

class BsrGroupPrefix {
public:
    BsrGroupPrefix(BsrZone& bsr_zone,
		   bool is_admin_scope_zone,
		   const IPvXNet& group_prefix,
		   uint8_t expected_rp_count);
    BsrGroupPrefix(BsrZone& bsr_zone, const BsrGroupPrefix& bsr_group_prefix);
    ~BsrGroupPrefix();
    
    BsrZone&	bsr_zone()		{ return (_bsr_zone);		}
    bool	is_admin_scope_zone() const { return (_is_admin_scope_zone); }
    const IPvXNet& group_prefix() const	{ return (_group_prefix);	}
    uint8_t	expected_rp_count() const { return (_expected_rp_count); }
    uint8_t	received_rp_count() const { return (_received_rp_count); }
    void	set_received_rp_count(uint8_t v) { _received_rp_count = v; }
    void	set_expected_rp_count(uint8_t v) { _expected_rp_count = v; }
    const list<BsrRp *>& rp_list()	const	{ return (_rp_list);	}
    BsrRp	*add_rp(const IPvX& rp_addr, uint8_t rp_priority,
			uint16_t rp_holdtime, string& error_msg);
    void	delete_rp(BsrRp *bsr_rp);
    BsrRp	*find_rp(const IPvX& rp_addr) const;
    bool is_completed() const { return (_expected_rp_count == _received_rp_count); }
    
private:
    BsrZone&	_bsr_zone;		// The BSR zone I belong to
    
    // Group prefix state
    bool	_is_admin_scope_zone;	// True if prefix for admin. scope zone
    IPvXNet	_group_prefix;		// The group address prefix
    uint8_t	_expected_rp_count;	// Expected number of RPs
    uint8_t	_received_rp_count;	// Received number of RPs so far
    list<BsrRp *> _rp_list;		// The list of received RPs
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
