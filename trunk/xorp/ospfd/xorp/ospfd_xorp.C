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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <net/if.h>

#include "ospf_module.h"

#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"

#include "ospfinc.h"
#include "monitor.h"
#include "system.h"

#include "tcppkt.h"
#include "os-instance.h"
#include "ospf_config.h"
#include "ospfd_xorp.h"
#include "xrl_target.h"

/* Initialize the FreeBSD interface.
 * Open the network interface.
 * Start the random number generator.
 */

XorpOspfd::XorpOspfd(EventLoop& e, XrlRouter& r) 
    : OSInstance(e, OSPFD_MON_PORT), _router(r), _rib_client(&r)
{
    const char *ospfd_log_file = "/tmp/ospfd.log";
    debug_msg("----\nXorpOspfd\n");
    rlimit rlim;

    next_phyint = 0;
    memset(phys, 0, sizeof(phys));
    (void) gettimeofday(&last_time, NULL);
    _changing_routerid = false;
    _change_complete = false;
    _dumping_remnants = false;
    // Allow core files
    rlim.rlim_max = RLIM_INFINITY;
    (void) setrlimit(RLIMIT_CORE, &rlim);

    // Open log file
    if ((_logstr = fopen(ospfd_log_file, "w"))==NULL) {
	XLOG_FATAL("Logfile open failed: %s", strerror(errno));
    }

    // Open monitoring listen socket
    monitor_listen();

    // Open network
    if ((_netfd = socket(AF_INET, SOCK_RAW, PROT_OSPF)) == -1) {
	XLOG_FATAL("Network open failed: %s", strerror(errno));
    }

    // We will supply headers on output
    int hincl = 1;
    setsockopt(_netfd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

    // Call receive raw when _netfd has data to be read
    if (_eventloop.add_selector(_netfd, SEL_RD, 
		       callback(this, &XorpOspfd::raw_receive)) == false) {
	XLOG_FATAL("Failed to add net fd.");
    }
    // Open ioctl socket
    if ((_udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	XLOG_FATAL("Failed to open UDP socket: %s", strerror(errno));
    }
    // Start random number generator
    srand(getpid());

    // Will open multicast fd if requested to later
    _igmpfd = -1;

    // Set up one second timer
    _one_sec_ticker = _eventloop.new_periodic(1000,
					       callback(this, &XorpOspfd::one_second_tick));
}

char buffer[MAX_IP_PKTSIZE];

// External declarations
bool get_prefix(const char *prefix, InAddr &net, InMask &mask);

void quit(int)
{
    XorpOspfd* ospfd_sys = static_cast<XorpOspfd*>(sys);
    ospfd_sys->_changing_routerid = false;
    ospf->shutdown(10);
}

/* Process packets received on a raw socket. Could
 * be either the OSPF socket or the IGMP socket.
 */

void XorpOspfd::raw_receive(int fd, SelectorMask m)

{
    assert(SEL_RD == m);
    debug_msg("----\nraw_receive\n");
    int plen;
    int rcvint = -1;
#if 1
    unsigned int fromlen;
    plen = recvfrom(fd, buffer, sizeof(buffer), 0, 0, &fromlen);
    if (plen < 0) {
        XLOG_ERROR("recvfrom: %s", strerror(errno));
	return;
    }
#else
    msghdr msg;
    iovec iov;
    byte cmsgbuf[128];
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    iov.iov_len = sizeof(buffer);
    iov.iov_base = buffer;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    plen = recvmsg(fd, &msg, 0);
    if (plen < 0) {
        XLOG_ERROR("recvmsg: %s", strerror(errno));
	return;
    }
    else {
	cmsghdr *cmsg;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	    if (cmsg->cmsg_level == SOL_IP &&
		cmsg->cmsg_type == IP_PKTINFO) {
	        in_pktinfo *pktinfo;
		pktinfo = (in_pktinfo *) CMSG_DATA(cmsg);
		rcvint = pktinfo->ipi_ifindex;
		break;
	    }
	}
    }
#endif

    struct ip *iph = (struct ip *)buffer;

    InPkt *pkt = (InPkt *) buffer;

    //FreeBSD gives us the packet with the IP length in host
    //byte order, but ospfd expects it in network byte order.  Fix it
    //here so we don't need to change the rest of ospfd.
    //
    //Even worse, FreeBSD modifies the IP header length to remove the IP
    //length to remove the IP header - add it back on.
    pkt->i_len = ntoh16(pkt->i_len + (iph->ip_hl*4));

    //    assert(pkt->i_len <= sizeof(buffer)); // always true

    // Dispatch based on IP protocol
    switch (pkt->i_prot) {
      case PROT_OSPF:
	  debug_msg("it's OSPF\n");
        ospf->rxpkt(rcvint, pkt, plen);
	break;
      case PROT_IGMP:
	  debug_msg("it's IGMP\n");
        ospf->rxigmp(rcvint, pkt, plen);
	break;
      case 0:
	ospf->mclookup(pkt->i_src, pkt->i_dest);
	break;
      default:
	break;
    }
}


/* Update the program's notion of time, which is in milliseconds
 * since program start. Wait until receiving the timer signal
 * to update a full second.
 */

void XorpOspfd::time_update()

{
    debug_msg("----\ntime_update\n");

    timeval now;	// Current idea of time
    int timediff;

    (void) gettimeofday(&now, NULL);
    timediff = 1000*(now.tv_sec - last_time.tv_sec);
    timediff += (now.tv_usec - last_time.tv_usec)/1000;
    if ((timediff + sys_etime.msec) < 1000)
	sys_etime.msec += timediff;
    last_time = now;
}

/* Signal handler for the one second timer.
 * Up the elapsed time to the next whole second.
 */

bool XorpOspfd::one_second_tick()

{
    //    debug_msg("----\none_second_timer\n");
    timeval now;	// Current idea of time

    (void) gettimeofday(&now, NULL);
    sys_etime.sec++; 
    sys_etime.msec = 0;
    last_time = now;
    return true;
}

/* Destructor not expected to be called during the life
 * of the program.
 */

XorpOspfd::~XorpOspfd()

{

}

/* Find the physical interface to which a given address
 * belongs. Returns -1 if no matching interface
 * can be found.
 */

int XorpOspfd::get_phyint(InAddr addr)

{
    debug_msg("----\nget_phyint\n");
    int i;
    for (i=0; i < MAXIFs; i++) {
	BSDPhyInt *phyp;
	phyp = phys[i];
	if (phyp && (phyp->addr & phyp->mask) == (addr & phyp->mask))
	    return(i);
    }

    return(-1);
}

/* Read the IP interface information out of the FreeBSD
 * kernel.
 */

void XorpOspfd::read_kernel_interfaces()

{
    debug_msg("----\nread_kernel_interfaces\n");
    ifconf cfgreq;
    ifreq *ifrp;
    ifreq *end;
    size_t size;
    char *ifcbuf;
    int blen;
    AVLsearch iter(&directs);
    DirectRoute *rte;

    blen = MAXIFs*sizeof(ifreq);
    ifcbuf = new char[blen];
    cfgreq.ifc_buf = ifcbuf;
    cfgreq.ifc_len = blen;
    if (ioctl(_udpfd, SIOCGIFCONF, (char *)&cfgreq) < 0) {
	XLOG_ERROR("Failed to read interface config: %s", strerror(errno));
	exit(1);
    }
    /* Clear current list of interfaces and directly
     * attached subnets, since we're going to reread
     * them.
     */
    interface_map.clear();
    while ((rte = (DirectRoute *)iter.next()))
        rte->valid = false;

    ifrp = (ifreq *) ifcbuf;
    end = (ifreq *)(ifcbuf + cfgreq.ifc_len);
    for (; ifrp < end; ifrp = (ifreq *)(((byte *)ifrp) + size)) {
	BSDPhyInt *phyp;
	byte *phystr;
	ifreq ifr;
	sockaddr_in *insock;
	InAddr addr;
	// Find next interface structure in list
	size=_SIZEOF_ADDR_IFREQ(*ifrp) ;
	
	// IP interfaces only
	if (ifrp->ifr_addr.sa_family != AF_INET) 
	    continue;
	debug_msg("IFname: %s\n", ifrp->ifr_name);
	// Ignore loopback interfaces
	// Also ignore "down" interfaces
	// Get interface flags
	short ifflags;
	memcpy(ifr.ifr_name, ifrp->ifr_name, sizeof(ifr.ifr_name));
	if (ioctl(_udpfd, SIOCGIFFLAGS, (char *)&ifr) < 0) {
	    XLOG_ERROR("SIOCGIFFLAGS Failed: %s", strerror(errno));
	    exit(1);
	}
	if ((ifr.ifr_flags & IFF_LOOPBACK) != 0)
	    continue;
	ifflags = ifr.ifr_flags;
	debug_msg("Flags=%x\n", ifflags);
	/* Found a legitimate interface
	 * Add physical interface and
	 * IP address maps
	 */
	if (!(phyp = (BSDPhyInt *) phyints.find(ifrp->ifr_name))) {
	    phyp = new BSDPhyInt;
	    phystr = new byte[sizeof(ifrp->ifr_name)];
	    bzero(phystr, sizeof(ifrp->ifr_name));
	    memcpy(phystr, ifrp->ifr_name, sizeof(ifrp->ifr_name));
	    phyp->key = phystr;
	    phyp->keylen = strlen(ifrp->ifr_name);
	    phyp->phyint = ++next_phyint;
	    debug_msg("phyint number = %d, key=%s keylen=%d\n",  phyp->phyint, 
		   phyp->key, phyp->keylen);
	    phyp->flags = 0;
	    phyints.add(phyp);
	    // May have multiple interfaces attached to same physical
	    // net
	    if (!phys[phyp->phyint])
	        phys[phyp->phyint] = phyp;
	}
	if (!memchr(ifrp->ifr_name, ':', strlen(ifrp->ifr_name))) {
	    set_flags(phyp, ifflags);
	}
	insock = (sockaddr_in *) &ifrp->ifr_addr;
	addr = phyp->addr = ntoh32(insock->sin_addr.s_addr);
	debug_msg("addr = %s\n", inet_ntoa(insock->sin_addr));
	// Get subnet mask
	if (ioctl(_udpfd, SIOCGIFNETMASK, (char *)&ifr) < 0) {
	    XLOG_ERROR("SIOCGIFNETMASK Failed: %s", strerror(errno));
	    exit(1);
	}
	insock = (sockaddr_in *) &ifr.ifr_addr;
	phyp->mask = ntoh32(insock->sin_addr.s_addr);
	debug_msg("netmask = %x\n", phyp->mask);
	add_direct(phyp, addr, phyp->mask);
	add_direct(phyp, addr, 0xffffffffL);
	// Get interface MTU
	phyp->mtu = ((phyp->flags & IFF_BROADCAST) != 0) ? 1500 : 576;
	if (ioctl(_udpfd, SIOCGIFMTU, (char *)&ifr) >= 0)
	    phyp->mtu = ifr.ifr_mtu;
	debug_msg("MTU: %d\n", phyp->mtu);
	// For point-to-point links, get other end's address
	phyp->dstaddr = 0;
	if ((phyp->flags & IFF_POINTOPOINT) != 0 &&
	    (ioctl(_udpfd, SIOCGIFDSTADDR, (char *)&ifr) >= 0)) {
	    debug_msg("Point-to-point\n");
	    addr = phyp->dstaddr = ntoh32(insock->sin_addr.s_addr);
	    add_direct(phyp, addr, 0xffffffffL);
	}
	// Install map from IP address to physical interface
	if (!interface_map.find(addr, 0)) {
	    BSDIfMap *map;
	    map = new BSDIfMap(addr, phyp);
	    interface_map.add(map);
	}
    }

    /* Put back any routes that were obscured by formerly
     * operational direct routes. Take away routes that are
     * now supplanted by direct routes.
     */
    iter.seek(0, 0);
    while ((rte = (DirectRoute *)iter.next())) {
        InAddr net=rte->index1();
	InMask mask=rte->index2();
        if (!rte->valid) {
	    directs.remove(rte);
	    delete rte;
	    ospf->krt_delete_notification(net, mask);
	} else
	    sys->rtdel(net, mask, 0);
    }

    delete [] ifcbuf;
}

/* Set the interface flags, If the IFF_UP flag
 * has changed, call the appropriate OSPFD API
 * routine.
 */

void XorpOspfd::set_flags(BSDPhyInt *phyp, short flags)

{
    debug_msg("----\nset_flags\n");
    short old_flags=phyp->flags;

    phyp->flags = flags;
    if (((old_flags^flags) & IFF_UP) != 0 && ospf) {
        if ((flags & IFF_UP) != 0)
	    ospf->phy_up(phyp->phyint);
	else
	    ospf->phy_down(phyp->phyint);
    }
}

/* Add to the list of directly attached prefixes. These
 * we will let the kernel manage directly.
 */

void XorpOspfd::add_direct(BSDPhyInt *phyp, InAddr addr, InMask mask)

{
    debug_msg("----\nadd_direct\n");
    DirectRoute *rte;
    if ((phyp->flags & IFF_UP) == 0) {
	debug_msg("Interface is down!\n");
        return;
    }
    addr = addr & mask;
    if (!(rte = (DirectRoute *)directs.find(addr, mask))) {
	rte = new DirectRoute(addr, mask);
	debug_msg("new direct route: %x, %x\n", addr, mask);
	directs.add(rte);
    }
    rte->valid = true;
}

/* Parse an interface identifier, which can either be an address
 * or a name like "eth0".
 */

bool XorpOspfd::parse_interface(const char *arg, in_addr &addr, 
				BSDPhyInt *&phyp)

{
    phyp = 0;

    debug_msg("---\nparse_interface: %s\n", arg);

    if (inet_aton(arg, &addr) == 1) {
	BSDIfMap *map;
	InAddr ifaddr;
	ifaddr = ntoh32(addr.s_addr);
	map = (BSDIfMap *) interface_map.find(ifaddr, 0);
	if (map != 0)
	    phyp = map->phyp;
    }
    else {
	char ifname[IFNAMSIZ];
	strncpy((char *)ifname, arg, IFNAMSIZ);
	phyp = (BSDPhyInt *) phyints.find(ifname);
#if 0
	// Try to detect unnumbered interfaces
	if (!phyp) {
	    int ifindex;
	    byte *phystr;
	    ifreq ifr;
	    short ifflags;
	    memcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	    if (memchr(ifr.ifr_name, ':', sizeof(ifr.ifr_name)))
	        goto done;
	    if (ioctl(_udpfd, SIOCGIFFLAGS, (char *)&ifr) < 0)
	        goto done;
	    if ((ifr.ifr_flags & IFF_LOOPBACK) != 0)
	        goto done;
	    ifflags = ifr.ifr_flags;
	    // Get interface index
	    if (ioctl(_udpfd, SIOCGIFINDEX, (char *)&ifr) < 0)
	        goto done;
	    ifindex = ifr.ifr_ifindex;
	    if (phys[ifindex])
	        goto done;
	    phyp = new BSDPhyInt;
	    phystr = new byte[sizeof(ifr.ifr_name)];
	    memcpy(phystr, ifr.ifr_name, sizeof(ifr.ifr_name));
	    phyp->key = phystr;
	    phyp->keylen = strlen(ifr.ifr_name);
	    phyp->phyint = ifindex;
	    phyp->flags = 0;
	    phyints.add(phyp);
	    phys[phyp->phyint] = phyp;
	    set_flags(phyp, ifflags);
	    phyp->addr = 0;
	    phyp->mask = 0;
	    phyp->dstaddr = 0;
	}
#endif
    }

#if 0
  done:
#endif
    if (!phyp) {
	XLOG_ERROR("Bad interface identifier %s", arg);
	return(false);
    }

    return(true);
}

void XorpOspfd::store_hitless_parms(int, int, struct MD5Seq *)

{
    XLOG_WARNING("unhandled call to XorpOspfd::store_hitless_parms.");
}

/* ------------------------------------------------------------------------- */
/* Xorp RIB callbacks and related*/

void 
XorpOspfd::rib_add_table_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add igp table for OSPF.");
	initiate_rib_add_table();
    }
}

void 
XorpOspfd::rib_add_delete_route_cb(const XrlError& e,
				   const char* action)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to %s route.", action);
	initiate_rib_add_table();
    }
}

void
XorpOspfd::rib_add_table()
{
    _rib_client.send_add_igp_table4("rib", "ospf", 
				   /* unicast */ true, 
				   /* multicast */ false,
				   callback(this, 
					    &XorpOspfd::rib_add_table_cb));
}

void
XorpOspfd::initiate_rib_add_table() 
{
    _rib_add_table_ticker = 
	_eventloop.new_oneoff_after_ms(
	    100, callback(this, &XorpOspfd::rib_add_table)
	    );
}


/* ------------------------------------------------------------------------- */
/* The main OSPF loop. Loops getting messages (packets, timer
 * ticks, configuration messages, etc.) and never returns
 * until the OSPF process is told to exit.
 */
static void ospfd_main()

{
    EventLoop eventloop;
    XrlStdRouter xrtr(eventloop, "ospfd");

    XorpOspfd ospfd_sys(eventloop, xrtr);
    sys = &ospfd_sys; // Set global ospf system pointer to the one in use

    // Instantiate Xrl Handler object
    XrlOspfTarget xrl_target(eventloop, xrtr, ospfd_sys, &ospf);
    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;
	
	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (xrtr.ready() == false && timed_out == false) {
	    eventloop.run();
	}
	
	if (xrtr.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }

    XLOG_INFO("Starting v%d.%d", OSPF::vmajor, OSPF::vminor);
    XLOG_INFO("Awaiting router id");

    // Spin until ospf object instantiated.
    ospf = 0;
    while (ospf == 0)
	eventloop.run();
    XLOG_INFO("Router ID set.");

    ospfd_sys.read_kernel_interfaces();
    ospfd_sys.initiate_rib_add_table();

    // Set up signals
    sigset_t sigset, osigset;
    signal(SIGHUP, quit);
    signal(SIGTERM, quit);
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGTERM);
    sigprocmask(SIG_BLOCK, &sigset, &osigset);

    while (1) {
	// Time till next timer firing
	int msec_tmo = ospf->timeout();

	// Run until next OSPFD timer is scheduled to fire
	bool breakout = false;
	XorpTimer t = eventloop.set_flag_after_ms(msec_tmo, &breakout);
	while (false == breakout)
	    eventloop.run();

	// Process pending OSPFD timers
	ospf->tick();

	// Flush any logging messages
	ospf->logflush();
    }
}

int main(int, const char* argv[])

{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	ospfd_main();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return 0;
}
