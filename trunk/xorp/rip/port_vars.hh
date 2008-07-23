// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/port_vars.hh,v 1.14 2008/01/04 03:17:31 pavlin Exp $

#ifndef __RIP_PORT_VARS_HH__
#define __RIP_PORT_VARS_HH__

#include "constants.hh"

/**
 * @short Container of counters associated with a Port.
 */
struct PortCounters {
public:
    PortCounters() : _packets_recv(0), _requests_recv(0), _updates_recv(0),
		     _bad_routes(0), _bad_packets(0),
		     _bad_auth_packets(0), _tr_sent(0), _tr_recv(),
		     _triggered_updates(0), _unsol_updates(0), _nr_req_recv(0),
		     _nr_updates(0)
    {}

    /**
     * Get the total number of packets received.
     */
    uint32_t packets_recv() const	{ return _packets_recv; }

    /**
     * Increment the total number of packets received.
     */
    void incr_packets_recv()		{ _packets_recv++; }

    /**
     * Get total number of update packets received.
     */
    uint32_t update_packets_recv() const { return _updates_recv; }

    /**
     * Increment total number of update packets received.
     */
    void incr_update_packets_recv()	{ _updates_recv++; }

    /**
     * Get the number of bad routes received (eg invalid metric,
     * invalid address family).
     */
    uint32_t bad_routes() const		{ return _bad_routes; }

    /**
     * Increment the number of bad routes received.
     */
    void incr_bad_routes()		{ _bad_routes++; }

    /**
     * Get the number of bad response packets received.
     */
    uint32_t bad_packets() const	{ return _bad_packets; }

    /**
     * Increment the number of bad response packets received.
     */
    void incr_bad_packets()		{ _bad_packets++; }

    /**
     * Get the number of authentication failing packets received.
     */
    uint32_t bad_auth_packets() const	{ return _bad_auth_packets; }

    /**
     * Increment the number of bad authentication packets received.
     */
    void incr_bad_auth_packets()	{ _bad_auth_packets++; }

    /**
     * Get the number of triggered updates sent.
     */
    uint32_t triggered_updates() const	{ return _triggered_updates; }

    /**
     * Increment the number of triggered updates sent.
     */
    void incr_triggered_updates() 	{ _triggered_updates++; }

    /**
     * Get the number of unsolicited updates sent.
     */
    uint32_t unsolicited_updates() const	{ return _unsol_updates; }

    /**
     * Increment the number of unsolicited updates sent.
     */
    void incr_unsolicited_updates()		{ _unsol_updates++; }

    /**
     * Get the number of table requests sent.
     */
    uint32_t table_requests_sent() const	{ return _tr_sent; }

    /**
     * Increment the number of table requests updates sent.
     */
    void incr_table_requests_sent()		{ _tr_sent++; }

    /**
     * Get the number of table requests received.
     */
    uint32_t table_requests_recv() const	{ return _tr_recv; }

    /**
     * Increment the number of table requests updates received.
     */
    void incr_table_requests_recv()		{ _tr_recv++; }

    /**
     * Get number of non-RIP request packets received.
     */
    uint32_t non_rip_requests_recv() const	{ return _nr_req_recv; }

    /**
     * Increment the number of non-RIP request packets received.
     */
    void incr_non_rip_requests_recv()		{ _nr_req_recv++; }

    /**
     * Get number of non-RIP update packets sent.
     */
    uint32_t non_rip_updates_sent() const	{ return _nr_updates; }

    /**
     * Increment the number of non-RIP request packets received.
     */
    void incr_non_rip_updates_sent()		{ _nr_updates++; }

protected:
    uint32_t _packets_recv;
    uint32_t _requests_recv;
    uint32_t _updates_recv;
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
    PortTimerConstants();

    /**
     * Set the route expiration time.
     * @param t the expiration time in seconds.
     * @return true on success.
     */
    bool set_expiry_secs(uint32_t t);

    /**
     * Get the route route expiration time.
     * @return expiry time in seconds.
     */
    uint32_t expiry_secs() const;

    /**
     * Set the route deletion time.
     * @param t the deletion time in seconds (must be >= 1).
     * @return true on success, false if t == 0.
     */
    bool set_deletion_secs(uint32_t t);

    /**
     * Get the route deletion time.
     * @return deletion time in seconds.
     */
    uint32_t deletion_secs() const;

    /**
     * Set request packet transmission period.  Request packets are only
     * sent when there are no peers associated with a port.
     * @param t inter-packet interval in seconds.
     * @return true on success.
     */
    bool set_table_request_period_secs(uint32_t t);

    /**
     * Set request packet transmission period.
     * @return inter-packet interval in seconds.
     */
    uint32_t table_request_period_secs() const;

    /**
     * Set unsolicitied response time.
     * @param t_secs unsolicited response time in seconds.
     * @return true on success.
     */
    bool set_update_interval(uint32_t t_secs);

    /**
     * Get unsolicitied response time.
     * @return unsolicited response time in seconds.
     */
    uint32_t update_interval();

    /**
     * Set unsolicitied response time jitter.
     * @param t_jitter unsolicited response time jitter
     * (in percents of the time period).
     * @return true on success.
     */
    bool set_update_jitter(uint32_t t_jitter);

    /**
     * Get unsolicitied response time jitter.
     * @return unsolicited response time jitter
     * (in percents of the time period).
     */
    uint32_t update_jitter();

    /**
     * Set the triggered update delay.
     * @param t_secs the triggered update delay in seconds.
     * @return true on success.
     */
    bool set_triggered_update_delay(uint32_t t_secs);

    /**
     * Get the triggered update delay.
     * @return the triggered update delay in seconds.
     */
    uint32_t triggered_update_delay() const;

    /**
     * Set the triggered update jitter.
     * @param t_jitter the triggered update jitter
     * (in percents of the time delay).
     * @return true on success.
     */
    bool set_triggered_update_jitter(uint32_t t_jitter);

    /**
     * Get the triggered update jitter.
     * @return the triggered update jitter (in percents of the time delay).
     */
    uint32_t triggered_update_jitter() const;

    /**
     * Set the interpacket packet delay.
     * @param t the interpacket delay for back-to-back packets in
     * milliseconds.
     * @return true on success, false if t is greater than
     * MAXIMUM_INTERPACKET_DELAY_MS.
     */
    bool set_interpacket_delay_ms(uint32_t t);

    /**
     * Get the interpacket packet delay in milliseconds.
     */
    uint32_t interpacket_delay_ms() const;

    /**
     * Set the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Queries arriving
     * at a faster rate are ignored.
     * @param t the interquery delay in milliseconds.
     * @return true on success.
     */
    bool set_interquery_delay_ms(uint32_t t);

    /**
     * Get the interquery gap.  This is the minimum temporal gap between
     * route request packets that query specific routes.  Fast arriving
     * queries are ignored.
     * @return the interquery delay in milliseconds.
     */
    uint32_t interquery_delay_ms() const;

protected:
    uint32_t _expiry_secs;
    uint32_t _deletion_secs;
    uint32_t _table_request_secs;
    uint32_t _update_interval;
    uint32_t _update_jitter;
    uint32_t _triggered_update_delay;
    uint32_t _triggered_update_jitter;
    uint32_t _interpacket_msecs;
    uint32_t _interquery_msecs;
};


// ----------------------------------------------------------------------------
// Inline PortTimerConstants accessor and modifiers.

inline
PortTimerConstants::PortTimerConstants()
    : _expiry_secs(DEFAULT_EXPIRY_SECS),
      _deletion_secs(DEFAULT_DELETION_SECS),
      _table_request_secs(DEFAULT_TABLE_REQUEST_SECS),
      _update_interval(DEFAULT_UPDATE_INTERVAL),
      _update_jitter(DEFAULT_UPDATE_JITTER),
      _triggered_update_delay(DEFAULT_TRIGGERED_UPDATE_DELAY),
      _triggered_update_jitter(DEFAULT_TRIGGERED_UPDATE_JITTER),
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
PortTimerConstants::set_update_interval(uint32_t t_secs)
{
    _update_interval = t_secs;
    return true;
}

inline uint32_t
PortTimerConstants::update_interval()
{
    return _update_interval;
}

inline bool
PortTimerConstants::set_update_jitter(uint32_t t_jitter)
{
    if (t_jitter > 100)
	return false;
    _update_jitter = t_jitter;
    return true;
}

inline uint32_t
PortTimerConstants::update_jitter()
{
    return _update_jitter;
}

inline bool
PortTimerConstants::set_table_request_period_secs(uint32_t t)
{
    //
    // XXX: value of 0 is accepted because it is used to disable the
    // periodic request messages.
    //
    _table_request_secs = t;
    return true;
}

inline uint32_t
PortTimerConstants::table_request_period_secs() const
{
    return _table_request_secs;
}

inline bool
PortTimerConstants::set_triggered_update_delay(uint32_t t_secs)
{
    _triggered_update_delay = t_secs;
    return true;
}

inline uint32_t
PortTimerConstants::triggered_update_delay() const
{
    return _triggered_update_delay;
}

inline bool
PortTimerConstants::set_triggered_update_jitter(uint32_t t_jitter)
{
    if (t_jitter > 100)
	return false;
    _triggered_update_jitter = t_jitter;
    return true;
}

inline uint32_t
PortTimerConstants::triggered_update_jitter() const
{
    return _triggered_update_jitter;
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
