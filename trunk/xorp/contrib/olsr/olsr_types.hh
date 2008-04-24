// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP$

#ifndef __OLSR_OLSR_TYPES_HH__
#define __OLSR_OLSR_TYPES_HH__

/**
 * @short A scope to hold primitive types specific to OLSR.
 *
 * These are named exactly as they are in RFC 3626, to avoid
 * any confusion.
 */
struct OlsrTypes {
    /**
     * @short The default static cost of an OLSR interface.
     */
    static const int DEFAULT_STATIC_FACE_COST = 0;

    /**
     * @short The maximum time-to-live of an OLSR message.
     */
    static const int MAX_TTL = 255;

    /*
     * Constants from Section 18.2.
     */

    /**
     * @short The default interval between HELLO advertisements, in seconds.
     */
    static const int DEFAULT_HELLO_INTERVAL = 2;

    /**
     * @short The default interval between link advertisements, in seconds.
     * Used in situations where HELLO advertisements are segmented.
     */
    static const int DEFAULT_REFRESH_INTERVAL = 2;

    /**
     * @short The default interval between TC broadcasts, in seconds.
     */
    static const int DEFAULT_TC_INTERVAL = 5;

    /**
     * @short The default lifetime for each histogram-based duplicate
     * message detection entry.
     */
    static const int DEFAULT_DUP_HOLD_TIME = 30;

    /**
     * @short The default interval between MID broadcasts, in seconds.
     */
    static const int DEFAULT_MID_INTERVAL = DEFAULT_TC_INTERVAL;

    /**
     * @short The default interval between HNA broadcasts, in seconds.
     */
    static const int DEFAULT_HNA_INTERVAL = DEFAULT_TC_INTERVAL;

    /**
     * @short The type of an OLSR external route ID.
     */
    typedef uint32_t ExternalID;

    static const ExternalID UNUSED_EXTERNAL_ID = 0;

    /**
     * @short The type of an OLSR interface ID.
     */
    typedef uint32_t FaceID;

    static const FaceID UNUSED_FACE_ID = 0;

    /*
     * Defaults controlling link hysteresis defined in link.cc.
     * Used to derive link quality when RFC-compliant
     * link hysteresis is enabled.
     */

    /**
     * The default link hysteresis high threshold.
     */
    static double DEFAULT_HYST_THRESHOLD_HIGH;

    /**
     * The default link hysteresis low threshold.
     */
    static double DEFAULT_HYST_THRESHOLD_LOW;

    /**
     * The default link hysteresis scaling factor.
     */
    static double DEFAULT_HYST_SCALING;

    /**
     * @short The type of an OLSR link, as found in a HELLO message.
     */
    typedef uint8_t LinkType;

    enum LinkTypes {
	UNSPEC_LINK = 0,
	ASYM_LINK = 1,
	SYM_LINK = 2,
	LOST_LINK = 3,
	// May be used as a vector or array index.
	LINKTYPE_START = UNSPEC_LINK,
	LINKTYPE_END = LOST_LINK
    };
    static const int NUM_LINKTYPE =
	LINKTYPE_END - LINKTYPE_START + 1;

    /**
     * @short The type of an OLSR link ID.
     */
    typedef uint32_t LogicalLinkID;

    static const LogicalLinkID UNUSED_LINK_ID = 0;

    /**
     * @short The type of an OLSR protocol message.
     */
    typedef uint8_t MessageType;

    enum MessageTypes {
	HELLO_MESSAGE = 1,
	TC_MESSAGE = 2,
	MID_MESSAGE = 3,
	HNA_MESSAGE = 4,
	LQ_HELLO_MESSAGE = 201,	    // HELLO with ETX measurements.
	LQ_TC_MESSAGE = 202	    // TC with ETX measurements.
    };

    /**
     * @short The type of a multiple interface descriptor (MID) ID.
     */
    typedef uint32_t MidEntryID;

    static const MidEntryID UNUSED_MID_ID = 0;

    /**
     * @short The type of an OLSR neighbor ID.
     */
    typedef uint32_t NeighborID;

    static const NeighborID UNUSED_NEIGHBOR_ID = 0;

    /**
     * @short The type of an OLSR neighbor, as found in a HELLO message.
     */
    typedef uint8_t NeighborType;

    enum NeighborTypes {
	NOT_NEIGH = 0,
	SYM_NEIGH = 1,
	MPR_NEIGH = 2,
	// May be used as a vector or array index.
	NEIGHBORTYPE_START = NOT_NEIGH,
	NEIGHBORTYPE_END = MPR_NEIGH,
    };
    static const int NUM_NEIGHBORTYPE =
	NEIGHBORTYPE_END - NEIGHBORTYPE_START + 1;

    /**
     * @short The default UDP port for the OLSR protocol.
     */
    static const uint16_t DEFAULT_OLSR_PORT = 698;

    /**
     * @short Type representing a node's willingness to forward.
     */
    typedef uint8_t WillType;

    enum Willingness {
	WILL_NEVER = 0,
	WILL_LOW = 1,
	WILL_DEFAULT = 3,
	WILL_HIGH = 6,
	WILL_ALWAYS = 7,
	// limits
	WILL_MIN = WILL_LOW,
	WILL_MAX = WILL_ALWAYS,
	// bounds
	WILL_START = WILL_NEVER,
	WILL_END = WILL_ALWAYS
    };

    /**
     * @short Type representing a node's TC redundancy mode.
     * Section 15.1: TC_REDUNDANCY parameter.
     */
    typedef uint8_t TcRedundancyType;

    enum TcRedundancyMode {
	TCR_MPRS_IN = 0,	// MPR selectors only
	TCR_MPRS_INOUT = 1,	// MPR selectors and MPRs
	TCR_ALL = 2,		// The full neighbor set.
	// bounds
	TCR_START = TCR_MPRS_IN,
	TCR_END = TCR_ALL
    };

    /**
     * @short The default TC_REDUNDANCY value.
     */
    static const uint8_t DEFAULT_TC_REDUNDANCY = TCR_MPRS_IN;

    /**
     * @short The default number of MPRs which must cover a two-hop
     * neighbor when the OLSR MPR selection algorithm is enabled.
     */
    static const uint8_t DEFAULT_MPR_COVERAGE = 1;

    /**
     * @short The type of an OLSR topology set entry ID.
     */
    typedef uint32_t TopologyID;

    static const TopologyID UNUSED_TOPOLOGY_ID = 0;

    /**
     * @short The type of an OLSR two-hop node ID.
     */
    typedef uint32_t TwoHopNodeID;

    static const TwoHopNodeID UNUSED_TWOHOP_NODE_ID = 0;

    /**
     * @short The type of an OLSR two-hop link ID.
     */
    typedef uint32_t TwoHopLinkID;

    static const TwoHopLinkID UNUSED_TWOHOP_LINK_ID = 0;

    /**
     * @short Type representing the type of a Vertex in the
     * shortest-path-tree.
     *
     * SPT is a meta-class which embeds the Vertex as a POD type,
     * therefore, inheritance cannot be used to represent the vertex type.
     */
    enum VertexType {
	VT_ORIGINATOR = 0,	// iff origin() is true
	VT_UNKNOWN = 0,
	VT_NEIGHBOR = 1,	// Nodes at radius 1.
	VT_TWOHOP = 2,		// Nodes at radius 2.
	VT_TOPOLOGY = 3,
	VT_MID = 4,		// Not a vertex type; used by routing table
	VT_HNA = 5,		// do.
	// bounds
	VT_START = VT_ORIGINATOR,
	VT_END = VT_HNA
    };

    /**
     * @short Type fields used when saving OLSR databases.
     */
    enum TlvType {
	TLV_VERSION = 1,	// Version number (u32)
	TLV_SYSTEM_INFO = 2,	// Where created (string)
	TLV_OLSR_VERSION = 3,	// Version of OLSR in use (u32)
	TLV_OLSR_FAMILY = 4,	// Protocol family (u32)
	TLV_OLSR_N1 = 5,	// One-hop neighbor
	TLV_OLSR_L1 = 6,	// One-hop link
	TLV_OLSR_N2 = 7,	// Two-hop neighbor
	TLV_OLSR_L2 = 8,	// Two-hop link
	TLV_OLSR_MID = 9,	// MID entry
	TLV_OLSR_TC = 10,	// Topology Control entry
	TLV_OLSR_HNA = 11,	// HNA entry
    };
};

/**
 * @return true if the given vertex type maps directly to a single
 * OLSR node.
 */
inline bool
is_node_vertex(const OlsrTypes::VertexType vt)
{
    if (vt == OlsrTypes::VT_NEIGHBOR || vt == OlsrTypes::VT_TWOHOP ||
	vt == OlsrTypes::VT_TOPOLOGY || vt == OlsrTypes::VT_MID)
	return true;
    return false;
}

/**
 * @short Helper class to encode a TimeVal as an 8 bit binary
 * floating point value.
 */
class EightBitTime {
public:
	/**
	 * @short Helper function to convert from a TimeVal
	 * to the eight-bit floating point format used by OLSR.
	 */
	static inline TimeVal to_timeval(const uint8_t byte) {
		unsigned int mant = byte >> 4;
		unsigned int exp = byte & 0x0F;
		double sec = ((16 + mant) << exp) * _scaling_factor / 16.0;
		return (TimeVal(sec));
	}

	/**
	 * @short Helper function to convert the eight-bit floating point
	 * format used by OLSR to a TimeVal.
	 */
	static inline uint8_t from_timeval(const TimeVal& tv) {
		double sec = tv.get_double();
		int isec = static_cast<int>(sec / _scaling_factor);
		int mant = 0;
		int exp = 0;
		while (isec >= (1 << exp))
			exp++;
		if (exp == 0) {
			mant = 1;
		} else {
			exp--;
			if (mant > (_mod - 1)) {
				// ceiling
				mant = exp = (_mod - 1);
			} else {
				mant = static_cast<int>(_mod * sec /
				    _scaling_factor / (1 << exp) - _mod);
				exp += mant >> 4;
				mant &= 0x0F;
			}
		}
		return (static_cast<uint8_t>(mant << 4 | (exp & 0x0F)));
	}

private:
	static const int	_mod = 16;
	static const double	_scaling_factor = 0.0625f;
};

/**
 * Helper function to compare two sequence numbers.
 * RFC 3626 Section 19: Sequence Numbers.
 *
 * @param seq1 An OLSR sequence number
 * @param seq2 An OLSR sequence number
 * @return true if seq1 is newer than seq2.
 */
inline bool
is_seq_newer(const uint16_t seq1, const uint16_t seq2)
{
    // UINT16_MAX is defined in C99 stdint.h, however this is not
    // part of the C++ Standard yet.
    static const uint16_t uint16_max = 65535;
    return  seq1 > seq2 && seq1 - seq2 <= uint16_max/2 ||
	    seq2 > seq1 && seq2 - seq1 > uint16_max/2;
}

/**
 * @short Return a C string describing a topology control flooding mode.
 */
const char* tcr_to_str(const OlsrTypes::TcRedundancyType t);

/**
 * @short Return a C string describing a willingness-to-forward mode.
 */
const char* will_to_str(const OlsrTypes::WillType t);

/**
 * @return a C string describing a vertex type.
 */
const char* vt_to_str(const OlsrTypes::VertexType vt);

#endif // __OLSR_OLSR_TYPES_HH__
