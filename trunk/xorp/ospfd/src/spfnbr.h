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

/* The Neighbor timer definitions
 */

class SpfNbr;

// Single shot timer. When fires, indicates neighbor inoperative

class InactTimer : public Timer {
    SpfNbr *np;
public:
    inline InactTimer(SpfNbr *);
    virtual void action();
};

inline InactTimer::InactTimer(SpfNbr *nbr) : np(nbr)
{
}

// Interval timer. When fires, Hello packet sent to neighbor

class NbrHelloTimer : public ITimer {
    SpfNbr *np;
public:
    inline NbrHelloTimer(SpfNbr *);
    virtual void action();
};

inline NbrHelloTimer::NbrHelloTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, frees last DD packet

class HoldTimer : public Timer {
    SpfNbr *np;
public:
    inline HoldTimer(SpfNbr *);
    virtual void action();
};

inline HoldTimer::HoldTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, retransmits DD packet.

class DDRxmtTimer : public ITimer {
    SpfNbr *np;
public:
    inline DDRxmtTimer(SpfNbr *);
    virtual void action();
};

inline DDRxmtTimer::DDRxmtTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, retransmits Link State Requests

class RqRxmtTimer : public ITimer {
    SpfNbr *np;
public:
    inline RqRxmtTimer(SpfNbr *);
    virtual void action();
};

inline RqRxmtTimer::RqRxmtTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, retransmits LSAs to neighbor

class LsaRxmtTimer : public Timer {
    SpfNbr *np;
public:
    inline LsaRxmtTimer(SpfNbr *);
    virtual void action();
};

inline LsaRxmtTimer::LsaRxmtTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, tears down neighbor connection
// due to lack of progress

class ProgressTimer : public Timer {
    SpfNbr *np;
public:
    inline ProgressTimer(SpfNbr *);
    virtual void action();
};

inline ProgressTimer::ProgressTimer(SpfNbr *nbr) : np(nbr)
{
}

// Single shot timer. When fires, exit helper mode for the neighbor
// due to timeout

class HelperTimer : public Timer {
    SpfNbr *np;
public:
    inline HelperTimer(SpfNbr *);
    virtual void action();
};

inline HelperTimer::HelperTimer(SpfNbr *nbr) : np(nbr)
{
}

/* Definition of the OSPF neighbor class, and its associated
 * fields. As usual, the configured part of the neighbor structure
 * is in the initial piece.
 *
 * Neighbors can be configured, as for example, on NBMA networks.
 * Unless a neighbor is configured, it is deleted an the associate
 * neighbor structure freed when the neighbor transitions to down state.
 *
 * The Neighbor Data Structure is defined in Section 10 of the OSPF
 * specification, with the configuration of neighbors described in
 * Section C.5 (NBMA networks) and Section C.4 (virtual links).
 */

class SpfNbr {
    InAddr n_addr; 	// Its IP address
    rtid_t n_id; 	// Its Router ID
    int	n_index;	// Used to offset timers
    // Dynamically learned parameters
    int	n_state;	// Current neighbor state
    uns32 md5_seqno;	// Cryptographic sequence number
    byte n_opts; 	// Options advertised by neighbor
    byte n_imms; 	// Bits rcvd in last DD packet
    uns32 n_ddseq;	// DD sequence number
    bool database_sent; // Sent entire database description?
    int	n_adj_pend:1,	// A pending adjacency?
	n_rmt_init:1;	// Remotely initiated?
    bool rq_suppression;// Requested hello suppression?
    bool hellos_suppressed; // Hellos suppressed?
    SpfNbr *n_next_pend; // Pending adjacency list

    // Four-part retransmission list
    LsaList n_pend_rxl;	// LSAs recently retransmitted
    LsaList n_rxlst;	// Flooded, but not time to rxmt
    LsaList n_failed_rxl; // Failed retransmissions
    uns32 rxmt_count;	// Count of all rxmt queues together
    uns16 n_rxmt_window;// # consecutive retransmissions allowed

    LsaList n_ddlst;	// Database summary list
    LsaList n_rqlst;	// Request list
    int	rq_goal;	// Number of LSA requests left to send

    Pkt	n_update;	// Pending update
    Pkt	n_imack;	// Immediate acks to send to nbr
    Pkt	n_ddpkt;	// DD packet currently sending
    InactTimer n_acttim; // Inactivity timer
    NbrHelloTimer n_htim; // Hello timer (NBMA, P-2-mp)
    HoldTimer n_holdtim; // DD hold, slave only
    DDRxmtTimer n_ddrxtim; // DD retransmission timer
    RqRxmtTimer n_rqrxtim; // Request retransmit timer
    LsaRxmtTimer n_lsarxtim; // LSA retransmit timer
    ProgressTimer n_progtim; // DD progress timer
    HelperTimer n_helptim; // Timeout helper mode

protected:
    SpfNbr *next; 	// List, per OSPF interface
    SpfIfc *n_ifp; 	// Associated OSPF interface

public:
    byte n_pri;		// Its Router Priority
    InAddr n_dr;	// Neighbors idea of DR
    InAddr n_bdr; 	// Neighbors idea of Backup DR

    SpfNbr(SpfIfc *, rtid_t id, InAddr addr);
    virtual ~SpfNbr();

    inline SpfIfc *ifc();
    inline int state();
    inline int declared_dr();
    inline int declared_bdr();
    inline int is_dr();
    inline int is_bdr();
    inline InAddr addr() const;
    inline rtid_t id();
    inline int priority();
    inline int supports(int option);
    inline void build_imack(LShdr *hdr);

    // Manipulation of retransmission lists
    void add_to_rxlist(LSA *lsap);
    bool remove_from_rxlist(LSA *lsap);
    bool remove_from_pending_rxmt(LSA *lsap);
    LSA	*get_next_rxmt(LsaList * &list, uns32 &nexttime);
    void clear_rxmt_list();
    bool changes_pending();

    // Packet reception functions
    void recv_dd(Pkt *pdesc);
    void process_dd_contents(LShdr *hdr, byte *end);
    void recv_req(Pkt *pdesc);
    void recv_update(Pkt *pdesc);
    void recv_ack(Pkt *pdesc);
    void negotiate_demand(byte opts);

    // Packet send functions
    void send_hello();
    void send_dd();
    void rxmt_dd();
    void send_req();
    void rxmt_updates();

    // Other neighbor functions
    void dd_free();
    void nbr_fsm(int event);
    void nba_eval1();
    void nba_eval2();
    void nba_snapshot();
    void nba_exchdone();
    void nba_reeval();
    void nba_clr_lists();
    void nba_delete();
    int add_to_update(LShdr *hdr);
    bool ospf_rmrxl(LSA *lsap);
    int ospf_rmreq(LShdr *hdr, int *rq_cmp);
    void start_adjacency();
    void AddPendAdj();
    void DelPendAdj();
    void exit_dbxchg();
    void dump_stats(struct NbrRsp *nrsp);
    bool adv_as_full();
    bool we_are_helping();
    void exit_helper_mode(const char *reason, bool actions=true);
    
    virtual bool configured();
    virtual bool dr_eligible() const;

    friend class NbrIterator;
    friend void SpfIfc::ifa_elect();
    friend class InactTimer;
    friend class NbrHelloTimer;
    friend class HoldTimer;
    friend class DDRxmtTimer;
    friend class RqRxmtTimer;
    friend class LsaRxmtTimer;
    friend class ProgressTimer;
    friend class DBageTimer;
    friend class SpfIfc;
    friend class PPIfc;
    friend class NBMAIfc;
    friend class P2mPIfc;
    friend class OSPF;
    friend SpfNbr *GetNextAdj();
    friend class StaticNbr;
};

// Inline functions
inline SpfIfc *SpfNbr::ifc()
{
    return(n_ifp);
}
inline int SpfNbr::state()
{
    return(n_state);
}
inline int SpfNbr::declared_dr()
{
    return (n_dr == n_addr);
}
inline int SpfNbr::declared_bdr()
{
    return (n_bdr == n_addr);
}
inline int SpfNbr::is_dr()
{
    return (n_ifp->dr() == n_addr);
}
inline int SpfNbr::is_bdr()
{
    return (n_ifp->bdr() == n_addr);
}
inline InAddr SpfNbr::addr() const
{
    return(n_addr);
}
inline rtid_t SpfNbr::id()
{
    return(n_id);
}
inline int SpfNbr::priority()
{
    return((int) n_pri);
}
inline int SpfNbr::supports(int option)
{
    return((n_opts & option) != 0);
}
inline void SpfNbr::build_imack(LShdr *hdr)
{
    n_ifp->if_build_ack(hdr, &n_imack, this);
}

/* A configured neighbor, used on non-broadcast networks such
 * as NBMA and Point-to-MultiPoint segments.
 */

class StaticNbr : public SpfNbr, public ConfigItem {
    bool _dr_eligible;
    bool active;
public:
    inline StaticNbr(SpfIfc *, InAddr);
    virtual bool configured();
    virtual bool dr_eligible() const;
    virtual void clear_config();

friend class OSPF;
};

// Inline functions
inline StaticNbr::StaticNbr(SpfIfc *ip, InAddr _addr) : SpfNbr(ip, 0, _addr)
{
    active = true;
}
