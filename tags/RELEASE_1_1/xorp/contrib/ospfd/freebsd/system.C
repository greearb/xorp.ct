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
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
//#include <linux/sysctl.h>
//#include <linux/version.h>
// Hack to include mroute.h file
//#define _LINUX_SOCKIOS_H
//#define _LINUX_IN_H
#include <netinet/ip_mroute.h>
#include "ospfinc.h"
#include "monitor.h"
#include "system.h"
#include "tcppkt.h"
#include "freebsd.h"
#include "ospfd_freebsd.h"


/* Send an OSPF packet. If it is to be sent out
 * a serial lines, or to a specific IP next hop, adjust
 * the destination IP address to make the UNIX kernel happy.
 */

void FreeBSDOspfd::sendpkt(InPkt *pkt, int phyint, InAddr gw)

{
    printf("----\nsendpkt phyint=%d gw=%x\n", phyint, gw);
    BSDPhyInt *phyp;
    msghdr msg;
    iovec iov;
    size_t len;
    sockaddr_in to;

    phyp = phys[phyint];
#if LINUX_VERSION_CODE < LINUX22
    if (phyp->flags & IFF_POINTOPOINT)
	pkt->i_dest = hton32(phyp->dstaddr);
    else
#endif
    if (gw != 0)
	pkt->i_dest = hton32(gw);
    printf("pkt->i_dest = %lx\n", htonl(pkt->i_dest));
    pkt->i_chksum = ~incksum((uns16 *)pkt, sizeof(pkt));

    if (IN_CLASSD(ntoh32(pkt->i_dest))) {
	printf("it's multicast!\n");
	in_addr mreq;
	mreq.s_addr = hton32(phyp->addr);
	printf("mreq.s_addr = %x\n", phyp->addr);
	if (setsockopt(netfd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&mreq, sizeof(mreq)) < 0) {
	    syslog(LOG_ERR, "IP_MULTICAST_IF phyint %d: %m", phyint);
	    return;
	}
    }

    len = ntoh16(pkt->i_len);
    printf("len=%d, pkt->i_len=%d\n", len, pkt->i_len);

    //XXX  FreeBSD needs IP len in *host* byte order
    pkt->i_len = len;

    bzero(&to, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_len = sizeof(struct sockaddr_in);
    to.sin_addr.s_addr = pkt->i_dest;

    bzero(&msg, sizeof(msg));
    msg.msg_name = (caddr_t) &to;
    msg.msg_namelen = sizeof(to);
    iov.iov_len = len;
    iov.iov_base = (char *)pkt;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = MSG_DONTROUTE;
#if 1
    msg.msg_control = 0;
    msg.msg_controllen = 0;
#else
    byte cmsgbuf[128];
    cmsghdr *cmsg;
    in_pktinfo *pktinfo;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = CMSG_SPACE(sizeof(in_pktinfo));
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));
    cmsg->cmsg_level = SOL_IP;
    cmsg->cmsg_type = IP_PKTINFO;
    pktinfo = (in_pktinfo *) CMSG_DATA(cmsg);
    pktinfo->ipi_ifindex = phyint;
    pktinfo->ipi_spec_dst.s_addr = 0;
#endif
    if (sendmsg(netfd, &msg, /*MSG_DONTROUTE*/0) == -1)
	syslog(LOG_ERR, "sendmsg failed: %m");
    printf("done sending\n");
}

/* Send an OSPF packet, interface not specified.
 * This is used for virtual links.
 */

void FreeBSDOspfd::sendpkt(InPkt *pkt)

{
    printf("----\nsendpkt (virtual links)\n");
    size_t len;
    sockaddr_in to;

    len = ntoh16(pkt->i_len);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = pkt->i_dest;
    if (sendto(netfd, pkt, len, 0, (sockaddr *) &to, sizeof(to)) == -1)
	syslog(LOG_ERR, "sendto failed: %m");
}

/* Return whether or not a physical interface is
 * operational. On UNIX, the best we can do is check the
 * interface flags.
 */

bool FreeBSDOspfd::phy_operational(int phyint)

{
    printf("----\nphy_operational %d\n", phyint);
    BSDPhyInt *phyp;

    if (phyint == -1)
        return(false);
    if (!(phyp = phys[phyint]))
        return(false);
    return((phyp->flags & IFF_UP) != 0);
}

/* Open a network interface for the sending and receiving of OSPF
 * packets. We're just using a single raw socket, so this
 * is a no-op.
 */

void FreeBSDOspfd::phy_open(int)

{
    printf("----\nphy_open\n");
}

/* Closing a particular network interface is also a no-op.
 */

void FreeBSDOspfd::phy_close(int)

{
    printf("----\nphy_close\n");
}

/* Join a particular multicast group on a particular interface.
 * Executed when we become DR/Backup, or when an interface first
 * comes up.
 */

void FreeBSDOspfd::join(InAddr group, int phyint)

{
    printf("----\njoin\n");
#if LINUX_VERSION_CODE < LINUX22
    ip_mreq mreq;
#else
    ip_mreqn mreq;
#endif
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    mreq.imr_multiaddr.s_addr = hton32(group);
#if LINUX_VERSION_CODE < LINUX22
    mreq.imr_interface.s_addr = hton32(phyp->addr);
#else
    mreq.imr_ifindex = phyint;
    mreq.imr_address.s_addr = 0;
#endif

    if (setsockopt(netfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		   (char *)&mreq, sizeof(mreq)) < 0)
	syslog(LOG_ERR, "Join error, phyint %d: %m", phyint);
}

/* Leave a particular multicast group, again on a given
 * interface.
 */

void FreeBSDOspfd::leave(InAddr group, int phyint)

{
    printf("----\nleave\n");
#if LINUX_VERSION_CODE < LINUX22
    ip_mreq mreq;
#else
    ip_mreqn mreq;
#endif
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    mreq.imr_multiaddr.s_addr = hton32(group);
#if LINUX_VERSION_CODE < LINUX22
    mreq.imr_interface.s_addr = hton32(phyp->addr);
#else
    mreq.imr_ifindex = phyint;
    mreq.imr_address.s_addr = 0;
#endif

    if (setsockopt(netfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		   (char *)&mreq, sizeof(mreq)) < 0)
	syslog(LOG_ERR, "Leave error, phyint %d: %m", phyint);
}

/* Enable or disable IP forwarding.
 */

void FreeBSDOspfd::ip_forward(bool enabled)

{
    if (enabled)
	printf("----\nip_forward (enable)\n");
    else
	printf("----\nip_forward (disable)\n");
    int mib[4];
    int value;

    mib[0] = CTL_NET;
    mib[1] = PF_INET;
    mib[2] = 0; /* IP */
    mib[3] = IPCTL_FORWARDING;
    value = (enabled ? 1 : 0);
    sysctl(&mib[0], 4, 0, 0, &value, sizeof(value));
}
    
/* Enable or disable multicast forwarding globally.
 */

void FreeBSDOspfd::set_multicast_routing(bool enabled)

{
    printf("----\nset_multicast_routing\n");
    int on;

    if (enabled) {
        if ((igmpfd = socket(AF_INET, SOCK_RAW, PROT_IGMP)) == -1) {
	    syslog(LOG_ERR, "IGMP socket open failed: %m");
	    return;
	}
#if 0
	// Request notification of receiving interface
	int pktinfo = 1;
	setsockopt(igmpfd, IPPROTO_IP, IP_PKTINFO, &pktinfo, sizeof(pktinfo));
#endif
	on = 1;
	if (setsockopt(igmpfd, IPPROTO_IP, MRT_INIT, &on, sizeof(on)) == -1){
	    syslog(LOG_ERR, "MRT_INIT failed: %m");
	    return;
	}
    }
    else {
	on = 0;
	if (setsockopt(igmpfd, IPPROTO_IP, MRT_DONE, &on, sizeof(on)) == -1){
	    syslog(LOG_ERR, "MRT_DONE failed: %m");
	    return;
	}
	close(igmpfd);
	igmpfd = -1;
    }

}

/* Enable or disable multicast forwarding on a given
 * interface.
 */

void FreeBSDOspfd::set_multicast_routing(int phyint, bool enabled)

{
    printf("----\nset_multicast_routing (pre interface)\n");
    vifctl vif;
    BSDPhyInt *phyp;
    int optname;

    phyp = phys[phyint];
    if (igmpfd == -1)
        return;
    if ((phyp->flags & IFF_MULTICAST) == 0)
	return;
    vif.vifc_vifi = phyint;
    vif.vifc_flags = 0;
    vif.vifc_threshold = 1;
    vif.vifc_rate_limit = 0;
    vif.vifc_lcl_addr.s_addr = hton32(phyp->addr);
    vif.vifc_rmt_addr.s_addr = 0;
    optname = (enabled ? MRT_ADD_VIF : MRT_DEL_VIF);
    if (setsockopt(igmpfd, IPPROTO_IP, optname, &vif, sizeof(vif)) == -1)
        syslog(LOG_ERR, "MRT_ADD/DEL_VIF failed: %m");
}

/* Prepare to send a routing table add/delete to the FreeBSD
 * kernel.
 */

void FreeBSDOspfd::rtentry_prepare(InAddr net, InMask mask, MPath *mpp)

{
    printf("----\nrtentry_prepare\n");
    sockaddr_in *isp;

    memset(&m, 0, sizeof(m));
    // Copy net 
    isp = (sockaddr_in *) &m.rt_dst;
    isp->sin_family = AF_INET;
    isp->sin_addr.s_addr = hton32(net);
    isp->sin_len = sizeof(m.rt_dst);
    // Copy mask
    isp = (sockaddr_in *) &m.rt_mask;
    isp->sin_family = AF_INET;
    isp->sin_addr.s_addr = hton32(mask);
    isp->sin_len = sizeof(m.rt_mask);
    // Install next hop
    if (mpp) {
	InAddr gw;
	BSDPhyInt *phyp=0;
	gw = mpp->NHs[0].gw;
	if (mpp->NHs[0].phyint != -1)
	    phyp = phys[mpp->NHs[0].phyint];
	if (phyp && (phyp->flags & IFF_POINTOPOINT) != 0)
	    gw = phyp->dstaddr;
	isp = (sockaddr_in *) &m.rt_gateway;
	isp->sin_family = AF_INET;
	isp->sin_addr.s_addr = hton32(gw);
	isp->sin_len = sizeof(m.rt_gateway);
    }
    m.rt_rtm.rtm_msglen = sizeof(m);
    m.rt_rtm.rtm_version = RTM_VERSION;
    if (mask == 0xffffffff)
    	m.rt_rtm.rtm_flags |= RTF_HOST;
    m.rt_rtm.rtm_pid = getpid();
    m.rt_rtm.rtm_seq = rtsock_seq++;
    m.rt_rtm.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
}

/* Add/modify a kernel routing table entry.
 * Multipath is not supported, so just use the first path
 * found in the routing table entry.
 * If there is no next hop pointer, it means that there
 * is either a) and unresolved next hop or b) and entry
 * whose interface has been deleted, but the router-LSA
 * not yet reoriginated. In either case, just delete
 * the routing table entry.
 */

void FreeBSDOspfd::rtadd(InAddr net, InMask mask, MPath *mpp, 
		     MPath *ompp, bool reject)

{
    int bytes;
    printf("----\nrtadd\n");
    if (directs.find(net, mask))
	return;
    if (!mpp) {
	rtdel(net, mask, ompp);
	return;
    }
    if (ompp)
	rtdel(net, mask, ompp);
    rtentry_prepare(net, mask, mpp);
    // Reject route? 
    if (reject)
	m.rt_rtm.rtm_flags |=  RTF_REJECT;
    else
	m.rt_rtm.rtm_flags |= (RTF_UP | RTF_GATEWAY);

    m.rt_rtm.rtm_type = RTM_ADD;

    bytes = write(rtsock, &m, m.rt_rtm.rtm_msglen);
    if (bytes < 0) {
	if (errno == EEXIST) {
	    syslog(LOG_ERR, "Route already exists\n");
	    return;
	}
	syslog(LOG_ERR, "Failed to add route\n");
	return;
    }
#ifdef NOTDEF
    // Add through ioctl
    if (-1 == ioctl(udpfd, SIOCADDRT, (char *)&m)) {
        if (errno == EEXIST) {
	    ioctl(udpfd, SIOCDELRT, (char *)&m);
	    if (-1 != ioctl(udpfd, SIOCADDRT, (char *)&m))
	        return;
	}
	syslog(LOG_ERR, "SIOCADDRT: %m");
    }
#endif
}

/* Delete a kernel routing table entry.
 */

void FreeBSDOspfd::rtdel(InAddr net, InMask mask, MPath *ompp)

{
    int bytes;
    printf("----\nrtdel\n");
    if (directs.find(net, mask))
	return;
    rtentry_prepare(net, mask, ompp);
    m.rt_rtm.rtm_type = RTM_DELETE;

    bytes = write(rtsock, &m, m.rt_rtm.rtm_msglen);
    if (bytes < 0) {
	syslog(LOG_ERR, "Failed to add route\n");
	return;
    }
#ifdef NOTDEF
    // Delete through ioctl
    if (-1 == ioctl(udpfd, SIOCDELRT, (char *)&m))
	syslog(LOG_ERR, "SIOCDELRT: %m");
#endif
}

/* Uploading remnant routing table entries is not supported
 * on older linux versions.
 */

void FreeBSDOspfd::upload_remnants()

{
    printf("----\nupload_remnants (sysctl)\n");
    size_t size;
    int mib[6];
    char *rtbuf, *next;
    struct rt_msghdr *rtm;

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_INET;
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;
    if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
	syslog(LOG_ERR, "Failed on to get size for RT dump\n");
	return;
    }

    if ((rtbuf = new char[size]) == 0) {
	syslog(LOG_ERR, "Failed on to malloc memory for RT dump\n");
	return;
    }
    if (sysctl(mib, 6, rtbuf, &size, NULL, 0) < 0) {
	syslog(LOG_ERR, "Failed RT dump\n");
	return;
    }

    for (next = rtbuf; next < rtbuf+size; next += rtm->rtm_msglen) {
	rtm = (struct rt_msghdr *)next;
	rtsock_process(rtm);
    }
}


/* Add a multicast routing table entry to the kernel.
 */

void FreeBSDOspfd::add_mcache(InAddr, InAddr, MCache *)

{
}

/* Delete a multicast routing table entry from the kernel.
 */

void FreeBSDOspfd::del_mcache(InAddr, InAddr)

{
}

/* Return the printable name of a physical interface.
 */

char *FreeBSDOspfd::phyname(int phyint)

{
    printf("----\nphyname %d\n", phyint);
    BSDPhyInt *phyp;

    phyp = phys[phyint];
    return((char *) phyp->key);
}

/* Print an OSPF logging message into the
 * log file.
 */

void FreeBSDOspfd::sys_spflog(int msgno, char *msgbuf)

{
    time_t t;
    tm *tp;

    t = time(0);
    tp = localtime(&t);
    fprintf(logstr, "%02d:%02d:%02d OSPF.%03d: %s\n",
	    tp->tm_hour, tp->tm_min, tp->tm_sec, msgno, msgbuf);
    fflush(logstr);
    printf("LOG: %02d:%02d:%02d OSPF.%03d: %s\n",
	    tp->tm_hour, tp->tm_min, tp->tm_sec, msgno, msgbuf);
}

/* Exit the ospfd program, printing a diagnostic message in
 * the process.
 */

void FreeBSDOspfd::halt(int code, char *string)

{
    syslog(LOG_ERR, "Exiting: %s, code %d", string, code);
    if (code !=  0)
	abort();
    else if (changing_routerid)
        change_complete = true;
    else
        exit(0);
}
