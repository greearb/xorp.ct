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

class TNode;
class LSA;
class SpfIfc;
class SpfData;
class INrte;

/* Data structure storing multiple equal-cost paths.
 */

struct NH {
    InAddr if_addr; // IP address of outgoing interface
    int phyint; // Physical interface
    InAddr gw;	// New hop gateway
};

class MPath : public PatEntry {
  public:
    int	npaths;
    NH	NHs[MAXPATH];
    int pruned_phyint;
    MPath *pruned_mpath;
    static PatTree nhdb;
    static MPath *create(int, NH *);
    static MPath *create(SpfIfc *, InAddr);
    static MPath *create(int, InAddr);
    static MPath *merge(MPath *, MPath *);
    static MPath *addgw(MPath *, InAddr);
    MPath *prune_phyint(int phyint);
    bool all_in_area(class SpfArea *);
    bool some_transit(class SpfArea *);
};	

/* Defines for type of routing table entry
 * Organized from most preferred to least preferred.
 */

enum {
    RT_DIRECT=1,  // Directly attached
    RT_SPF,	// Intra-area
    RT_SPFIA,	// Inter-area
    RT_EXTT1,	// External type 1
    RT_EXTT2,	// External type 2
    RT_REJECT,	// Reject route, for own area ranges
    RT_STATIC,	// External routes we're importing
    RT_NONE,	// Deleted, inactive
};

/* The IP routing table.
 */

class INtbl {
  protected:
    AVLtree root; 	// Root of AVL tree
  public:
    INrte *add(uns32 net, uns32 mask);
    inline INrte *find(uns32 net, uns32 mask);
    INrte *best_match(uns32 addr);
    friend class INiterator;
    friend class OSPF;
};

inline INrte *INtbl::find(uns32 net, uns32 mask)

{
    return((INrte *) root.find(net, mask));
}

/* Data that is used only for internal OSPF routes (intra-
 * and inter-area routes).
 */

class SpfData {
  public:
    byte lstype; 	// LSA leading to entry
    lsid_t lsid;
    rtid_t rtid;
    aid_t r_area; 	// Associated area
    MPath *old_mpath;	// Old next hops
    uns32 old_cost;	// Old cost
};

/* Definition of the generic routing table entry. Organized as a
 * balanced or AVL tree, this is the base class for both
 * IP and router routing table entries.
 */

class RTE : public AVLitem {
  protected:
    byte r_type; 	// Type of routing table entry
    SpfData *r_ospf;	// Intra-AS OSPF-specific data

  public:
    byte changed:1,	// Entry changed?
	dijk_run:1;	// Dijkstra run, sequence number
    MPath *r_mpath; 	// Next hops
    MPath *last_mpath; 	// Last set of Next hops given to kernel
    uns32 cost;		// Cost of routing table entry
    uns32 t2cost;	// Type 2 cost of entry

    RTE(uns32 key_a, uns32 key_b);
    void new_intra(TNode *V, bool stub, uns16 stub_cost, int index);
    void host_new_intra(SpfIfc *ip, uns32 new_cost);
    virtual void set_origin(LSA *V);
    virtual void declare_unreachable();
    LSA	*get_origin();
    void save_state();
    bool state_changed();
    void run_transit_areas(class rteLSA *lsap);
    void set_area(aid_t);
    SpfIfc *ifc();
    inline void update(MPath *newnh);
    inline byte type();
    inline int valid();
    inline int intra_area();
    inline int inter_area();
    inline int intra_AS();
    inline aid_t area();

    friend class SpfArea;
    friend class summLSA;
    friend class FWDrte;
};

// Inline functions
inline void RTE::update(MPath *newnh)
{
    r_mpath = newnh;
}
inline byte RTE::type()
{
    return(r_type);
}
inline int RTE::valid()
{
    return(r_type != RT_NONE);
}
inline int RTE::intra_area()
{
    return(r_type == RT_SPF);
}
inline int RTE::inter_area()
{
    return(r_type == RT_SPFIA);
}
inline int RTE::intra_AS()
{
    return(r_type == RT_SPF || r_type == RT_SPFIA);
}
inline aid_t RTE::area()
{
    return((r_ospf != 0) ? r_ospf->r_area : 0);
}

/* Definition of the IP routing table entry. Organized as a balanced
 * or AVL tree, with the key being the combination of the
 * network number (most significant) and the network mask (least
 * significant.
 *
 * Non-contiguous subnet masks are not supported.
 *
 * Each entry has a pointer to the previous least significant match
 * (prefix).
 */

class INrte : public RTE {
  protected:
    INrte *_prefix;	// Previous less specific match
    uns32 tag;		// Advertised tag

  public:
    class summLSA *summs;	// summary-LSAs
    class ASextLSA *ases;	// AS-external-LSAs
    class ExRtData *exlist;	// Statically configured routes
    class ExRtData *exdata;	// When we're importing information
    byte range:1,		// Configured area address range?
	 ase_orig:1;		// Have we originated an AS-external-LSA?

    inline INrte(uns32 xnet, uns32 xmask);
    inline uns32 net();
    inline uns32 mask();
    inline bool matches(InAddr addr);
    inline uns32 broadcast_address();
    inline INrte *prefix();
    inline int is_range();
    inline int is_child(INrte *o);
    int within_range();
    summLSA *my_summary_lsa(); // Find the one we originated
    ASextLSA *my_ase_lsa();	// Find one we originated
    void run_inter_area(); // Calculate inter-area routes
    void run_external();	// Calculate external routes
    void incremental_summary(SpfArea *);
    void sys_install();  // Install routes into kernel
    virtual void declare_unreachable();

    friend class SpfArea;
    friend class summLSA;
    friend class INiterator;
    friend class INtbl;
    friend class OSPF;
};

// Inline functions
inline INrte::INrte(uns32 xnet, uns32 xmask) : RTE(xnet, xmask)
{
    _prefix = 0;
    tag = 0;
    summs = 0;
    ases = 0;
    exdata = 0;
    exlist = 0;
    range = false;
    ase_orig = false;
}
inline uns32 INrte::net()
{
    return(index1());
}
inline uns32 INrte::mask()
{
    return(index2());
}
inline bool INrte::matches(InAddr addr)
{
    return((addr & mask()) == net());
}
inline uns32 INrte::broadcast_address()
{
    return(net() | ~mask());
}
inline INrte *INrte::prefix()
{
    return(_prefix);
}
inline int INrte::is_range()
{
    return(range != 0);
}
inline int INrte::is_child(INrte *o)
{
    return((net() & o->mask()) == o->net() && mask() >= o->mask());
}

/* Iterator for the routing table. Simply follows the singly
 * linked list.
 */

class INiterator : public AVLsearch {
  public:
    inline INiterator(INtbl *);
    inline INrte *nextrte();
};

// Inline functions
inline INiterator::INiterator(INtbl *t) : AVLsearch(&t->root)
{
}
inline INrte *INiterator::nextrte()
{
    return((INrte *) next());
}

/* Routing table entry for routers, per-area.
 */

class RTRrte : public RTE {
  public:
    byte rtype;		// Router type
    RTRrte *asbr_link;	// Linked in ASBR entry
    class ASBRrte *asbr; // ASBR entry
    class VLIfc *VL;	// configured VL w/ this endpoint
    class SpfArea *ap;	// area to which router belongs

    RTRrte(uns32 rtrid, class SpfArea *ap);
    virtual ~RTRrte();

    inline uns32 rtrid();
    inline bool	b_bit();// area border router?
    inline bool	e_bit();// AS boundary router?
    inline bool	v_bit();// virtual link endpoint?
    inline bool	w_bit();// wild-card multicast receiver?
    virtual void set_origin(LSA *V);
};

// Inline functions
inline uns32 RTRrte::rtrid()
{
    return(index1());
}
inline bool RTRrte::b_bit()
{
    return((rtype & RTYPE_B) != 0);
}
inline bool RTRrte::e_bit()
{
    return((rtype & RTYPE_E) != 0);
}
inline bool RTRrte::v_bit()
{
    return((rtype & RTYPE_V) != 0);
}
inline bool RTRrte::w_bit()
{
    return((rtype & RTYPE_W) != 0);
}

/* ASBR routing table entries
 */

class ASBRrte : public RTE {
  public:
    RTRrte *parts; 	// Component RTRrtes
    ASBRrte *sll; 	// Singly-linked list
    class asbrLSA *summs; // ASBR-summary-LSAs

    ASBRrte(uns32 rtrid);
    inline uns32 rtrid();
    inline ASBRrte *next();
    void run_calculation();// Run full routing calculations
    void run_inter_area(); // Calculate inter-area routes
    friend class asbrLSA;
};

// Inline functions
inline uns32 ASBRrte::rtrid()
{
    return(index1());
}
inline ASBRrte *ASBRrte::next()
{
    return(sll);
}

/* Forwarding addresses are in a separate table.
 * We keep track of their reachability, and the local external
 * routes that have been originated with each forwarding
 * address.
 */

class ExRtData : public ConfigItem {
    INrte *rte;		// For this routing table entry
    int phyint;		// Outgoing interface
    InAddr gw;		// Next hop
    class FWDrte *faddr;
    ExRtData *sll_rte;	// Link in routing table entry
    ExRtData *sll_pend;	// Pending list forward pointer
    int	type2:1,	// external type, 1 or 2
	mc:1,		// Multicast source?
        direct:1,	// Directly attached subnet?
	noadv:1,	// Don't advertise in AS-external-LSAs
	orig_pending:1, // On origination pending list?
        forced:1;	// Force new AS-external-LSA origination
    uns32 cost;		// cost to advertise
    uns32 tag;		// Tag to advertise
    MPath *mpath; 	// Next hop in MPath form
  public:
    ExRtData(InAddr xnet, InAddr xmask);
    inline uns32 net() const;
    inline uns32 mask() const;
    inline int external_type() const;
    inline bool is_multicast() const;
    inline bool is_direct() const;
    inline bool is_not_advertised() const;
    inline uns32 adv_cost() const;
    inline uns32 adv_tag() const;
    inline InAddr gateway() const;
    inline int physical_interface() const;
    void clear_config();
    class FWDrte *fwd_rte();
    friend class OSPF;
    friend class FWDrte;
    friend class FWDtbl;
    friend class LocalOrigTimer;
    friend class INrte;
    friend class SpfArea;
};

// Inline functions
inline uns32 ExRtData::net() const
{
    return(rte->net());
}
inline uns32 ExRtData::mask() const
{
    return(rte->mask());
}
inline int ExRtData::external_type() const
{
    return(type2);
}
inline bool ExRtData::is_multicast() const
{
    return(mc);
}
inline bool ExRtData::is_direct() const
{
    return(direct);
}
inline bool ExRtData::is_not_advertised() const
{
    return(noadv);
}
inline uns32 ExRtData::adv_cost() const
{
    return(cost);
}
inline uns32 ExRtData::adv_tag() const
{
    return(tag);
}
inline InAddr ExRtData::gateway() const
{
    return(gw);
}
inline int ExRtData::physical_interface() const
{
    return(phyint);
}

class FWDtbl {
  protected:
    AVLtree root; 	// Root of AVL tree
  public:
    FWDrte *add(uns32 addr);
    void resolve();
    void resolve(INrte *changed_rte);
    friend class OSPF;
};

class FWDrte : public RTE {
    SpfIfc *ifp;	// On this interface
    INrte *match;	// Best matching routing table entry
  public:
    inline FWDrte(uns32 addr);
    inline InAddr address();
    void resolve();
    friend class OSPF;
    friend class INrte;
    friend class ExRtData;
    friend class FWDtbl;
};

// Inline functions
inline FWDrte::FWDrte(uns32 addr) : RTE(addr, 0)
{
    ifp = 0;
    match = 0;
}
inline InAddr FWDrte::address()
{
    return(index1());
}

/* Data structure used to store differences between the
 * kernel routing table and OSPF's. Time is when the
 * kernel has reported a deletion that we didn't know
 * about.
 */

class KrtSync : public AVLitem {
  public:
    SPFtime tstamp;
    KrtSync(InAddr net, InMask mask);
};
