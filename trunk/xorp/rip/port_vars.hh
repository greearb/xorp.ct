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

// $XORP: xorp/rip/port_vars.hh,v 1.3 2004/03/02 18:02:34 hodson Exp $

#ifndef __RIP_PORT_VARS_HH__
#define __RIP_PORT_VARS_HH__

#include "constants.hh"

/**
 * @short Container of counters associated with a Port.
 */
struct PortCounters {
public:
    PortCounters() : _packets_recv(0), _bad_routes(0), _bad_packets(0),
		     _bad_auth_packets(0), _tr_sent(0), _tr_recv(),
		     _triggered_updates(0), _unsol_updates(0), _nr_req_recv(0),
		     _nr_updates(0)
    {}

    /**
     * Get the total number of packets received.
     */
    inline uint32_t packets_recv() const	{ return _packets_recv; }

    /**
     * Increment the total number of packets received.
     */
    inline void incr_packets_recv()		{ _packets_recv++; }

    /**
     * Get the number of bad routes received (eg invalid metric,
     * invalid address family).
     */
    inline uint32_t bad_routes() const		{ return _bad_routes; }

    /**
     * Increment the number of bad routes received.
     */
    inline void incr_bad_routes()		{ _bad_routes++; }

    /**
     * Get the number of bad response packets received.
     */
    inline uint32_t bad_packets() const		{ return _bad_packets; }

    /**
     * Increment the number of bad response packets received.
     */
    inline void incr_bad_packets()		{ _bad_packets++; }

    /**
     * Get the number of authentication failing packets received.
     */
    inline uint32_t bad_auth_packets() const	{ return _bad_auth_packets; }

    /**
     * Increment the number of bad authentication packets received.
     */
    inline void incr_bad_auth_packets()		{ _bad_auth_packets++; }

    /**
     * Get the number of triggered updates sent.
     */
    inline uint32_t triggered_updates() const	{ return _triggered_updates; }

    /**
     * Increment the number of triggered updates sent.
     */
    inline void incr_triggered_updates() 	{ _triggered_updates++; }

    /**
     * Get the number of unsolicited updates sent.
     */
    inline uint32_t unsolicited_updates() const	{ return _unsol_updates; }

    /**
     * Increment the number of unsolicited updates sent.
     */
    inline void incr_unsolicited_updates() 	{ _unsol_updates++; }

    /**
     * Get the number of table requests sent.
     */
    inline uint32_t table_requests_sent() const	{ return _tr_sent; }

    /**
     * Increment the number of table requests updates sent.
     */
    inline void incr_table_requests_sent() 	{ _tr_sent++; }

    /**
     * Get the number of table requests received.
     */
    inline uint32_t table_requests_recv() const	{ return _tr_recv; }

    /**
     * Increment the number of table requests updates received.
     */
    inline void incr_table_requests_recv() 	{ _tr_recv++; }

    /**
     * Get number of non-RIP request packets received.
     */
    inline uint32_t non_rip_requests_recv() const { return _nr_req_recv; }

    /**
     * Increment the number of non-RIP request packets received.
     */
    inline void incr_non_rip_requests_recv() { _nr_req_recv++; }

    /**
     * Get number of non-RIP update packets sent.
     */
    inline uint32_t non_rip_updates_sent() const { return _nr_updates; }

    /**
     * Increment the number of non-RIP request packets received.
     */
    inline void incr_non_rip_updates_sent() { _nr_updates++; }

protected:
    uint32_t _packets_recv;
    uint32_t _bad_routes;
    uint32_t _bad_packets;
    uint32_t _bad_auth_packets;
    uint32_t _tr_sent;
    uint32_t _tr_recv;
    uint32_t _triggered_updates;
    uint32_t _unsol_updates;
    uint32_t _nr_req_recv;
    uint32_t _nr_updates;
};


/**
 * @short Container of timer constants associated with a RIP port.
 */
class PortTimerConstants {
public:
    /**
     * Initialize contants with default values from RIPv2 spec.  The values
     * are defined in constants.hh.
     */
    inline PortTimerConstants();

    /**
     * Set the route expiration time.
     * @param t the expiration time in seconds.
     * @return true on success.
     */
    inline bool set_expiry_secs(uint32_t t);

    /**
     * Get the route route expiration time.
     * @return expiry time in seconds.
     */
    inline uint32_t expiry_secs() const;

    /**
     * Set the route deletion time.
     * @param t the deletion time in seconds (must be >= 1).
     * @return true on success, false if t == 0.
     */
    inline bool set_deletion_secs(uint32_t t);

    /**
     * Get the route deletion time.
     * @return deletion time in seconds.
     */
    inline uint32_t deletion_secs() const;

    /**
     * Set request packet transmission period.  Request packets are only
     * sent when there are no peers associated with a port.
     * @param t inter-packet interval in seconds.
     * @return true on success.
     */
    inline bool set_table_request_period_secs(uint32_t t);

    /**
     * Set request packet transmission period.
     * @return inter-packet interval in seconds.
     */
    inline uint32_t table_request_period_secs() const;

    /**
     * Set minimum unsolicitied response time.
     * @param t minimum unsolicited response time in seconds.
     * @return true on success.
     */
    inline bool set_unsolicited_response_min_secs(uint32_t t);

    /**
     * Get minimum unsolicitied response time.
     * @return minimum unsolicited response time in seconds.
     */
    inline uint32_t unsolicited_response_min_secs();

    /**
     * Set maximum unsolicitied response time.
     * @param t maximum unsolicited response time in seconds.
     * @return true on success.
     */
    inline bool set_unsolicited_response_max_secs(uint32_t t);

    /**
     * Get maximum unsolicitied response time.
     * @return maximum unsolicited response time in seconds.
     */
    inline uint32_t unsolicited_response_max_secs();

    /**
     * Set the lower bound of the triggered update interval.
     * @param t the lower bound of the triggered update interval in seconds.
     * @return true on success.
     */
    inline bool set_triggered_update_min_wait_secs(uint32_t t);

    /**
     * Get the lower bound of the triggered update interval.
     * @return the lower bound of the triggered update interval in seconds.
     */
    inline uint32_t triggered_update_min_wait_secs() const;

    /**
     * Set the upper bound of the triggered update interval.
     * @param t the upper bound of the triggered update interval in seconds.
     * @return true on success.
     */
    inline bool set_triggered_update_max_wait_secs(uint32_t t);

    /**
     * Get the upper bound of the triggered update interval.
     * @return the upper bound of the triggered update interval in seconds.
     */
    inline uint32_t triggered_update_max_wait_secs() const;

    /**
     * Set the interpacket packet delay.
     * @param t the interpacket delay for back-to-back packets in
     * milliseconds.
     * @return true on success, false if t is greater than
     * MAXIMUM_INTERPACKET_DELAY_MS.
     */
    inline bool	set_interpacket_delay_ms(uint32_t t);

    /**
     * Get the interpacket packet delay in milliseconds.
     */
    inline uint32_t interpacket_delay_ms() const;

    /**
     * Set the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Queries arriving
     * at a faster rate are ignored.
     * @param t the interquery delay in milliseconds.
     * @return true on success.
     */
    inline bool	set_interquery_delay_ms(uint32_t t);

    /**
     * Get the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Fast arriving
     * queries are ignored.
     * @return the interquery delay in milliseconds.
     */
    inline uint32_t interquery_delay_ms() const;

protected:
    uint32_t _expiry_secs;
    uint32_t _deletion_secs;
    uint32_t _table_request_secs;
    uint32_t _unsolicited_response_min_secs;
    uint32_t _unsolicited_response_max_secs;
    uint32_t _triggered_update_min_wait_secs;
    uint32_t _triggered_update_max_wait_secs;
    uint32_t _interpacket_msecs;
    uint32_t _interquery_msecs;
};


// ----------------------------------------------------------------------------
// Inline PortTimerConstants accessor and modifiers.

PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _table_request_secs(DEFAULT_TABLE_REQUEST_SECS),
      _unsolicited_response_min_secs(DEFAULT_UNSOLICITED_RESPONSE_SECS - DEFAULT_VARIATION_UNSOLICITED_RESPONSE_SECS),
      _unsolicited_response_max_secs(DEFAULT_UNSOLICITED_RESPONSE_SECS + DEFAULT_VARIATION_UNSOLICITED_RESPONSE_SECS),
      _triggered_update_min_wait_secs(DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS),
      _triggered_update_max_wait_secs(DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS),
      _interpacket_msecs(DEFAULT_INTERPACKET_DELAY_MS),
      _interquery_msecs(DEFAULT_INTERQUERY_GAP_MS)
{
}

inline bool
PortTimerConstants::set_expiry_secs(uint32_t t)
{
    if (t == 0)
	return false;
    _expiry_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::expiry_secs() const
{
    return _expiry_secs;
}

inline bool
PortTimerConstants::set_deletion_secs(uint32_t t)
{
    if (t == 0)
	return false;
    _deletion_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::deletion_secs() const
{
    return _deletion_secs;
}

inline bool
PortTimerConstants::set_unsolicited_response_min_secs(uint32_t t)
{
    _unsolicited_response_min_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::unsolicited_response_min_secs()
{
    return _unsolicited_response_min_secs;
}

inline bool
PortTimerConstants::set_unsolicited_response_max_secs(uint32_t t)
{
    _unsolicited_response_max_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::unsolicited_response_max_secs()
{
    return _unsolicited_response_max_secs;
}

inline bool
PortTimerConstants::set_table_request_period_secs(uint32_t t)
{
    if (t == 0)
	return false;
    _table_request_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::table_request_period_secs() const
{
    return _table_request_secs;
}

inline bool
PortTimerConstants::set_triggered_update_min_wait_secs(uint32_t t)
{
    _triggered_update_min_wait_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::triggered_update_min_wait_secs() const
{
    return _triggered_update_min_wait_secs;
}

inline bool
PortTimerConstants::set_triggered_update_max_wait_secs(uint32_t t)
{
    if (t < triggered_update_min_wait_secs())
	return false;
    _triggered_update_max_wait_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::triggered_update_max_wait_secs() const
{
    return _triggered_update_max_wait_secs;
}

inline bool
PortTimerConstants::set_interpacket_delay_ms(uint32_t t)
{
    if (t > MAXIMUM_INTERPACKET_DELAY_MS)
	return false;
    _interpacket_msecs = t;
    return true;
}

inline uint32_t
PortTimerConstants::interpacket_delay_ms() const
{
    return _interpacket_msecs;
}

inline bool
PortTimerConstants::set_interquery_delay_ms(uint32_t t)
{
    _interquery_msecs = t;
    return true;
}

inline uint32_t
PortTimerConstants::interquery_delay_ms() const
{
    return _interquery_msecs;
}

#endif // __RIP_PORT_VARS_HH__
