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

/* Representation of an LSA when stored in the router's
 * database. Contains the link state header (converted to
 * machine byte-order), and the body of the advertisement
 * (untouched from network).
 */

class SpfIfc;
class TLink;

/*
 * Base class for the internal representation of all LSA
 * types.
 */

class LSA : public AVLitem {
protected:
    // Link state header fields
    age_t lsa_rcvage;	// LS age when received
    byte lsa_opts;	// LS Options
    const byte lsa_type;	// Type of LSA
    seq_t lsa_seqno;	// LS Sequence number
    xsum_t lsa_xsum;	// LS checksum (fletcher)
    uns16 lsa_length;	// Length of LSA, in bytes

    byte *lsa_body;	// LSA body
    class SpfIfc *lsa_ifp;// Interface for link-scoped LSAs
    class SpfArea *lsa_ap; // Containing area
    LSA	*lsa_agefwd;	// forward link in age bins
    LSA	*lsa_agerv;	// reverse link in age bins
    uns16 lsa_agebin;	// Age bin
    uns16 lsa_rxmt;	// #Retransmission lists
    uns16 in_agebin:1,	// In an age bin?
	deferring:1,	// Awaiting deferred origination
	changed:1,	// Changed since last flood
	exception:1,	// Must store complete body
	rollover:1,	// LS sequence being rolled over
	e_bit:1,	// Type-2 external metric
	parsed:1,	// Parsed for easy calculation?
        sent_reply:1,	// Sent reply for older LSA received
        checkage:1,	// Queued for xsum verification
        min_failed:1,	// MinArrival failed
        we_orig:1;	// We have originated this LSA
    uns16 lsa_hour;	// Hour counter, for DoNotAge refresh

    static LSA *AgeBins[MaxAge+1];// Aging Bins
    static int Bin0;	// Current age 0 bin
    static int32 RefreshBins[MaxAgeDiff]; // Refresh bins
    static int RefreshBin0; // Current refresh bin

    void hdr_parse(LShdr *hdr);
    virtual void parse(LShdr *);
    virtual void unparse();
    virtual void process_donotage(bool parse);
    virtual void build(LShdr *);
    virtual void delete_actions();

public:
    RTE	*source;	// Routing table entry of orig. rtr

    LSA(class SpfIfc *, class SpfArea *, LShdr *, int blen = 0);
    virtual ~LSA();

    inline byte ls_type();
    inline lsid_t ls_id();
    inline rtid_t adv_rtr();
    inline seq_t ls_seqno();
    inline uns16 ls_length();
    inline class SpfArea *area();
    inline bool do_not_age();
    inline age_t lsa_age();
    inline int is_aging();
    inline age_t since_received();

    int	cmp_instance(LShdr *hdr);
    int	cmp_contents(LShdr *hdr);
    void start_aging();
    void stop_aging();
    int	refresh(seq_t);
    void flood(class SpfNbr *from, LShdr *hdr);

    virtual void reoriginate(int) {}
    virtual RTE *rtentry();
    virtual void update_in_place(LSA *);

    friend class OSPF;
    friend class SpfNbr;
    friend class SpfIfc;
    friend class SpfArea;
    friend class LsaListIterator;
    friend class LocalOrigTimer;
    friend class DBageTimer;
    friend void hdr_parse(LSA *, LShdr *);
    friend LShdr& LShdr::operator=(class LSA &lsa);
    friend inline uns16 Age2Bin(age_t);
    friend inline age_t Bin2Age(uns16);
    friend void lsa_flush(LSA *);
};

// Inline functions
inline byte LSA::ls_type()
{
    return(lsa_type);
}
inline lsid_t LSA::ls_id()
{
    return((lsid_t) index1());
}
inline rtid_t LSA::adv_rtr()
{
    return((rtid_t) index2());
}
inline seq_t LSA::ls_seqno()
{
    return(lsa_seqno);
}
inline uns16 LSA::ls_length()
{
    return(lsa_length);
}
inline class SpfArea *LSA::area()
{
    return(lsa_ap);
}
inline bool LSA::do_not_age()
{
    return((lsa_rcvage & DoNotAge) != 0);
}


// Values for flooding scope

enum {
    LocalScope = 1,  	// Per Segment
    AreaScope, 		// Per area
    GlobalScope,  	// Global significance
};

/* Representation of a transit node (router or a transit network).
 * Built on top of an LSA, the transit node contains all the information
 * necessary to run the shortest path calculation.
 * Tranist nodes contain collections of trsnit and stub links.
 * A transit link is either a link between
 * two routers, or a link between a router and a transit network.
 * Stub links appear only in router-LSAs; they are not processed in
 * the Dijkstra calculation, but are instead processed by a second-stage
 * Bellman-Ford-like calculation.
 */

class TNode : public LSA, public PriQElt {
protected:
    class Link *t_links; // transit or stub links
    RTE	*t_dest;	// Destination routing table entry
    // Dynamically changing Dijkstra fields
    byte dijk_run:1,	// Dijkstra run, sequence number
	t_direct:1,	// Directly attached to root?
	t_downstream:1,	// Downstream from MOSPF root
	in_mospf_cache:1;// Linked into MOSPF entry under construction
    byte t_state;	// Uninit, on cand or SPF
    byte il_type;	// Incoming link type (MOSPF)
    byte t_ttl;		// TTL from us to node, on MOSPF branch
    byte closest_member;// TTL to closest group member, through this next hop
    TNode *t_parent;	// Parent on SPF tree
    TNode *t_mospf_dsnode; // Node directly downstream on this branch
    MPath *t_mpath;	// Multipath entry
public:
    TNode(class SpfArea *, LShdr *, int blen);
    virtual ~TNode();
    void tlp_link(TLink *tlp);
    void unlink();
    void dijk_install();
    void add_next_hop(TNode *parent, int index);
    bool has_members(InAddr group);
    InAddr ospf_find_gw(TNode *parent, InAddr, InAddr);
    virtual bool is_wild_card();

    friend class OSPF;
    friend class netLSA;
    friend class rtrLSA;
    friend class RTE;
    friend class VLIfc;
    friend class SpfArea;
};

// Encoding of t_state

enum {
    DS_UNINIT = 0, 	// Uninitialized
    DS_ONCAND,		// On candidate list
    DS_ONTREE,		// On SPF tree
};

// Representation of a link within a transit node
// Could be either a transit or stub link
class Link {
public:
    Link *l_next;	// Singly linked list
    uns32 l_id;		// Neighboring node ID
    uns32 l_data;	// Link data
    byte l_ltype;	// Link type
    uns16 l_fwdcst;	// Forward cost

    inline Link();
    friend class rtrLSA;
    friend class netLSA;
    friend class OSPF;
};

inline Link::Link() : l_next(0)
{
}

// Representation of a transit link within a transit node
class TLink : public Link {
public:
    TNode *tl_nbr;	// Neighboring node
    uns16 tl_rvcst;	// Reverse cost

    inline TLink();
    friend class rtrLSA;
    friend class netLSA;
    friend class OSPF;
};

inline TLink::TLink() : tl_nbr(0), tl_rvcst(MAX_COST)
{
}

// Representation of a stub link within a transit node
class SLink : public Link {
    INrte *sl_rte;	// Stubs routing table entry
public:
    friend class rtrLSA;
    friend class OSPF;
};

/* Common base class for those LSA representing stub links
 * on the shortest-path tree (summary-LSAs, AS-external-LSAs)
 */

class rteLSA : public LSA {
public:
    INrte *rte;		// Destination routing table entry
    INrte *orig_rte;	// Routing table leading to own LSA
    rteLSA *link;	// Linked together in rte
    uns32 adv_cost;	// advertised cost (TOS 0)

    rteLSA(class SpfArea *, LShdr *, int blen = 0);
    virtual RTE *rtentry();
    virtual void update_in_place(LSA *);
    friend class OSPF;
};

/* Internal representation of each of the seven types of OSPF LSAs
 * that we support. Built on top of either the LSA or the TNode classes:
 *	Router-LSA (rtrLSA): Built on top of the TNode class.
 *		Fields added include the list of stub links in the
 *		the router-LSA, router type (ABR and/or ASBR), and the
 *		ASBR routing table entry.
 *	Network-LSA (netLSA):
 *	Summary-LSA (summLSA):
 *	ASBR-summary-LSA (asbrLSA):
 *	AS-external-LSA (ASextLSA):
 *	Group-membership-LSA (grpLSA):
 */

class rtrLSA : public TNode {
    uns16 n_links;
    byte rtype;
public:
    rtrLSA(class SpfArea *, LShdr *, int blen);
    inline bool is_abr();
    inline bool is_asbr();
    inline bool has_VLs();
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);
    virtual bool is_wild_card();
    friend class OSPF;
    friend class RTRrte;
};

inline bool rtrLSA::is_abr()
{
    return((rtype & RTYPE_B) != 0);
}
inline bool rtrLSA::is_asbr()
{
    return((rtype & RTYPE_E) != 0);
}
inline bool rtrLSA::has_VLs()
{
    return((rtype & RTYPE_V) != 0);
}

class netLSA : public TNode {
public:
    netLSA(class SpfArea *, LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);
    virtual void delete_actions();
    friend class OSPF;
};

class summLSA : public rteLSA {
public:
    summLSA(class SpfArea *, LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);

    friend void INrte::run_inter_area();
    friend void RTE::run_transit_areas(rteLSA *lsap);
    friend class OSPF;
};

class asbrLSA : public rteLSA {
    ASBRrte *asbr; 	// Corresponding routing table entry
public:
    asbrLSA(class SpfArea *, LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void process_donotage(bool parse);
    virtual void build(LShdr *hdr);
    virtual RTE *rtentry();
    friend class OSPF;
};

class ASextLSA : public rteLSA {
    FWDrte *fwd_addr;	// Forwarding address routing entry
    uns32 adv_tag;	// Advertised tag
public:
    ASextLSA(LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);
    friend class OSPF;
    friend class INrte;
    friend class SpfArea;
};

/* Body of group-membership-LSA points to one or more
 * router-LSAs and/or network-LSAs.
 * We don't bother to parse these, since there's not much
 * we can do to speed the calculation.
 */

class grpLSA : public LSA {
public:
    grpLSA(class SpfArea *, LShdr *, int blen);
    virtual void reoriginate(int forced);
    virtual void parse(LShdr *hdr);
    virtual void unparse();
    virtual void build(LShdr *hdr);
    friend class TNode;
};
