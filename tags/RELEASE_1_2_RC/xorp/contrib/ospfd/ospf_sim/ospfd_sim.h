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

extern char *LOOPADDR;

class SimPktQ;
class SimRte;

/* Routing table kept within a simulated OSPF router. We don't
 * use the simulated ospfd's table, so that we can simulate
 * hitless restart scenarios.
 */

class SimRttbl {
  public:
    AVLtree routes;
    SimRte *add(InAddr net, InMask mask);
    SimRte *best_match(InAddr addr);
};

class SimRte : public AVLitem {
  public:
    SimRte *prefix;
    bool reachable;
    bool reject;
    int phyint;		// Outgoing interface
    InAddr if_addr;
    InAddr gw;		// Next hop address
    
    inline SimRte(InAddr, InMask);
    inline InAddr net();
    inline InAddr mask();
    inline int is_child(SimRte *o);
};    

inline SimRte::SimRte(InAddr a, InMask m) : AVLitem(a, m)
{
    prefix = 0;
    reachable = false;
    reject = false;
}
inline InAddr SimRte::net()
{
    return(index1());
}
inline InAddr SimRte::mask()
{
    return(index2());
}
inline int SimRte::is_child(SimRte *o)
{
    return((net() & o->mask()) == o->net() && mask() >= o->mask());
}

/* Global class representing a simulated OSPF router. Contains
 * the implementation of its system interface.
 */

class SimSys : public OSInstance {
    int ctl_fd;     // Connection to controller
    TcpPkt ctl_pkt; // Packet processing for controller connection
    int uni_fd;	// Connection for packets addressed to us
    uns16 uni_port; // Our port for locally addressed packets
    int ticks; // Elapsed time in simulated ticks
    InAddr my_addr;	// One of router's IP addresses
    AVLtree address_map; // IP address to group mapping
    AVLtree port_map; // Phyint to file descriptor mapping
    AVLtree membership; // Interface group membership		   
    SimRttbl rttbl;	// Routing table
    SimPktQ *rcv_head;  // Queued receives
    SimPktQ *rcv_tail;
    SPFtime xmt_stamp; // Transmission timestamp
    bool ipforwarding; // Whether IP forwarding is enabled
    AVLtree pings;	// Active ping sessions
    AVLtree traceroutes;// Active traceroute sessions
    AVLtree mtraces;	// Active multicast traceroutes
    uns32 mtrace_qid;	// Next mtrace query ID
    // Stored hitless restart parameters
    bool hitless_preparation;
    bool hitless_preparation_complete;
    SPFtime grace_period;
  public:
    SimSys(int fd);
    ~SimSys();
    
    void sendpkt(InPkt *pkt, int phyint, InAddr gw=0);
    void sendpkt(InPkt *pkt);
    bool phy_operational(int phyint);
    void phy_open(int phyint);
    void phy_close(int phyint);
    void join(InAddr group, int phyint);
    void leave(InAddr group, int phyint);
    void ip_forward(bool enabled);
    void set_multicast_routing(bool enabled);
    void set_multicast_routing(int phyint, bool enabled);
    void rtadd(InAddr, InMask, MPath *, MPath *, bool); 
    void rtdel(InAddr, InMask, MPath *);
    void add_mcache(InAddr src, InAddr group, MCache *);
    void del_mcache(InAddr src, InAddr group);
    virtual void upload_remnants();
    char *phyname(int phyint);
    void sys_spflog(int code, char *buffer);
    virtual void store_hitless_parms(int, int, struct MD5Seq *);
    void halt(int code, char *string);

    void process_uni_fd();
    void rxpkt(struct SimPktHdr *pkthdr);
    void local_demux(SimPktHdr *pkthdr);
    void igmp_demux(int phyint, InPkt *pkt);
    uns16 get_port_addr(InAddr, InAddr &home);
    int get_fd(int phyint);
    void queue_rcv(struct SimPktHdr *pkt, int plen);
    void process_rcvqueue();
    void mc_fwd(int in_phyint, InPkt *);
    void send_multicast(struct SimPktHdr *, size_t, bool loopback=false);
    void sendicmp(byte, byte, InAddr, InAddr, InPkt *, uns16, uns16, byte);
    void mtrace_query_received(int phyint, InPkt *pkt);
    void mtrace_response_received(InPkt *pkt);
    void recv_ctl_command();
    void config(int type, int subtype, void *msg);
    void send_tick_response();
    InAddr ip_source(InAddr dest);

    friend int main(int argc, char *argv[]);
    friend class PingSession;
    friend class TRSession;
    friend class MTraceSession;
};

/* Map an IP address of a simulated router to the
 * port that router will be listening to.
 */

class AddressMap : public AVLitem {
    uns16 port;
    InAddr home;
  public:
    AddressMap(InAddr, InAddr);
    friend class SimSys;
};

/* Map a phyint to a data structure describing
 * the interface.
 */

class PhyintMap : public AVLitem {
    bool working;
    bool promiscuous;
    char name[8];
    InAddr addr;
    InMask mask;
  public:
    PhyintMap(int phyint);
    friend class SimSys;
};

// Maximum size of an IP packet
const int MAX_IP_PKTSIZE = 65535;

/* Class for queueing control packets.
 */

struct SimPktHdr {
    SPFtime ts;		// Packet timestamp;
    int phyint;		// Associate physical interface
};

class SimPktQ {
    SimPktQ *next;
    SimPktHdr *ip_data;
  public:
    friend class SimSys;
};

/* Class implementing a ping session. Pings sent
 * on a timer.
 */

class PingSession : public AVLitem, public ITimer {
    InAddr src;
    InAddr dest;
    byte ttl;
    uns32 seqno;
 public:
    PingSession(uns16 id, InAddr s, InAddr d, byte t);
    virtual void action();
};

/* Class implementing a traceroute session. Probes sent
 * on a timer, or when a response is received.
 */

class TRSession : public AVLitem, public Timer {
    InAddr dest;
    byte maxttl;	// Maximum number of probes
    byte ttl;		// Current probe
    byte iteration;	// Iteration in current probe sequence
    uns32 seqno;
    bool terminating;
    enum {
      MAXITER = 2,
    };
 public:
    TRSession(uns16 id, InAddr d, byte t);
    void response_received(byte icmp_type);
    virtual void action();
};

/* Class implementing a multicast traceroute session. Probes sent
 * on a timer, or when an incomplete response is received.
 */

class MTraceSession : public AVLitem, public Timer {
    MTraceHdr hdr;
    InAddr chop;	// Current probe destination
    int cphyint;
    int hops_from_dest;
    byte cttl;		// Current max # hops
    byte iteration;	// Iteration in current probe sequence
    uns32 query_id;
    enum {
      MAXITER = 2,
    };
 public:
    MTraceSession(uns16 id, int phyint, MTraceHdr *hp);
    void send_query();
    void query_received(InPkt *);
    void response_received(InPkt *);
    void print(InAddr src, MtraceBody *last_block);
    bool done(InAddr src, MtraceBody *last_block);
    virtual void action();
    friend class SimSys;
};
