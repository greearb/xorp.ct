// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ifndef __OSPF_PACKET_HH__
#define __OSPF_PACKET_HH__

/**
 * All packet decode routines must inherit from this interface.
 * Also provides some utility routines to perform common packet processing.
 */
class Packet {
 public:
    static const size_t STANDARD_HEADER_V2 = 24;
    static const size_t STANDARD_HEADER_V3 = 16;

    Packet(OspfTypes::Version version)
	: _version(version), _auth_type(0), _instance_id(0)
    {
	memset(&_auth[0], 0, sizeof(_auth));
    }

    virtual ~Packet()
    {}

    /**
     * Decode standard header.
     *
     * Used by the derived classes to extract the standard header and
     * options field.
     *
     * @return the offset where the specific header starts.
     */
    size_t decode_standard_header(uint8_t *ptr, size_t len) throw(BadPacket);

    /**
     * Decode the packet.
     * The returned packet must be free'd.
     */
    virtual Packet *decode(uint8_t *ptr, size_t len)
	const throw(BadPacket) = 0;

    /**
     * Encode standard header.
     *
     * Used by the derived classes to put the standard header in the
     * packet. Which includes the checksum so all other fields should
     * already be in the packet.
     *
     * @param ptr location to start writing header.
     * @param len size of the buffer.
     * @return The offset that the header has been written to. Returns
     * 0 on failure, such as the buffer is too small.
     */
    size_t encode_standard_header(uint8_t *ptr, size_t len);

    /**
     * Encode the packet.
     *
     * @param pkt vector into which the packet should be placed.
     * @return true if the encoding suceeded.
     */
    virtual bool encode(vector<uint8_t>& pkt) = 0;

    /**
     * @return The version this packet represents.
     */
    OspfTypes::Version get_version() const { return _version; }

    /**
     * It is the responsibilty of the derived type to return this
     * information.
     * @return The type this packet represents.
     */
    virtual OspfTypes::Type get_type() const = 0;

    /**
     * Get the Router ID.
     */
    OspfTypes::RouterID get_router_id() const { return _router_id; }

    /**
     * Set the Router ID.
     */
    void set_router_id(OspfTypes::RouterID id) { _router_id = id; }

    /**
     * Get the Area ID.
     */
    OspfTypes::AreaID get_area_id() const { return _area_id; }

    /**
     * Set the Area ID.
     */
    void set_area_id(OspfTypes::AreaID id) { _area_id = id; }    

    /**
     * Get the Auth Type.
     */
    uint16_t get_auth_type() const { return _auth_type; }

    /**
     * Set the Auth Type.
     */
    void set_auth_type(uint16_t auth_type) { _auth_type = auth_type; }

    /**
     * Get the Instance ID.
     */
    uint8_t get_instance_id() const { return _instance_id; }

    /**
     * Set the Instance ID.
     */
    void set_instance_id(uint8_t instance_id) { _instance_id = instance_id; }

    /**
     * @return the standard header length for this version of OSPF.
     */
    inline size_t
    get_standard_header_length() {
	switch(_version) {
	case OspfTypes::V2:
	    return STANDARD_HEADER_V2;
	    break;
	case OspfTypes::V3:
	    return STANDARD_HEADER_V3;
	    break;
	}
	XLOG_UNREACHABLE();
	return 0;
    }

    /**
     * Decode Point.
     * 
     * If a packet is being decoded the standard header has already been
     * processed. This method returns the offset at which the specific
     * data starts.
     *
     * @return The offset at which a derived class should start decoding.
     */
    size_t decode_point() const;

    /**
     * Generate a printable representation of the standard header.
     * Used by the derived classes implementing str().
     */
    string standard() const;

    /**
     * Generate a printable representation of the packet.
     */
    virtual string str() const = 0;

 private:
    const OspfTypes::Version 	_version;

    /**
     * Set to true when the standard header fields are set.
     */
    bool _valid;

    /**
     * Standard header.
     * Version and Type information are constant so therefore not present.
     */
    OspfTypes::RouterID	_router_id;
    OspfTypes::AreaID	_area_id;
    OspfTypes::AuType	_auth_type;	// OSPF V2 Only
    uint8_t 		_auth[8];	// OSPF V2 Only
    uint8_t		_instance_id;	// OSPF V3 Only
};

/**
 * Packet byte streams are decoded through this class.
 */
class PacketDecoder {
 public:    
     ~PacketDecoder();

     /**
      * Register the packet/decode routines
      *
      * @param packet decoder
      */
    void register_decoder(Packet *packet);

#if	0
    /**
     * Register the packet/decode routines
     *
     * @param packet decoder
     * @param version OSPF version of the decoder
     * @param type of decoder
     */
    void register_decoder(Packet *packet,
			  OspfTypes::Version version,
			  OspfTypes::Type type);
#endif

    /**
     * Decode byte stream.
     *
     * @param ptr to data packet
     * @param length of data packet
     *
     * @return a packet structure, which must be free'd
     */
    Packet *decode(uint8_t *ptr, size_t len) throw(BadPacket);
 private:
    map<OspfTypes::Type , Packet *> _ospfv2;	// OSPF V2 Packet decoders
    map<OspfTypes::Type , Packet *> _ospfv3;	// OSPF V3 Packet decoders
};

/**
 * Hello packet
 */
class HelloPacket : public Packet {
 public:
    static const size_t MINIMUM_LENGTH = 20;	// The minumum length
						// of a hello packet.
						// The same for OSPF V2 and V3
						// How did that happen?

    HelloPacket(OspfTypes::Version version)
	: Packet(version), _network_mask(0), _interface_id(0),
	  _hello_interval(0), _options(0), _router_priority(0),
	  _router_dead_interval(0)
    {}

    OspfTypes::Type get_type() const { return 1; }

    Packet *decode(uint8_t *ptr, size_t len) const throw(BadPacket);

    /**
     * Encode the packet.
     *
     * @param pkt vector into which the packet should be placed.
     * @return true if the encoding succeeded.
     */
    bool encode(vector<uint8_t>& pkt);

    // Network Mask.
    void set_network_mask(uint32_t network_mask) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_network_mask = network_mask;
    }

    uint32_t get_network_mask() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _network_mask;
    }

    // Interface ID.
    void set_interface_id(uint32_t interface_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_interface_id = interface_id;
    }

    uint32_t get_interface_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _interface_id;
    }

    // Hello Interval.
    void set_hello_interval(uint16_t hello_interval) {
	_hello_interval = hello_interval;
    }

    uint16_t get_hello_interval() const {
	return _hello_interval;
    }

    // Options.
    void set_options(uint32_t options) {
	switch(get_version()) {
	case OspfTypes::V2:
	    if (options > 0xff)
		XLOG_WARNING("Attempt to set %#x in an 8 bit field",
			     options);
	    _options = options & 0xff;
	    break;
	case OspfTypes::V3:
	    if (options  > 0xffffff)
		XLOG_WARNING("Attempt to set %#x in a 24 bit field",
			     options);
	    _options = options & 0xffffff;
	    break;
	}
    }

    uint32_t get_options() const {
	return _options;
    }

    // Router Priority.
    void set_router_priority(uint8_t router_priority) {
	_router_priority = router_priority;
    }

    uint8_t get_router_priority() const {
	return _router_priority;
    }

    // Router Dead Interval.
    void set_router_dead_interval(uint32_t router_dead_interval) {
	switch(get_version()) {
	case OspfTypes::V2:
	    _router_dead_interval = router_dead_interval;
	    break;
	case OspfTypes::V3:
	    if ( router_dead_interval > 0xffff)
		XLOG_WARNING("Attempt to set %#x in a 16 bit field",
			     router_dead_interval);
	    _router_dead_interval = router_dead_interval & 0xffff;
	    break;
	}
    }

    uint32_t get_router_dead_interval() const {
	return _router_dead_interval;
    }

    // Designated Router
    void set_designated_router(OspfTypes::RouterID dr) {
	_dr = dr;
    }

    OspfTypes::RouterID get_designated_router() const {
	return _dr;
    }
    
    // Backup Designated Router
    void set_backup_designated_router(OspfTypes::RouterID bdr) {
	_bdr = bdr;
    }

    OspfTypes::RouterID get_backup_designated_router() const {
	return _bdr;
    }
    
    list<OspfTypes::RouterID>& get_neighbours() {
	return _neighbours;
    }

    /**
     * Generate a printable representation of the packet.
     */
    string str() const;

 private:
    uint32_t _network_mask;	// OSPF V2 Only
    uint32_t _interface_id;	// OSPF V3 Only
    uint16_t _hello_interval;
    uint32_t _options;		// Large enough to accomodate V2 and V3 options
    uint8_t  _router_priority;	// Router Priority.
    uint32_t _router_dead_interval;	// In seconds.
    OspfTypes::RouterID _dr;	// Designated router.
    OspfTypes::RouterID _bdr;	// Backup Designated router.

    list<OspfTypes::RouterID> _neighbours; // Router IDs of neighbours.
};

/**
 * Database Description Packet
 */
class DataDescriptionPacket : public Packet {
 public:
    DataDescriptionPacket(OspfTypes::Version version)
	: Packet(version), _interface_mtu(0), _options(0),
	  _i_bit(false), _m_bit(false), _ms_bit(false), _DD_seqno(0)
    {}

    OspfTypes::Type get_type() const { return 2; }

    size_t minimum_length() const {
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

    Packet *decode(uint8_t *ptr, size_t len) const throw(BadPacket);

    /**
     * Encode the packet.
     *
     * @param pkt vector into which the packet should be placed.
     * @return true if the encoding suceeded.
     */
    bool encode(vector<uint8_t>& pkt);

    // Interface MTU
    void set_interface_mtu(uint16_t mtu) {
	_interface_mtu = mtu;
    }

    uint16_t get_interface_mtu() const {
	return _interface_mtu;
    }

    // Options.
    void set_options(uint32_t options) {
	switch(get_version()) {
	case OspfTypes::V2:
	    if (options > 0xff)
		XLOG_WARNING("Attempt to set %#x in an 8 bit field",
			     options);
	    _options = options & 0xff;
	    break;
	case OspfTypes::V3:
	    if (options  > 0xffffff)
		XLOG_WARNING("Attempt to set %#x in a 24 bit field",
			     options);
	    _options = options & 0xffffff;
	    break;
	}
    }

    uint32_t get_options() const {
	return _options;
    }

    // Init bit.
    void set_i_bit(bool bit) {
	_i_bit = bit;
    }

    bool get_i_bit() const {
	return _i_bit;
    }

    // More bit.
    void set_m_bit(bool bit) {
	_m_bit = bit;
    }

    bool get_m_bit() const {
	return _m_bit;
    }

    // Master/Slave bit
    void set_ms_bit(bool bit) {
	_ms_bit = bit;
    }

    bool get_ms_bit() const {
	return _ms_bit;
    }
    
    // Database description sequence number.
    void set_dd_seqno(uint32_t seqno) {
	_DD_seqno = seqno;
    }
    
    uint32_t get_dd_seqno() const {
	return _DD_seqno;
    }

    list<Lsa_header>& get_lsa_headers() {
	return _lsa_headers;
    }
    
    /**
     * Generate a printable representation of the packet.
     */
    string str() const;

 private:
    uint16_t	_interface_mtu;
    uint32_t	_options;	// Large enough to accomodate V2 and V3 options
    bool	_i_bit;		// The init bit.
    bool	_m_bit;		// The more bit.
    bool	_ms_bit;	// The Master/Slave bit.
    uint32_t	_DD_seqno;	// Database description sequence number.
    
    list<Lsa_header> _lsa_headers;
};

/**
 * Link State Request Packet
 */
class LinkStateRequestPacket : public Packet {
 public:
    LinkStateRequestPacket(OspfTypes::Version version)
	: Packet(version)
    {}

    OspfTypes::Type get_type() const { return 3; }

    Packet *decode(uint8_t *ptr, size_t len) const throw(BadPacket);

    /**
     * Encode the packet.
     *
     * @param pkt vector into which the packet should be placed.
     * @return true if the encoding succeeded.
     */
    bool encode(vector<uint8_t>& pkt);
    
    list<Ls_request>& get_ls_request() {
	return _ls_request;
    }
    
    /**
     * Generate a printable representation of the packet.
     */
    string str() const;
 private:
    list<Ls_request> _ls_request;
};

/**
 * Link State Upate Packet
 */
class LinkStateUpdatePacket : public Packet {
 public:
    LinkStateUpdatePacket(OspfTypes::Version version, LsaDecoder& lsa_decoder)
	: Packet(version), _lsa_decoder(lsa_decoder)
    {}

    OspfTypes::Type get_type() const { return 4; }

    Packet *decode(uint8_t *ptr, size_t len) const throw(BadPacket);

    /**
     * Encode the packet.
     *
     * @param pkt vector into which the packet should be placed.
     * @return true if the encoding succeeded.
     */
    bool encode(vector<uint8_t>& pkt);

    list<Lsa::LsaRef>& get_lsas() {
	return _lsas;
    }
    
    /**
     * Generate a printable representation of the packet.
     */
    string str() const;
    
 private:
    LsaDecoder& _lsa_decoder;	// LSA decoders.

    // The packet contains a field with the number os LSAs that it
    // contains we don't bother to store it.

    list<Lsa::LsaRef> _lsas;	// The list of LSAs in the packet.
};

inline
uint16_t
extract_16(uint8_t *ptr)
{
    uint16_t val;
    val = ptr[0];
    val <<= 8;
    val |= ptr[1];

    return val;
}

inline
void
embed_16(uint8_t *ptr, uint16_t val)
{
    ptr[0] = (val >> 8) & 0xff;
    ptr[1] = val & 0xff;
}

inline
uint32_t
extract_32(uint8_t *ptr)
{
    uint32_t val;
    val = ptr[0];
    val <<= 8;
    val |= ptr[1];
    val <<= 8;
    val |= ptr[2];
    val <<= 8;
    val |= ptr[3];

    return val;
}

inline
void
embed_32(uint8_t *ptr, uint32_t val)
{
    ptr[0] = (val >> 24) & 0xff;
    ptr[1] = (val >> 16) & 0xff;
    ptr[2] = (val >> 8) & 0xff;
    ptr[3] = val & 0xff;
}

#endif // __OSPF_PACKET_HH__
