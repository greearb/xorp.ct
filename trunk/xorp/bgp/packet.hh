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

// $XORP: xorp/bgp/packet.hh,v 1.5 2003/01/21 18:54:27 rizzo Exp $

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

#include "parameter.hh"
#include "local_data.hh"
#include "peer_data.hh"
#include "path_attribute.hh"
#include "update_attrib.hh"
#include "withdrawn_route.hh"
#include "net_layer_reachability.hh"

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
#define BGP_COMMON_HEADER_LEN			19

/* 
 * Various min and max packet size, accounting for the basic
 * information in the packet itself
 */
#define MINPACKETSIZE		BGP_COMMON_HEADER_LEN
#define MAXPACKETSIZE		4096

#define MINOPENPACKET		(BGP_COMMON_HEADER_LEN + 10)
#define MINUPDATEPACKET		(BGP_COMMON_HEADER_LEN + 2 + 2)
#define MINKEEPALIVEPACKET	BGP_COMMON_HEADER_LEN
#define MINNOTIFICATIONPACKET	(BGP_COMMON_HEADER_LEN + 2)

/**
 * This exception is thrown when a bad input message is received.
 */
class CorruptMessage : public XorpReasonedException {
public:
    CorruptMessage(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(0), _subcode(0), _data(0), _len(0)
    {}

    CorruptMessage(const char* file, size_t line,
		   const string init_why,
		   const int error, const int subcode)
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(error), _subcode(subcode), _data(0), _len(0)
    {}

    CorruptMessage(const char* file, size_t line,
		   const string init_why,
		   const int error, const int subcode,
		   const uint8_t *data, const size_t len)
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(error), _subcode(subcode), _data(data), _len(len)
    {}

    const int error() const { return _error; }
    const int subcode() const { return _subcode; }
    const uint8_t *data() const { return _data; }
    const size_t len() const { return _len; }
private:
    const int _error;
    const int _subcode;
    const uint8_t *_data;
    const size_t _len;
};


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
    uint16_t length() const			{ return _Length; }
    virtual string str() const = 0;

    virtual const uint8_t *encode(int &len) const = 0;
protected:
    /*
     * Return the external representation of the packet into a buffer.
     * If the caller supplies the buffer it is its responsibility to make
     * sure that it has the correct size, otherwise the routine will
     * allocate it with new[length()].
     * In all cases it is responsibility of the caller to dispose
     * of the buffer.
     * Note that this routine will only copy the fixed_header part.
     * The derived-class methods are in charge of filling up any
     * additional data past it.
     */
    uint8_t *basic_encode(int &len, uint8_t *buf=0) const;

    const uint8_t *flatten(struct iovec *iov, int cnt, int& len) const;
    // don't allow the use of the default copy constructor
    BGPPacket(const BGPPacket& BGPPacket);
    uint16_t _Length;  /*length is the total packet length, including
                         the BGP common header*/
    uint8_t _Type;
private:
};

/* **************** OpenPacket *********************** */

class OpenPacket : public BGPPacket {
public:
    OpenPacket(const uint8_t *d, uint16_t l);
    OpenPacket(const AsNum& as, const IPv4& bgpid, const uint16_t holdtime);
    ~OpenPacket()				{}
    const uint8_t *encode(int& len) const;
    string str() const;

    const uint8_t Version() const { return _Version; }
    const AsNum AutonomousSystemNumber() const {
	return _AutonomousSystemNumber;
    }
    const uint16_t HoldTime() const { return _HoldTime; }
    const IPv4 BGPIdentifier() const { return _BGPIdentifier; }
    const uint8_t OptParmLen() const { return _OptParmLen; }
    bool operator==(const OpenPacket& him) const;
    void add_parameter(const BGPParameter *p);

    const list <BGPParameter*>& parameter_list() const {
	return _parameter_list;
    }

protected:

private:
    void decode(const uint8_t *d, uint16_t l) throw(CorruptMessage);
    OpenPacket();
    // don't allow the use of the default copy constructor
    OpenPacket(const OpenPacket& OpenPacket);

    uint8_t _Version;
    AsNum _AutonomousSystemNumber;
    IPv4 _BGPIdentifier;
    uint16_t _HoldTime;
    uint8_t _OptParmLen;

    list <BGPParameter*> _parameter_list;
    uint8_t _num_parameters;
};

/* **************** UpdatePacket *********************** */

class UpdatePacket : public BGPPacket {
public:
    UpdatePacket();
    UpdatePacket(const uint8_t *d, uint16_t l);
    ~UpdatePacket();

    void add_withdrawn(const BGPWithdrawnRoute& wdr);
    void add_pathatt(const PathAttribute& pa);
    void add_nlri(const NetLayerReachability& nlri);
    const list <BGPWithdrawnRoute>& withdrawn_list() const {
	return _withdrawn_list;
    }
    const list <PathAttribute*>& pathattribute_list() const {
	return _att_list;
    }
    const list <NetLayerReachability>& nlri_list() const {
	return _nlri_list;
    }
    const uint8_t *encode(int& len) const;

    bool big_enough() const;

    string str() const;
    bool operator==(const UpdatePacket& him) const;
protected:

private:
    void decode(const uint8_t *d, uint16_t l) throw(CorruptMessage);
    // don't allow the use of the default copy constructor
    UpdatePacket(const UpdatePacket& UpdatePacket);
    list <BGPWithdrawnRoute> _withdrawn_list;
    list <PathAttribute*> _att_list;
    list <NetLayerReachability> _nlri_list;
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
    const uint8_t *encode(int &len) const;
    string str() const;
    bool operator==(const NotificationPacket& him) const;
protected:

private:
    // void decode() throw(InvalidPacket);
    // don't allow the use of the default copy constructor
    NotificationPacket(const NotificationPacket& Notificationpacket);
    uint8_t _error_code;
    uint8_t _error_subcode;
    const uint8_t* _error_data;
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
    KeepAlivePacket(const uint8_t *, uint16_t l)	{
	_Type = MESSAGETYPEKEEPALIVE;
	_Length = l;
    }

    KeepAlivePacket()					{
	_Type = MESSAGETYPEKEEPALIVE;
	_Length = MINKEEPALIVEPACKET;
    }
    ~KeepAlivePacket()					{}
    const uint8_t *encode(int &len) const		{
	return basic_encode(len);
    }
    virtual string str() const		{ return "Keepalive Packet\n"; }
    bool operator==(const KeepAlivePacket&) const	{ return true; }
protected:

private:
    // don't allow the use of the default copy constructor
    KeepAlivePacket(const KeepAlivePacket& KeepAlivePacket);
};

#endif // __BGP_PACKET_HH__
