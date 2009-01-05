// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/ospf/transmit.hh,v 1.8 2008/10/02 21:57:50 bms Exp $

#ifndef __OSPF_TRANSMIT_HH__
#define __OSPF_TRANSMIT_HH__

/**
 * The base class for all transmissions.
 *
 * Typically a large number of Transmit objects could be queued for
 * transmission. By the time a Transmit object is scheduled for
 * transmission circumstances could have changed. It may no longer be
 * appropriate to transmit this packet. Therefore allow a transmit
 * object to become invalid. A single transmit object can generate a
 * number of packets. For example for the database synchronisation
 * process.
 *
 */

template <typename A>
class Transmit {
 public:
    typedef ref_ptr<Transmit> TransmitRef;

    virtual ~Transmit()
    {}

    /**
     * Is this object still valid?
     *
     * @return True if this transmit object is still valid.
     */
    virtual bool valid() = 0;

    /**
     * A transmit object may be able to generate multiple packets; not
     * just one.
     *
     * @return True if this object can be invoked multiple times?
     */
    virtual bool multiple() = 0;

    /**
     * Make a copy of this object. If the same data is being sent to
     * multiple locations, provide a mechanism to make a copy for each
     * location.
     */
    virtual TransmitRef clone() = 0;

    /**
     * Generate a packet for transmission.
     *
     * @param len length of the encoded packet.
     * 
     * @return A pointer that must be delete'd.
     */
    virtual uint8_t *generate(size_t &len) = 0;

    /**
     * @return the destination address of this packet.
     */
    virtual A destination() = 0;

    /**
     * @return the source address of this packet.
     */
    virtual A source() = 0;
};

/**
 * A transmit object that sends fixed data.
 */
template <typename A>
class SimpleTransmit : public Transmit<A> {
 public:
    SimpleTransmit(vector<uint8_t>& pkt, A dst, A src) : _dst(dst), _src(src)
    {
	_pkt.resize(pkt.size());
	memcpy(&_pkt[0], &pkt[0], pkt.size());
    }

    bool valid() { return true; }
    bool multiple() { return false; }
    typename Transmit<A>::TransmitRef clone() { return this; }
    uint8_t *generate(size_t &len) {
	len = _pkt.size();
	return &_pkt[0];
    }
    A destination() { return _dst; }
    A source() { return _src; }

 private:
    vector<uint8_t> _pkt;
    A _dst;	// Destination address
    A _src;	// Source address
};
#endif // __OSPF_TRANSMIT_HH__
