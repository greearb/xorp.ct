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
 * LSA Header. Common header for all LSAs.
 * Never store or pass a pointer, just deal with it inline.
 */
class Lsa_header {
 public:
    Lsa_header(OspfTypes::Version version) :
	_version(version), _LS_age(0), _options(0), _ls_type(0),
	_link_state_id(0), _advertising_router(0), _ls_sequence_number(0),
	_ls_checksum(0), _length(0)
    {}

    Lsa_header(const Lsa_header& rhs) {
	copy(rhs);
    }

    Lsa_header operator=(const Lsa_header& rhs) {
	if(&rhs == this)
	    return *this;
	copy(rhs);
	return *this;
    }

#define	lsa_copy(var)	var = rhs.var;

    void copy(const Lsa_header& rhs) {
	lsa_copy(_version);
	lsa_copy(_LS_age);
	lsa_copy(_options);
	lsa_copy(_ls_type);
	lsa_copy(_link_state_id);
	lsa_copy(_advertising_router);
	lsa_copy(_ls_sequence_number);
	lsa_copy(_ls_checksum);
	lsa_copy(_length);
    }
#undef	lsa_copy

    /**
     * @return the length of an LSA header.
     */
    static size_t length() { return 20; }

    /**
     * Decode a LSA header and return a LSA header inline not a pointer.
     */
    Lsa_header
    decode(uint8_t *ptr) throw(BadPacket);
    
    /**
     * Copy a wire format representation to the pointer provided.
     * @return the number of bytes written.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    OspfTypes::Version get_version() const {
	return _version;
    }

    // LS age
    void set_ls_age(uint16_t ls_age) {
	_LS_age = ls_age;
    }

    uint16_t get_ls_age() const {
	return _LS_age;
    }

    // Options
    void set_options(uint8_t options) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_options = options;
    }

    uint8_t get_options() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _options;
    }

    // LS type
    void set_ls_type(uint16_t ls_type) {
	switch(get_version()) {
	case OspfTypes::V2:
	    if (ls_type > 0xff)
		XLOG_WARNING("Attempt to set %#x in an 8 bit field",
			     ls_type);
	    _ls_type = ls_type & 0xff;
	    break;
	case OspfTypes::V3:
	    _ls_type = ls_type;
	    break;
	}
    }

    uint16_t get_ls_type() const {
	return _ls_type;
    }

    // Link State ID
    void set_link_state_id(uint32_t link_state_id) {
	_link_state_id = link_state_id;
    }

    uint32_t get_link_state_id() const {
	return _link_state_id;
    }
    
    // Advertising Router
    void set_advertising_router(uint32_t advertising_router) {
	_advertising_router = advertising_router;
    }

    uint32_t get_advertising_router() const {
	return _advertising_router;
    }

    // LS sequence number
    void set_ls_sequence_number(uint32_t ls_sequence_number) {
	_ls_sequence_number = ls_sequence_number;
    }

    uint32_t get_ls_sequence_number() const {
	return _ls_sequence_number;
    }

    // LS checksum
    void set_ls_checksum(uint16_t ls_checksum) {
	_ls_checksum = ls_checksum;
    }

    uint16_t get_ls_checksum() const {
	return _ls_checksum;
    }

    // Length
    void set_length(uint16_t length) {
	_length = length;
    }

    uint16_t get_length() const {
	return _length;
    }

    /**
     * Generate a printable representation of the header.
     */
    string str() const;

 private:
    OspfTypes::Version _version;
    uint16_t 	_LS_age;
    uint8_t	_options;	// OSPF V2 Only  
    uint16_t	_ls_type;	// OSPF V2 1 byte OSPF V3 2 bytes.
    uint32_t	_link_state_id;
    uint32_t	_advertising_router;
    uint32_t	_ls_sequence_number;
    uint16_t	_ls_checksum;
    uint16_t	_length;
};

/**
 * Link State Advertisement (LSA)
 *
 * A generic LSA. All actual LSAs should be derived from this LSA.
 */
class Lsa {
 public:
    /**
     * A reference counted pointer to an LSA which will be
     * automatically deleted.
     */
    typedef ref_ptr<Lsa> LsaRef;

    Lsa(OspfTypes::Version version)
	:  _header(version), _version(version)
    {}

    virtual ~Lsa();

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    virtual LsaRef decode(uint8_t *buf, size_t& len) = 0;

    /**
     * Encode an LSA for transmission.
     *
     * @param len length of the encoded packet.
     * 
     * @return A pointer that must be delete'd.
     */
//     virtual uint8_t *encode(size_t &len) = 0;

    /**
     * Add the LSA type bindings.
     */
//     void install_type(LsaType type, Lsa *lsa); 

 protected:
    Lsa_header _header;	// Common LSA header.
    uint8_t *ptr;	// Encoded or decoded packets are stored here.
    size_t _len;	// Length of the packet.

 private:
    const OspfTypes::Version 	_version;

//     AckList _ack_list;		// List of ACKs received for this LSA.
//     XorpTimer _retransmit;	// Retransmit timer.

//     XorpTimer _timeout;		// Timeout this LSA.
};

/**
 * Defines a link/interface, carried in a RouterLsa.
 */
class RouterLink {
 public:
    RouterLink(OspfTypes::Version version) : _version(version)
    {}
 private:
    const OspfTypes::Version 	_version;

    enum type_description {
	p2p = 1,	// Point-to-point connection to another router
	transit = 2,	//
    };

    uint16_t	_type;
    uint16_t	_metric;

    uint32_t	_link_id;		// OSPF V2 Only
    uint32_t	_link_data;		// OSPF V2 Only

    uint32_t	_interface_id;		// OSPF V3 Only
    uint32_t	_neighbour_interface_id;// OSPF V3 Only
    uint32_t	_neighbour_router_id;	// OSPF V3 Only
};

class RouterLsa : public Lsa {
 public:
    RouterLsa(OspfTypes::Version version)
	: Lsa(version)
    {}

 private:
    list<RouterLink> _router_links;
};

#if	0
class LsaTransmit : class Transmit {
 public:
    bool valid();

    bool multiple() { return false;}

    Transmit *clone();

    uint8_t *generate(size_t &len);
 private:
    LsaRef _lsaref;	// LSA.
}
#endif

/**
 * Link State Request as sent in a Link State Request Packet.
 * Never store or pass a pointer, just deal with it inline.
 */
class Ls_request {
 public:
    Ls_request(OspfTypes::Version version) :
	_version(version),  _ls_type(0),_link_state_id(0),
	_advertising_router(0)
    {}

    Ls_request(OspfTypes::Version version, uint32_t ls_type,
	       uint32_t link_state_id, uint32_t advertising_router) :
	_version(version),  _ls_type(ls_type),_link_state_id(link_state_id),
	_advertising_router(advertising_router)
    {}
    
    Ls_request(const Ls_request& rhs) {
	copy(rhs);
    }

    Ls_request operator=(const Ls_request& rhs) {
	if(&rhs == this)
	    return *this;
	copy(rhs);
	return *this;
    }

#define	ls_copy(var)	var = rhs.var;

    void copy(const Ls_request& rhs) {
	ls_copy(_version);
	ls_copy(_ls_type);
	ls_copy(_link_state_id);
	ls_copy(_advertising_router);
    }
#undef	ls_copy
    
    /**
     * @return the length of an link state request header.
     */
    static size_t length() { return 12; }
    
    /**
     * Decode a Link State Request and return value inline not a pointer.
     */
    Ls_request
    decode(uint8_t *ptr) throw(BadPacket);

    /**
     * Copy a wire format representation to the pointer provided.
     * @return the number of bytes written.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    OspfTypes::Version get_version() const {
	return _version;
    }
    
    // LS type
    void set_ls_type(uint32_t ls_type) {
	switch(get_version()) {
	case OspfTypes::V2:
	    _ls_type = ls_type;
	    break;
	case OspfTypes::V3:
	    if (ls_type > 0xffff)
		XLOG_WARNING("Attempt to set %#x in an 16 bit field",
			     ls_type);
	    _ls_type = ls_type & 0xffff;
	    break;
	}
    }

    uint32_t get_ls_type() const {
	return _ls_type;
    }

    // Link State ID
    void set_link_state_id(uint32_t link_state_id) {
	_link_state_id = link_state_id;
    }

    uint32_t get_link_state_id() const {
	return _link_state_id;
    }
    
    // Advertising Router
    void set_advertising_router(uint32_t advertising_router) {
	_advertising_router = advertising_router;
    }

    uint32_t get_advertising_router() const {
	return _advertising_router;
    }

    /**
     * Generate a printable representation of the request.
     */
    string str() const;
    
 private:
    OspfTypes::Version _version;
    uint32_t 	_ls_type;	// OSPF V2 4 bytes OSPF V3 2 bytes.
    uint32_t 	_link_state_id;
    uint32_t	_advertising_router;
};

#endif // __OSPF_LSA_HH__
