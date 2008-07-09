// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/ospf/lsa.hh,v 1.110 2008/01/04 03:16:56 pavlin Exp $

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
	_link_state_id(0), _advertising_router(0),
	_ls_sequence_number(OspfTypes::InitialSequenceNumber),
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
     * Get the length of the LSA from the buffer provided
     */
    static uint16_t get_lsa_len_from_buffer(uint8_t *ptr);

    /**
     * Decode a LSA header and return a LSA header inline not a pointer.
     */
    Lsa_header decode(uint8_t *ptr) const throw(InvalidPacket);

    /**
     * Decode this lsa header in this context.
     */
    void decode_inline(uint8_t *ptr) throw(InvalidPacket);

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
    void set_ls_sequence_number(int32_t ls_sequence_number) {
	_ls_sequence_number = ls_sequence_number;
    }

    int32_t get_ls_sequence_number() const {
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
    void decode(Lsa_header& header, uint8_t *ptr) const throw(InvalidPacket);

    OspfTypes::Version _version;
    uint16_t 	_LS_age;
    uint8_t	_options;	// OSPFv2 Only  
    uint16_t	_ls_type;	// OSPFv2 1 byte, OSPFv3 2 bytes.
    uint32_t	_link_state_id;
    uint32_t	_advertising_router;
    int32_t	_ls_sequence_number;
    uint16_t	_ls_checksum;
    uint16_t	_length;
};

/**
 * Compare the three fields that make an LSA equivalent.
 *
 *   RFC 2328 Section 12.1.  The LSA Header:
 *
 *	"The LSA header contains the LS type, Link State ID and
 *	Advertising Router fields.  The combination of these three
 *	fields uniquely identifies the LSA."
 */
inline
bool
operator==(const Lsa_header& lhs, const Lsa_header& rhs)
{
    // We could check for lhs == rhs this will be such a rare
    // occurence why bother.
    if (lhs.get_ls_type() != rhs.get_ls_type())
	return false;

    if (lhs.get_link_state_id() != rhs.get_link_state_id())
	return false;

    if (lhs.get_advertising_router() != rhs.get_advertising_router())
	return false;

    return true;
}

/**
 * RFC 2328 Section 13.7.  Receiving link state acknowledgments
 *
 * All the fields in the header need to be compared except for the age.
 */
inline
bool
compare_all_header_fields(const Lsa_header& lhs, const Lsa_header& rhs)
{
    // We could check for lhs == rhs this will be such a rare
    // occurence why bother.

    // Try and order the comparisons so in the no match case we blow
    // out early.

#define	lsa_header_compare(func)	if (lhs.func != rhs.func) return false;

    lsa_header_compare(get_ls_checksum());
    lsa_header_compare(get_length());
    switch(lhs.get_version()) {
    case OspfTypes::V2:
	lsa_header_compare(get_options());
	break;
    case OspfTypes::V3:
	break;
    }
    lsa_header_compare(get_ls_sequence_number());
    lsa_header_compare(get_ls_type());
    lsa_header_compare(get_link_state_id());
    lsa_header_compare(get_advertising_router());

#undef lsa_header_compare

    return true;
}

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
	:  _header(version), _version(version), _valid(true),
	   _self_originating(false),  _initial_age(0), _transmitted(false),
	   _trace(false), _peerid(OspfTypes::ALLPEERS)
    {}

    /**
     * Note passing in the LSA buffer does not imply that it will be
     * decoded. The buffer is just being stored.
     */
    Lsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	:  _header(version), _version(version), _valid(true),
	   _self_originating(false),  _initial_age(0), _transmitted(false),
	   _trace(false), _peerid(OspfTypes::ALLPEERS)
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
     *
     * @return The type this lsa represents.
     */
    virtual uint16_t get_ls_type() const = 0;

    /**
     * OSPFv3 only is this a known LSA.
     *
     * @return true if this is a known LSA.
     */
    virtual bool known() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return true;
    }

    /**
     * @return True if this is an AS-external-LSA.
     */
    virtual bool external() const { return false; };

    /**
     * @return True if this LSA is a Type-7-LSA.
     */
    virtual bool type7() const { return false; }

    /**
     * It is the responsibilty of the derived type to return this
     * information.
     * @return The minmum possible length of this LSA.
     */
    virtual size_t min_length() const = 0;

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    virtual LsaRef decode(uint8_t *buf, size_t& len) const 
	throw(InvalidPacket) = 0;

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
	XLOG_ASSERT(0 != len);
	return &_pkt[0];
    }

    /**
     * Is a wire format version available?
     *
     * For all non self orignating LSAs there should be a wire version
     * available.
     *
     * Self originating LSAs such as Router LSAs can exist that do not
     * yet have any valid fields (no interfaces to describe). Use this
     * field to check if this LSA is available.
     *
     * @return true is a wire format version is available.
     */
    bool available() const { return 0 != _pkt.size(); }

    Lsa_header& get_header() {return _header; }

    const Lsa_header& get_header() const {return _header; }

    /**
     * Is this LSA valid?
     */
    bool valid() const { return _valid; }

    /**
     * Unconditionally clear the timer and invalidate the LSA if
     * required, the default is to invalidate the LSA.
     */
    void invalidate(bool invalidate = true) {
	if(invalidate)
	    _valid = false;
	_timer.clear();
    }

    /**
     * @return true of this is a self originating LSA.
     */
    bool get_self_originating() const { return _self_originating; }

    /**
     * Set the state of this LSA with respect to self originating or not.
     */
    void set_self_originating(bool orig) { _self_originating = orig; }

    /**
     * Record the time of creation and initial age.
     * @param now the current time.
     */
    void record_creation_time(TimeVal now) {
	_creation = now;
	_initial_age = _header.get_ls_age();
    }

    /**
     * Get arrival time.
     * @param now out parameter which will be contain the time the LSA
     * arrived.
     */
    void get_creation_time(TimeVal& now) {
	now = _creation;
    }

    /**
     * Revive an LSA that is at MaxAge and MaxSequenceNumber. The age
     * is taken back to zero and sequence number InitialSequenceNumber.
     */
    void revive(const TimeVal& now);

    /**
     * Update the age and sequence number of a self originated
     * LSAs. Plus encode.
     */
    void update_age_and_seqno(const TimeVal& now);

    /**
     * Update the age field based on the current time.
     *
     * @param now the current time.
     */
    void update_age(TimeVal now);

    /**
     * Increment the age field of an LSA by inftransdelay.
     *
     * @param ptr to the age field, first field in a LSA.
     * @param inftransdelay delay to add in seconds.
     */
    static void update_age_inftransdelay(uint8_t *ptr, uint16_t inftransdelay);

    /**
     * Set the age to MaxAge.
     */
    void set_maxage();

    /**
     * Is the age of this LSA MaxAge.
     */
    bool maxage() const;

    /**
     * Is the LS Sequence Number MaxSequenceNumber?
     */
    bool max_sequence_number() const;

    /**
     * Get the LS Sequence Number.
     */
    int32_t get_ls_sequence_number() const {
	return _header.get_ls_sequence_number();
    }

    /**
     * Set the LS Sequence Number.
     */
    void set_ls_sequence_number(int32_t seqno) {
	_header.set_ls_sequence_number(seqno);
    }

    /**
     * Increment sequence number.
     */
    void increment_sequence_number() {
	int32_t seqno = _header.get_ls_sequence_number();
	if (OspfTypes:: MaxSequenceNumber == seqno)
	    XLOG_FATAL("Bummer sequence number reached %d",
		       OspfTypes::MaxSequenceNumber);
	seqno += 1;
	_header.set_ls_sequence_number(seqno);
    }

    /**
     * Add a neighbour ID to the NACK list.
     */
    void add_nack(OspfTypes::NeighbourID nid) {
	_nack_list.insert(nid);
    }

    /**
     * Remove a neighbour ID from the NACK list.
     */
    void remove_nack(OspfTypes::NeighbourID nid) {
	_nack_list.erase(nid);
    }

    /**
     * Does this neighbour exist on the NACK list.
     */
    bool exists_nack(OspfTypes::NeighbourID nid) {
	return _nack_list.end() != _nack_list.find(nid);
    }

    /**
     * If the NACK list is empty return true. All of the neighbours
     * have now nacked this LSA.
     */
    bool empty_nack() const {
	return _nack_list.empty();
    }

    /**
     * @return true if this LSA has been transmitted.
     */
    bool get_transmitted() { return _transmitted; }

    /**
     * Set the transmitted state of this LSA.
     */
    void set_transmitted(bool t) { _transmitted = t; }

    /**
     * Get a reference to the internal timer.
     */
    XorpTimer& get_timer() { return _timer; }

    void set_tracing(bool trace) { _trace = trace; }

    bool tracing() const { return _trace; }

    /**
     * For OSPFv3 only LSAs with Link-local flooding scope save the
     * ingress PeerID.
     */
    void set_peerid(OspfTypes::PeerID peerid) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	XLOG_ASSERT(OspfTypes::ALLPEERS == _peerid);
	_peerid = peerid;
    }

    /**
     * For OSPFv3 only LSAs with Link-local flooding scope get the
     * ingress PeerID.
     */
    OspfTypes::PeerID get_peerid() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	XLOG_ASSERT(OspfTypes::ALLPEERS != _peerid);
	return _peerid;
    }


    // OSPFv3 only, if an LSA is not known and does not have the U-bit
    // set then it should be treated as if it has Link-local flooding scope.

    /**
     * OSPFv3 only Link-local scope.
     */
    bool link_local_scope() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (!understood())
	    return true;

	return 0 == (get_ls_type() & 0x6000);
    }

    /**
     * OSPFv3 only area scope.
     */
    bool area_scope() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (!understood())
	    return false;

	return 0x2000 == (get_ls_type() & 0x6000);
    }

    /**
     * OSPFv3 only AS scope.
     */
    bool as_scope() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (!understood())
	    return false;

 	return 0x4000 == (get_ls_type() & 0x6000);
    }

    /**
     * Printable name of this LSA.
     */
    virtual const char *name() const = 0;

    /**
     * Generate a printable representation of the LSA.
     */
    virtual string str() const = 0;

    /**
     * Add the LSA type bindings.
     */
//     void install_type(LsaType type, Lsa *lsa);
 protected:
    Lsa_header _header;		// Common LSA header.
    vector<uint8_t> _pkt;	// Raw LSA.

 private:
    const OspfTypes::Version 	_version;
    bool _valid;		// True if this LSA is still valid.
    bool _self_originating;	// True if this LSA is self originating.

    uint16_t _initial_age;	// Age when this LSA was created.
    TimeVal _creation;		// Time when this LSA was created.
    
    XorpTimer _timer;		// If this is a self originated LSA
				// this timer is used to retransmit
				// the LSA, otherwise this timer fires
				// when MaxAge is reached.

    bool _transmitted;		// Set to true when this LSA is transmitted.

    bool _trace;		// True if this LSA should be traced.

    // List of neighbours that have not yet acknowledged this LSA.

    set<OspfTypes::NeighbourID> _nack_list;	

    /**
     * Set the age and update the stored packet if it exists.
     */
    void set_ls_age(uint16_t ls_age);

    /**
     * If this is an OSPFv3 LSA and the flooding scope is Link-local
     * then the PeerID should be set to the peer that the LSA is
     * associated with.
     */
    OspfTypes::PeerID _peerid;

    /**
     * OSPFv3 only is the U-bit set, i.e. should this LSA be processed
     * as if it is understood; obviously if we already know about it
     * it is understood.
     */
    bool understood() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (known())
	    return true;
	else
	    return 0x8000 == (get_ls_type() & 0x8000);
    }
};

/**
 * LSA byte streams are decoded through this class.
 */
class LsaDecoder {
 public:    
    LsaDecoder(OspfTypes::Version version)
	: _version(version), _min_lsa_length(0), _unknown_lsa_decoder(0)
    {}

    ~LsaDecoder();

    /**
     * Register the LSA decoders, called multiple times with each LSA.
     *
     * @param LSA decoder
     */
    void register_decoder(Lsa *lsa);

    /**
     * Register the unknown LSA decoder, called once.
     *
     * @param LSA decoder
     */
    void register_unknown_decoder(Lsa *lsa);

    /**
     * Decode an LSA.
     *
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    Lsa::LsaRef decode(uint8_t *ptr, size_t& len) const throw(InvalidPacket);

    /**
     * @return The length of the smallest LSA that can be decoded.
     */
    size_t min_length() const {
	return _min_lsa_length + Lsa_header::length();
    }

    /**
     * Validate type field.
     * If we know how to decode an LSA of this type we must know how
     * to process it.
     *
     * @return true if we know about this type of LSA.
     */
    bool validate(uint16_t type) const {
	if (0 != _unknown_lsa_decoder)
	    return true;
	return _lsa_decoders.end() != _lsa_decoders.find(type);
    }

    /**
     * Is an LSA of this type an AS-external-LSA?
     *
     * @return true if this type is an AS-external-LSA
     */
    bool external(uint16_t type) {
	map<uint16_t, Lsa *>::iterator i = _lsa_decoders.find(type);
	XLOG_ASSERT(_lsa_decoders.end() != i);
	return i->second->external();
    }

    /**
     * Return the name of this LSA.
     */
     const char *name(uint16_t type) const {
	 map<uint16_t, Lsa *>::const_iterator i = _lsa_decoders.find(type);
	 XLOG_ASSERT(_lsa_decoders.end() != i);
	 return i->second->name();
     }

    OspfTypes::Version get_version() const {
	return _version;
    }
 private:
    const OspfTypes::Version 	_version;
    size_t _min_lsa_length;		// The smallest LSA that can be
					// decoded, excluding LSA header.

    map<uint16_t, Lsa *> _lsa_decoders;	// OSPF LSA decoders
    Lsa * _unknown_lsa_decoder;		// OSPF Unknown LSA decoder
};

/**
 * RFC 2470 A.4.1 IPv6 Prefix Representation
 * OSPFv3 only
 */
class IPv6Prefix {
public:
    static const uint8_t NU_bit = 0x1;
    static const uint8_t LA_bit = 0x2;
    static const uint8_t MC_bit = 0x4;
    static const uint8_t P_bit = 0x8;
    static const uint8_t DN_bit = 0x10;

    IPv6Prefix(OspfTypes::Version version, bool use_metric = false)
	: _version(version), _use_metric(use_metric), _metric(0),
	  _prefix_options(0)
    {
    }

    IPv6Prefix(const IPv6Prefix& rhs)
	: _version(rhs._version), _use_metric(rhs._use_metric)
    {
	copy(rhs);
    }

    IPv6Prefix operator=(const IPv6Prefix& rhs) {
	if(&rhs == this)
	    return *this;
	copy(rhs);
	return *this;
    }

#define	ipv6prefix_copy(var)	var = rhs.var;

    void copy(const IPv6Prefix& rhs) {
	ipv6prefix_copy(_network);
	ipv6prefix_copy(_metric);
	ipv6prefix_copy(_prefix_options);
    }
#undef	ipv6prefix_copy

    /**
     * @return the number of bytes the encoded data will occupy.
     */
    size_t length() const;

     /**
     * Decode a IPv6Prefix.
     *
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     * @param prefixlen prefix length
     * @param option prefix option
     *
     * @return A IPv6Prefix.
     */
    IPv6Prefix decode(uint8_t *ptr, size_t& len, uint8_t prefixlen,
		      uint8_t option) const
	throw(InvalidPacket);

    /**
     * Copy a wire format representation to the pointer provided.
     *
     * length() should be called by the caller to verify enough space
     * is available.
     * @return the number of bytes written.
     */
    size_t copy_out(uint8_t *to_uint8) const;

    /**
     * @return Number of bytes that will be occupied by this prefix.
     */
    static size_t bytes_per_prefix(uint8_t prefix) {
	return ((prefix + 31) / 32) * 4;
    }

    OspfTypes::Version get_version() const {
	return _version;
    }

    void set_network(const IPNet<IPv6>& network) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_network = network;
    }

    IPNet<IPv6> get_network() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _network;
    }

    bool use_metric() const {
	return _use_metric;
    }

    void set_metric(uint16_t metric) {
	XLOG_ASSERT(_use_metric);
	_metric = metric;
    }

    uint16_t get_metric() const {
	XLOG_ASSERT(_use_metric);
	return _metric;
    }

    void set_prefix_options(uint8_t prefix_options) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_prefix_options = prefix_options;
    }

    uint8_t get_prefix_options() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _prefix_options;
    }
    
    void set_bit(bool set, uint8_t bit) {
	 XLOG_ASSERT(OspfTypes::V3 == get_version());
	 if (set)
	     _prefix_options |= bit;
	 else
	     _prefix_options &= ~bit;
	
    }

    bool get_bit(uint8_t bit) const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _prefix_options & bit ? true : false;
    }

    void set_nu_bit(bool set) { set_bit(set, NU_bit); }
    bool get_nu_bit() const { return get_bit(NU_bit); }
    
    void set_la_bit(bool set) { set_bit(set, LA_bit); }
    bool get_la_bit() const { return get_bit(LA_bit); }

    void set_mc_bit(bool set) { set_bit(set, MC_bit); }
    bool get_mc_bit() const { return get_bit(MC_bit); }

    void set_p_bit(bool set) { set_bit(set, P_bit); }
    bool get_p_bit() const { return get_bit(P_bit); }

    void set_dn_bit(bool set) { set_bit(set, DN_bit); }
    bool get_dn_bit() const { return get_bit(DN_bit); }

    /**
     * Generate a printable representation.
     */
    string str() const;

private:
    const OspfTypes::Version _version;
    const bool _use_metric;

    IPNet<IPv6> _network;
    uint16_t _metric;
    uint8_t _prefix_options;
};

/**
 * Defines a link/interface, carried in a RouterLsa.
 */
class RouterLink {
 public:
    enum Type {
	p2p = 1,	// Point-to-point connection to another router
	transit = 2,	// Connection to a transit network
	stub = 3,	// Connection to a stub network OSPFv2 only
	vlink = 4	// Virtual link
    };

    RouterLink(OspfTypes::Version version) 
	: _version(version), _type(p2p), _metric(0), _link_id(0),
	  _link_data(0), _interface_id(0), _neighbour_interface_id(0),
	  _neighbour_router_id(0)
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
    
#define	routerlink_compare(var)	if (var != rhs.var) return false;
    bool operator==(const RouterLink& rhs) {
	routerlink_compare(_type);
	routerlink_compare(_metric);
	switch (get_version()) {
	case OspfTypes::V2:
	    routerlink_compare(_link_id);
	    routerlink_compare(_link_data);
	    break;
	case OspfTypes::V3:
	    routerlink_compare(_interface_id);
	    routerlink_compare(_neighbour_interface_id);
	    routerlink_compare(_neighbour_router_id);
	    break;
	}


	return true;
    }
#undef	routerlink_compare

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
    decode(uint8_t *ptr, size_t& len) const throw(InvalidPacket);
    
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
    void set_type(Type t) {
	if (stub == t)
	    XLOG_ASSERT(OspfTypes::V2 == get_version());
	_type = t;
    }

    Type get_type() const {
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

    Type	_type;
    uint16_t	_metric;		// Only store TOS 0 metric

    uint32_t	_link_id;		// OSPFv2 Only
    uint32_t	_link_data;		// OSPFv2 Only

    uint32_t	_interface_id;		// OSPFv3 Only
    uint32_t	_neighbour_interface_id;// OSPFv3 Only
    uint32_t	_neighbour_router_id;	// OSPFv3 Only
};

class UnknownLsa : public Lsa {
public:
    UnknownLsa(OspfTypes::Version version)
	: Lsa(version)
    {}

    UnknownLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {}
    /**
     * @return the minimum length of a Router-LSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    XLOG_FATAL("OSPFv3 only");
	    break;
	case OspfTypes::V3:
	    return 0;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    XLOG_FATAL("OSPFv3 only");
	    break;
	case OspfTypes::V3:
	    return _header.get_ls_type();
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * OSPFv3 only is this a known LSA, of course not this is the
     * unknown LSA.
     *
     * @return false.
     */
    bool known() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return false;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "Unknown";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
};

class RouterLsa : public Lsa {
 public:
    RouterLsa(OspfTypes::Version version)
	: Lsa(version), _nt_bit(false), _w_bit(false), _v_bit(false),
	  _e_bit(false), _b_bit(false), _options(0)
    {
	_header.set_ls_type(get_ls_type());
    }

    RouterLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {}

    /**
     * @return the minimum length of a Router-LSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 4;
	    break;
	case OspfTypes::V3:
	    return 4;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
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

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    // NSSA translation
    void set_nt_bit(bool bit) {
	_nt_bit = bit;
    }

    bool get_nt_bit() const {
	return _nt_bit;
    }

    // Wildcard multicast receiver! OSPFv3 Only
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

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "Router";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;

 private:
    bool _nt_bit;	// NSSA Translation.
    bool _w_bit;	// Wildcard multicast receiver! OSPFv3 Only
    bool _v_bit;	// Virtual link endpoint
    bool _e_bit;	// AS boundary router (E for external)
    bool _b_bit;	// Area border router.

    uint32_t _options;	// OSPFv3 only.

    list<RouterLink> _router_links;
};

class NetworkLsa : public Lsa {
 public:
    NetworkLsa(OspfTypes::Version version)
	: Lsa(version)
    {
	_header.set_ls_type(get_ls_type());
    }

    NetworkLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {}

    /**
     * @return the minimum length of a Network-LSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 8;
	    break;
	case OspfTypes::V3:
	    return 8;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 2;
	    break;
	case OspfTypes::V3:
	    return 0x2002;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

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

    void set_network_mask(uint32_t network_mask) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_network_mask = network_mask;
    }

    uint32_t get_network_mask() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _network_mask;
    }

    list<OspfTypes::RouterID>& get_attached_routers() {
	return _attached_routers;
    }

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "Network";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
 private:
    uint32_t _options;			// OSPFv3 only.

    uint32_t _network_mask;		// OSPFv2 only.
    list<OspfTypes::RouterID> _attached_routers;
};

/**
 * OSPFv2: Summary-LSA Type 3
 * OSPFv3: Inter-Area-Prefix-LSA
 */
class SummaryNetworkLsa : public Lsa {
 public:
    static const size_t IPV6_PREFIX_OFFSET = 28;

    SummaryNetworkLsa(OspfTypes::Version version)
	: Lsa(version), _ipv6prefix(version)
    {
	_header.set_ls_type(get_ls_type());
    }

    SummaryNetworkLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len), _ipv6prefix(version)
    {}

    /**
     * @return the minimum length of a SummaryNetworkLsa.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 8;
	    break;
	case OspfTypes::V3:
	    return 8;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 3;
	    break;
	case OspfTypes::V3:
	    return 0x2003;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    void set_metric(uint32_t metric) {
	_metric = metric;
    }

    uint32_t get_metric() const {
	return _metric;
    }

    void set_network_mask(uint32_t network_mask) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_network_mask = network_mask;
    }

    uint32_t get_network_mask() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _network_mask;
    }

    void set_ipv6prefix(const IPv6Prefix& ipv6prefix) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_ipv6prefix = ipv6prefix;
    }

    IPv6Prefix get_ipv6prefix() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _ipv6prefix;;
    }

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "SummaryN";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
 private:
    uint32_t _metric;
    uint32_t _network_mask;		// OSPFv2 only.
    IPv6Prefix _ipv6prefix;		// OSPFv3 only.
};

/**
 * OSPFv2: Summary-LSA Type 4
 * OSPFv3: Inter-Area-Router-LSA
 */
class SummaryRouterLsa : public Lsa {
 public:
    SummaryRouterLsa(OspfTypes::Version version)
	: Lsa(version)
    {
	_header.set_ls_type(get_ls_type());
    }

    SummaryRouterLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {}

    /**
     * @return the minimum length of a RouterLSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 8;
	    break;
	case OspfTypes::V3:
	    return 12;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 4;
	    break;
	case OspfTypes::V3:
	    return 0x2004;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

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

    void set_metric(uint32_t metric) {
	_metric = metric;
    }

    uint32_t get_metric() const {
	return _metric;
    }

    void set_network_mask(uint32_t network_mask) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_network_mask = network_mask;
    }

    uint32_t get_network_mask() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _network_mask;
    }

    void set_destination_id(OspfTypes::RouterID destination_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_destination_id = destination_id;
    }

    OspfTypes::RouterID get_destination_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _destination_id;
    }

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "SummaryR";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
 private:
    uint32_t _metric;

    uint32_t _network_mask;		// OSPFv2 only.

    uint8_t _options;			// OSPFv3 only.
    OspfTypes::RouterID _destination_id;// OSPFv3 only.
};

/**
 * AS-external-LSA
 */
class ASExternalLsa : public Lsa {
 public:
    static const size_t IPV6_PREFIX_OFFSET = 28;

    ASExternalLsa(OspfTypes::Version version)
	: Lsa(version), _network_mask(0), _e_bit(false), _f_bit(false),
	  _t_bit(false), _ipv6prefix(version), _referenced_ls_type(0),
	  _metric(0), _external_route_tag(0), _referenced_link_state_id(0)
    {
	_header.set_ls_type(get_ls_type());
    }

    ASExternalLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len), _ipv6prefix(version)
    {}

    /**
     * @return the minimum length of an AS-external-LSA.
     */
    size_t min_length() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 16;
	    break;
	case OspfTypes::V3:
	    return 12;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 5;
	    break;
	case OspfTypes::V3:
	    return 0x4005;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * @return True this is an AS-external-LSA.
     */
    bool external() const {return true; };

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    void set_network_mask(uint32_t network_mask) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_network_mask = network_mask;
    }

    uint32_t get_network_mask() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _network_mask;
    }

    void set_ipv6prefix(const IPv6Prefix& ipv6prefix) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_ipv6prefix = ipv6prefix;
    }

    IPv6Prefix get_ipv6prefix() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _ipv6prefix;;
    }

    template <typename A>
    void set_network(IPNet<A>);

    template <typename A>
    IPNet<A> get_network(A) const;

    void set_e_bit(bool bit) {
	_e_bit = bit;
    }

    bool get_e_bit() const {
	return _e_bit;
    }

    void set_f_bit(bool bit) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_f_bit = bit;
    }

    bool get_f_bit() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _f_bit;
    }

    void set_t_bit(bool bit) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_t_bit = bit;
    }

    bool get_t_bit() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _t_bit;
    }

    void set_referenced_ls_type(uint16_t referenced_ls_type) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_referenced_ls_type = referenced_ls_type;
    }

    uint16_t get_referenced_ls_type() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _referenced_ls_type;
    }

    void set_forwarding_address_ipv6(IPv6 forwarding_address_ipv6) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	XLOG_ASSERT(_f_bit);
	_forwarding_address_ipv6 = forwarding_address_ipv6;
    }

    IPv6 get_forwarding_address_ipv6() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	XLOG_ASSERT(_f_bit);
	return _forwarding_address_ipv6;
    }

    void set_metric(uint32_t metric) {
	_metric = metric;
    }

    uint32_t get_metric() const {
	return _metric;
    }

    void set_forwarding_address_ipv4(IPv4 forwarding_address_ipv4) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_forwarding_address_ipv4 = forwarding_address_ipv4;
    }

    IPv4 get_forwarding_address_ipv4() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _forwarding_address_ipv4;
    }

    template <typename A>
    void set_forwarding_address(A);

    template <typename A>
    A get_forwarding_address(A) const;

    void set_external_route_tag(uint32_t external_route_tag) {
	switch(get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    XLOG_ASSERT(_t_bit);
	    break;
	}
	_external_route_tag = external_route_tag;
    }

    uint32_t get_external_route_tag() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    XLOG_ASSERT(_t_bit);
	    break;
	}
	return _external_route_tag;
    }

    void set_referenced_link_state_id(uint32_t referenced_link_state_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (0 == _referenced_ls_type)
	    XLOG_WARNING("Referenced LS Type is zero");
	_referenced_link_state_id = referenced_link_state_id;
    }

    uint32_t get_referenced_link_state_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	if (0 == _referenced_ls_type)
	    XLOG_WARNING("Referenced LS Type is zero");
	return _referenced_link_state_id;
    }

    /**
     * Create a new instance of this LSA, allows the decode routine to
     * call either this or the Type7 donew.
     */
    virtual ASExternalLsa *donew(OspfTypes::Version version, uint8_t *buf,
				 size_t len) const {
	return new ASExternalLsa(version, buf, len);
    }

    /**
     * Name used in the str() method.
     */
    virtual string str_name() const {
	return "As-External-LSA";
    } 

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return get_e_bit() ? "ASExt-2" : "ASExt-1";
    }

    void set_suppressed_lsa(Lsa::LsaRef lsar) {
	_suppressed_lsa = lsar;
    }

    Lsa::LsaRef get_suppressed_lsa() const {
	return _suppressed_lsa;
    }

    void release_suppressed_lsa() {
	_suppressed_lsa.release();
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
 private:
    uint32_t _network_mask;		// OSPFv2 only.
    bool _e_bit;
    bool _f_bit;			// OSPFv3 only.
    bool _t_bit;			// OSPFv3 only.

    IPv6Prefix _ipv6prefix;		// OSPFv3 only.

    uint16_t _referenced_ls_type;	// OSPFv3 only.
    IPv6 _forwarding_address;		// OSPFv3 only.
    
    uint32_t _metric;
    IPv4 _forwarding_address_ipv4;	// OSPFv2 only.
    IPv6 _forwarding_address_ipv6;	// OSPFv3 only.
    uint32_t _external_route_tag;
    uint32_t _referenced_link_state_id;	// OSPFv3 only.

    Lsa::LsaRef _suppressed_lsa;	// If a self originated LSA is
					// being suppressed here it is.
};

/**
 * Type-7 LSA used to convey external routing information in NSSAs.
 */
class Type7Lsa : public ASExternalLsa {
 public:
    Type7Lsa(OspfTypes::Version version) : ASExternalLsa(version)
    {
	_header.set_ls_type(get_ls_type());
    }

    Type7Lsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: ASExternalLsa(version, buf, len)
    {}

    uint16_t get_ls_type() const {
	switch(get_version()) {
	case OspfTypes::V2:
	    return 7;
	    break;
	case OspfTypes::V3:
	    return 0x2007;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * @return True this is an AS-external-LSA.
     */
    bool external() const {return false; };

    /**
     * @return True if this LSA is a Type-7-LSA.
     */
    bool type7() const { return true; }

    virtual ASExternalLsa *donew(OspfTypes::Version version, uint8_t *buf,
				 size_t len) const {
	return new Type7Lsa(version, buf, len);
    }

    /**
     * Name used in the str() method.
     */
    string str_name() const {
	return "Type-7-LSA";
    } 

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return get_e_bit() ? "Type7-2" : "Type7-1";
    }
};

/**
 * OSPFv3 only: Link-LSA
 */
class LinkLsa : public Lsa {
public:
    LinkLsa(OspfTypes::Version version)
	: Lsa(version)
    {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_header.set_ls_type(get_ls_type());
    }

    LinkLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
    }

    /**
     * @return the minimum length of a Link-LSA.
     */
    size_t min_length() const {
	return 24;
    }
    
    uint16_t get_ls_type() const {
	return 8;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    void set_rtr_priority(uint8_t rtr_priority) {
	_rtr_priority = rtr_priority;
    }

    uint8_t get_rtr_priority() const {
	return _rtr_priority;
    }

    void set_options(uint32_t options) {
	if (options  > 0xffffff)
	    XLOG_WARNING("Attempt to set %#x in a 24 bit field", options);
	_options = options & 0xffffff;
    }

    uint32_t get_options() const {
	return _options;
    }

    void set_link_local_address(IPv6 link_local_address) {
	_link_local_address = link_local_address;
    }

    IPv6 get_link_local_address() const {
	return _link_local_address;
    }

    const list<IPv6Prefix>& get_prefixes() const {
	return _prefixes;
    }

    list<IPv6Prefix>& get_prefixes() {
	return _prefixes;
    }

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "Link";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
private:
    uint8_t _rtr_priority;
    uint32_t _options;
    IPv6 _link_local_address;
    
    list<IPv6Prefix> _prefixes;
};

/**
 * OSPFv3 only: Intra-Area-Prefix-LSA
 */
class IntraAreaPrefixLsa : public Lsa {
public:
    IntraAreaPrefixLsa(OspfTypes::Version version)
	: Lsa(version)
    {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_header.set_ls_type(get_ls_type());
    }

    IntraAreaPrefixLsa(OspfTypes::Version version, uint8_t *buf, size_t len)
	: Lsa(version, buf, len)
    {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
    }

    /**
     * @return the minimum length of a Link-LSA.
     */
    size_t min_length() const {
	return 12;
    }
    
    uint16_t get_ls_type() const {
	return 0x2009;
    }

    /**
     * Decode an LSA.
     * @param buf pointer to buffer.
     * @param len length of the buffer on input set to the number of
     * bytes consumed on output.
     *
     * @return A reference to an LSA that manages its own memory.
     */
    LsaRef decode(uint8_t *buf, size_t& len) const throw(InvalidPacket);

    bool encode();

    void set_referenced_ls_type(uint16_t referenced_ls_type) {
	_referenced_ls_type = referenced_ls_type;
    }

    uint16_t get_referenced_ls_type() const {
	return _referenced_ls_type;
    }

    void set_referenced_link_state_id(uint32_t referenced_link_state_id) {
	_referenced_link_state_id = referenced_link_state_id;
    }

    uint32_t get_referenced_link_state_id() const {
	return _referenced_link_state_id;
    }

    void set_referenced_advertising_router(uint32_t referenced_adv_router) {
	_referenced_advertising_router = referenced_adv_router;
    }

    uint32_t get_referenced_advertising_router() const {
	return _referenced_advertising_router;
    }

    list<IPv6Prefix>& get_prefixes() {
	return _prefixes;
    }

    /**
     * Given a referenced LS type and an interface ID generate a
     * candidate Link State ID for Intra-Area-Prefix-LSAs. This is
     * *NOT* part of the protocol, just a way to create a unique
     * mapping.
     *
     * The underlying assumption is that for every area only one
     * Intra-Area-Prefix-LSA will be generated per Router-LSA and
     * Network-LSA. More importantly one Router-LSA will be generated
     * per area although it is legal to generate many. The size of the
     * Router-LSA is a function of the number of interfaces on the
     * generating router and for the time being we only generate one
     * Router-LSA. If the number of interfaces exceeds the capacity of
     * a single Router-LSA then this method and the Router-LSA code
     * will need to be re-visited.
     */
    uint32_t create_link_state_id(uint16_t ls_type, uint32_t interface_id)
	const
    {
	XLOG_ASSERT(OspfTypes::V3 == get_version());

	if (RouterLsa(get_version()).get_ls_type() == ls_type) {
	    return OspfTypes::UNUSED_INTERFACE_ID;
	} else if (NetworkLsa(get_version()).get_ls_type() == ls_type) {
	    return interface_id;
	} else {
	    XLOG_FATAL("Unknown LS Type %#x\n", ls_type);
	}

	return 0;
    }

    /**
     * Printable name of this LSA.
     */
    const char *name() const {
	return "IntraArPfx";
    }

    /**
     * Generate a printable representation.
     */
    string str() const;
    
private:
    uint16_t _referenced_ls_type;
    uint32_t _referenced_link_state_id;
    uint32_t _referenced_advertising_router;
    list<IPv6Prefix> _prefixes;
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
    decode(uint8_t *ptr) throw(InvalidPacket);

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
    uint32_t 	_ls_type;	// OSPFv2 4 bytes, OSPFv3 2 bytes.
    uint32_t 	_link_state_id;
    uint32_t	_advertising_router;
};

/**
 * The definitive list of LSAs. All decoder lists should be primed
 * using this function.
 */
inline
void
initialise_lsa_decoder(OspfTypes::Version version, LsaDecoder& lsa_decoder)
{
    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	lsa_decoder.register_unknown_decoder(new UnknownLsa(version));
	break;
    }

    lsa_decoder.register_decoder(new RouterLsa(version));
    lsa_decoder.register_decoder(new NetworkLsa(version));
    lsa_decoder.register_decoder(new SummaryNetworkLsa(version));
    lsa_decoder.register_decoder(new SummaryRouterLsa(version));
    lsa_decoder.register_decoder(new ASExternalLsa(version));
    lsa_decoder.register_decoder(new Type7Lsa(version));
    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
  	lsa_decoder.register_decoder(new LinkLsa(version));
  	lsa_decoder.register_decoder(new IntraAreaPrefixLsa(version));
	break;
    }
}

/**
 * Given an address and a mask generate an IPNet both of the values
 * are in host order.
 */
inline
IPNet<IPv4>
lsa_to_net(uint32_t lsid, uint32_t mask)
{
    IPv4 prefix = IPv4(htonl(mask));
    IPNet<IPv4> net = IPNet<IPv4>(IPv4(htonl(lsid)), prefix.mask_len());

    return net;
}

/**
 * Given a link state ID and a mask both in host order return a link
 * state ID with the host bits set.
 */
inline
uint32_t
set_host_bits(uint32_t lsid, uint32_t mask)
{
    return lsid | ~mask;
}

#endif // __OSPF_LSA_HH__
