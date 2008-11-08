
// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/packet.hh,v 1.47 2008/10/02 21:56:16 bms Exp $

#ifndef __BGP_PACKET_HH__
#define __BGP_PACKET_HH__

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv4.hh"

#include "path_attribute.hh"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "libproto/packet.hh"

#include "exceptions.hh"
#include "parameter.hh"
#include "local_data.hh"
#include "peer_data.hh"
#include "path_attribute.hh"
#include "update_attrib.hh"

class BGPPeer;

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
#define		BADASPEER    2		// Bad BGPPeer AS
#define		BADBGPIDENT  3		// Bad BGP Identifier
#define		UNSUPOPTPAR  4		// Unsupported Optional Parameter
#define		AUTHFAIL     5		// Authentication Failure
#define		UNACCEPTHOLDTIME 6	// Unacceptable Hold Time
#define		UNSUPCAPABILITY	7	// Unsupported Capability (RFC 3392)
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

    //
    // The BGP common header (on the wire)
    //
    // struct fixed_header {
    //     uint8_t	marker[MARKER_SIZE];	// normally all bits are ones.
    //     uint16_t	length;			// this is in network format 
    //     uint8_t	type;			// enum BgpPacketType
    // };
    //

    // Various header-related constants
    static const size_t MARKER_SIZE = 16;
    static const size_t COMMON_HEADER_LEN = MARKER_SIZE
				+ sizeof(uint16_t) + sizeof(uint8_t);
    static const size_t MARKER_OFFSET = 0;
    static const size_t LENGTH_OFFSET = MARKER_SIZE;
    static const size_t TYPE_OFFSET = LENGTH_OFFSET + sizeof(uint16_t);

    // 
    // Various min and max packet size, accounting for the basic
    // information in the packet itself.
    //
    static const size_t MINPACKETSIZE = COMMON_HEADER_LEN;
    static const size_t MAXPACKETSIZE = 4096;
    static const size_t MINOPENPACKET = COMMON_HEADER_LEN + 10;
    static const size_t MINUPDATEPACKET = COMMON_HEADER_LEN + 2 + 2;
    static const size_t MINKEEPALIVEPACKET = COMMON_HEADER_LEN;
    static const size_t MINNOTIFICATIONPACKET = COMMON_HEADER_LEN + 2;

    // The default marker.
    static const uint8_t Marker[MARKER_SIZE];

    BGPPacket()					{}
    virtual ~BGPPacket()			{}
    uint8_t type() const			{ return _Type; }
    virtual string str() const = 0;

    virtual bool encode(uint8_t *buf, size_t &len, const BGPPeerData *peerdata) const = 0;
protected:
    /*
     * Return the external representation of the packet into a buffer.
     * If the caller supplies the buffer it is its responsibility to make
     * sure that it has the correct size, otherwise the routine will
     * allocate it with new uint8_t[len].
     * It is responsibility of the caller to dispose of the buffer.
     * Note that this routine will only copy the BGP common header part.
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
    bool encode(uint8_t *buf, size_t& len, const BGPPeerData *peerdata) const;
    string str() const;

    uint8_t Version() const			{ return _Version; }
    const AsNum as() const			{ return _as; }
    uint16_t HoldTime() const			{ return _HoldTime; }
    const IPv4 id() const			{ return _id; }
    uint8_t OptParmLen() const			{ return _OptParmLen; }
    bool operator==(const OpenPacket& him) const;
    void add_parameter(const ParameterNode& p);

    const ParameterList& parameter_list() const { return _parameter_list; }

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

    ParameterList _parameter_list;
};

/* **************** UpdatePacket *********************** */

class UpdatePacket : public BGPPacket {
public:
    UpdatePacket();
    UpdatePacket(const uint8_t *d, uint16_t l, const BGPPeerData *peerdata,
		 BGPMain* mainprocess, bool do_checks)
	throw(CorruptMessage);

    ~UpdatePacket();

    void add_withdrawn(const BGPUpdateAttrib& wdr);
    void replace_pathattribute_list(FPAList4Ref& pa_list);

    void add_pathatt(const PathAttribute& pa);
    void add_pathatt(PathAttribute *pa);

    void add_nlri(const BGPUpdateAttrib& nlri);
    const BGPUpdateAttribList& wr_list() const		{ return _wr_list; }
    FPAList4Ref& pa_list() 	                        { return  _pa_list; }
    const BGPUpdateAttribList& nlri_list() const	{ return _nlri_list; }

    template <typename A> const MPReachNLRIAttribute<A> *mpreach(Safi) const;
    template <typename A> const MPUNReachNLRIAttribute<A> *mpunreach(Safi) const;

    bool encode(uint8_t *buf, size_t& len, const BGPPeerData *peerdata) const;

    bool big_enough() const;

    string str() const;
    bool operator==(const UpdatePacket& him) const;
protected:

private:
    // don't allow the use of the default copy constructor
    UpdatePacket(const UpdatePacket& UpdatePacket);

    BGPUpdateAttribList		_wr_list;
    FPAList4Ref	                _pa_list;
    BGPUpdateAttribList		_nlri_list;
};

template <typename A> 
const MPReachNLRIAttribute<A> *
UpdatePacket::mpreach(Safi safi) const
{
    XLOG_ASSERT(!(A::ip_version() == 4 && SAFI_UNICAST == safi));
    FastPathAttributeList<IPv4>& fpalist = *_pa_list;
    MPReachNLRIAttribute<A>* mpreach = fpalist.mpreach<A>(safi);
    return mpreach;
}

template <typename A> 
const MPUNReachNLRIAttribute<A> *
UpdatePacket::mpunreach(Safi safi) const
{
    XLOG_ASSERT(!(A::ip_version() == 4 && SAFI_UNICAST == safi));
    FastPathAttributeList<IPv4>& fpalist = *_pa_list;
    MPUNReachNLRIAttribute<A>* mpunreach = fpalist.mpunreach<A>(safi);
    return mpunreach;
}


/* **************** BGPNotificationPacket *********************** */

class NotificationPacket : public BGPPacket {
public:
    NotificationPacket(const uint8_t *d, uint16_t l) throw(CorruptMessage);
    NotificationPacket(uint8_t ec, uint8_t esc = 0,
		       const uint8_t *d = 0, size_t l=0);
    NotificationPacket();
    ~NotificationPacket()			{ delete[] _error_data; }
    uint8_t error_code() const 			{ return _error_code; }
    uint8_t error_subcode() const 		{ return _error_subcode; }
    /**
    * Verify that the supplied error code and subcode are legal.
    */
    static bool validate_error_code(const int error, const int subcode);
    /**
     * Generate a human readable error string.
     */
    static string pretty_print_error_code(const int error, const int subcode,
					  const uint8_t* error_data = 0,
					  const size_t len = 0);
    const uint8_t* error_data() const 		{ return _error_data; }
    bool encode(uint8_t *buf, size_t &len, const BGPPeerData *peerdata) const;
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
    KeepAlivePacket(const uint8_t *buf, uint16_t l)
		throw(CorruptMessage) {
	if (l != BGPPacket::MINKEEPALIVEPACKET)
	    xorp_throw(CorruptMessage,
		       c_format("KeepAlivePacket length %d instead of %u",
				l,
				XORP_UINT_CAST(BGPPacket::MINKEEPALIVEPACKET)),
		       MSGHEADERERR, BADMESSLEN, buf + BGPPacket::MARKER_SIZE,
		       2);

	_Type = MESSAGETYPEKEEPALIVE;
    }

    KeepAlivePacket() {
	_Type = MESSAGETYPEKEEPALIVE;
    }
    ~KeepAlivePacket()					{}
    bool encode(uint8_t *buf, size_t &len, const BGPPeerData *peerdata) const {
	UNUSED(peerdata);
	len = BGPPacket::MINKEEPALIVEPACKET;
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
