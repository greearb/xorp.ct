// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/pim/pim_proto_join_prune_message.hh,v 1.4 2003/06/23 18:56:55 pavlin Exp $


#ifndef __PIM_PIM_PROTO_JOIN_PRUNE_MESSAGE_HH__
#define __PIM_PIM_PROTO_JOIN_PRUNE_MESSAGE_HH__


//
// PIM Join/Prune message creation implementation-specific definitions.
//

#include <list>

#include "mrt/buffer.h"
#include "mrt/multicast_defs.h"
#include "pim_proto.h"
#include "pim_mre.hh"


//
// Constants definitions
//

//
// When we compute the Join/Prune message size whether it fits within
// a buffer, we need to take into account the expected max. number of
// multicast sources. Here we define this number.
// TODO: get rid of this predefined number
//
#define PIM_JP_EXTRA_SOURCES_N		32


//
// Structures/classes, typedefs and macros
//
// The type of entries: (*,*,RP), (*,G), (S,G), (S,G,rpt)
enum mrt_entry_type_t {
    MRT_ENTRY_UNKNOWN = 0,
    MRT_ENTRY_RP = PIM_MRE_RP,
    MRT_ENTRY_WC = PIM_MRE_WC,
    MRT_ENTRY_SG = PIM_MRE_SG,
    MRT_ENTRY_SG_RPT = PIM_MRE_SG_RPT
};

//
// Various structures to handle the assembly of source-group entries
// that are pending to be (re)send to a neighbor in a Join/Prune (or a Graft)
// message.
// XXX: the various numbers fields in a PIM Join/Prune message are no
// larger than 16 bits. The purpose of the structures below
// is to collect all necessary information about the pending source-group
// entries instead of completely spliting them according to the protocol
// messages they are going to be included into. Thus, the final job of
// assigning which entries go into which J/P protocol message can be
// performed by an independent block which may take care of controlling
// the overall control bandwidth for example.
//

//
// Class to keep list of the source-group entries that are pending
// to be (re)send to a neighbor in a Join-Prune (or Graft) message.
//

class IPvX;
class PimJpGroup;
class PimMrt;
class PimNbr;
class PimNode;

class PimJpHeader {
public:
    PimJpHeader(PimNode& pim_node);
    ~PimJpHeader();
    void	reset();
    
    PimNode&	pim_node() const	{ return (_pim_node);		}
    int		family() const		{ return (_family);		}
    PimMrt&	pim_mrt() const;
    
    size_t message_size() const {
	    return ( sizeof(struct pim) + ENCODED_UNICAST_ADDR_SIZE(_family)
		     + 2 * sizeof(uint8_t) + sizeof(uint16_t)
		     + _jp_groups_n * (ENCODED_GROUP_ADDR_SIZE(_family)
				       + 2 * sizeof(uint16_t))
		     + _jp_sources_n * (ENCODED_SOURCE_ADDR_SIZE(_family)));
    }
    size_t extra_source_size() const {
	return (ENCODED_SOURCE_ADDR_SIZE(_family));
    }
    
    int		jp_entry_add(const IPvX& source_addr, const IPvX& group_addr,
			     uint8_t group_mask_len,
			     mrt_entry_type_t mrt_entry_type,
			     action_jp_t action_jp, uint16_t holdtime,
			     bool new_group_bool);
    int		mrt_commit(PimVif *pim_vif, const IPvX& target_nbr_addr);
    int		network_commit(PimVif *pim_vif, const IPvX& target_nbr_addr);
    int		network_send(PimVif *pim_vif, const IPvX& target_nbr_addr);
    
    uint32_t	jp_groups_n() const		{ return (_jp_groups_n);  }
    uint32_t	jp_sources_n() const		{ return (_jp_sources_n); }
    void	set_jp_groups_n(uint32_t v)	{ _jp_groups_n = v;	}
    void	incr_jp_groups_n()		{ _jp_groups_n++;	}
    void	decr_jp_groups_n()		{ _jp_groups_n--;	}
    void	set_jp_sources_n(uint32_t v)	{ _jp_sources_n = v;	}
    void	incr_jp_sources_n()		{ _jp_sources_n++;	}
    void	decr_jp_sources_n()		{ _jp_sources_n--;	}
    
private:
    PimNode&	_pim_node;		// The PIM node
    int		_family;		// The address family
    list<PimJpGroup *> _jp_groups_list;// The list of groups
    uint32_t	_jp_groups_n;		// Total number of groups
    uint32_t	_jp_sources_n;		// Total number of sources (all groups)
    uint16_t	_holdtime;		// The Holdtime in the J/P message
};

//
// Class to keep info about the list of sources entries to Join/Prune
//
class PimJpSources {
public:
    PimJpSources() : _j_n(0), _p_n(0) {}
    ~PimJpSources() {}
    
    list<IPvX>& j_list()		{ return (_j_list);		}
    list<IPvX>& p_list()		{ return (_p_list);		}
    bool	j_list_found(const IPvX& ipaddr);
    bool	p_list_found(const IPvX& ipaddr);
    bool	j_list_remove(const IPvX& ipaddr);
    bool	p_list_remove(const IPvX& ipaddr);
    uint32_t	j_n()	const		{ return (_j_n);		}
    void	set_j_n(uint32_t v)	{ _j_n = v;			}
    void	incr_j_n()		{ _j_n++;			}
    void	decr_j_n()		{ _j_n--;			}
    uint32_t	p_n()	const		{ return (_p_n);		}
    void	set_p_n(uint32_t v)	{ _p_n = v;			}
    void	incr_p_n()		{ _p_n++;			}
    void	decr_p_n()		{ _p_n--;			}
    
private:
    list<IPvX>	_j_list;		// List of Join entries
    list<IPvX>	_p_list;		// List of Prune entries
    uint32_t	_j_n;			// Number of Join entries
    uint32_t	_p_n;			// Number of Prune entries
};

//
// Class to keep info about a group entry that is pending to be
// (re)send to a neighbor in a Join-Prune (or Graft) message,
// or to be commited to the Multicast Routing Table.
//
class PimJpGroup {
public:
    PimJpGroup(PimJpHeader& jp_header, int family);
    int		family()	const	{ return (_family);		}
    PimJpHeader& jp_header()		{ return (_jp_header);		}
    const IPvX&	group_addr()	const	{ return (_group_addr);		}
    void	set_group_addr(const IPvX& v) { _group_addr = v;	}
    uint8_t	group_mask_len() const	{ return (_group_mask_len);	}
    void	set_group_mask_len(uint8_t v) { _group_mask_len = v;	}
    void	incr_jp_groups_n()	{ jp_header().incr_jp_groups_n();  }
    void	decr_jp_groups_n()	{ jp_header().decr_jp_groups_n();  }
    uint32_t	j_sources_n()	const	{ return (_j_sources_n);	}
    uint32_t	p_sources_n()	const	{ return (_p_sources_n);	}
    void	set_j_sources_n(uint32_t v) { _j_sources_n = v; }
    void	set_p_sources_n(uint32_t v) { _p_sources_n = v; }
    void	incr_j_sources_n()	{ _j_sources_n++; jp_header().incr_jp_sources_n(); }
    void	decr_j_sources_n()	{ _j_sources_n--; jp_header().decr_jp_sources_n(); }
    void	incr_p_sources_n()	{ _p_sources_n++; jp_header().incr_jp_sources_n(); }
    void	decr_p_sources_n()	{ _p_sources_n--; jp_header().decr_jp_sources_n(); }
    PimJpSources *rp()			{ return (&_rp);		}
    PimJpSources *wc()			{ return (&_wc);		}
    PimJpSources *sg()			{ return (&_sg);		}
    PimJpSources *sg_rpt()		{ return (&_sg_rpt);		}
    
    size_t message_size() const {
	return ( ENCODED_GROUP_ADDR_SIZE(_family)
		 + 2 * sizeof(uint16_t)
		 + (ENCODED_SOURCE_ADDR_SIZE(_family)
		    * (_rp.j_n() + _rp.p_n()
		       + _wc.j_n() + _wc.p_n()
		       + _sg.j_n() + _sg.p_n()
		       + _sg_rpt.j_n() + _sg_rpt.p_n())));
    }
    
private:
    PimJpHeader& _jp_header;		// The J/P header for the groups
					//   to (re)send
    int		_family;		// The address family
    IPvX	_group_addr;		// The address of the multicast group
    uint8_t	_group_mask_len;	// The 'group_addr' mask length
    uint32_t	_j_sources_n;		// The total number of Joined sources
    uint32_t	_p_sources_n;		// The total number of Pruned sources
    // The lists for each type of entry: (*,*,RP), (*,G), (S,G), (S,G,rpt)
    PimJpSources _rp;			// The (*,*,RP)  Join/Prune entries
    PimJpSources _wc;			// The (*,G)     Join/Prune entries
    PimJpSources _sg;			// The (S,G)     Join/Prune entries
    PimJpSources _sg_rpt;		// The (S,G,rpt) Join/Prune entries
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_PROTO_JOIN_PRUNE_MESSAGE_HH__
