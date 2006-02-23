
/* Format and definitions for the packets used
 * by multicast traceroute.
 * Include the IGMP header.
 */

struct MTraceHdr {
    byte igmp_type;
    byte n_hops;
    uns16 ig_xsum;
    InAddr group;
    InAddr src;
    InAddr dest;
    InAddr respond_to;
    uns32 ttl_qid;

    inline byte ttl();
    inline uns32 query_id();
};

/* Number of hops to record is the high byte of
 * the last 32-bit word.
 */
inline byte MTraceHdr::ttl()
{
    return((ntoh32(ttl_qid) >> 24) & 0xff);
}
/* Query ID, used to match the response, is the
 * last 24-bits.
 */
inline uns32 MTraceHdr::query_id()
{
    return(ntoh32(ttl_qid) & 0xffffff);
}

/* Defines for the IGMP type. Most of these
 * are defined in src/igmp.h. One IGMP type
 * is used for the request as it is sent from
 * router-to-router, and then it is changed as the
 * multicast traceroute packet is returned back
 * to the originator.
 */

const byte IGMP_MTRACE_QUERY = 0x1F;
const byte IGMP_MTRACE_RESPONSE = 0x1E;

/* The body of the multicast traceroute packet
 * is a sequence of MtraceBody structures recording
 * the information on each hop of the multicast
 * datagram, in the reverse direction from the
 * group to the multicast datagram's source.
 */

struct MtraceBody {
    uns32 arrival_time;
    InAddr incoming_addr;
    InAddr outgoing_addr;
    InAddr prev_router;
    uns32 incoming_pkts;
    uns32 outgoing_pkts;
    uns32 fwd_count;
    byte prot;
    byte fwd_ttl;
    byte source_mask;
    byte fwd_code;
};

/* Defines for the MtraceBody::prot field, which indicates the
 * the multicast routing protocol that created the multicast
 * forwarding entry.
 */
const byte MTRACE_DVMRP = 1;
const byte MTRACE_MOSPF = 2;
const byte MTRACE_PIM = 3;
const byte MTRACE_CBT = 4;
const byte MTRACE_PIM_SPECIAL = 5;
const byte MTRACE_PIM_STATIC = 6;
const byte MTRACE_DVMRP_STATIC = 7;
const byte MTRACE_PIM_MBGP = 8;
const byte MTRACE_CBT_SPECIAL = 9;
const byte MTRACE_CBT_STATIC = 10;
const byte MTRACE_PIM_ASSERT = 11;

/* Definition of the forwarding code
 * MtraceBody::fwd_code.
 */

const byte MTRACE_NO_ERROR = 0x00;
const byte MTRACE_WRONG_IF = 0x01;
const byte MTRACE_PRUNE_SENT = 0x02;
const byte MTRACE_PRUNE_RCVD = 0x03;
const byte MTRACE_SCOPED = 0x04;
const byte MTRACE_NO_ROUTE = 0x05;
const byte MTRACE_WRONG_LAST_HOP = 0x06;
const byte MTRACE_NOT_FORWARDING = 0x07;
const byte MTRACE_REACHED_RP = 0x08;
const byte MTRACE_RPF_IF = 0x09;
const byte MTRACE_NO_MULTICAST = 0x0A;
const byte MTRACE_INFO_HIDDEN = 0x0B;
const byte MTRACE_NO_SPACE = 0x81;
const byte MTRACE_OLD_ROUTER = 0x82;
const byte MTRACE_ADMIN_PROHIB = 0x83;
