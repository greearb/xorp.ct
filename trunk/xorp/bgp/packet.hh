// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/bgp/packet.hh,v 1.12 2003/01/29 23:38:12 rizzo Exp $

#ifndef __BGP_PACKET_HH__
#define __BGP_PACKET_HH__

#include "config.h"

#include <iostream>

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <unistd.h>

#include "exceptions.hh"
#include "parameter.hh"
#include "local_data.hh"
#include "peer_data.hh"
#include "path_attribute.hh"
#include "update_attrib.hh"

#include "libxorp/ipv4net.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/debug.h"

enum BgpPacketType {
    MESSAGETYPEOPEN =		1,
    MESSAGETYPEUPDATE =		2,
    MESSAGETYPENOTIFICATION =	3,
    MESSAGETYPEKEEPALIVE =	4
};

/**
 * Notification message error codes with associated subcodes.
 */
enum Notify {
#define		UNSPECIFIED  0          // Unspecified subcode error
    MSGHEADERERR = 1,		// Message Header Error
#define		CONNNOTSYNC  1		// Connection Not Synchronized
#define		BADMESSLEN   2		// Bad Message Length
#define		BADMESSTYPE  3		// Bad Message Type
    OPENMSGERROR = 2,		// OPEN Message Error
#define		UNSUPVERNUM  1		// Unsupported Version Number
#define		BADASPEER    2		// Bad Peer AS
#define		BADBGPIDENT  3		// Bad BGP Identifier
#define		UNSUPOPTPAR  4		// Unsupported Optional Parameter
#define		AUTHFAIL     5		// Authentication Failure
#define		UNACCEPTHOLDTIME 6	// Unacceptable Hold Time
    UPDATEMSGERR = 3,		// UPDATE Message Error
#define		MALATTRLIST  1		// Malformed Attribute List
#define		UNRECOGWATTR 2		// Unrecognized Well-known Attribute
#define		MISSWATTR    3		// Missing Well-known Attribute
#define		ATTRFLAGS    4		// Attribute Flags Error
#define		ATTRLEN	     5		// Attribute Length Error
#define		INVALORGATTR 6		// Invalid ORIGIN Attribute
//#define		
#define		INVALNHATTR  8		// Invalid NEXT_HOP Attribute
#define		OPTATTR	     9		// Optional Attribute Error
#define		INVALNETFIELD 10	// Invalid Network Field
#define		MALASPATH    11		// Malformed AS_PATH
    HOLDTIMEEXP = 4,		// Hold Timer Expired
    FSMERROR = 5,		// Finite State Machine Error
    CEASE = 6			// Cease
};

#define	MARKER_SIZE		16
/**
 * Overlay for the BGP common header (on the wire)
 */
struct fixed_header {
    uint8_t	marker[MARKER_SIZE];	// normally all bits are ones.
    uint16_t	length;			// this is in network format 
    uint8_t	type;			// enum BgpPacketType
};

// size of fixed_header
const size_t BGP_COMMON_HEADER_LEN = 19;

/* 
 * Various min and max packet size, accounting for the basic
 * information in the packet itself
 */
const size_t MINPACKETSIZE = BGP_COMMON_HEADER_LEN;
const size_t MAXPACKETSIZE = 4096;

const size_t MINOPENPACKET = BGP_COMMON_HEADER_LEN + 10;
const size_t MINUPDATEPACKET = BGP_COMMON_HEADER_LEN + 2 + 2;
const size_t MINKEEPALIVEPACKET = BGP_COMMON_HEADER_LEN;
const size_t MINNOTIFICATIONPACKET = BGP_COMMON_HEADER_LEN + 2;

/**
 * The main container for BGP messages (packets) which are sent
 * back and forth.
 *
 * This base class only contains the standard fields (length, type)
 * leaving other information to be stored in the derived objects.
 */

class BGPPacket {
public:
    /**
     * Status returned by message reader.
     */
    enum Status {
	GOOD_MESSAGE,
	ILLEGAL_MESSAGE_LENGTH,
	CONNECTION_CLOSED,
    };

    // The default marker.
    static const uint8_t Marker[MARKER_SIZE];

    BGPPacket()					{}
    virtual ~BGPPacket()			{}
    uint8_t type() const			{ return _Type; }
    virtual string str() const = 0;

    virtual const uint8_t *encode(size_t &len, uint8_t *buf = 0) const = 0;
protected:
    /*
     * Return the external representation of the packet into a buffer.
     * If the caller supplies the buffer it is its responsibility to make
     * sure that it has the correct size, otherwise the routine will
     * allocate it with new uint8_t[len].
     * It is responsibility of the caller to dispose of the buffer.
     * Note that this routine will only copy the fixed_header part.
     * The derived-class methods are in charge of filling up any
     * additional data past it.
     */
    uint8_t *basic_encode(size_t len, uint8_t *buf) const;

    // don't allow the use of the default copy constructor
    BGPPacket(const BGPPacket& BGPPacket);
    uint8_t _Type;
private:
};

/* **************** OpenPacket *********************** */

class OpenPacket : public BGPPacket {
public:
    OpenPacket(const uint8_t *d, uint16_t l)
		throw(CorruptMessage);
    OpenPacket(const AsNum& as, const IPv4& bgpid, const uint16_t holdtime);
    ~OpenPacket()				{}
    const uint8_t *encode(size_t& len, uint8_t *buf = 0) const;
    string str() const;

    const uint8_t Version() const		{ return _Version; }
    const AsNum as() const			{ return _as; }
    const uint16_t HoldTime() const		{ return _HoldTime; }
    const IPv4 id() const			{ return _id; }
    const uint8_t OptParmLen() const		{ return _OptParmLen; }
    bool operator==(const OpenPacket& him) const;
    void add_parameter(const BGPParameter *p);

    const list <BGPParameter*>& parameter_list() const {
	return _parameter_list;
    }

protected:

private:
    OpenPacket();
    // don't allow the use of the default copy constructor
    OpenPacket(const OpenPacket& OpenPacket);

    IPv4	_id;
    AsNum	_as;
    uint16_t	_HoldTime;
    uint8_t	_OptParmLen;
    uint8_t	_Version;

    list <BGPParameter*> _parameter_list;
    uint8_t _num_parameters;
};

/* **************** UpdatePacket *********************** */

class UpdatePacket : public BGPPacket {
public:
    UpdatePacket();
    UpdatePacket(const uint8_t *d, uint16_t l)
	throw(CorruptMessage);

    ~UpdatePacket();

    void add_withdrawn(const BGPUpdateAttrib& wdr);
    void add_pathatt(const PathAttribute& pa);
    void add_nlri(const BGPUpdateAttrib& nlri);
    const BGPUpdateAttribList& wr_list() const		{ return _wr_list; }
    const list <PathAttribute*>& pa_list() const	{ return _pa_list; }
    const BGPUpdateAttribList& nlri_list() const	{ return _nlri_list; }
    const uint8_t *encode(size_t& len, uint8_t *buf = 0) const;

    bool big_enough() const;

    string str() const;
    bool operator==(const UpdatePacket& him) const;
protected:

private:
    // don't allow the use of the default copy constructor
    UpdatePacket(const UpdatePacket& UpdatePacket);
    BGPUpdateAttribList		_wr_list;
    list <PathAttribute*>	_pa_list;
    BGPUpdateAttribList		_nlri_list;
};

/* **************** BGPNotificationPacket *********************** */

class NotificationPacket : public BGPPacket {
public:
    NotificationPacket(const uint8_t *d, uint16_t l) throw(InvalidPacket);
    NotificationPacket(uint8_t ec, uint8_t esc = 0,
		const uint8_t *d = 0, size_t l=0);
    NotificationPacket();
    ~NotificationPacket()			{ delete[] _error_data; }
    uint8_t error_code() const { return _error_code; }
    uint8_t error_subcode() const { return _error_subcode; }
    /*
    ** Verify that the supplied error code and subcode are legal.
    */
    static bool validate_error_code(const int error, const int subcode);
    const uint8_t* error_data() const { return _error_data; }
    const uint8_t *encode(size_t &len, uint8_t *buf = 0) const;
    string str() const;
    bool operator==(const NotificationPacket& him) const;
protected:

private:
    // don't allow the use of the default copy constructor
    NotificationPacket(const NotificationPacket& Notificationpacket);
    size_t		_Length;	// total size incl. header
    const uint8_t*	_error_data;
    uint8_t		_error_code;
    uint8_t		_error_subcode;
};

/**
 * KeepAlivePacket are extremely simple, being made only of a header.
 * with the appropriate type and length.
 */
class KeepAlivePacket : public BGPPacket {
public:
    /**
     * need nothing to parse incoming data
     */
    KeepAlivePacket(const uint8_t *, uint16_t l)
		throw(CorruptMessage)			{
	if (l != MINKEEPALIVEPACKET)
	    xorp_throw(CorruptMessage,
		c_format("KeepAlivePacket length %d instead of %u",
			l, (uint32_t)MINKEEPALIVEPACKET),
			MSGHEADERERR, UNSPECIFIED);

	_Type = MESSAGETYPEKEEPALIVE;
    }

    KeepAlivePacket()					{
	_Type = MESSAGETYPEKEEPALIVE;
    }
    ~KeepAlivePacket()					{}
    const uint8_t *encode(size_t &len, uint8_t *buf = 0) const		{
	len = MINKEEPALIVEPACKET;
	return basic_encode(len, buf);
    }
    virtual string str() const		{ return "Keepalive Packet\n"; }
    bool operator==(const KeepAlivePacket&) const	{ return true; }
protected:

private:
    // don't allow the use of the default copy constructor
    KeepAlivePacket(const KeepAlivePacket& KeepAlivePacket);
};

#endif // __BGP_PACKET_HH__
