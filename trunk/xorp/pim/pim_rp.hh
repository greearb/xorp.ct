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

// $XORP: xorp/pim/pim_rp.hh,v 1.36 2002/12/09 18:29:31 hodson Exp $


#ifndef __PIM_PIM_RP_HH__
#define __PIM_PIM_RP_HH__


//
// PIM RP information definitions
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
class PimRp;
class RpTable;
class PimMre;

// The PIM RP class
class PimRp {
public:
    enum rp_learned_method_t {
	RP_LEARNED_METHOD_AUTORP,
	RP_LEARNED_METHOD_BOOTSTRAP,
	RP_LEARNED_METHOD_STATIC,
	RP_LEARNED_METHOD_UNKNOWN
    };
    
    PimRp(RpTable& rp_table, const IPvX& rp_addr, uint8_t rp_priority,
	  const IPvXNet& group_prefix, uint8_t hash_masklen,
	  rp_learned_method_t rp_learned_method);
    PimRp(RpTable& rp_table, const PimRp& pim_rp);
    ~PimRp();
    
    RpTable&	rp_table()		{ return (_rp_table);		}
    
    const IPvX&	rp_addr()	const	{ return (_rp_addr);		}
    uint8_t	rp_priority()	const	{ return (_rp_priority);	}
    void	set_rp_priority(uint8_t v) { _rp_priority = v;		}
    const IPvXNet& group_prefix() const	{ return (_group_prefix);	}
    uint8_t	hash_masklen()	const	{ return (_hash_masklen);	}
    void	set_hash_masklen(uint8_t v) { _hash_masklen = v;	}
    rp_learned_method_t rp_learned_method() const { return (_rp_learned_method); }
    static const string rp_learned_method_str(rp_learned_method_t rp_learned_method);
    const string rp_learned_method_str() const;
    bool	is_updated()	const	{ return (_is_updated);		}
    void	set_is_updated(bool v)	{ _is_updated = v;		}
    
    list<PimMre *>& pim_mre_wc_list()	{ return (_pim_mre_wc_list);	}
    list<PimMre *>& pim_mre_sg_list()	{ return (_pim_mre_sg_list);	}
    list<PimMre *>& pim_mre_sg_rpt_list() { return (_pim_mre_sg_rpt_list); }
    list<PimMfc *>& pim_mfc_list()	{ return (_pim_mfc_list);	}
    list<PimMre *>& processing_pim_mre_wc_list() {
	return (_processing_pim_mre_wc_list);
    }
    list<PimMre *>& processing_pim_mre_sg_list() {
	return (_processing_pim_mre_sg_list);
    }
    list<PimMre *>& processing_pim_mre_sg_rpt_list() {
	return (_processing_pim_mre_sg_rpt_list);
    }
    list<PimMfc *>& processing_pim_mfc_list() {
	return (_processing_pim_mfc_list);
    }
    void	init_processing_pim_mre_wc();
    void	init_processing_pim_mre_sg();
    void	init_processing_pim_mre_sg_rpt();
    void	init_processing_pim_mfc();
    
    bool	i_am_rp() const { return (_i_am_rp); }
    
    
private:
    RpTable&	_rp_table;		// The RP table I belong to
    IPvX	_rp_addr;		// The RP address
    uint8_t	_rp_priority;		// The RP priority
    IPvXNet	_group_prefix;		// The group address prefix
    uint8_t	_hash_masklen;		// The RP hash masklen
    rp_learned_method_t _rp_learned_method; // How learned about this RP
    bool	_is_updated;		// True if just created or updated
    list<PimMre *> _pim_mre_wc_list;	// List of all related (*,G) entries
					// for this RP.
    list<PimMre *> _pim_mre_sg_list;	// List of all related (S,G) entries
					// for this RP that have no (*,G) entry
    list<PimMre *> _pim_mre_sg_rpt_list;// List of all related (S,G,rpt)
					// entries for this RP that have no
					// (*,G) entry
    list<PimMfc *> _pim_mfc_list;	// List of all related MFC entries
					// for this RP.
					// NOTE: those MFC entries _may_ have
					// existing (*,G) entry.
    list<PimMre *> _processing_pim_mre_wc_list; // List of all related
					// (*,G) entries for this RP that are
					// awaiting to be processed.
    list<PimMre *> _processing_pim_mre_sg_list; // List of all related
					// (S,G) entries for this RP that
					// have no (*,G) entry and that are
					// awaiting to be processed.
    list<PimMre *> _processing_pim_mre_sg_rpt_list; // List of all related
					// (S,G,rpt) entries for this RP that
					// have no (*,G) entry and that are
					// awaiting to be processed.
    list<PimMfc *> _processing_pim_mfc_list; // List of all related
					// MFC entries for this RP that are
					// awaiting to be processed.
					// NOTE: those MFC entries _may_ have
					// existing (*,G) entry.
    
    bool	_i_am_rp;		// True if this RP is me
};

// The RP table class
class RpTable : public ProtoUnit {
public:
    RpTable(PimNode& pim_node);
    ~RpTable();
    
    int		start();
    int		stop();
    PimNode&	pim_node()	{ return (_pim_node);			}
    PimRp	*rp_find(const IPvX& group_addr);
    PimRp	*add_rp(const IPvX& rp_addr,
			uint8_t rp_priority,
			const IPvXNet& group_prefix,
			uint8_t hash_masklen,
			PimRp::rp_learned_method_t rp_learned_method);
    int		delete_rp(const IPvX& rp_addr,
			  const IPvXNet& group_prefix,
			  PimRp::rp_learned_method_t rp_learned_method);
    bool	apply_rp_changes();
    
    void	add_pim_mre(PimMre *pim_mre);
    void	add_pim_mfc(PimMfc *pim_mfc);
    void	delete_pim_mre(PimMre *pim_mre);
    void	delete_pim_mfc(PimMfc *pim_mfc);
    
    list<PimRp *>& rp_list() { return (_rp_list); }
    list<PimRp *>& processing_rp_list() { return (_processing_rp_list); }
    
    void	init_processing_pim_mre_wc(const IPvX& rp_addr);
    void	init_processing_pim_mre_sg(const IPvX& rp_addr);
    void	init_processing_pim_mre_sg_rpt(const IPvX& rp_addr);
    void	init_processing_pim_mfc(const IPvX& rp_addr);
    PimRp	*find_processing_pim_mre_wc(const IPvX& rp_addr);
    PimRp	*find_processing_pim_mre_sg(const IPvX& rp_addr);
    PimRp	*find_processing_pim_mre_sg_rpt(const IPvX& rp_addr);
    PimRp	*find_processing_pim_mfc(const IPvX& rp_addr);
    PimRp	*find_processing_rp_by_addr(const IPvX& rp_addr);
    bool	has_rp_addr(const IPvX& rp_addr);
    
private:
    PimRp	*compare_rp(const IPvX& group_addr, PimRp *rp1,
			    PimRp *rp2) const;
    uint32_t	derived_addr(const IPvX& addr) const;
    
    PimNode&	_pim_node;		// The associated PIM node
    
    list<PimRp *> _rp_list;		// The list of RPs
    list<PimRp *> _processing_rp_list;	// The list of obsolete RPs to process:
    // XXX: the PimRp entry with address of IPvX::ZERO(family()) contains
    // the list of PimMre and PimMfc entries that have no RP.
};

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __PIM_PIM_RP_HH__
