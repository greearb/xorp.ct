
/* The global class implementing
 * the simulation controller.
 */

class SimCtl {
    Tcl_Interp *interp;
    AVLtree simnodes;
    class SimNode *nodes[NOFILE];
    int maxfd;
    int server_fd;
    int n_ticks;
    bool running;
    bool frozen;
    // Port assignments
    int assigned_port;
    AVLtree ifmaps;
    // Database statistics
    PatTree stats;
  public:
    inline SimCtl(Tcl_Interp *ti);
    inline int elapsed_seconds();
    inline int elapsed_milliseconds();
    void delete_router(class SimNode *node);
    bool process_replies();
    bool send_data();
    void incoming_call();
    void simnode_handler_read(int fd);
    void store_mapping(uns32 net_or_addr, uns32 rtr);
    void send_addrmap(class SimNode *);
    void send_addrmap_increment(class IfMap *, class SimNode *);
    void restart_node(SimNode *, InAddr, int, uns16);

    friend class SimNode;
    friend int main(int argc, char *argv[]);
    friend int Tcl_AppInit(Tcl_Interp *);
    friend void tick(ClientData);
    friend int StartRouter(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int ToggleRouter(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int RestartRouter(ClientData,Tcl_Interp *interp, int, char *argv[]);
    friend int HitlessRestart(ClientData,Tcl_Interp *interp,int, char *argv[]);
    friend int AddMapping(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int AddNetMember(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int TimeStop(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int TimeResume(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendArea(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendInterface(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendVL(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendHost(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int StartPing(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int StopPing(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int StartTraceroute(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int StartMtrace(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendGroup(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int LeaveGroup(ClientData, Tcl_Interp *, int, char *argv[]);
};

inline SimCtl::SimCtl(Tcl_Interp *ti) {
    interp = ti;
    n_ticks = 0;
    running = false;
    frozen = false;
    memset(nodes, 0, sizeof(nodes));
    maxfd = 0;
}
inline int SimCtl::elapsed_seconds() {
    return(n_ticks/TICKS_PER_SECOND);
}
inline int SimCtl::elapsed_milliseconds() {
    return((n_ticks%TICKS_PER_SECOND)*(1000/TICKS_PER_SECOND));
}

/* Class storing database statistics.
 */

class NodeStats : public PatEntry {
    DBStats dbstats;	// Latest database statistics
    int refct;
  public:
    NodeStats(DBStats *);
    friend class SimCtl;
    friend void tick(ClientData);
};

/* Class representing each simulated node.
 */

class SimNode : public AVLitem {
    int fd;		// Control connection to node
    TcpPkt pktdata;	// Pending packetized data
    bool got_tick;	// Received tick response (init to true!)
    NodeStats *dbstats; // Stored database statistics
    uns16 home_port;	// Unicast listening port
    bool awaiting_htl_restart;
    int color;		// Current node color
    enum {		// Available colors
        WHITE,		// not synched
	GREEN,		// in synch
        ORANGE,		// partial synch
        RED,		// defunct
    };
  public:
    SimNode(uns32 id, int file);
    inline InAddr id();
    friend class SimCtl;
    friend int main(int argc, char *argv[]);
    friend void tick(ClientData);
    friend int ToggleRouter(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int RestartRouter(ClientData,Tcl_Interp *interp, int, char *argv[]);
    friend int HitlessRestart(ClientData,Tcl_Interp *interp,int, char *argv[]);
    friend int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendArea(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendInterface(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendVL(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendHost(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int StartPing(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int StopPing(ClientData, Tcl_Interp *interp, int, char *argv[]);
    friend int StartTraceroute(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int StartMtrace(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int SendGroup(ClientData, Tcl_Interp *, int, char *argv[]);
    friend int LeaveGroup(ClientData, Tcl_Interp *, int, char *argv[]);
};

inline InAddr SimNode::id() {
    return(index1());
}

/* Data structure mapping interface address to
 * node, from which we can then get the port
 * to send down to the simulated routers.
 */

class IfMap : public AVLitem {
    rtid_t owner;
  public:
    inline IfMap(InAddr addr, rtid_t id);
    friend class SimCtl;
};

inline IfMap::IfMap(InAddr addr, rtid_t id) : AVLitem(addr, id)
{
    owner = id;
}
