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

// $XORP: xorp/mfea/mfea_dataflow.hh,v 1.15 2002/12/09 18:29:17 hodson Exp $


#ifndef __MFEA_MFEA_DATAFLOW_HH__
#define __MFEA_MFEA_DATAFLOW_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) dataflow definition.
//


#include <list>

#include "libproto/proto_unit.hh"
#include "mrt/mrt.hh"
#include "mrt/timer.hh"
#include "mfea_unix_comm.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class MfeaDfe;
class MfeaDfeLookup;
class MfeaNode;

/**
 * @short The MFEA (S,G) dataflow table for monitoring forwarded bandwidth.
 */
class MfeaDft : public Mrt<MfeaDfeLookup> {
public:
    /**
     * Constructor for a given MFEA node.
     * 
     * @param mfea_node the @ref MfeaNode this table belongs to.
     */
    MfeaDft(MfeaNode& mfea_node);
    
    /**
     * Destructor
     */
    virtual ~MfeaDft();
    
    /**
     * Get a reference to to the MFEA node this table belongs to.
     * 
     * @return a reference to the MFEA node (@ref MfeaNode) this table
     * belongs to.
     */
    MfeaNode&	mfea_node() const { return (_mfea_node); }
    
    /**
     * Get the address family.
     * 
     * @return the address family (e.g., AF_INET or AF_INET6
     * for IPv4 and IPv6 respectively).
     */
    int		family() const;
    
    /**
     * Add a dataflow entry.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param source the source address.
     * @param group the group address.
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_entry(const IPvX& source, const IPvX& group,
			  const struct timeval& threshold_interval,
			  uint32_t threshold_packets,
			  uint32_t threshold_bytes,
			  bool is_threshold_in_packets,
			  bool is_threshold_in_bytes,
			  bool is_geq_upcall,
			  bool is_leq_upcall);
    
    /**
     * Delete a dataflow entry.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param source the source address.
     * @param group the group address.
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_entry(const IPvX& source, const IPvX& group,
			     const struct timeval& threshold_interval,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall);
    
    /**
     * Delete all dataflow entries for a given source and group address.
     * 
     * @param source the source address.
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_entry(const IPvX& source, const IPvX& group);
    
    /**
     * Delete a given @ref MfeaDfe dataflow entry.
     * 
     * @param mfea_dfe the @ref MfeaDfe dataflow entry to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_entry(MfeaDfe *mfea_dfe);
    
private:
    MfeaNode&	_mfea_node;	// The Mfea node
};

/**
 * @short A class for storing all dataflow entries per (S,G).
 */
class MfeaDfeLookup : public Mre<MfeaDfeLookup> {
public:
    /**
     * Constructor for a given dataflow table, source and group address.
     * 
     * @param mfea_dft the dataflow table (@ref MfeaDft) this entries
     * belongs to.
     * @param source the source address.
     * @param group the group address.
     */
    MfeaDfeLookup(MfeaDft& mfea_dft, const IPvX& source, const IPvX& group);
    
    /**
     * Destructor
     */
    ~MfeaDfeLookup();
    
    /**
     * Get a reference to the dataflow table this entry belongs to.
     * 
     * @return a reference to the dataflow table (@ref MfeaDataflow) this
     * entry belongs to.
     */
    MfeaDft& mfea_dft() const { return (_mfea_dft); }
    
    /**
     * Get the address family.
     * 
     * @return the address family (e.g., AF_INET or AF_INET6 for IPv4 and
     * IPv6 respectively).
     */
    int		family() const;
    
    /**
     * Find a @ref MfeaDfe dataflow entry.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return the corresponding @ref MfeaDfe dataflow entry on success,
     * otherwise NULL.
     */
    MfeaDfe	*find(const struct timeval& threshold_interval,
		      uint32_t threshold_packets,
		      uint32_t threshold_bytes,
		      bool is_threshold_in_packets,
		      bool is_threshold_in_bytes,
		      bool is_geq_upcall,
		      bool is_leq_upcall);
    
    /**
     * Insert a @ref MfeaDfe dataflow entry.
     * 
     * @param mfea_dfe the @ref MfeaDfe dataflow entry to insert.
     */
    void	insert(MfeaDfe *mfea_dfe);

    /**
     * Remove a @ref MfeaDfe dataflow entry.
     * 
     * @param mfea_dfe the @ref MfeaDfe dataflow entry to remove.
     */
    void	remove(MfeaDfe *mfea_dfe);
    
    /**
     * Test if there are @ref MfeaDfe entries inserted within this entry.
     * 
     * @return true if there are @ref MfeaDfe entries inserted within this
     * entry, otherwise false.
     */
    bool	is_empty() const { return (_mfea_dfe_list.empty()); }
    
private:
    MfeaDft&		_mfea_dft;	// The Mfea dataflow table (yuck!)
    list<MfeaDfe *>	_mfea_dfe_list;	// The list of dataflow monitor entries
};

/**
 * @short Multicast dataflow entry class.
 * 
 * This entry contains all the information about the condition a dataflow
 * must satisfy to deliver a signal. It is (S,G)-specific, but there could be
 * more than one @ref MfeaDfe entries per (S,G).
 */
class MfeaDfe {
public:
    /**
     * Constructor with information with the dataflow condition to satisfy.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param mfea_dfe_lookup the @ref MfeaDfeLookup entry this entry
     * belongs to.
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     */
    MfeaDfe(MfeaDfeLookup& mfea_dfe_lookup,
	    const struct timeval& threshold_interval,
	    uint32_t threshold_packets,
	    uint32_t threshold_bytes,
	    bool is_threshold_in_packets,
	    bool is_threshold_in_bytes,
	    bool is_geq_upcall,
	    bool is_leq_upcall);
    
    /**
     * Destructor
     */
    ~MfeaDfe();
    
    /**
     * Get a reference to the @ref MfeaDfeLookup entry this entry belongs to.
     * 
     * @return a reference to the @ref MfeaDfeLookup entry this entry
     * belongs to.
     */
    MfeaDfeLookup& mfea_dfe_lookup() const { return (_mfea_dfe_lookup); }
    
    /**
     * Get a reference to the @ref MfeaDft dataflow table this entry
     * belongs to.
     * 
     * @return a reference to the @ref MfeaDft dataflow table this entry
     * belongs to.
     */
    MfeaDft& mfea_dft() const;
    
    /**
     * Get the address family.
     * 
     * @return the address family (e.g., AF_INET or AF_INET6
     * for IPv4 and IPv6 respectively).
     */
    int		family() const;
    
    /**
     * Get the source address.
     * 
     * @return the source address.
     */
    const IPvX& source_addr() const { return (_mfea_dfe_lookup.source_addr());}
    
    /**
     * Get the group address.
     * 
     * @return the group address.
     */
    const IPvX& group_addr() const { return (_mfea_dfe_lookup.group_addr()); }
    
    /**
     * Test if this entry is valid.
     * 
     * An entry is valid if, for example:
     * (a) either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * (b) either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * (c) the threshold interval is not too small.
     * (d) the bandwidth-related statistics do not contain invalid values.
     * 
     * @return true if this entry is valid, otherwise false.
     */
    bool is_valid() const;
    
    /**
     * Compare whether the information contained within this @ref MfeaDfe
     * entry is same as the specified information.
     * 
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return true if the information contained within this @ref MfeaDfe
     * entry is same as the specified information, otherwise false.
     */
    bool is_same(const struct timeval& threshold_interval,
		 uint32_t threshold_packets,
		 uint32_t threshold_bytes,
		 bool is_threshold_in_packets,
		 bool is_threshold_in_bytes,
		 bool is_geq_upcall,
		 bool is_leq_upcall) const;
    
    /**
     * Initialize this entry with the current multicast forwarding information.
     * 
     * The current multicast forwarding bandwidth information is read from
     * the kernel.
     */
    void init_sg_count();
    
    /**
     * Test if the dataflow bandwidth satisfies the pre-defined condition.
     * 
     * The multicast forwarding bandwidth information is read from
     * the kernel, and then is tested whether is above/below the
     * pre-defined threshold.
     * 
     * @return true if the dataflow bandwidth satisifes the pre-defined
     * condition, otherwise false.
     * Note: if both "is_threshold_in_packets" and "is_threshold_in_bytes"
     * are true, then return true if the test is positive for either unit
     * (i.e., packets or bytes).
     */
    bool test_sg_count();
    
    /**
     * Start bandwidth measurement.
     */
    void start_measurement();
    
    /**
     * Send a dataflow signal that the pre-defined condition is true.
     */
    void dataflow_signal_send();
    
private:
    // Private state
    MfeaDfeLookup& _mfea_dfe_lookup;  // The Mfea dataflow lookup entry (yuck!)
    struct timeval _threshold_interval; // Measurements interval
    uint32_t	_threshold_packets;	// The threshold value (in packets)
    uint32_t	_threshold_bytes;	// The threshold value (in bytes)
    bool	_is_threshold_in_packets; // If true, _threshold_packets
					  // is valid
    bool	_is_threshold_in_bytes;	// If true, _threshold_bytes is valid
    bool	_is_geq_upcall;		// If true, the operation is ">=".
    bool	_is_leq_upcall;		// If true, the operation is "<=".
    
#define MFEA_DATAFLOW_TEST_FREQUENCY	4
    SgCount	_last_sg_count;		// Last measurement result
    SgCount	_measured_sg_count;	// Measured result
    SgCount	_delta_sg_count[MFEA_DATAFLOW_TEST_FREQUENCY];	// Delta measurement result
    size_t	_delta_sg_count_index; // Index into next '_delta_sg_count'
    bool	_is_bootstrap_completed;
    
    struct timeval _measurement_interval; // Interval between two measurements
    Timer	_measurement_timer;	// Timer to perform measurements
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __MFEA_MFEA_DATAFLOW_HH__
