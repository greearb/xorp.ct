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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <tcl8.3/tcl.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "../src/ospfinc.h"
#include "../src/monitor.h"
#include "../src/system.h"
#include "tcppkt.h"
#include "freebsd.h"
#include "ospfd_freebsd.h"
#include <time.h>

FreeBSDOspfd *ospfd_sys;
char buffer[MAX_IP_PKTSIZE];

// External declarations
bool get_prefix(char *prefix, InAddr &net, InMask &mask);

/* Signal handlers
 */
void timer(int)
{
    signal(SIGALRM, timer);
    ospfd_sys->one_second_timer();
}
void quit(int)
{
    ospfd_sys->changing_routerid = false;
    ospf->shutdown(10);
}
void reconfig(int)
{
    signal(SIGUSR1, reconfig);
    ospfd_sys->read_config();
}

/* The main OSPF loop. Loops getting messages (packets, timer
 * ticks, configuration messages, etc.) and never returns
 * until the OSPF process is told to exit.
 */

int main(int, char * [])

{
    int n_fd;
    itimerval itim;
    fd_set fdset;
    fd_set wrset;
    sigset_t sigset, osigset;

    sys = ospfd_sys = new FreeBSDOspfd();
    syslog(LOG_INFO, "Starting v%d.%d",
	   OSPF::vmajor, OSPF::vminor);
    // Read configuration
    ospfd_sys->read_config();
    if (!ospf) {
	syslog(LOG_ERR, "ospfd initialization failed");
	exit(1);
    }
    // Set up signals
    signal(SIGALRM, timer);
    signal(SIGHUP, quit);
    signal(SIGTERM, quit);
    signal(SIGUSR1, reconfig);
    itim.it_interval.tv_sec = 1;
    itim.it_value.tv_sec = 1;
    itim.it_interval.tv_usec = 0;
    itim.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &itim, NULL) < 0)
	syslog(LOG_ERR, "setitimer: %m");
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGUSR1);
    // Block signals in OSPF code
    sigprocmask(SIG_BLOCK, &sigset, &osigset);

    while (1) {
	int msec_tmo;
	int err;
	FD_ZERO(&fdset);
	FD_ZERO(&wrset);
	n_fd = ospfd_sys->netfd;
	FD_SET(ospfd_sys->netfd, &fdset);
	ospfd_sys->mon_fd_set(n_fd, &fdset, &wrset);
	if (ospfd_sys->igmpfd != -1) {
	    FD_SET(ospfd_sys->igmpfd, &fdset);
	    n_fd = MAX(n_fd, ospfd_sys->igmpfd);
	}
	if (ospfd_sys->rtsock != -1) {
	    FD_SET(ospfd_sys->rtsock, &fdset);
	    n_fd = MAX(n_fd, ospfd_sys->rtsock);
	}
	// Process any pending timers
	ospf->tick();
	// Time till next timer firing
	msec_tmo = ospf->timeout();
	// Flush any logging messages
	ospf->logflush();
	// Allow signals during select
	sigprocmask(SIG_SETMASK, &osigset, NULL);
	if (msec_tmo != -1) {
	    timeval timeout;
	    timeout.tv_sec = msec_tmo/1000;
	    timeout.tv_usec = (msec_tmo % 1000) * 1000;
	    err = select(n_fd+1, &fdset, &wrset, 0, &timeout);
	}
	else
	    err = select(n_fd+1, &fdset, &wrset, 0, 0);
	// Handle errors in select
	if (err == -1 && errno != EINTR) {
	    syslog(LOG_ERR, "select failed %m");
	    exit(1);
	}
	// Check for change of Router ID
	ospfd_sys->process_routerid_change();
	// Update elapsed time
	ospfd_sys->time_update();
	// Block signals in OSPF code
	sigprocmask(SIG_BLOCK, &sigset, &osigset);
	// Process received data packet, if any
	if (err <= 0)
	    continue;
	if (FD_ISSET(ospfd_sys->netfd, &fdset))
	    ospfd_sys->raw_receive(ospfd_sys->netfd);
	if (ospfd_sys->igmpfd != -1 &&
	    FD_ISSET(ospfd_sys->igmpfd, &fdset))
	    ospfd_sys->raw_receive(ospfd_sys->igmpfd);
	if (ospfd_sys->rtsock != -1 &&
	    FD_ISSET(ospfd_sys->rtsock, &fdset))
	    ospfd_sys->rtsock_receive(ospfd_sys->rtsock);
	// Process monitor queries and responses
	ospfd_sys->process_mon_io(&fdset, &wrset);
    }
}

/* Process packets received on a raw socket. Could
 * be either the OSPF socket or the IGMP socket.
 */

void FreeBSDOspfd::raw_receive(int fd)

{
    printf("----\nraw_receive\n");
    int plen;
    int rcvint = -1;
#if 1
    unsigned int fromlen;
    plen = recvfrom(fd, buffer, sizeof(buffer), 0, 0, &fromlen);
    if (plen < 0) {
        syslog(LOG_ERR, "recvfrom: %m");
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
        syslog(LOG_ERR, "recvmsg: %m");
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

    // Dispatch based on IP protocol
    switch (pkt->i_prot) {
      case PROT_OSPF:
	  printf("it's OSPF\n");
        ospf->rxpkt(rcvint, pkt, plen);
	break;
      case PROT_IGMP:
	  printf("it's IGMP\n");
        ospf->rxigmp(rcvint, pkt, plen);
	break;
      case 0:
	ospf->mclookup(pkt->i_src, pkt->i_dest);
	break;
      default:
	break;
    }
}

#define RTF_NOT_FOR_US  (RTF_LLINFO | RTF_LOCAL | RTF_MULTICAST \
        | RTF_BROADCAST	| RTF_DYNAMIC | RTF_BLACKHOLE | RTF_WASCLONED \
        | RTF_STATIC )

//#define RTF_NOT_FOR_US  RTF_LLINFO 

void FreeBSDOspfd::rtsock_receive(int fd)
{
    int plen;
    unsigned int fromlen;
    struct rt_msghdr* rtm;
    struct sockaddr_in *rt_sa;
    in_addr in;
    InAddr net;
    InMask mask;

    plen = recvfrom(fd, buffer, sizeof(buffer), 0, 0, &fromlen);
    printf("----\nrtsock_receive, plen=%d\n", plen);
    if (plen <= 0) {
        syslog(LOG_ERR, "rtsock recvfrom: %m");
	return;
    }
    rtm = (struct rt_msghdr*)buffer;

    printf("rtm->rtm_type: %d addrs: %x\n", rtm->rtm_type, rtm->rtm_addrs);
    if (rtm->rtm_errno != 0) {
	printf("Error reading from routing socket\n");
	syslog(LOG_ERR, "Error reading from routing socket: %m");
	return;
    }

    switch (rtm->rtm_type) {
    case RTM_IFINFO:
    case RTM_NEWADDR:
    case RTM_DELADDR:
	//XXX we should probably be more explicit about what happened!
	syslog(LOG_NOTICE, "Interface changed\n");
	read_config();
	return;
    case RTM_DELETE:
	printf("RTM_DELETE\n");
	if (rtm->rtm_pid == getpid()) {
	    printf("pin == my pid, so I don't care about this\n");
	    return;
	}
	if (rtm->rtm_flags & RTF_NOT_FOR_US) {
	    printf("not for us: rtm->rtm_flags = %x\n", rtm->rtm_flags);
	    return;
	}
	
	if (!(rtm->rtm_addrs & RTA_DST)) {
	    printf("no RTA_DST\n");
	    return;
	}

	//dst addr
	rt_sa = (struct sockaddr_in*) (((char *)rtm)
				       + sizeof(struct rt_msghdr));
	if (rt_sa->sin_family != AF_INET) {
	    printf("rt_sa->sin_family = %d\n", rt_sa->sin_family);
	    return;
	}
	memcpy(&in.s_addr, &rt_sa->sin_addr.s_addr, 4);
	    
	rt_sa = (struct sockaddr_in*)((char *)rt_sa + rt_sa->sin_len);

	//skip gateway
	if (rtm->rtm_addrs & RTA_GATEWAY) 
	    rt_sa = (struct sockaddr_in*)((char *)rt_sa + rt_sa->sin_len);

	//this should be the netmask
	//if (rt_sa->sin_family != AF_INET) 
	//    return;
	memcpy(&mask, &(rt_sa->sin_addr.s_addr), 4);

	mask = ntoh32(mask);
	net = ntoh32(in.s_addr) & mask;
	printf("Krt Delete %s %x\n", inet_ntoa(in), mask);
	printf("%x,%x\n", net, mask);
	syslog(LOG_NOTICE, "Krt Delete %s", inet_ntoa(in));
	ospf->krt_delete_notification(net, mask);
	break;
    case RTM_GET:
	rtsock_process(rtm);
	break;
    default:
	return;
    }

}

//rtsock_process parses the results of an RTM_GET message.  This can be
//called from rtsock_receive, or directly from upload_remnants

void FreeBSDOspfd::rtsock_process(struct rt_msghdr* rtm)
{
    struct sockaddr_in *rt_dst;
    struct sockaddr_in *rt_mask;
    struct sockaddr_in *rt_genmask;
    struct sockaddr_in *rt_gateway;

    if (rtm->rtm_errno != 0) {
	syslog(LOG_ERR, "Error reading from routing socket: %m");
	return;
    }

    // We don't want link layer info, multicast or broadcast routes,
    // or routes associated with directly connected interfaces.
    // We also don't want static routes.
    // The remainder are dynamic routes for remote sites, and it's these
    // we care about.

    printf("----\nrtsock_process\n");
    if (rtm->rtm_flags & RTF_NOT_FOR_US) {
	printf("Flags indicate it's not for us (%x)\n", rtm->rtm_flags);
	//	return;
    }

    printf("rtm_msglen = %d\n", rtm->rtm_msglen);
    printf("rtm_type = %d\n", rtm->rtm_type);
    printf("rtm_flags = %x\n", rtm->rtm_flags);
    printf("rtm_addrs = %x\n", rtm->rtm_addrs);
    printf("rtm_pid = %d\n", rtm->rtm_pid);
    printf("rtm_errno = %d\n", rtm->rtm_errno);
    printf("rtm_use = %d\n", rtm->rtm_use);
    
    rt_dst = (struct sockaddr_in*) ((char *)rtm + sizeof(struct rt_msghdr));
    if (rtm->rtm_addrs & RTA_DST) {
	rt_gateway = (struct sockaddr_in*)((char *)rt_dst + rt_dst->sin_len);
	if (rt_dst->sin_family != AF_INET) {
	    printf("Dst is not a v4 address\n");
	    return;
	}
	printf("rt_dst = %s\n", inet_ntoa(rt_dst->sin_addr));
    } else {
	rt_gateway = rt_dst;
    }
    if (rtm->rtm_addrs & RTA_GATEWAY) {
	rt_mask = (struct sockaddr_in*)((char *)rt_gateway + 
					rt_gateway->sin_len);
	if (rt_gateway->sin_family == AF_INET) {
	    printf("rt_gateway = %s\n", inet_ntoa(rt_gateway->sin_addr));
	} else {
	    printf("rt_gateway with AF %d, len=%d\n", rt_gateway->sin_family,
		   rt_dst->sin_len);
	    return;
	}
    } else {
	rt_mask = rt_gateway;
    }
    if (rtm->rtm_addrs & RTA_NETMASK) {
	rt_genmask = (struct sockaddr_in*)((char *)rt_mask + 
					   rt_mask->sin_len);
	if (rt_mask->sin_family == AF_INET) {
	    printf("rt_mask = %s\n", inet_ntoa(rt_mask->sin_addr));
	} else {
	    printf("rt_mask with AF %d, len=%d\n", rt_mask->sin_family,
		   rt_dst->sin_len);
	    printf("rt_mask = %s\n", inet_ntoa(rt_mask->sin_addr));
	}
    } else {
	rt_genmask = rt_mask;
    }
    printf("We should manage this one\n");
}


/* Update the program's notion of time, which is in milliseconds
 * since program start. Wait until receiving the timer signal
 * to update a full second.
 */

void FreeBSDOspfd::time_update()

{
    //    printf("----\ntime_update\n");
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

void FreeBSDOspfd::one_second_timer()

{
    //    printf("----\none_second_timer\n");
    timeval now;	// Current idea of time

    (void) gettimeofday(&now, NULL);
    sys_etime.sec++; 
    sys_etime.msec = 0;
    last_time = now;
}

/* Initialize the FreeBSD interface.
 * Open the network interface.
 * Start the random number generator.
 */

char *ospfd_log_file = "/var/log/ospfd.log";

FreeBSDOspfd::FreeBSDOspfd() : OSInstance(OSPFD_MON_PORT)

{
    printf("----\nFreeBSDOspfd\n");
    rlimit rlim;

    next_phyint = 0;
    memset(phys, 0, sizeof(phys));
    (void) gettimeofday(&last_time, NULL);
    changing_routerid = false;
    change_complete = false;
    dumping_remnants = false;
    // Allow core files
    rlim.rlim_max = RLIM_INFINITY;
    (void) setrlimit(RLIMIT_CORE, &rlim);
    // Open syslog
    openlog("ospfd", LOG_PID, LOG_DAEMON);
    // Open log file
    if (!(logstr = fopen(ospfd_log_file, "w"))) {
	syslog(LOG_ERR, "Logfile open failed: %m");
	exit(1);
    }
    // Open monitoring listen socket
    monitor_listen();
    // Open network
    if ((netfd = socket(AF_INET, SOCK_RAW, PROT_OSPF)) == -1) {
	syslog(LOG_ERR, "Network open failed: %m");
	exit(1);
    }
    // We will supply headers on output
    int hincl = 1;
    setsockopt(netfd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

    //Open Routing Socket
    if ((rtsock = socket(PF_ROUTE, SOCK_RAW, /*AF_INET*/0)) < 0) {
	syslog(LOG_ERR, "Failed to open routing socket: %m");
	exit(1);
    }
    rtsock_seq = 0;

    // Open ioctl socket
    if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	syslog(LOG_ERR, "Failed to open UDP socket: %m");
	exit(1);
    }
    // Start random number generator
    srand(getpid());
    // Will open multicast fd if requested to later
    igmpfd = -1;
}

/* Destructor not expected to be called during the life
 * of the program.
 */

FreeBSDOspfd::~FreeBSDOspfd()

{
}

/* TCL procedures to send configuration data to the ospfd
 * application.
 */

int SetRouterID(ClientData, Tcl_Interp *, int, char *argv[]);
int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[]);
int SendArea(ClientData, Tcl_Interp *, int, char *argv[]);
int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[]);
int SendHost(ClientData, Tcl_Interp *, int, char *argv[]);
int SendInterface(ClientData, Tcl_Interp *, int, char *argv[]);
int SendVL(ClientData, Tcl_Interp *, int, char *argv[]);
int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[]);
int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[]);
int SendMD5Key(ClientData, Tcl_Interp *, int, char *argv[]);

/* Read the ospfd config out of the file /etc/ospfd.conf
 */

char *ospfd_tcl_src = "/ospfd.tcl";
char *ospfd_config_file = "/etc/ospfd.conf";
rtid_t new_router_id;

void FreeBSDOspfd::read_config()

{
    printf("----\nread_config\n");
    Tcl_Interp *interp; // Interpretation of config commands
    char sendcfg[] = "sendcfg";
    int namlen;
    char *filename;

    // In process of changing router ID?
    if (changing_routerid)
        return;

    syslog(LOG_NOTICE, "reconfiguring");
    new_router_id = 0;
    interp = Tcl_CreateInterp();
    // Install C-language TCl commands
    Tcl_CreateCommand(interp, "routerid", SetRouterID, 0, 0);
    Tcl_CreateCommand(interp, "sendgen", SendGeneral, 0, 0);
    Tcl_CreateCommand(interp, "sendarea", SendArea, 0, 0);
    Tcl_CreateCommand(interp, "sendagg", SendAggregate, 0, 0);
    Tcl_CreateCommand(interp, "sendhost", SendHost, 0, 0);
    Tcl_CreateCommand(interp, "sendifc", SendInterface, 0, 0);
    Tcl_CreateCommand(interp, "sendvl", SendVL, 0, 0);
    Tcl_CreateCommand(interp, "sendnbr", SendNeighbor, 0, 0);
    Tcl_CreateCommand(interp, "sendextrt", SendExtRt, 0, 0);
    Tcl_CreateCommand(interp, "sendmd5", SendMD5Key, 0, 0);
    // Read additional TCL commands
    namlen = strlen(INSTALL_DIR) + strlen(ospfd_tcl_src);
    filename = new char[namlen+1];
    strcpy(filename, INSTALL_DIR);
    strcat(filename, ospfd_tcl_src);
    if (Tcl_EvalFile(interp, filename) != TCL_OK)
	syslog(LOG_INFO, "No additional TCL commands");
    delete [] filename;
    // (Re)read kernel interfaces
    read_kernel_interfaces();
    // (Re)read config file
    if (Tcl_EvalFile(interp, ospfd_config_file) != TCL_OK) {
	syslog(LOG_ERR, "Error in config file, line %d", interp->errorLine);
	return;
    }
    // Verify router ID was given
    if (!ospf ||  new_router_id == 0) {
	syslog(LOG_ERR, "Failed to set Router ID");
	return;
    }

    // Request to change OSPF Router ID?
    if (ospf->my_id() != new_router_id) {
        changing_routerid = true;
	ospf->shutdown(10);
	return;
    }

    // Reset current config
    ospf->cfgStart();
    // Download new config
    Tcl_Eval(interp, sendcfg);
    Tcl_DeleteInterp(interp);
    // Signal configuration complete
    ospf->cfgDone();
}

/* Complete the changing of the OSPF Router ID.
 */

void FreeBSDOspfd::process_routerid_change()

{
    if (changing_routerid && change_complete) {
	printf("----\nprocess_routerid_change\n");
        changing_routerid = false;
	change_complete = false;
	delete ospf;
	ospf = 0;
	read_config();
	if (!ospf) {
	    syslog(LOG_ERR, "Router ID change failed");
	    exit(1);
	}
    }
}

/* Find the physical interface to which a given address
 * belongs. Returns -1 if no matching interface
 * can be found.
 */

int FreeBSDOspfd::get_phyint(InAddr addr)

{
    printf("----\nget_phyint\n");
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

void FreeBSDOspfd::read_kernel_interfaces()

{
    printf("----\nread_kernel_interfaces\n");
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
    if (ioctl(udpfd, SIOCGIFCONF, (char *)&cfgreq) < 0) {
	syslog(LOG_ERR, "Failed to read interface config: %m");
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
	printf("IFname: %s\n", ifrp->ifr_name);
	// Ignore loopback interfaces
	// Also ignore "down" interfaces
	// Get interface flags
	short ifflags;
	memcpy(ifr.ifr_name, ifrp->ifr_name, sizeof(ifr.ifr_name));
	if (ioctl(udpfd, SIOCGIFFLAGS, (char *)&ifr) < 0) {
	    syslog(LOG_ERR, "SIOCGIFFLAGS Failed: %m");
	    exit(1);
	}
	if ((ifr.ifr_flags & IFF_LOOPBACK) != 0)
	    continue;
	ifflags = ifr.ifr_flags;
	printf("Flags=%x\n", ifflags);
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
	    printf("phyint number = %d, key=%s keylen=%d\n",  phyp->phyint, 
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
	printf("addr = %s\n", inet_ntoa(insock->sin_addr));
	// Get subnet mask
	if (ioctl(udpfd, SIOCGIFNETMASK, (char *)&ifr) < 0) {
	    syslog(LOG_ERR, "SIOCGIFNETMASK Failed: %m");
	    exit(1);
	}
	insock = (sockaddr_in *) &ifr.ifr_addr;
	phyp->mask = ntoh32(insock->sin_addr.s_addr);
	printf("netmask = %x\n", phyp->mask);
	add_direct(phyp, addr, phyp->mask);
	add_direct(phyp, addr, 0xffffffffL);
	// Get interface MTU
	phyp->mtu = ((phyp->flags & IFF_BROADCAST) != 0) ? 1500 : 576;
	if (ioctl(udpfd, SIOCGIFMTU, (char *)&ifr) >= 0)
	    phyp->mtu = ifr.ifr_mtu;
	printf("MTU: %d\n", phyp->mtu);
	// For point-to-point links, get other end's address
	phyp->dstaddr = 0;
	if ((phyp->flags & IFF_POINTOPOINT) != 0 &&
	    (ioctl(udpfd, SIOCGIFDSTADDR, (char *)&ifr) >= 0)) {
	    printf("Point-to-point\n");
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
	}	    
#if LINUX_VERSION_CODE >= LINUX22
	else
	    sys->rtdel(net, mask, 0);
#endif
    }

    delete [] ifcbuf;
}

/* Set the interface flags, If the IFF_UP flag
 * has changed, call the appropriate OSPFD API
 * routine.
 */

void FreeBSDOspfd::set_flags(BSDPhyInt *phyp, short flags)

{
    printf("----\nset_flags\n");
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

void FreeBSDOspfd::add_direct(BSDPhyInt *phyp, InAddr addr, InMask mask)

{
    printf("----\nadd_direct\n");
    DirectRoute *rte;
    if ((phyp->flags & IFF_UP) == 0) {
	printf("Interface is down!\n");
        return;
    }
    addr = addr & mask;
    if (!(rte = (DirectRoute *)directs.find(addr, mask))) {
	rte = new DirectRoute(addr, mask);
	printf("new direct route: %x, %x\n", addr, mask);
	directs.add(rte);
    }
    rte->valid = true;
}

/* Parse an interface identifier, which can either be an address
 * or a name like "eth0".
 */

bool FreeBSDOspfd::parse_interface(char *arg, in_addr &addr, BSDPhyInt *&phyp)

{
    phyp = 0;

    printf("---\nparse_interface: %s\n", arg);

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
	    if (ioctl(udpfd, SIOCGIFFLAGS, (char *)&ifr) < 0)
	        goto done;
	    if ((ifr.ifr_flags & IFF_LOOPBACK) != 0)
	        goto done;
	    ifflags = ifr.ifr_flags;
	    // Get interface index
	    if (ioctl(udpfd, SIOCGIFINDEX, (char *)&ifr) < 0)
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
#endif /* 0 */
    if (!phyp) {
	syslog(LOG_ERR, "Bad interface identifier %s", arg);
	return(false);
    }

    return(true);
}

/* Set the Router ID of the OSPF Process.
 * Refuse to reset the OSPF Router ID if it has already
 * been set.
 */

int SetRouterID(ClientData, Tcl_Interp *, int, char *argv[])

{
    new_router_id = ntoh32(inet_addr(argv[1]));
    if (!ospf)
	ospf = new OSPF(new_router_id);
    return(TCL_OK);
}

/* Download the global configuration values into the ospfd
 * software. If try to change Router ID, refuse reconfig.
 * If first time, create OSPF protocol instance.
 */

int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgGen m;

    m.lsdb_limit = atoi(argv[1]);
    m.mospf_enabled = atoi(argv[2]);
    m.inter_area_mc = atoi(argv[3]);
    m.ovfl_int = atoi(argv[4]);
    m.new_flood_rate = atoi(argv[5]);
    m.max_rxmt_window = atoi(argv[6]);
    m.max_dds = atoi(argv[7]);
    m.host_mode = atoi(argv[9]);
    m.log_priority = atoi(argv[8]);
    m.refresh_rate = atoi(argv[10]);
    m.PPAdjLimit = atoi(argv[11]);
    m.random_refresh = atoi(argv[12]);
    ospf->cfgOspf(&m);

    return(TCL_OK);
}

/* Dowload configuration of a single area
 */

int SendArea(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgArea m;

    m.area_id = ntoh32(inet_addr(argv[1]));
    m.stub = atoi(argv[2]);
    m.dflt_cost = atoi(argv[3]);
    m.import_summs = atoi(argv[4]);
    ospf->cfgArea(&m, ADD_ITEM);

    return(TCL_OK);
}

int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgRnge m;
    InAddr net;
    InAddr mask;

    if (get_prefix(argv[1], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.area_id = ntoh32(inet_addr(argv[2]));
	m.no_adv = atoi(argv[3]);
	ospf->cfgRnge(&m, ADD_ITEM);
    }

    return(TCL_OK);
}

int SendHost(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgHost m;
    InAddr net;
    InAddr mask;

    if (get_prefix(argv[1], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.area_id = ntoh32(inet_addr(argv[2]));
	m.cost = atoi(argv[3]);
	ospf->cfgHost(&m, ADD_ITEM);
    }

    return(TCL_OK);
}

/* Download an interface's configuration.
 * Interface can by identified by its address, name, or
 * for point-to-point addresses, the other end of the link.
 */

int SendInterface(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgIfc m;
    in_addr addr;
    BSDPhyInt *phyp;
    int intval;
    printf("SendInterface\n");

    if (!ospfd_sys->parse_interface(argv[1], addr, phyp))
	return(TCL_OK);

    printf("here23\n");
    m.address = phyp->addr;
    m.phyint = phyp->phyint;
    m.mask = phyp->mask;
    intval = atoi(argv[2]);
    m.mtu = (intval ? intval : phyp->mtu);
    m.IfIndex = atoi(argv[3]);
    m.area_id = ntoh32(inet_addr(argv[4]));
    intval = atoi(argv[5]);
    if (intval)
	m.IfType = intval;
    else if ((phyp->flags & IFF_BROADCAST) != 0)
	m.IfType = IFT_BROADCAST;
    else if ((phyp->flags & IFF_POINTOPOINT) != 0)
	m.IfType = IFT_PP;
    else
	m.IfType = IFT_NBMA;
    m.dr_pri = atoi(argv[6]);
    m.xmt_dly = atoi(argv[7]);
    m.rxmt_int = atoi(argv[8]);
    m.hello_int = atoi(argv[9]);
    m.if_cost = atoi(argv[10]);
    m.dead_int = atoi(argv[11]);
    m.poll_int = atoi(argv[12]);
    m.auth_type = atoi(argv[13]);
    memset(m.auth_key, 0, 8);
    strncpy((char *) m.auth_key, argv[14], (size_t) 8);
    m.mc_fwd = atoi(argv[15]);
    m.demand = atoi(argv[16]);
    m.passive = atoi(argv[17]);
    ospf->cfgIfc(&m, ADD_ITEM);

    return(TCL_OK);
}

int SendVL(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgVL m;

    m.nbr_id = ntoh32(inet_addr(argv[1]));
    m.transit_area = ntoh32(inet_addr(argv[2]));
    m.xmt_dly = atoi(argv[3]);
    m.rxmt_int = atoi(argv[4]);
    m.hello_int = atoi(argv[5]);
    m.dead_int = atoi(argv[6]);
    m.auth_type = atoi(argv[7]);
    strncpy((char *) m.auth_key, argv[8], (size_t) 8);
    ospf->cfgVL(&m, ADD_ITEM);

    return(TCL_OK);
}

int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgNbr m;

    m.nbr_addr = ntoh32(inet_addr(argv[1]));
    m.dr_eligible = atoi(argv[2]);
    ospf->cfgNbr(&m, ADD_ITEM);

    return(TCL_OK);
}

int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgExRt m;
    InAddr net;
    InMask mask;

    if (get_prefix(argv[1], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.type2 = (atoi(argv[3]) == 2);
	m.mc = (atoi(argv[5]) != 0);
	m.direct = 0;
	m.noadv = 0;
	m.cost = atoi(argv[4]);
	m.gw = ntoh32(inet_addr(argv[2]));
	m.phyint = ospfd_sys->get_phyint(m.gw);
	m.tag = atoi(argv[6]);
	ospf->cfgExRt(&m, ADD_ITEM);
    }
    return(TCL_OK);
}

int SendMD5Key(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgAuKey m;
    in_addr addr;
    BSDPhyInt *phyp;
    timeval now;
    tm tmstr;

    if (!ospfd_sys->parse_interface(argv[1], addr, phyp))
	return(TCL_OK);

    gettimeofday(&now, 0);
    m.address = phyp->addr;
    m.phyint = phyp->phyint;
    m.key_id = atoi(argv[2]);
    memset(m.auth_key, 0, 16);
    strncpy((char *) m.auth_key, argv[3], (size_t) 16);

    if (strptime(argv[4], "%D@%T", &tmstr)) {
	m.start_accept = SPFtime(mktime(&tmstr), 0);
    } else {
	m.start_accept = now;
    }
    
    if (strptime(argv[5], "%D@%T", &tmstr)) {
	m.start_generate = SPFtime(mktime(&tmstr), 0);
    } else {
	m.start_accept = now;
    }

    if (strptime(argv[6], "%D@%T", &tmstr)) {
	m.stop_generate = SPFtime(mktime(&tmstr), 0);
	m.stop_generate_specified = true;
    } else {
	m.stop_generate_specified = false;
    }

    if (strptime(argv[7], "%D@%T", &tmstr)) {
	m.stop_accept = SPFtime(mktime(&tmstr), 0);
	m.stop_accept_specified = true;
    } else {
	m.stop_accept_specified = false;
    }

    ospf->cfgAuKey(&m, ADD_ITEM);

    return(TCL_OK);
}
