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
    Lsa_header decode(uint8_t *ptr) const throw(BadPacket);

    /**
     * Decode this lsa header in this context.
     */
    void decode_inline(uint8_t *ptr) throw(BadPacket);
    
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
    void decode(Lsa_header& header, uint8_t *ptr) const throw(BadPacket);

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

    /**
     * Note passing in the LSA buffer does not imply that it will be
     * decoded. The buffer is just being stored.
     */
    Lsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	:  _header(version), _version(version)
    {
	_pkt.resize(len);
	memcpy(&_pkt[0], buf, len);
    }

    virtual ~Lsa()
    {}

    OspfTypes::Version get_version() const {
	return _version;
    }

    /**
     * It is the responsibilty of the derived type to return this
     * information.
     * @return The type this lsa represents.
     */
    virtual uint16_t get_lsa_type() const = 0;

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    virtual LsaRef decode(uint8_t *buf, size_t& len) const 
	throw(BadPacket) = 0;

    /**
     * Encode an LSA for transmission.
     *
     * @return True on success.
     */
    virtual bool encode() = 0;

    /**
     * Get a reference to the raw LSA
     *
     * @param len The length of the LSA.
     *
     * @return pointer to start of the LSA.
     */
    uint8_t *lsa(size_t &len) {
	len = _pkt.size();
	return &_pkt[0];
    }

    /**
     * Add the LSA type bindings.
     */
//     void install_type(LsaType type, Lsa *lsa); 
    
 protected:
    Lsa_header _header;		// Common LSA header.
    vector<uint8_t> _pkt;	// Raw LSA.

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
    enum type {
	p2p = 1,	// Point-to-point connection to another router
	transit = 2,	// Connection to a transit network
	stub = 3,	// Connection to a stub network OSPF V2 only
	vlink = 4	// Virtual link
    };

    RouterLink(OspfTypes::Version version) : _version(version)
    {}

    RouterLink(const RouterLink& rhs) : _version(rhs._version) {
	copy(rhs);
    }

    RouterLink operator=(const RouterLink& rhs) {
	if(&rhs == this)
	    return *this;
	copy(rhs);
	return *this;
    }

#define	routerlink_copy(var)	var = rhs.var;

    void copy(const RouterLink& rhs) {
	routerlink_copy(_type);
	routerlink_copy(_metric);
	switch (get_version()) {
	case OspfTypes::V2:
	    routerlink_copy(_link_id);
	    routerlink_copy(_link_data);
	    break;
	case OspfTypes::V3:
	    routerlink_copy(_interface_id);
	    routerlink_copy(_neighbour_interface_id);
	    routerlink_copy(_neighbour_router_id);
	    break;
	}
    }
#undef	routerlink_copy
    
    /**
     * @return the number of bytes the encoded data will occupy.
     */
    size_t length() const;

     /**
     * Decode a RouterLink.
     *
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A RouterLink.
     */
    RouterLink
    decode(uint8_t *ptr, size_t& len) throw(BadPacket);
    
    /**
     * Copy a wire format representation to the pointer provided.
     *
     * length() should be called by the caller to verify enough space
     * is available.
     * @return the number of bytes written.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    OspfTypes::Version get_version() const {
	return _version;
    }

    // Type
    void set_type(type t) {
	if (stub == t)
	    XLOG_ASSERT(OspfTypes::V2 == get_version());
	_type = t;
    }

    type get_type() const {
	return _type;
    }

    // Metric
    void set_metric(uint16_t metric) {
	_metric = metric;
    }

    uint16_t get_metric() const {
	return _metric;
    }

    // Link ID
    void set_link_id(uint32_t link_id) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_link_id = link_id;
    }

    uint32_t get_link_id() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _link_id;
    }

    // Link Data
    void set_link_data(uint32_t link_data) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_link_data = link_data;
    }

    uint32_t get_link_data() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _link_data;
    }

    // Interface ID
    void set_interface_id(uint32_t interface_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_interface_id = interface_id;
    }

    uint32_t get_interface_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _interface_id;
    }

    // Neighbour Interface ID
    void set_neighbour_interface_id(uint32_t neighbour_interface_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_neighbour_interface_id = neighbour_interface_id;
    }

    uint32_t get_neighbour_interface_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _neighbour_interface_id;
    }

    // Neighbour Router ID
    void set_neighbour_router_id(uint32_t neighbour_router_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_neighbour_router_id = neighbour_router_id;
    }

    uint32_t get_neighbour_router_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _neighbour_router_id;
    }

    /**
     * Generate a printable representation of the header.
     */
    string str() const;

 private:
    const OspfTypes::Version 	_version;

    type	_type;
    uint16_t	_metric;		// Only store TOS 0 metric

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

    RouterLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {}

    /**
     * @return the minimum length of a RouterLSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 20;
	    break;
	case OspfTypes::V3:
	    return 24;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_lsa_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 1;
	    break;
	case OspfTypes::V3:
	    return 0x2001;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    LsaRef decode(uint8_t *buf, size_t& len) const throw(BadPacket);

    bool encode();

    // Wildcard multicast receiver! OSPFV3 Only
    void set_w_bit(bool bit) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_w_bit = bit;
    }

    bool get_w_bit() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _w_bit;
    }

    // Virtual link endpoint
    void set_v_bit(bool bit) {
	_v_bit = bit;
    }

    bool get_v_bit() const {
	return _v_bit;
    }

    // AS boundary router (E for external)
    void set_e_bit(bool bit) {
	_e_bit = bit;
    }

    bool get_e_bit() const {
	return _e_bit;
    }

    // Area border router.
    void set_b_bit(bool bit) {
	_b_bit = bit;
    }

    bool get_b_bit() const {
	return _b_bit;
    }

    void set_options(uint32_t options) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (options  > 0xffffff)
	    XLOG_WARNING("Attempt to set %#x in a 24 bit field", options);
	_options = options & 0xffffff;
    }

    uint32_t get_options() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _options;
    }

    list<RouterLink>& get_router_links() {
	return _router_links;
    }

 private:
    bool _w_bit;	// Wildcard multicast receiver! OSPFV3 Only
    bool _v_bit;	// Virtual link endpoint
    bool _e_bit;	// AS boundary router (E for external)
    bool _b_bit;	// Area border router.

    uint32_t _options;	// OSPF V3 only.

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
