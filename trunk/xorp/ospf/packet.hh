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
 * Bad Packet exception.
 */
class BadPacket : public XorpReasonedException
{
public:
    BadPacket(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("BadPacket", file, line, init_why)
    {}
};

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
    virtual Packet *decode(uint8_t *ptr, size_t len) throw(BadPacket) = 0;

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
     * @param len an ouput paramater that returns the length of the
     * encoded packet.
     * @return pointer to packet, must be free'd.
     */
    virtual uint8_t *encode(size_t &len) = 0;
    
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
    static const size_t MINIMUM_LENGTH = 20;	// The same of OSPF V2 and V3
						// How did that happen?

    HelloPacket(OspfTypes::Version version)
	: Packet(version), _network_mask(0), _interface_id(0),
	  _hello_interval(0), _options(0), _router_priority(0),
	  _router_dead_interval(0)
    {}

    OspfTypes::Type get_type() const { return 1; }

    Packet *decode(uint8_t *ptr, size_t len) throw(BadPacket);

    /**
     * Encode the packet.
     *
     * @param len an ouput paramater that returns the length of the
     * encoded packet.
     * @return pointer to packet, must be free'd.
     */
    uint8_t *encode(size_t &len);

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
	_options = options;
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

#endif // __OSPF_PACKET_HH__
