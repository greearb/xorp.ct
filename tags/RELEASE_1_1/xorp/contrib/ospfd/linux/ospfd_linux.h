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

#define LINUX22 0x20200

class LinuxOspfd : public Linux {
    enum { 
	MAXIFs=255, // Maximum number of interfaces
    };
    int netfd;	// File descriptor used to send and receive
    int igmpfd; // File descriptor for multicast routing
    int udpfd;	// UDP file descriptor for ioctl's
    int rtsock; // rtnetlink file descriptor
    timeval last_time; // Last return from gettimeofday
    int next_phyint; // Next phyint value
    AVLtree phyints; // Physical interfaces
    AVLtree interface_map; // IP addresses to phyint
    AVLtree directs; // Directly attached prefixes
    rtentry m;
    uns32 nlm_seq;
    FILE *logstr;
    bool changing_routerid;
    bool change_complete;
    bool dumping_remnants;
    int vifs[MAXVIFS];
  public:
    LinuxOspfd();
    ~LinuxOspfd();
    void time_update();
    
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
    void rtdel(InAddr, InMask, MPath *ompp);
    void add_mcache(InAddr src, InAddr group, MCache *);
    void del_mcache(InAddr src, InAddr group);
    void upload_remnants();
    char *phyname(int phyint);
    void sys_spflog(int msgno, char *msgbuf);
    void store_hitless_parms(int, int, struct MD5Seq *);
    void halt(int code, char *string);

    void read_config();
    void read_kernel_interfaces();
    void one_second_timer();
    void rtentry_prepare(InAddr, InMask, MPath *mpp);
    void add_direct(class BSDPhyInt *, InAddr, InMask);
    int get_phyint(InAddr);
    bool parse_interface(char *, in_addr &, BSDPhyInt * &);
    void raw_receive(int fd);
    void netlink_receive(int fd);
    void process_routerid_change();
    void set_flags(class BSDPhyInt *, short flags);
    friend int main(int argc, char *argv[]);
    friend int SendInterface(void *,struct Tcl_Interp *, int,char *[]);
    friend void quit(int);
};

/* Representation of a physical interface.
 * Indexed by IfIndex (phyint).
 */

class BSDPhyInt : public AVLitem {
    char *phyname;
    InAddr addr;
    short flags;
    InMask mask;
    InAddr dstaddr;	// Other end of p-p link
    int mtu;
    bool tunl;
    int vifno;
    InAddr tsrc;	// Tunnel endpoint addresses
    InAddr tdst;

    inline BSDPhyInt(int index);
    friend class LinuxOspfd;
    friend int SendInterface(void *,struct Tcl_Interp *, int,char *[]);
    friend int SendMD5Key(void *, Tcl_Interp *, int, char *argv[]);
    inline int phyint();
};

inline BSDPhyInt::BSDPhyInt(int index) : AVLitem(index, 0)
{
    addr = 0;
    mask = 0;
    dstaddr = 0;
    tunl = false;
    vifno = 0;
    tsrc = 0;
    tdst = 0;
}

inline int BSDPhyInt::phyint()
{
    return(index1());
}

/* Map an IP interface (either belonging to an interface, or
 * the other end of a point-to-point interface) to a
 * physical interface.
 */

class BSDIfMap : public AVLitem {
    BSDPhyInt *phyp;
  public:
    inline BSDIfMap(InAddr, BSDPhyInt *);
    friend class LinuxOspfd;
    friend int SendInterface(void *,struct Tcl_Interp *, int,char *[]);
};

inline BSDIfMap::BSDIfMap(InAddr addr, BSDPhyInt *_phyp) : AVLitem(addr, 0)
{
    phyp = _phyp;
}

/* Store one of the directly connected prefixes, with an indication
 * as to whether it is still valid. This enables us to
 * ask OSPF whether it has better information when a direct
 * interface is no longer available.
 */

class DirectRoute : public AVLitem {
  public:
    bool valid;
    DirectRoute(InAddr addr, InMask mask) : AVLitem(addr, mask) {}
};

// Maximum size of an IP packet
const int MAX_IP_PKTSIZE = 65535;
