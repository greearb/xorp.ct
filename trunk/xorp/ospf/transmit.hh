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

// $XORP$

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

class Transmit {
 public:
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
    virtual Transmit* clone() = 0;

    /**
     * Generate a packet for transmission.
     *
     * @param len length of the encoded packet.
     * 
     * @return A pointer that must be delete'd.
     */
    virtual uint8_t *generate(size_t &len) = 0;
};

typedef ref_ptr<Transmit> TransmitRef;

#endif // __OSPF_TRANSMIT_HH__
