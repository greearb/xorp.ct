/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Definition of the link state advertisement (LSA) formats, as
 * they are received from the network. These are described in
 * Appendix A of the OSPF specification.
 *
 * LSA instances are compared by examining the LS sequence number,
 * LS age and LS checksum fields. For this reason, these fields
 * get their own defines. Link State ID is also compared in certain
 * tie-breakers (like forwarding addresses in AS-external-LSAs).
 * Advertising router comes from this same space.
 */

/*
 * The common 20-byte header for all LSAs. Described in
 * Section A.4.1 of the OSPF spec.
 */

struct LShdr {
    age_t ls_age; 	/* Age of LSA, in seconds */
    byte ls_opts;	/* Options field */
    byte ls_type;	/* Type of LSA (see below) */
    lsid_t ls_id; 	/* Link state ID */
    rtid_t ls_org; 	/* Advertising router */
    seq_t ls_seqno;	/* LS Sequence number */
    xsum_t ls_xsum;	/* LS checksum (fletcher) */
    uns16 ls_length;	/* Length of LSA, including header (bytes) */

    LShdr& operator=(class LSA &);
    bool verify_cksum();
    void generate_cksum();
    inline bool do_not_age();
};

/* Definitions for the "ls_type" field */

enum {
    LST_RTR = 1,	// Router-LSAs
    LST_NET = 2,	// Network-LSAs
    LST_SUMM = 3,	// Summary-link LSAs (inter-area routes)
    LST_ASBR = 4,	// ASBR-summaries (inter-area)
    LST_ASL = 5,	// AS-external_LSAs
    LST_GM = 6,		// Group-membership-LSA (MOSPF)
    LST_NSSA = 7,	// NSSA externals
    LST_EXATTR = 8,	// External-attributes LSA
    LST_LINK_OPQ = 9,	// Link-scoped Opaque-LSA
    LST_AREA_OPQ = 10,	// Area-scoped Opaque-LSA
    LST_AS_OPQ = 11,	// AS-scoped Opaque-LSA
    MAX_LST = 11,	// Maximum number of supported LSA types
};

inline bool LShdr::do_not_age()
{
    return((ls_age & DoNotAge) != 0);
}

/* Way of expressing per-TOS metric values.
 * Found in the body of router-LSAs and summary-LSAs.
 */

struct TOSmetric {	// non-zero TOS metric description
    byte tos;		// non-zero TOS value
    byte hibyte;	// high byte of metric
    uns16 metric;
};

/* The Router-LSA. Consists of a standard link state header, followed
 * by 4 bytes of "router type" and "# links", followed by a variable
 * number of link records. Each link consists of 12 bytes of
 * "Link ID", "Link Data", "type", "# additional TOS" and the TOS 0
 * metric, followed by a variable number of non-zero TOS metrics.
 */

struct RTRhdr {
    byte rtype;		// Router type
    byte zero;		// Unused
    uns16 nlinks;	// # links in router-LSA
};

enum {			// Bits in router type field
    RTYPE_B = 0x01,	// Area border router
    RTYPE_E = 0x02,	// ASBR
    RTYPE_V = 0x04,	// Virtual link endpoint
    RTYPE_W = 0x08,	// Wild-card multicast receiver
};

struct RtrLink {	// Per-link data
    rtid_t link_id;
    InAddr link_data;
    byte link_type;
    byte n_tos;
    uns16 metric;
};

enum {			// Values for link type
    LT_PP = 1,		// Point-to-point router link
    LT_TNET = 2,	// Link to transit network
    LT_STUB = 3,	// Link to stub network
    LT_VL = 4,		// Virtual link
};

/* All network-LSAs, summary-LSAs and AS-external-LSAs begin
 * with the standard 20 byte link state header, and then a network
 * mask. Network-LSAs then proceed with a list of router-IDs,
 * summary-LSAs with a list of "TOSmetric"s, and AS-external-LSAs
 * with a list of "ASmetric"s.
 */

struct NetLShdr {
    InAddr netmask;	// Network mask
};

/* Format of type-3 and type-4 summary-LSAs.
 * Standard LS header, plus network mask and a metric.
 * Optionally contains further TOS metrics.
 */

struct SummHdr {
    uns32 mask;		// network mask
    uns32 metric;	// Metric (24-bits only)
};

/* Body of an AS-external-LSA.
 * TOS metric together with forwarding address and
 * External Route tage may be repeated multiple times.
 */

struct ASEhdr {
    uns32 mask;		// network mask
    byte tos;		// non-zero TOS value
    byte hibyte;	// high byte of metric
    uns16 metric;
    uns32 faddr;	// Forwarding address
    uns32 rtag;		// External route tag
};

// bit indicating external-type 2 metric within TOS field
const byte E_Bit = 0x80;

/* Extra TOS metrics, forwarding addresses and tags that can
 * be appended to an AS-external-LSA.
 */

struct ASEmetric {
    byte tos;		// non-zero TOS value
    byte hibyte;	// high byte of metric
    uns16 metric;
    uns32 faddr;	// Forwarding address
    uns32 rtag;		// External route tag
};

/* References contained in the body of the group-membership-LSA
 */

struct GMref {
    uns32 ls_type;	// Router-LSA or network-LSA
    rtid_t ls_id;	// Link State ID
};

/* Assigned Opaque Types.
 */

enum {
    OPQ_T_TE = 1,	// Traffic Engineering extensions
    OPQ_T_HLRST = 3,	// Hitless restart extensions
};

/* Format of the TLVs found in the body of some Opaque-LSAs.
 * Length field covers only the body, not the header, and
 * when the length is not a multiple of 4 bytes, the TLV
 * is padded with zeroes.
 */

struct TLV {
    uns16 type;
    uns16 length;
};

/* TLV types used in the Hitless Restart extensions
 */

enum {
    HLRST_PERIOD = 1,	// Length of grace period
    HLRST_REASON = 2,	// Reason for restart
    HLRST_IFADDR = 3,	// Interface address
};

/* Encodings for the reason for a hitless restart.
 */

enum {
   HLRST_REASON_UNKNOWN = 0,
   HLRST_REASON_RESTART = 1,
   HLRST_REASON_RELOAD = 2,	// Reload/upgrade
   HLRST_REASON_SWITCH = 3,	// Switch to redundant processor
};
