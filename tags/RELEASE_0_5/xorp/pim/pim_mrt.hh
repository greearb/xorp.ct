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

// $XORP: xorp/pim/pim_mrt.hh,v 1.7 2003/07/12 01:14:38 pavlin Exp $


#ifndef __PIM_PIM_MRT_HH__
#define __PIM_PIM_MRT_HH__


//
// PIM Multicast Routing Table header file.
//


#include "libproto/proto_unit.hh"
#include "mrt/mifset.hh"
#include "mrt/mrt.hh"
#include "pim_mre_track_state.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class IPvX;
class PimMfc;
class PimMre;
class PimMreTask;
class PimMrt;
class PimNode;
class PimVif;
class PimMribTable;

//
// PIM-specific (S,G) Multicast Routing Table
//
class PimMrtSg : public Mrt<PimMre> {
public:
    PimMrtSg(PimMrt& pim_mrt);
    virtual ~PimMrtSg();
    
    PimMrt&	pim_mrt() const { return (_pim_mrt); }
    
private:
    PimMrt&	_pim_mrt;	// The PIM Multicast Routing Table
};

//
// PIM-specific (*,G) Multicast Routing Table
//
class PimMrtG : public Mrt<PimMre> {
public:
    PimMrtG(PimMrt& pim_mrt);
    virtual ~PimMrtG();
    
    PimMrt&	pim_mrt() const { return (_pim_mrt); }
    
private:
    PimMrt&	_pim_mrt;	// The PIM Multicast Routing Table
};

//
// PIM-specific (*,*,RP) Multicast Routing Table
//
class PimMrtRp : public Mrt<PimMre> {
public:
    PimMrtRp(PimMrt& pim_mrt);
    virtual ~PimMrtRp();
    
    PimMrt&	pim_mrt() const { return (_pim_mrt); }
    
private:
    PimMrt&	_pim_mrt;	// The PIM Multicast Routing Table
};

//
// PIM-specific Multicast Forwarding Cache Table
//
class PimMrtMfc : public Mrt<PimMfc> {
public:
    PimMrtMfc(PimMrt& pim_mrt);
    virtual ~PimMrtMfc();
    
    PimMrt&	pim_mrt() const { return (_pim_mrt); }
    
private:
    PimMrt&	_pim_mrt;	// The PIM Multicast Routing Table
};

//
// PIM-specific Multicast Routing Table
//
class PimMrt {
public:
    PimMrt(PimNode& pim_node);
    virtual ~PimMrt();
    
    PimNode&	pim_node() const	{ return (_pim_node);		}
    PimMrtSg&	pim_mrt_sg()		{ return (_pim_mrt_sg);		}
    PimMrtSg&	pim_mrt_sg_rpt()	{ return (_pim_mrt_sg_rpt);	}
    PimMrtG&	pim_mrt_g()		{ return (_pim_mrt_g);		}
    PimMrtRp&	pim_mrt_rp()		{ return (_pim_mrt_rp);		}
    PimMrtMfc&	pim_mrt_mfc()		{ return (_pim_mrt_mfc);	}
    
    PimMre	*pim_mre_find(const IPvX& source, const IPvX& group,
			      uint32_t lookup_flags, uint32_t create_flags);
    PimMfc	*pim_mfc_find(const IPvX& source, const IPvX& group,
			      bool is_create_bool);
    int		remove_pim_mre(PimMre *pim_mre);
    int		remove_pim_mfc(PimMfc *pim_mfc);
    
    //
    // MFC-related methods
    //
    int signal_message_nocache_recv(const string& src_module_instance_name,
				    xorp_module_id src_module_id,
				    uint16_t vif_index,
				    const IPvX& src,
				    const IPvX& dst);
    int signal_message_wrongvif_recv(const string& src_module_instance_name,
				     xorp_module_id src_module_id,
				     uint16_t vif_index,
				     const IPvX& src,
				     const IPvX& dst);
    int signal_message_wholepkt_recv(const string& src_module_instance_name,
				     xorp_module_id src_module_id,
				     uint16_t vif_index,
				     const IPvX& src,
				     const IPvX& dst,
				     const uint8_t *rcvbuf,
				     size_t rcvlen);
    void receive_data(uint16_t iif_vif_index, const IPvX& src,
		      const IPvX& dst);
    
    int signal_dataflow_recv(const IPvX& source_addr,
			     const IPvX& group_addr,
			     uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t measured_interval_sec,
			     uint32_t measured_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     uint32_t measured_packets,
			     uint32_t measured_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall);
    
    // Redirection functions (to the pim_node)
    int		family() const;
    PimMribTable& pim_mrib_table();
    Mifset&	i_am_dr();
    PimVif	*vif_find_by_vif_index(uint16_t vif_index);
    PimVif	*vif_find_pim_register();
    uint16_t	pim_register_vif_index() const;
    
    //
    // Track-state related methods
    //
    const PimMreTrackState&	pim_mre_track_state() const {
	return (_pim_mre_track_state);
    }
    void	track_state_print_actions_name() const {
	_pim_mre_track_state.print_actions_name();
    }
    void	track_state_print_actions_num() const {
	_pim_mre_track_state.print_actions_num();
    }
    
    //
    // Tasks related methods
    //
    void	add_task(PimMreTask *pim_mre_task);
    void	delete_task(PimMreTask *pim_mre_task);
    void	schedule_task(PimMreTask *pim_mre_task);
    //
    // The "add_task_*" methods
    //
    void add_task_rp_changed(const IPvX& affected_rp_addr);
    void add_task_mrib_changed(const IPvXNet& modified_prefix_addr);
    void add_task_mrib_next_hop_changed(const IPvXNet& modified_prefix_addr);
    void add_task_mrib_next_hop_rp_gen_id_changed(const IPvX& rp_addr);
    void add_task_pim_nbr_changed(uint16_t vif_index,
				  const IPvX& pim_nbr_addr);
    void add_task_pim_nbr_gen_id_changed(uint16_t vif_index,
					 const IPvX& pim_nbr_addr);
    void add_task_assert_rpf_interface_wc(uint16_t old_rpf_interface_rp,
					  const IPvX& group_addr);
    void add_task_assert_rpf_interface_sg(uint16_t old_rpf_interface_s,
					  const IPvX& source_addr,
					  const IPvX& group_addr);
    void add_task_receive_join_rp(uint16_t vif_index, const IPvX& rp_addr);
    void add_task_receive_join_wc(uint16_t vif_index, const IPvX& group_addr);
    void add_task_receive_join_sg(uint16_t vif_index, const IPvX& source_addr,
				  const IPvX& group_addr);
    void add_task_receive_join_sg_rpt(uint16_t vif_index,
				      const IPvX& source_addr,
				      const IPvX& group_addr);
    void add_task_receive_prune_rp(uint16_t vif_index, const IPvX& rp_addr);
    void add_task_receive_prune_wc(uint16_t vif_index, const IPvX& group_addr);
    void add_task_see_prune_wc(uint16_t vif_index, const IPvX& group_addr,
			       const IPvX& target_nbr_addr);
    void add_task_receive_prune_sg(uint16_t vif_index, const IPvX& source_addr,
				   const IPvX& group_addr);
    void add_task_receive_prune_sg_rpt(uint16_t vif_index,
				       const IPvX& source_addr,
				       const IPvX& group_addr);
    void add_task_receive_end_of_message_sg_rpt(uint16_t vif_index,
						const IPvX& group_addr);
    void add_task_downstream_jp_state_rp(uint16_t vif_index,
					 const IPvX& rp_addr);
    void add_task_downstream_jp_state_wc(uint16_t vif_index,
					 const IPvX& group_addr);
    void add_task_downstream_jp_state_sg(uint16_t vif_index,
					 const IPvX& source_addr,
					 const IPvX& group_addr);
    void add_task_downstream_jp_state_sg_rpt(uint16_t vif_index,
					     const IPvX& source_addr,
					     const IPvX& group_addr);
    void add_task_upstream_jp_state_sg(const IPvX& source_addr,
				       const IPvX& group_addr);
    void add_task_local_receiver_include_wc(uint16_t vif_index,
					    const IPvX& group_addr);
    void add_task_local_receiver_include_sg(uint16_t vif_index,
					    const IPvX& source_addr,
					    const IPvX& group_addr);
    void add_task_local_receiver_exclude_sg(uint16_t vif_index,
					    const IPvX& source_addr,
					    const IPvX& group_addr);
    void add_task_assert_state_wc(uint16_t vif_index, const IPvX& group_addr);
    void add_task_assert_state_sg(uint16_t vif_index,
				  const IPvX& source_addr,
				  const IPvX& group_addr);
    void add_task_i_am_dr(uint16_t vif_index);
    void add_task_my_ip_address(uint16_t vif_index);
    void add_task_my_ip_subnet_address(uint16_t vif_index);
    void add_task_spt_switch_threshold_changed();
    void add_task_keepalive_timer_sg(const IPvX& source_addr,
				     const IPvX& group_addr);
    void add_task_sptbit_sg(const IPvX& source_addr, const IPvX& group_addr);
    void add_task_start_vif(uint16_t vif_index);
    void add_task_stop_vif(uint16_t vif_index);
    void add_task_add_pim_mre(PimMre *pim_mre);
    void add_task_delete_pim_mre(PimMre *pim_mre);
    
    void add_task_delete_pim_mfc(PimMfc *pim_mfc);
    
    list<PimMreTask *>& pim_mre_task_list() { return (_pim_mre_task_list); }
    
    
private:
    PimNode&	_pim_node;	// The PIM node
    
    //
    // The lookup tables
    //
    // XXX: all entries in the (*,G) table have source address = IPvX::ZERO()
    // XXX: all entries in the (*,*,RP) table have group address = IPvX::ZERO()
    //
    PimMrtSg	_pim_mrt_sg;		// The PIM-specific (S,G) MRT
    PimMrtSg	_pim_mrt_sg_rpt;	// The PIM-specific (S,G,rpt) MRT
    PimMrtG	_pim_mrt_g;		// The PIM-specific (*,G) MRT
    PimMrtRp	_pim_mrt_rp;		// The PIM-specific (*,*,RP) MRT
    PimMrtMfc	_pim_mrt_mfc;		// The PIM-specific MFC MRT
    
    PimMreTrackState _pim_mre_track_state; // The state-tracking information
    list<PimMreTask *> _pim_mre_task_list; // The list of tasks
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_MRT_HH__
