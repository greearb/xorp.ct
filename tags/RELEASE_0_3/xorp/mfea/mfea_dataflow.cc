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

#ident "$XORP: xorp/mfea/mfea_dataflow.cc,v 1.9 2003/04/22 23:27:20 hodson Exp $"


//
// MFEA (Multicast Forwarding Engine Abstraction) dataflow implementation.
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "libxorp/utils.hh"
#include "mfea_dataflow.hh"
#include "mfea_node.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


MfeaDft::MfeaDft(MfeaNode& mfea_node)
    : _mfea_node(mfea_node)
{
    
}

MfeaDft::~MfeaDft()
{
    
}

int
MfeaDft::family() const
{
    return (_mfea_node.family());
}

int
MfeaDft::add_entry(const IPvX& source, const IPvX& group,
		   const TimeVal& threshold_interval,
		   uint32_t threshold_packets,
		   uint32_t threshold_bytes,
		   bool is_threshold_in_packets,
		   bool is_threshold_in_bytes,
		   bool is_geq_upcall,
		   bool is_leq_upcall)
{
    MfeaDfe *mfea_dfe;
    MfeaDfeLookup *mfea_dfe_lookup;
    
    mfea_dfe_lookup = find(source, group);
    
    if (mfea_dfe_lookup == NULL) {
	// Create and add a new dataflow lookup entry
	mfea_dfe_lookup = new MfeaDfeLookup(*this, source, group);
	insert(mfea_dfe_lookup);
    }
    
    // Search for a dataflow entry.
    mfea_dfe = mfea_dfe_lookup->find(threshold_interval,
				     threshold_packets,
				     threshold_bytes,
				     is_threshold_in_packets,
				     is_threshold_in_bytes,
				     is_geq_upcall,
				     is_leq_upcall);
    if (mfea_dfe != NULL) {
	// Already have this entry
	return (XORP_OK);
    }
    
    // Create a new entry
    mfea_dfe = new MfeaDfe(*mfea_dfe_lookup,
			   threshold_interval,
			   threshold_packets,
			   threshold_bytes,
			   is_threshold_in_packets,
			   is_threshold_in_bytes,
			   is_geq_upcall,
			   is_leq_upcall);
    mfea_dfe->init_sg_count();
    if (! mfea_dfe->is_valid()) {
	delete mfea_dfe;
	if (mfea_dfe_lookup->is_empty()) {
	    remove(mfea_dfe_lookup);
	    delete mfea_dfe_lookup;
	}
	return (XORP_ERROR);
    }
    
    mfea_dfe_lookup->insert(mfea_dfe);
    mfea_dfe->start_measurement();
    
    return (XORP_OK);
}

int
MfeaDft::delete_entry(const IPvX& source, const IPvX& group,
		      const TimeVal& threshold_interval,
		      uint32_t threshold_packets,
		      uint32_t threshold_bytes,
		      bool is_threshold_in_packets,
		      bool is_threshold_in_bytes,
		      bool is_geq_upcall,
		      bool is_leq_upcall)
{
    MfeaDfe *mfea_dfe;
    MfeaDfeLookup *mfea_dfe_lookup;
    
    mfea_dfe_lookup = find(source, group);
    
    if (mfea_dfe_lookup == NULL)
	return (XORP_ERROR);	// Not found
    
    // Search for a dataflow entry.
    mfea_dfe = mfea_dfe_lookup->find(threshold_interval,
				     threshold_packets,
				     threshold_bytes,
				     is_threshold_in_packets,
				     is_threshold_in_bytes,
				     is_geq_upcall,
				     is_leq_upcall);
    if (mfea_dfe == NULL)
	return (XORP_ERROR);	// Not found
    
    if (delete_entry(mfea_dfe) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
MfeaDft::delete_entry(const IPvX& source, const IPvX& group)
{
    MfeaDfeLookup *mfea_dfe_lookup;
    
    mfea_dfe_lookup = find(source, group);
    if (mfea_dfe_lookup == NULL)
	return (XORP_ERROR);	// Nothing found
    
    remove(mfea_dfe_lookup);
    delete mfea_dfe_lookup;
    
    return (XORP_OK);
}

int
MfeaDft::delete_entry(MfeaDfe *mfea_dfe)
{
    MfeaDfeLookup *mfea_dfe_lookup = &mfea_dfe->mfea_dfe_lookup();
    
    mfea_dfe_lookup->remove(mfea_dfe);
    delete mfea_dfe;
    
    if (mfea_dfe_lookup->is_empty()) {
	remove(mfea_dfe_lookup);
	delete mfea_dfe_lookup;
    }
    
    return (XORP_OK);
}

MfeaDfeLookup::MfeaDfeLookup(MfeaDft& mfea_dft,
			     const IPvX& source, const IPvX& group)
    : Mre<MfeaDfeLookup>(source, group),
    _mfea_dft(mfea_dft)
{
    
}

MfeaDfeLookup::~MfeaDfeLookup()
{
    delete_pointers_list(_mfea_dfe_list);
}

int
MfeaDfeLookup::family() const
{
    return (_mfea_dft.family());
}

MfeaDfe *
MfeaDfeLookup::find(const TimeVal& threshold_interval,
		    uint32_t threshold_packets,
		    uint32_t threshold_bytes,
		    bool is_threshold_in_packets,
		    bool is_threshold_in_bytes,
		    bool is_geq_upcall,
		    bool is_leq_upcall)
{
    list<MfeaDfe *>::const_iterator iter;
    
    for (iter = _mfea_dfe_list.begin(); iter != _mfea_dfe_list.end(); ++iter) {
	MfeaDfe *mfea_dfe = *iter;
	if (mfea_dfe->is_same(threshold_interval,
			      threshold_packets,
			      threshold_bytes,
			      is_threshold_in_packets,
			      is_threshold_in_bytes,
			      is_geq_upcall,
			      is_leq_upcall))
	    return (mfea_dfe);
    }
    
    return (NULL);
}

void
MfeaDfeLookup::insert(MfeaDfe *mfea_dfe)
{
    _mfea_dfe_list.push_back(mfea_dfe);
}

void
MfeaDfeLookup::remove(MfeaDfe *mfea_dfe)
{
    // XXX: presumably, the list will be very short, so for simplicity
    // we use the _theoretically_ less-efficient remove() instead of find()
    _mfea_dfe_list.remove(mfea_dfe);
}

MfeaDfe::MfeaDfe(MfeaDfeLookup& mfea_dfe_lookup,
		 const TimeVal& threshold_interval,
		 uint32_t threshold_packets,
		 uint32_t threshold_bytes,
		 bool is_threshold_in_packets,
		 bool is_threshold_in_bytes,
		 bool is_geq_upcall,
		 bool is_leq_upcall)
    : _mfea_dfe_lookup(mfea_dfe_lookup),
      _threshold_interval(threshold_interval),
      _threshold_packets(threshold_packets),
      _threshold_bytes(threshold_bytes),
      _is_threshold_in_packets(is_threshold_in_packets),
      _is_threshold_in_bytes(is_threshold_in_bytes),
      _is_geq_upcall(is_geq_upcall),
      _is_leq_upcall(is_leq_upcall)
{
    _delta_sg_count_index = 0;
    _is_bootstrap_completed = false;
    _measurement_interval = _threshold_interval / MFEA_DATAFLOW_TEST_FREQUENCY;
    for (size_t i = 0; i < sizeof(_start_time)/sizeof(_start_time[0]); i++)
	_start_time[i] = TimeVal::ZERO();
}

MfeaDfe::~MfeaDfe()
{
    
}

MfeaDft&
MfeaDfe::mfea_dft() const
{
    return (_mfea_dfe_lookup.mfea_dft());
}

EventLoop&
MfeaDfe::eventloop() const
{
    return (mfea_dft().mfea_node().eventloop());
}

int
MfeaDfe::family() const
{
    return (_mfea_dfe_lookup.family());
}

bool
MfeaDfe::is_valid() const
{
    int min_sec = 3;		// XXX: the minimum threshold interval value
    int min_usec = 0;
    
    return ((_is_threshold_in_packets || _is_threshold_in_bytes)
	    && (_is_geq_upcall ^ _is_leq_upcall)
	    && (_threshold_interval >= TimeVal(min_sec, min_usec))
	    && _last_sg_count.is_valid());
}

bool
MfeaDfe::is_same(const TimeVal& threshold_interval_test,
		 uint32_t threshold_packets_test,
		 uint32_t threshold_bytes_test,
		 bool is_threshold_in_packets_test,
		 bool is_threshold_in_bytes_test,
		 bool is_geq_upcall_test,
		 bool is_leq_upcall_test) const
{
    if (is_threshold_in_packets_test)
	if (threshold_packets_test != _threshold_packets)
	    return (false);
    if (is_threshold_in_bytes_test)
	if (threshold_bytes_test != _threshold_bytes)
	    return (false);
    
    return ((threshold_interval_test == _threshold_interval)
	    && (is_threshold_in_packets_test == _is_threshold_in_packets)
	    && (is_threshold_in_bytes_test == _is_threshold_in_bytes)
	    && (is_geq_upcall_test == _is_geq_upcall)
	    && (is_leq_upcall_test == _is_leq_upcall));
}

void
MfeaDfe::init_sg_count()
{
    mfea_dft().mfea_node().get_sg_count(source_addr(), group_addr(),
					_last_sg_count);
}

//
// Read the count from the kernel, and test if it is above/below the threshold.
// XXX: if both packets and bytes are enabled, then return true if the test
// is positive for either.
//
bool
MfeaDfe::test_sg_count()
{
    SgCount saved_last_sg_count = _last_sg_count;
    uint32_t diff_value, threshold_value;
    bool ret_value = false;
    
    //
    // Perform the measurement
    //
    if (mfea_dft().mfea_node().get_sg_count(source_addr(), group_addr(),
					    _last_sg_count)
	< 0) {
	// Error
	return (false);		// TODO: what do we do when error occured?
    }
    
    //
    // Compute the delta since the last measurement
    //
    if ((_is_threshold_in_packets
	 && (saved_last_sg_count.pktcnt() > _last_sg_count.pktcnt()))
	|| (_is_threshold_in_bytes
	    && (saved_last_sg_count.bytecnt() > _last_sg_count.bytecnt()))) {
	// XXX: very likely the counter has round-up. We can try to be
	// smart and compute the difference between the old value and
	// the maximum possible value, and then just add that difference
	// to the new value.
	// However, the maximum possible value depends on the original size
	// of the counter. In case FreeBSD-4.5 it is u_long for IPv4,
	// but u_quad_t for IPv6.
	// Hence, we just ignore this measurement... Sigh...
	_delta_sg_count[_delta_sg_count_index].reset();
	ret_value = false;
	goto ret_label;
    }
    
    _delta_sg_count[_delta_sg_count_index] = _last_sg_count;
    _delta_sg_count[_delta_sg_count_index] -= saved_last_sg_count;
    
    //
    // Increment the counter to point to the next entry (to be used next time)
    //
    _delta_sg_count_index++;
    if (_delta_sg_count_index >= MFEA_DATAFLOW_TEST_FREQUENCY) {
	_delta_sg_count_index %= MFEA_DATAFLOW_TEST_FREQUENCY;
	_is_bootstrap_completed = true;
    }
    
    //
    // Compute the difference for the last threshold interval
    //
    _measured_sg_count.reset();
    if (_is_bootstrap_completed) {
	for (size_t i = 0; i < MFEA_DATAFLOW_TEST_FREQUENCY; i++) {
	    _measured_sg_count += _delta_sg_count[i];
	}
    } else {
	for (size_t i = 0; i < _delta_sg_count_index; i++) {
	    _measured_sg_count += _delta_sg_count[i];
	}
    }
    
    if (_is_threshold_in_packets) {
	threshold_value = _threshold_packets;
	diff_value = _measured_sg_count.pktcnt();
	
	if (_is_geq_upcall) {
	    if (diff_value >= threshold_value) {
		ret_value = true;
		goto ret_label;
	    }
	}
	if (_is_leq_upcall && _is_bootstrap_completed) {
	    if (diff_value <= threshold_value) {
		ret_value = true;
		goto ret_label;
	    }
	}
    }
    
    if (_is_threshold_in_bytes) {
	threshold_value = _threshold_bytes;
	diff_value = _measured_sg_count.bytecnt();
	
	if (_is_geq_upcall) {
	    if (diff_value >= threshold_value) {
		ret_value = true;
		goto ret_label;
	    }
	}
	if (_is_leq_upcall && _is_bootstrap_completed) {
	    if (diff_value <= threshold_value) {
		ret_value = true;
		goto ret_label;
	    }
	}
    }

 ret_label:
    return (ret_value);
}

void
MfeaDfe::start_measurement()
{
    _measurement_timer =
	eventloop().new_oneoff_after(_measurement_interval,
				     callback(this,
					      &MfeaDfe::measurement_timer_timeout));
    
    TimeVal now;
    
    mfea_dft().mfea_node().eventloop().current_time(now);
    _start_time[_delta_sg_count_index] = now;
}

void
MfeaDfe::dataflow_signal_send()
{
    // XXX: for simplicity, we assume that the threshold interval
    // is same as the measured interval
    mfea_dft().mfea_node().signal_dataflow_message_recv(
	source_addr(),
	group_addr(),
	_threshold_interval,
	_threshold_interval,
	_threshold_packets,
	_threshold_bytes,
	_measured_sg_count.pktcnt(),
	_measured_sg_count.bytecnt(),
	_is_threshold_in_packets,
	_is_threshold_in_bytes,
	_is_geq_upcall,
	_is_leq_upcall);
}

const TimeVal&
MfeaDfe::start_time() const
{
    if (! _is_bootstrap_completed)
	return (_start_time[0]);
    
    return (_start_time[_delta_sg_count_index]);
}

uint32_t
MfeaDfe::measured_packets() const
{
    SgCount result;
    
    if (_is_bootstrap_completed) {
	for (size_t i = 0; i < MFEA_DATAFLOW_TEST_FREQUENCY; i++) {
	    result += _delta_sg_count[i];
	}
    } else {
	for (size_t i = 0; i < _delta_sg_count_index; i++) {
	    result += _delta_sg_count[i];
	}
    }
    
    return (result.pktcnt());
}

uint32_t
MfeaDfe::measured_bytes() const
{
    SgCount result;
    
    if (_is_bootstrap_completed) {
	for (size_t i = 0; i < MFEA_DATAFLOW_TEST_FREQUENCY; i++) {
	    result += _delta_sg_count[i];
	}
    } else {
	for (size_t i = 0; i < _delta_sg_count_index; i++) {
	    result += _delta_sg_count[i];
	}
    }
    
    return (result.bytecnt());
}

void
MfeaDfe::measurement_timer_timeout()
{
    if (test_sg_count()) {
	// Time to deliver a signal
	dataflow_signal_send();
    }
    
    // Restart the measurements
    start_measurement();
}
