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

#ifndef __OSPF_LSA_HH__
#define __OSPF_LSA_HH__

/**
 * Link State Advertisement (LSA)
 *
 * A generic LSA. All actual LSAs should be derived from this LSA.
 */

class Lsa {
 public:
    /**
     * Decode an LSA.
     */
    void virtual decode(uint8_t *buf, size_t len) = 0;

    /**
     * Encode an LSA for transmission.
     *
     * @param len length of the encoded packet.
     * 
     * @return A pointer that must be delete'd.
     */
    virtual uint8_t *encode(size_t &len) = 0;

    /**
     * Add the LSA type bindings.
     */
    void install_type(LsaType type, Lsa *lsa); 

 private:
    AckList _ack_list;		// List of ACKs received for this LSA.
    XorpTimer _retransmit;	// Retransmit timer.

    XorpTimer _timeout;		// Timeout this LSA.
};

typedef ref_ptr<Lsa> LsaRef;

class LsaTransmit : class Transmit {
 public:
    bool valid();

    bool multiple() { return false;}

    Transmit *clone();

    uint8_t *generate(size_t &len);
 private:
    LsaRef _lsaref;	// LSA.
}

#endif // __OSPF_LSA_HH__
