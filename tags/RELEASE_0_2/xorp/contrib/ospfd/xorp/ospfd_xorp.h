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

#ifndef OSPFD_XORP_H
#define OSPFD_XORP_H

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "ospf_config.h"

class XorpOspfd : public OSInstance {
  public:
    XorpOspfd(EventLoop& event_loop, XrlRouter& r);
    ~XorpOspfd();
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
    char *phyname(int phyint);
    void sys_spflog(int msgno, char *msgbuf);
    void halt(int code, const char *string);
    void upload_remnants() {};

    void read_kernel_interfaces();
    bool one_second_tick();
    void add_direct(class BSDPhyInt *, InAddr, InMask);
    int get_phyint(InAddr);
    bool parse_interface(const char *, in_addr &, BSDPhyInt * &);
    void raw_receive(int fd, SelectorMask m = SEL_RD);

    void set_flags(class BSDPhyInt *, short flags);

    void store_hitless_parms(int, int, struct MD5Seq *);

    friend int SendInterface(void *,struct Tcl_Interp *, int,char *[]);
    friend void quit(int);

 protected:
    XrlRouter&	     _router;	// XrlRouter used to communicate with outside
    XrlRibV0p1Client _rib_client;
    XorpTimer	     _one_sec_ticker; // Periodic timer that updates sys_etime
    XorpTimer	     _rib_add_table_ticker; // Timer to initiate rib connect

 public:
    void initiate_rib_add_table();
 protected:
    void rib_add_table();

    void rib_add_table_cb(const XrlError& e); 

    void rib_add_delete_route_cb(const XrlError& e, 
				 const char* action);

 private:
    enum { 
	MAXIFs=255, // Maximum number of interfaces
    };
    int _netfd;	// File descriptor used to send and receive
    int _igmpfd; // File descriptor for multicast routing
    int _udpfd;	// UDP file descriptor for ioctl's
    timeval last_time; // Last return from gettimeofday
    int next_phyint; // Next phyint value
    PatTree phyints; // Physical interfaces
    AVLtree interface_map; // IP addresses to phyint
    AVLtree directs; // Directly attached prefixes
    class BSDPhyInt *phys[MAXIFs];

    //rtentry m;
    //We can't use ioctl to add a masked route on FreeBSD,
    //so use an appropriate structure to use the routing socket.
    struct {
	struct rt_msghdr rt_rtm;
	struct sockaddr_in rt_dst;
	struct sockaddr_in rt_gateway;
	struct sockaddr_in rt_mask;
    } m;

    bool _changing_routerid;
    bool _change_complete;
    bool _dumping_remnants;
    FILE *_logstr;
};

/* Representation of a physical interface.
 * Used to map strings into small integers.
 */

class BSDPhyInt : public PatEntry {
    int phyint;
    InAddr addr;
    short flags;
    InMask mask;
    InAddr dstaddr;	// Other end of p-p link
    int mtu;

    friend class XorpOspfd;
    friend class InterfaceConfig;
    friend class KeyConfig;
    friend int SendInterface(void *,struct Tcl_Interp *, int,char *[]);
    friend int SendMD5Key(void *, Tcl_Interp *, int, char *argv[]);

 public:
    /* Some accessors */
    int get_phyint() const { return phyint; }
    InAddr get_addr() const { return addr; }
    InMask get_mask() const { return mask; }
    int get_mtu() const { return mtu; }
};

/* Map an IP interface (either belonging to an interface, or
 * the other end of a point-to-point interface) to a
 * physical interface.
 */

class BSDIfMap : public AVLitem {
    BSDPhyInt *phyp;
  public:
    inline BSDIfMap(InAddr, BSDPhyInt *);
    friend class XorpOspfd;
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

#endif
