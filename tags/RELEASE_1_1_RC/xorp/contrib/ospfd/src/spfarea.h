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

/* Defintions related to OSPF area support. OSP area support
 * is defined in Sections 6 and C.2 of the OSPF specification.
 */

/* The OSPF area class. One defined for each OSPF area. The
 * initialized (configured) data is listed at the beginning.
 *
 * Also, MaxAge LSAs are not linked per-area, but instead are linked
 * in the single global list "sll_maxage". This is possible since
 * all LSAs have an area pointer.
 */

class SpfArea : public ConfigItem {
    const aid_t a_id;	// Area ID
    SpfArea *next; 	// Next area in linked list

    // Link-state database
    AVLtree rtrLSAs;	// router-LSAs
    AVLtree netLSAs;	// network-LSAs
    AVLtree summLSAs;	// summary-LSAs
    AVLtree asbrLSAs;	// asbr-summary-LSAs
    AVLtree grpLSAs;	// group-membership-LSAs
    AVLtree AreaOpqLSAs;// Area-scoped Opaque-LSAs
    uns32 db_xsum;	// Database checksum
    uns32 wo_donotage;	// #LSAs claiming no DoNotAge support
    uns32 dna_indications;// #LSAs claiming no DoNotAge support
    bool self_indicating;// We have generated indication 
    bool dna_change;	// Change in network DoNotAge support
    LsaList a_dna_flushq; // DoNotAge LSAs being flushed from lack of support

    // Dynamic parameters
    class SpfIfc *a_ifcs; // List of associated interfaces
    int	n_VLs;		// Fully adjacent VLs through this area
    bool a_transit;	// Transit area? 
    bool was_transit;	// Was a transit area? 
    // MOSPF parameters
    int size_mospf_incoming; // Size of incoming interface array
    int *mospf_in_phys; // Incoming phyints
    int mospf_in_count; // Number of same;
    // Hitless restart parameters
    int a_helping;	// # neighbors being helped in area
    bool cancel_help_sessions; // should we cancel helping sessions?

    AVLtree abr_tbl;	// RTRrte's for area border routers
    AVLtree AdjAggr;	// Aggregate adjacency information

  public:
    bool a_stub; 	// Options supported by area
    uns32 a_dfcst;	// Default cost to adv. (stubs)
    bool a_import;	// Import inter-area into stubs?
    AVLtree ranges; 	// Area address ranges
    AVLtree hosts;	// Hosts belonging to this area

    class rtrLSA *mylsa; // Our router-LSA
    int	n_active_if;	// Number of active interfaces
    int n_routers;	// Number of reachable routers
    class SpfIfc **ifmap; // Interfaces listed in router-LSA
    int	ifmap_valid;	// Is interface map valid?
    int	sz_ifmap;	// Size of interface map (# ifcs)
    int	n_ifmap;	// # ifcs currently in map
    uns16 a_mtu; 	// Max IP datagram for all interfaces
    Pkt	a_update;	// Current flood
    Pkt	a_demand_upd;	// Current flood out demand interfaces

    inline bool is_stub() const;
    inline bool	is_transit();
    inline int donotage();
    inline aid_t id() const;
    inline uns32 default_cost() const;
    inline bool import() const;
    inline uns32 database_xsum();

    SpfArea(aid_t id);
    ~SpfArea();
    virtual void clear_config();
    inline RTRrte *find_abr(uns32 rtrid);
    RTRrte *add_abr(uns32 rtrid);
    void rl_orig(int forced=0);	// Originate router-LSA
    RtrLink *rl_insert_hosts(SpfArea *home, RTRhdr *rtrhdr, RtrLink *rlp);
    void sl_orig(class INrte *, int forced=0);
    uns32 sl_cost(class INrte *rte);
    summLSA *sl_reorig(summLSA *, lsid_t, uns32 cost, INrte *rte, int);
    void asbr_orig(class ASBRrte *rte, int forced=0);
    bool needs_indication();
    void grp_orig(InAddr group, int forced=0);
    void delete_lsdb();
    void a_flush_donotage();
    void reinitialize();
    void generate_summaries();
    int n_LSAs();
    void RemoveIfc(class SpfIfc *);
    void IfcChange(int increment);
    void add_to_update(LShdr *hdr, bool demand);
    void add_to_ifmap(SpfIfc *ip);
    InAddr id_to_addr(rtid_t id);
    void adj_change(SpfNbr *, int n_ostate);

    // MOSPF routines
    void mospf_path_calc(InAddr, INrte *, int &, uns32 &, LsaList *);
    void mospf_dijkstra(InAddr, PriQ &cand, bool, LsaList *downstream_nodes);
    void mospf_possibly_add(PriQ &, TNode *, uns32, TNode *, int il_type, int);
    INrte *find_best_summlsa(INrte *);
    int mospf_init_intra_area(PriQ &cand, INrte *, uns32, int);
    void mospf_add_summlsas(PriQ &cand, INrte *, uns32);
    void mospf_add_ases(PriQ &cand, INrte *);

    friend class OSPF;
    friend class IfcIterator;
    friend class AreaIterator;
    friend class SpfIfc;
    friend class PPIfc;
    friend class DRIfc;
    friend class VLIfc;
    friend class SpfNbr;
    friend class TNode;
    friend class LocalOrigTimer;
    friend void LSA::process_donotage(bool parse);
    friend void asbrLSA::process_donotage(bool parse);
};

// Inline functions
inline bool SpfArea::is_stub() const
{
    return(a_stub);
}
inline bool SpfArea::is_transit()
{
    return(a_transit);
}
inline int SpfArea::donotage()
{
    return(wo_donotage == 0 && dna_indications == 0);
}
inline aid_t SpfArea::id() const
{
    return(a_id);
}
inline uns32 SpfArea::default_cost() const
{
    return a_dfcst;
}
inline bool SpfArea::import() const
{
    return a_import;
}
inline RTRrte *SpfArea::find_abr(uns32 rtrid)
{
    return ((RTRrte *) abr_tbl.find(rtrid));
}
inline uns32 SpfArea::database_xsum()
{
    return(db_xsum);
}

/* Description of an area address range.
 * Contains both static (net, mask, area, and suppress/advertise)
 * and dynamic (maximum cost, active or not)
 * information. Ranges attached to global ospf structure to
 * avoid configuration conflicts.
 */

class Range : public AVLitem, public ConfigItem {
  public:
    SpfArea *ap;
    INrte *r_rte;	// Network number and mask
    aid_t r_area;	// Area to summarize
    int	r_suppress:1,	// Suppress or advertise
	r_active:1;	// Active?
    uns32 r_cost;	// Current cost

    Range(SpfArea *ap, uns32 net, uns32 mask);
    virtual void clear_config();

    friend class SpfArea;
};

/* Host addresses to be advertised for this area.
 * By convention, those with cost 0 are assumed
 * to be our addresses, and the others belong to
 * hosts directly attached to us.
 */

class HostAddr : public AVLitem, public ConfigItem {
  public:
    SpfArea *ap;
    INrte *r_rte;	// Network number and mask
    aid_t r_area;	// containing area
    uns32 r_cost;	// Configured cost
			// Dynamically allocated
    class LoopIfc *ip;	// Associated loopback interface

    HostAddr(SpfArea *ap, uns32 net, uns32 mask);
    virtual void clear_config();

    friend class SpfArea;
};

/* Data structure which collects all the information
 * regarding point-to-point adjacencies to a particular node through
 * this area. This is used to limit the number of
 * point-to-point adjacencies, while still preserving
 * forwarding semantics.
 * Organized in SpfArea:AdjAggr by neighbor Router ID.
 * - PPAdjAggr::nbr_cost is the smallest cost of any of the
 * bidirectional links to the neighbor.
 * - PPAdjAggr::first_full is one of the full adjacencies to the
 * neighbor, and is used to trigger inclusion of the point-to-point
 * link in the router-LSA.
 * - PPAdjAggr::nbr_mpath is the collection of all the least cost
 * bidirectional links to the neighbor, pre-calculated for use by
 * the routing calculation.
 * - PPAdjAggr::n_adjs is the number of neighbor relationships in state
 * exstart or greater. This is limited by OSPF::PPAdjLimit.
 */

class PPAdjAggr : public AVLitem {
    uns16 nbr_cost;	// Best cost to neighbor
    SpfIfc *first_full;
    MPath *nbr_mpath;
    uns32 n_adjs;
  public:
    PPAdjAggr(rtid_t n_id);
    friend class SpfArea;
    friend class SpfIfc;
    friend class SpfNbr;
    friend class PPIfc;
};

