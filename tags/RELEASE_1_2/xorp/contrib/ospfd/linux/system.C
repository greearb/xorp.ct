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
#include <asm/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#if LINUX_VERSION_CODE >= LINUX22
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif
#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <linux/sysctl.h>
#include <linux/version.h>
// Hack to include mroute.h file
#define _LINUX_SOCKIOS_H
#define _LINUX_IN_H
#include <linux/mroute.h>
#include "ospfinc.h"
#include "monitor.h"
#include "system.h"
#include "tcppkt.h"
#include "linux.h"
#include "ospfd_linux.h"


/* Send an OSPF packet. If it is to be sent out
 * a serial lines, or to a specific IP next hop, adjust
 * the destination IP address to make the UNIX kernel happy.
 */

void LinuxOspfd::sendpkt(InPkt *pkt, int phyint, InAddr gw)

{
    BSDPhyInt *phyp;
    msghdr msg;
    iovec iov;
    size_t len;
    sockaddr_in to;

    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
#if LINUX_VERSION_CODE < LINUX22
    if (phyp->flags & IFF_POINTOPOINT)
	pkt->i_dest = hton32(phyp->dstaddr);
    else
#endif
    if (gw != 0)
	pkt->i_dest = hton32(gw);
    pkt->i_chksum = ~incksum((uns16 *)pkt, sizeof(pkt));

    if (IN_CLASSD(ntoh32(pkt->i_dest))) {
#if LINUX_VERSION_CODE < LINUX22
	in_addr mreq;
	mreq.s_addr = hton32(phyp->addr);
#else
	ip_mreqn mreq;
	mreq.imr_ifindex = phyint;
	mreq.imr_address.s_addr = 0;
#endif
	if (setsockopt(netfd, IPPROTO_IP, IP_MULTICAST_IF,
		       (char *)&mreq, sizeof(mreq)) < 0) {
	    syslog(LOG_ERR, "IP_MULTICAST_IF phyint %d: %m", phyint);
	    return;
	}
    }

    len = ntoh16(pkt->i_len);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = pkt->i_dest;
    msg.msg_name = (caddr_t) &to;
    msg.msg_namelen = sizeof(to);
    iov.iov_len = len;
    iov.iov_base = pkt;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = MSG_DONTROUTE;
#if LINUX_VERSION_CODE < LINUX22
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

    if (sendmsg(netfd, &msg, MSG_DONTROUTE) == -1)
	syslog(LOG_ERR, "sendmsg failed: %m");
}

/* Send an OSPF packet, interface not specified.
 * This is used for virtual links.
 */

void LinuxOspfd::sendpkt(InPkt *pkt)

{
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

bool LinuxOspfd::phy_operational(int phyint)

{
    BSDPhyInt *phyp;

    if (phyint == -1)
        return(false);
    if (!(phyp = (BSDPhyInt *)phyints.find(phyint, 0)))
        return(false);
    return((phyp->flags & IFF_UP) != 0);
}

/* Open a network interface for the sending and receiving of OSPF
 * packets. We're just using a single raw socket, so this
 * is a no-op.
 */

void LinuxOspfd::phy_open(int)

{
}

/* Closing a particular network interface is also a no-op.
 */

void LinuxOspfd::phy_close(int)

{
}

/* Join a particular multicast group on a particular interface.
 * Executed when we become DR/Backup, or when an interface first
 * comes up.
 */

void LinuxOspfd::join(InAddr group, int phyint)

{
#if LINUX_VERSION_CODE < LINUX22
    ip_mreq mreq;
#else
    ip_mreqn mreq;
#endif
    BSDPhyInt *phyp;

    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
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

void LinuxOspfd::leave(InAddr group, int phyint)

{
#if LINUX_VERSION_CODE < LINUX22
    ip_mreq mreq;
#else
    ip_mreqn mreq;
#endif
    BSDPhyInt *phyp;

    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
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

void LinuxOspfd::ip_forward(bool enabled)

{
    int mib[3];
    int value;

    mib[0] = CTL_NET;
    mib[1] = NET_IPV4;
    mib[2] = NET_IPV4_FORWARD;
    value = (enabled ? 1 : 0);
    sysctl(&mib[0], 3, 0, 0, &value, sizeof(value));
}
    
/* Enable or disable multicast forwarding globally.
 */

void LinuxOspfd::set_multicast_routing(bool enabled)

{
    int on;

    if (enabled) {
        if ((igmpfd = socket(AF_INET, SOCK_RAW, PROT_IGMP)) == -1) {
	    syslog(LOG_ERR, "IGMP socket open failed: %m");
	    return;
	}
#if LINUX_VERSION_CODE >= LINUX22
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

void LinuxOspfd::set_multicast_routing(int phyint, bool enabled)

{
    vifctl vif;
    BSDPhyInt *phyp;
    int optname;
    int vifno;

    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
    if (igmpfd == -1)
        return;
    // Kernel will enable multicast on tunnels
    if ((phyp->flags & IFF_MULTICAST) == 0 && !phyp->tunl)
	return;
    vifno = phyp->vifno;
    // Not a state change?
    if (enabled != (vifno == 0))
        return;
    // If enabling, allocate VIF. This is necessary since there
    // aren't many possible, so you can't just use IfIndex.
    // Reserve VIF of 0 to indicate "no interface"
    if (enabled) {
	for (int i = 1; ; i++) {
	    if (i >= MAXVIFS) {
	        syslog(LOG_ERR, "Can't allocate VIF, ifc %s", phyp->phyname);
		return;
	    }
	    if (vifs[i] == 0) {
	        phyp->vifno = vifno = i;
		vifs[i] = phyint;
		break;
	    }
	}
    }
    else {
	// De-allocate VIF
        vifs[vifno] = 0;
        phyp->vifno = 0;
    }

    vif.vifc_vifi = vifno;
    vif.vifc_flags = 0;
    vif.vifc_threshold = 1;
    vif.vifc_rate_limit = 0;
    if (!phyp->tunl) {
    vif.vifc_lcl_addr.s_addr = hton32(phyp->addr);
    vif.vifc_rmt_addr.s_addr = 0;
    }
    else {
        vif.vifc_lcl_addr.s_addr = hton32(phyp->tsrc);
	vif.vifc_rmt_addr.s_addr = hton32(phyp->tdst);
	vif.vifc_flags = VIFF_TUNNEL;
    }
    optname = (enabled ? MRT_ADD_VIF : MRT_DEL_VIF);
    if (setsockopt(igmpfd, IPPROTO_IP, optname, &vif, sizeof(vif)) == -1)
        syslog(LOG_ERR, "MRT_ADD/DEL_VIF failed: %m");
}

#if LINUX_VERSION_CODE < LINUX22
/* Prepare to send a routing table add/delete to the Linux
 * kernel.
 */

void LinuxOspfd::rtentry_prepare(InAddr net, InMask mask, MPath *mpp)

{
    sockaddr_in *isp;

    memset(&m, 0, sizeof(m));
    // Copy net 
    isp = (sockaddr_in *) &m.rt_dst;
    isp->sin_family = AF_INET;
    isp->sin_addr.s_addr = hton32(net);
    // Copy mask
    isp = (sockaddr_in *) &m.rt_genmask;
    isp->sin_family = AF_INET;
    isp->sin_addr.s_addr = hton32(mask);
    // Set flags
    m.rt_flags = 0;
    if (mask == 0xffffffff)
	m.rt_flags |= RTF_HOST;
    // Install next hop
    if (mpp) {
	InAddr gw;
	BSDPhyInt *phyp=0;
	gw = mpp->NHs[0].gw;
	if (mpp->NHs[0].phyint != -1)
	    phyp = (BSDPhyInt *)phyints.find(mpp->NHs[0].phyint, 0);
	if (phyp && (phyp->flags & IFF_POINTOPOINT) != 0)
	    gw = phyp->dstaddr;
	isp = (sockaddr_in *) &m.rt_gateway;
	isp->sin_family = AF_INET;
	isp->sin_addr.s_addr = hton32(gw);
    }
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

void LinuxOspfd::rtadd(InAddr net, InMask mask, MPath *mpp, 
		     MPath *ompp, bool reject)

{
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
	m.rt_flags |=  RTF_REJECT;
    else
	m.rt_flags |= (RTF_UP | RTF_GATEWAY);

    // Add through ioctl
    if (-1 == ioctl(udpfd, SIOCADDRT, (char *)&m)) {
        if (errno == EEXIST) {
	    ioctl(udpfd, SIOCDELRT, (char *)&m);
	    if (-1 != ioctl(udpfd, SIOCADDRT, (char *)&m))
	        return;
	}
	syslog(LOG_ERR, "SIOCADDRT: %m");
    }
}

/* Delete a kernel routing table entry.
 */

void LinuxOspfd::rtdel(InAddr net, InMask mask, MPath *ompp)

{
    if (directs.find(net, mask))
	return;
    rtentry_prepare(net, mask, ompp);
    // Delete through ioctl
    if (-1 == ioctl(udpfd, SIOCDELRT, (char *)&m))
	syslog(LOG_ERR, "SIOCDELRT: %m");
}

/* Uploading remnant routing table entries is not supported
 * on older Linux versions.
 */

void LinuxOspfd::upload_remnants()

{
}

#else
/* On Linux 2.2, add and delete routing table entries
 * via the rtnetlink interface. Note that we are setting
 * rtm_protocol to 89. The Linux kernel doesn't do anything
 * special with the value, but it allows us to delete entries
 * freely without worrying that we will bash some other
 * routing daemon's entries. We should register the rtm_protocol
 * value with the Linux guys.
 */

void LinuxOspfd::rtadd(InAddr net, InMask mask, MPath *mpp, 
		     MPath *ompp, bool reject)

{
    nlmsghdr *nlm;
    rtmsg *rtm;
    rtattr *rta_dest;
    rtattr *rta_gw;
    int size;
    int prefix_length;

    if (directs.find(net, mask) || !mpp) {
	rtdel(net, mask, ompp);
	return;
    }

    // Change mask to prefix length
    for (prefix_length = 32; prefix_length > 0; prefix_length--) {
	if ((mask & (1 << (32-prefix_length))) != 0)
	    break;
    }
    // Calculate size of routing message
    size = NLMSG_SPACE(sizeof(*rtm)); // Routing message itself
    if (prefix_length > 0)
	size += RTA_SPACE(4);	// For destination
    if (!reject)
	size += RTA_SPACE(4);	// For next hop
    // Allocate routing table message, and find place for
    // individual data items
    nlm = (nlmsghdr *) new char[size];
    nlm->nlmsg_len = size;
    nlm->nlmsg_type = RTM_NEWROUTE;
    nlm->nlmsg_flags = NLM_F_REQUEST|NLM_F_REPLACE|NLM_F_CREATE;
    nlm->nlmsg_seq = nlm_seq++;
    nlm->nlmsg_pid = 0;
    rtm = (rtmsg *) NLMSG_DATA(nlm);
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = prefix_length;
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = 0;
    rtm->rtm_protocol = PROT_OSPF;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;
    rtm->rtm_flags = 0;
    if (prefix_length > 0) {
        uns32 swnet;
	int attrlen;
	rta_dest = (rtattr *) RTM_RTA(rtm);
	rta_dest->rta_len = attrlen = RTA_SPACE(4);
	rta_dest->rta_type = RTA_DST;
	swnet = hton32(net);
	memcpy(RTA_DATA(rta_dest), &swnet, sizeof(swnet));
	rta_gw = (rtattr *) RTA_NEXT(rta_dest, attrlen);
    }
    else
	rta_gw = (rtattr *) RTM_RTA(rtm);
    // Reject route? 
    if (reject) {
	rtm->rtm_scope = RT_SCOPE_HOST;
	rtm->rtm_type = RTN_UNREACHABLE;
    }
    else {
	InAddr gw;
	BSDPhyInt *phyp=0;
	int phyint;
	gw = hton32(mpp->NHs[0].gw);
	if ((phyint = mpp->NHs[0].phyint) != -1)
	    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
	if (phyp && (phyp->flags & IFF_POINTOPOINT) != 0){
	    // Fill in gw attribute
	    rta_gw->rta_len = RTA_SPACE(sizeof(phyint));
	    rta_gw->rta_type = RTA_OIF;
	    memcpy(RTA_DATA(rta_gw), &phyint, sizeof(phyint));
	}
	else {
	    // Fill in gw attribute
	    rta_gw->rta_len = RTA_SPACE(4);
	    rta_gw->rta_type = RTA_GATEWAY;
	    memcpy(RTA_DATA(rta_gw), &gw, sizeof(gw));
	}
    }

    // Add through routing socket send
    if (-1 == send(rtsock, nlm, size, 0))
	syslog(LOG_ERR, "add route through routing socket: %m");

    delete [] ((char *)nlm);
}

void LinuxOspfd::rtdel(InAddr net, InMask mask, MPath *)

{
    nlmsghdr *nlm;
    rtmsg *rtm;
    rtattr *rta_dest;
    int size;
    int prefix_length;

    // Change mask to prefix length
    for (prefix_length = 32; prefix_length > 0; prefix_length--) {
	if ((mask & (1 << (32-prefix_length))) != 0)
	    break;
    }
    // Calculate size of routing message
    size = NLMSG_SPACE(sizeof(*rtm)); // Routing message itself
    if (prefix_length > 0)
	size += RTA_SPACE(4);	// For destination
    // Allocate routing table message, and find place for
    // individual data items
    nlm = (nlmsghdr *) new char[size];
    nlm->nlmsg_len = size;
    nlm->nlmsg_type = RTM_DELROUTE;
    nlm->nlmsg_flags = NLM_F_REQUEST;
    nlm->nlmsg_seq = nlm_seq++;
    nlm->nlmsg_pid = 0;
    rtm = (rtmsg *) NLMSG_DATA(nlm);
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = prefix_length;
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = 0;
    rtm->rtm_protocol = PROT_OSPF;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;
    rtm->rtm_flags = 0;
    if (prefix_length > 0) {
        uns32 swnet;
	int attrlen;
	rta_dest = (rtattr *) RTM_RTA(rtm);
	rta_dest->rta_len = attrlen = RTA_SPACE(4);
	rta_dest->rta_type = RTA_DST;
	swnet = hton32(net);
	memcpy(RTA_DATA(rta_dest), &swnet, sizeof(swnet));
    }

    // Delete through routing socket send
    if (-1 == send(rtsock, nlm, size, 0))
	syslog(LOG_ERR, "del route through routing socket: %m");

    delete [] ((char *)nlm);
}

/* Request the kernel to upload the current set of routing
 * table entries that it has.
 */

void LinuxOspfd::upload_remnants()

{
    nlmsghdr *nlm;
    rtmsg *rtm;
    int size;

    // Set state to dumping
    dumping_remnants = true;

    // Calculate size of netlink message
    size = NLMSG_SPACE(sizeof(*rtm)); // Only a routing message
    // Allocate netlink message
    nlm = (nlmsghdr *) new char[size];
    nlm->nlmsg_len = size;
    nlm->nlmsg_type = RTM_GETROUTE;
    nlm->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlm->nlmsg_seq = nlm_seq++;
    nlm->nlmsg_pid = 0;
    rtm = (rtmsg *) NLMSG_DATA(nlm);
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = 0;
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = 0;
    rtm->rtm_protocol = PROT_OSPF;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;
    rtm->rtm_flags = 0;

    // Send to routing socket
    if (-1 == send(rtsock, nlm, size, 0))
	syslog(LOG_ERR, "routing table dump: %m");

    delete [] ((char *)nlm);
}

#endif

/* Add a multicast routing table entry to the kernel.
 */

void LinuxOspfd::add_mcache(InAddr src, InAddr grp, MCache *e)

{
    mfcctl mfe;
    int i;

    // Initially set to drop matching packets
    mfe.mfcc_origin.s_addr = hton32(src);
    mfe.mfcc_mcastgrp.s_addr = hton32(grp);
    mfe.mfcc_parent = 0;
    for (i = 0; i < MAXVIFS; i++)
        mfe.mfcc_ttls[i] = 255;

    // Now fill in with MOSPF information
    if (e) {
        BSDPhyInt *phyp;
        if (e->n_upstream && (phyp=(BSDPhyInt *)phyints.find(*e->up_phys, 0)))
	    mfe.mfcc_parent = phyp->vifno;
	for (i = 0; i < e->n_downstream; i++) {
	    if (e->down_str[i].nbr_addr != 0)
  	        // NBMA multicast not supported by Linux
	        continue;
	    if (!(phyp = (BSDPhyInt *)phyints.find(e->down_str[i].phyint, 0)))
	        continue;
	    if (phyp->vifno == 0)
	        continue;
	    // Linux ignores TTL 0, so bump to 1 in that case
	    if ((mfe.mfcc_ttls[phyp->vifno] = e->down_str[i].ttl) == 0)
	        mfe.mfcc_ttls[phyp->vifno] = 1;
	}
    }

    if (setsockopt(igmpfd, IPPROTO_IP, MRT_ADD_MFC, &mfe, sizeof(mfe)) == -1)
        syslog(LOG_ERR, "MRT_ADD_MFC failed: %m");
}

/* Delete a multicast routing table entry from the kernel.
 */

void LinuxOspfd::del_mcache(InAddr src, InAddr grp)

{
    mfcctl mfe;

    mfe.mfcc_origin.s_addr = hton32(src);
    mfe.mfcc_mcastgrp.s_addr = hton32(grp);
    if (setsockopt(igmpfd, IPPROTO_IP, MRT_DEL_MFC, &mfe, sizeof(mfe)) == -1)
        syslog(LOG_ERR, "MRT_DEL_MFC failed: %m");
}

/* Return the printable name of a physical interface.
 */

char *LinuxOspfd::phyname(int phyint)

{
    BSDPhyInt *phyp;

    phyp = (BSDPhyInt *)phyints.find(phyint, 0);
    return(phyp ? (char *) phyp->phyname : 0);
}

/* Print an OSPF logging message into the
 * log file.
 */

void LinuxOspfd::sys_spflog(int msgno, char *msgbuf)

{
    time_t t;
    tm *tp;

    t = time(0);
    tp = localtime(&t);
    fprintf(logstr, "%02d:%02d:%02d OSPF.%03d: %s\n",
	    tp->tm_hour, tp->tm_min, tp->tm_sec, msgno, msgbuf);
    fflush(logstr);
}

/* Exit the ospfd program, printing a diagnostic message in
 * the process.
 */

void LinuxOspfd::halt(int code, char *string)

{
    syslog(LOG_ERR, "Exiting: %s, code %d", string, code);
    if (code !=  0)
	abort();
    else if (changing_routerid)
        change_complete = true;
    else
        exit(0);
}

/* Store the hitless restart parameters in the file
 * /etc/ospfd.restart. These are regular TCL commands, which
 * will get read again when the ospfd restarts.
 */

const char *ospfd_rst_file = "/etc/ospfd.restart";

void LinuxOspfd::store_hitless_parms(int grace_period, int n, MD5Seq *sns)

{
    FILE *f;
    extern rtid_t new_router_id;
    in_addr addr;

    if (!(f = fopen(ospfd_rst_file, "w"))) {
        syslog(LOG_ERR, "Can't open %s for writing: %m", ospfd_rst_file);
	return;
    }

    fprintf(f, "grace_period %d\n", grace_period);
    addr.s_addr = hton32(new_router_id);
    fprintf(f, "routerid %s\n", inet_ntoa(addr));
    for (int i = 0; i < n; i++) {
        if (sns[i].if_addr != 0) {
	    addr.s_addr = hton32(sns[i].if_addr);
	    fprintf(f, "interface %s 1\n", inet_ntoa(addr));
	}
	else
	    fprintf(f, "interface %s 1\n", phyname(sns[i].phyint));
	fprintf(f, "md5_seqno %d\n", sns[i].seqno);
    }
    fclose(f);
}
