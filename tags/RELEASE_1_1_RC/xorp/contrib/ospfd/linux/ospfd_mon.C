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
#include <unistd.h>
#include <tcl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "../src/ospfinc.h"
#include "../src/monitor.h"
#include "../src/system.h"
#include "tcppkt.h"
#include <time.h>

// External references
void print_lsa(LShdr *hdr);

// Forward references
void print_response();
void send_stat_request();
void print_statistics(StatRsp *);
void get_areas();
void get_interfaces();
void print_interface(MonMsg *m);
void get_neighbors();
void print_neighbor(MonMsg *m);
void get_database(byte lstype);
void get_lsa();
void get_rttbl();
void print_pair(char *, int, int);
const char *yesorno(byte val);
void prompt();
void syntax();

const int OSPFD_MON_PORT = 12767;

TcpPkt *monpkt;
char buffer[80];
uns32 ospf_router_id;
byte id;
int monfd;

/* The ospfd_mon program is a simple application
 * that established a TCP connection to the ospfd
 * routing daemon, and then turns keyboard commands
 * into monitoring requests, printing the responses
 * on standard output.
 *
 * Syntax:
 *	ospfd_mon [host] [-p port]
 *
 * Defaults to the default ospfd monitoring port on
 * the local host.
 */

int main(int argc, char *argv[])

{
    sockaddr_in saddr;
    int port;
    char hostname[64];
    hostent *hostp;

    // Set host and port defaults
    if (gethostname(hostname, sizeof(hostname)) != 0) {
	perror("gethostname");
	exit(1);
    }
    port = OSPFD_MON_PORT;

    // Parse command line arguments
    argv++, argc--;
    while (argc > 0) {
	if (strcmp(*argv, "-p") == 0) {
	    argv++, argc--;
	    if (argc > 0) {
	        port = atoi(*argv);
		argv++, argc--;
	    }
	} else {
	    strncpy(hostname, *argv, sizeof(hostname));
	    argv++, argc--;
	}
    }

    if (!(hostp = gethostbyname(hostname))) {
	herror("gethostbyname");
	exit(1);
    }

    // Open monitoring connection
    printf("Connecting to %s:%d ...", hostname, port);
    if (!(monfd = socket(AF_INET, SOCK_STREAM, 0))) {
	perror("socket");
	exit(1);
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = *(uns32 *)(hostp->h_addr);
    saddr.sin_port = hton16(port);
    if (connect(monfd, (sockaddr *) &saddr, sizeof(saddr)) < 0) {
	perror("connect");
	exit(1);
    }

    monpkt = new TcpPkt(monfd);
    // First execute a statistics command
    printf("connected\r\n\n");
    send_stat_request();
    // Wait for statistics response
    print_response();
        
    // Then loop, with prompt indicating ospf Router ID
    while (1) {
	prompt();
	fgets(buffer, sizeof(buffer), stdin);
	if (strncmp(buffer, "adv", 3) == 0)
	    get_lsa();
	else if (strncmp(buffer, "area", 4) == 0)
	    get_areas();
	else if (strncmp(buffer, "as", 2) == 0)
	    get_database(LST_ASL);
	else if (strncmp(buffer, "data", 4) == 0)
	    get_database(0);
	else if (strncmp(buffer, "int", 3) == 0)
	    get_interfaces();
	else if (strncmp(buffer, "nei", 3) == 0)
	    get_neighbors();
	else if (strncmp(buffer, "route", 5) == 0)
	    get_rttbl();
	else if (strncmp(buffer, "stat", 4) == 0) {
	    send_stat_request();
	    print_response();
	}
	else if (strncmp(buffer, "exit", 4) == 0)
	    exit(0);
	else
	    syntax();
    }
}

/* Pretty print the received response. Simply dispatch
 * on the response type. Errors indicate that a next
 * reached the end of list, or that an item was not
 * found.
 */

void print_response()

{
    MonHdr *mhdr;
    MonMsg *m;
    uns16 type;
    uns16 subtype;

    if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	perror("recv");
	exit(1);
    }

    m = (MonMsg *) mhdr;
    switch(type) {
      case Stat_Response:
	print_statistics(&m->body.statrsp);
	break;
      default:
	printf("Unknown response type %d\n", type);
	break;
    }
}

/* Print OSPF router ID in prompt.
 */

void prompt()

{
    byte *p;

    p = (byte *) &ospf_router_id;
    printf("%d.%d.%d.%d> ", p[0], p[1], p[2], p[3]);
}

/* Send statistics request.
 */

void send_stat_request()

{
    MonMsg req;
    int mlen;

    req.hdr.version = OSPF_MON_VERSION;
    req.hdr.retcode = 0;
    req.hdr.exact = 0;
    mlen = sizeof(MonHdr);
    req.hdr.id = hton16(id++);
    if (!monpkt->sendpkt_suspend(&req, MonReq_Stat, 0, mlen)) {
        printf("Send failed");
	exit(1);
    }
}

/* Print global statistics information.
 */

void print_statistics(StatRsp *s)

{
    byte *p;

    p = (byte *) &s->router_id;
    printf("OSPF Router ID:\t%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    printf("\t# AS-external-LSAs:\t%d\r\n", ntoh32(s->n_aselsas));
    printf("ASE checksum:\t0x%x", ntoh32(s->asexsum));
    printf("\t\t# ASEs originated:\t%d\r\n", ntoh32(s->n_ase_import));
    printf("ASEs allowed:\t%d", ntoh32(s->extdb_limit));
    printf("\t\t# Dijkstras:\t\t%d\r\n", ntoh32(s->n_dijkstra));
    printf("# Areas:\t%d", ntoh16(s->n_area));
    printf("\t\t# Nbrs in Exchange:\t%d\r\n", ntoh16(s->n_dbx_nbrs));
    printf("MOSPF enabled:\t%s", yesorno(s->mospf));
    printf("\t\tInter-area multicast:\t%s\r\n", yesorno(s->inter_area_mc));
    printf("Inter-AS multicast: %s", yesorno(s->inter_AS_mc));
    printf("\t\tIn overflow state:\t%s\r\n", yesorno(s->overflow_state));
    printf("ospfd version:\t%d.%d\r\n\n", s->vmajor, s->vminor);

    // Network byte order
    ospf_router_id = s->router_id;
}

/* Print out a line for each attached area.
 */

void get_areas()

{
    MonMsg req;
    int mlen;
    uns32 a_id;
    int i;

    printf("%-15s %-8s %-8s %-6s %-10s %s\r\n",
	   "Area ID", "#Ifcs", "#Routers", "#LSAs", "Xsum", "Comments");

    for (a_id = 0, i = 0; ; i++) {
	MonHdr *mhdr;
	MonMsg *m;
	in_addr in;
	char s[16];
	AreaRsp *arearsp;
	int n_lsas;
	uns16 type;
	uns16 subtype;

	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = (i == 0) ? 1 : 0;
	req.body.arearq.area_id = hton32(a_id);
	mlen = sizeof(MonHdr) + sizeof(MonRqArea);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_Area, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0) {
	    if (i != 0)
	        break;
	    continue;
	}

	a_id = ntoh32(m->body.arearsp.area_id);
	arearsp = &m->body.arearsp;

	// Print out Area info
	in = *((in_addr *) &arearsp->area_id);
	printf("%-15s ", inet_ntoa(in));
	print_pair(s, ntoh16(arearsp->n_ifcs), ntoh16(arearsp->n_cfgifcs));
	printf("%-8s ", s);
	print_pair(s, ntoh16(arearsp->n_routers), ntoh16(arearsp->n_rtrlsas));
	printf("%-8s ", s);
	n_lsas = ntoh16(arearsp->n_rtrlsas);
	n_lsas += ntoh16(arearsp->n_netlsas);
	n_lsas += ntoh16(arearsp->n_summlsas);
	n_lsas += ntoh16(arearsp->n_asbrlsas);
	n_lsas += ntoh16(arearsp->n_grplsas);
	printf("%-6d ", n_lsas);
	s[0] = '\0';
	sprintf(s, "0x%x", ntoh32(arearsp->dbxsum));
	printf("%-10s ", s);
	if (arearsp->transit)
	    printf("transit ");
	if (arearsp->demand)
	    printf("demand-capable ");
	if (arearsp->stub)
	    printf("stub ");
	if (!arearsp->import_summ)
	    printf("no-import ");
	printf("\r\n");
    }
}

/* Get a given LSA, and print it out in detail.
 * Area must be specified for all advertisements except
 * the AS-external-LSAs.
 */

void get_lsa()

{
    MonMsg req;
    int mlen;
    uns32 a_id;
    uns32 ls_id;
    uns32 adv_rtr;
    byte lstype;
    char *ptr;
    MonHdr *mhdr;
    MonMsg *m;
    LShdr *lshdr;
    uns16 type;
    uns16 subtype;

    ptr = buffer;
    if (!strsep(&ptr, " ")) {
        syntax();
	return;
    }
    lstype = atoi(ptr);
    if (!strsep(&ptr, " ")) {
        syntax();
	return;
    }
    ls_id = ntoh32(inet_addr(ptr));
    if (!strsep(&ptr, " ")) {
        syntax();
	return;
    }
    adv_rtr = ntoh32(inet_addr(ptr));
    if (lstype != LST_ASL) {
	if (!strsep(&ptr, " ")) {
            syntax();
	    return;
	}
	a_id = ntoh32(inet_addr(ptr));
    }
    else
	a_id = 0;

    req.hdr.version = OSPF_MON_VERSION;
    req.hdr.retcode = 0;
    req.hdr.exact = 1;
    req.body.lsarq.area_id = hton32(a_id);
    req.body.lsarq.ls_type = hton32(lstype);
    req.body.lsarq.ls_id = hton32(ls_id);
    req.body.lsarq.adv_rtr = hton32(adv_rtr);
    mlen = sizeof(MonHdr) + sizeof(MonRqLsa);
    req.hdr.id = hton16(id++);
    if (!monpkt->sendpkt_suspend(&req, MonReq_LSA, 0, mlen)) {
        printf("Send failed");
	exit(1);
    }

    if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	perror("recv");
	exit(1);
    }

    m = (MonMsg *) mhdr;
    if (m->hdr.retcode != 0) {
	printf("LSA not found\n");
	return;
    }
    // Print out LSA
    lshdr = (LShdr *) (((char *) m) + mlen);
    print_lsa(lshdr);
}

/* Get the link-state database of a given area, printing
 * one line for each LSA. If called with LS type of 5
 * dumps AS-external-LSAs instead.
 */

void get_database(byte lstype)

{
    MonMsg req;
    int mlen;
    uns32 a_id;
    uns32 ls_id;
    uns32 adv_rtr;
    byte new_lstype;
    int n_lsas = 0;
    uns32 xsum = 0;

    if (lstype != LST_ASL) {
	char *ptr;
	ptr = buffer;
	if (!strsep(&ptr, " ")) {
	    syntax();
	    return;
	}
	a_id = ntoh32(inet_addr(ptr));
    }
    else
	a_id = 0;

    ls_id = 0;
    adv_rtr = 0;
    new_lstype = lstype;

    printf("%4s %15s %15s %10s %6s %4s\r\n",
	   "Type", "LS_ID", "ADV_RTR", "Seqno", "Xsum", "Age");

    while (1) {
	MonHdr *mhdr;
	MonMsg *m;
	LShdr *lshdr;
	in_addr in;
	uns16 type;
	uns16 subtype;
	age_t age;

	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	req.body.lsarq.area_id = hton32(a_id);
	req.body.lsarq.ls_type = hton32(new_lstype);
	req.body.lsarq.ls_id = hton32(ls_id);
	req.body.lsarq.adv_rtr = hton32(adv_rtr);
	mlen = sizeof(MonHdr) + sizeof(MonRqLsa);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_LSA, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	    break;
	if (a_id != ntoh32(m->body.lsarq.area_id))
	    break;
	new_lstype = ntoh32(m->body.lsarq.ls_type);
	if (new_lstype != lstype) {
	    if (lstype == LST_ASL || new_lstype == LST_ASL)
		break;
	}
	ls_id = ntoh32(m->body.lsarq.ls_id);
	adv_rtr = ntoh32(m->body.lsarq.adv_rtr);

	n_lsas++;
	lshdr = (LShdr *) (((char *) m) + mlen);
	xsum += ntoh16(lshdr->ls_xsum);
	// Print out Link state header
	printf("%4d ", lshdr->ls_type);
	in = *((in_addr *) &lshdr->ls_id);
	printf("%15s ", inet_ntoa(in));
	in = *((in_addr *) &lshdr->ls_org);
	printf("%15s ", inet_ntoa(in));
	printf("0x%08x ", ntoh32(lshdr->ls_seqno));
	printf("0x%04x ", ntoh16(lshdr->ls_xsum));
	age = ntoh16(lshdr->ls_age);
	if ((age & DoNotAge) != 0) {
	    age &= ~DoNotAge;
	    printf("DNA+%d\r\n", age);
	}
	else
	    printf("%4d\r\n", age);
    }
    printf("\t\t# LSAs: %d\r\n", n_lsas);
    printf("\t\tDatabase xsum: 0x%x\r\n", xsum);
}

/* Print out a line for each interface.
 */

void get_interfaces()

{
    MonMsg req;
    int mlen;
    int phyint;
    InAddr addr;
    InAddr taid;
    InAddr endpt;
    MonHdr *mhdr;
    MonMsg *m;
    uns16 type;
    uns16 subtype;

    printf("%-15s %-15s %-15s %-8s %-8s %-4s %-4s %-5s\r\n",
	   "Phy", "Addr", "Area", "Type", "State", "#Nbr", "#Adj", "Cost");

    // First real interfaces
    for (addr = 0, phyint = 0; ; ) {
	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	req.body.ifcrq.phyint = hton32(phyint);
	req.body.ifcrq.if_addr = hton32(addr);
	mlen = sizeof(MonHdr) + sizeof(MonRqIfc);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_Ifc, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	phyint = ntoh32(m->body.ifcrsp.if_phyint);
	addr = ntoh32(m->body.ifcrsp.if_addr);
	print_interface(m);
    }
    // Then virtual links
    for (taid = 0, endpt = 0; ; ) {
	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	req.body.vlrq.transit_area = hton32(taid);
	req.body.vlrq.endpoint_id = hton32(endpt);
	mlen = sizeof(MonHdr) + sizeof(MonRqVL);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_VL, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	taid = ntoh32(m->body.ifcrsp.transit_id);
	endpt = ntoh32(m->body.ifcrsp.endpt_id);
	print_interface(m);
   }
}

/* Print the interface information. Used for both real interfaces
 * and virtual links.
 */

void print_interface(MonMsg *m)

{
    IfcRsp *ifcrsp;
    in_addr in;

    ifcrsp = &m->body.ifcrsp;

    // Print out Interface info
    printf("%-16s ", ifcrsp->phyname);
    in = *((in_addr *) &ifcrsp->if_addr);
    printf("%-15s ", inet_ntoa(in));
    in = *((in_addr *) &ifcrsp->area_id);
    printf("%-15s ", inet_ntoa(in));
    printf("%-8s ", ifcrsp->type);
    printf("%-8s ", ifcrsp->if_state);
    printf("%-4d ", ifcrsp->if_nnbrs);
    printf("%-4d ", ifcrsp->if_nfull);
    printf("%-5d\r\n", ntoh16(ifcrsp->if_cost));
}


/* Print out a line for each neighbor
 */

void get_neighbors()

{
    MonMsg req;
    int mlen;
    int phyint;
    InAddr addr;
    InAddr taid;
    InAddr endpt;
    MonHdr *mhdr;
    MonMsg *m;
    uns16 type;
    uns16 subtype;

    printf("%-15s %-15s %-15s %-8s %-4s %-4s %-5s\r\n",
	   "Phy", "Addr", "ID", "State", "#DD", "#Rq", "#Rxmt");

    // First the real neighbors
    for (addr = 0, phyint = 0; ; ) {
	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	req.body.nbrrq.phyint = hton32(phyint);
	req.body.nbrrq.nbr_addr = hton32(addr);
	mlen = sizeof(MonHdr) + sizeof(MonRqNbr);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_Nbr, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	phyint = ntoh32(m->body.nbrsp.phyint);
	addr = ntoh32(m->body.nbrsp.n_addr);
	print_neighbor(m);
    }
    // Then virtual links
    for (taid = 0, endpt = 0; ; ) {
	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	req.body.vlrq.transit_area = hton32(taid);
	req.body.vlrq.endpoint_id = hton32(endpt);
	mlen = sizeof(MonHdr) + sizeof(MonRqVL);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_VLNbr, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	taid = ntoh32(m->body.nbrsp.transit_id);
	endpt = ntoh32(m->body.nbrsp.endpt_id);
	print_neighbor(m);
    }
}

/* Print out single line for neighbor. Used by both
 * real neighbors and virtual neighbors.
 */

void print_neighbor(MonMsg *m)

{
    in_addr in;
    NbrRsp *nbrsp;
    char buffer[20];

    nbrsp = &m->body.nbrsp;

    // Print out Neighbor info
    printf("%-16s ", nbrsp->phyname);
    in = *((in_addr *) &nbrsp->n_addr);
    printf("%-15s ", inet_ntoa(in));
    in = *((in_addr *) &nbrsp->n_id);
    printf("%-15s ", inet_ntoa(in));
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, nbrsp->n_state, MON_STATELEN);
    printf("%-8s ", buffer);
    printf("%-4d ", ntoh32(nbrsp->n_ddlst));
    printf("%-4d ", ntoh32(nbrsp->n_rqlst));
    printf("%-5d\r\n", ntoh32(nbrsp->rxmt_count));
}

/* Print out a line for each prefix in the routing table.
 */

void get_rttbl()

{
    MonMsg req;
    int mlen;
    uns32 net;
    uns32 mask;
    int i;

    printf("%-18s %-8s %-8s %-8s %-15s %s\r\n",
	   "Prefix", "Type", "Cost", "Ifc", "Next-hop", "Mpaths");

    for (net = 0, mask = 0, i = 0; ; i++) {
	MonHdr *mhdr;
	MonMsg *m;
	in_addr in;
	char str[20];
	RteRsp *rtersp;
	int prefix_length;
	int n_paths;
	uns16 type;
	uns16 subtype;

	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = (i == 0) ? 1 : 0;
	req.body.rtrq.net = hton32(net);
	req.body.rtrq.mask = hton32(mask);
	mlen = sizeof(MonHdr) + sizeof(MonRqRte);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_Rte, 0, mlen)) {
            printf("Send failed");
	    exit(1);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    perror("recv");
	    exit(1);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0) {
	    if (i != 0)
	        break;
	    continue;
	}

	net = ntoh32(m->body.rtersp.net);
	mask = ntoh32(m->body.rtersp.mask);
	rtersp = &m->body.rtersp;

	// Print out Area info
	in = *((in_addr *) &m->body.rtersp.net);
	for (prefix_length = 32; prefix_length > 0; prefix_length--) {
	    if ((mask & (1 << (32-prefix_length))) != 0)
		break;
	}
	sprintf(str, "%s/%d", inet_ntoa(in), prefix_length);
	printf("%-18s ", str);
	printf("%-8s ", rtersp->type);
	printf("%-8d ", ntoh32(rtersp->cost));
	n_paths = ntoh32(rtersp->npaths);
	if (n_paths == 0)
	    printf("%-8s %-15s", "n/a", "n/a");
	else {
	    printf("%-8s ", rtersp->hops[0].phyname);
	    in = *((in_addr *) &rtersp->hops[0].gw);
	    if (rtersp->hops[0].gw == 0)
	        printf("%-15s ", "n/a");
	    else
	        printf("%-15s ", inet_ntoa(in));
	    if (n_paths > 1)
	        printf("%d", n_paths);
	}
	printf("\r\n");
    }
}

/* Print a pair of numbers. The second is printed only
 * if it is different from the first, and then in
 * parenthesis.
 */

void print_pair(char *s, int val1, int val2)

{
    s[0] = '\0';
    sprintf(s, "%d", val1);
    if (val2 != val2) {
	char *ptr;
	ptr = s + strlen(s);
	sprintf(ptr, "(%d)", val2);
    }
}

const char *yesorno(byte val)

{
    return((val != 0) ? "yes" : "no");
}

void syntax()

{
    printf("Command syntax:\n");
    printf("adv %%type %%ls_id %%adv_rtr %%area_id\n");
    printf("areas\n");
    printf("as-externals\n");
    printf("interfaces\n");
    printf("neighbors\n");
    printf("database %%area_id\n");
    printf("routes\n");
    printf("statistics\n");
    printf("exit\n");
}
