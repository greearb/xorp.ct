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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>

#include "ospf_module.h"
#include "ospfinc.h"
#include "monitor.h"
#include "system.h"

#include "tcppkt.h"

// External references
void print_lsa(LShdr *hdr);

// Forward references
const char *yesorno(byte val);
void get_statistics(bool print);
int get_areas(bool print);
void get_interfaces();
void print_interface(MonMsg *m);
void get_neighbors();
void print_neighbor(MonMsg *m);
void get_database(byte lstype);
void select_lsa();
void get_lsa();
void get_rttbl();
void display_html(const char *);
void display_error(const char *);
void get_opaques();

const int OSPFD_MON_PORT = 12767;

TcpPkt *monpkt;
char buffer[80];
uns32 ospf_router_id;
byte id;
int monfd;
PatTree pairs;
bool header_printed = false;

/* External defintions of the raw pages.
 */

extern const char *page_header;
extern const char *page_footer;
extern const char *query_attach;
extern const char *statistics_page;
extern const char *area_header;
extern const char *area_row;
extern const char *area_footer;
extern const char *database_page_top;
extern const char *ase_page_top;
extern const char *database_row;
extern const char *database_page_bottom;
extern const char *select_area_top;
extern const char *select_area_bottom;
extern const char *interface_page_top;
extern const char *interface_row;
extern const char *interface_page_bottom;
extern const char *neighbor_page_top;
extern const char *neighbor_row;
extern const char *neighbor_page_bottom;
extern const char *rttbl_page_top;
extern const char *rttbl_row;
extern const char *rttbl_page_bottom;
extern const char *select_lsa_top;
extern const char *select_lsa_bottom;
extern const char *expand_lsa_top;
extern const char *expand_lsa_bottom;
extern const char *error_page;
extern const char *opaque_page_top;
extern const char *opaque_row;

/* Class used to store the value-pairs returned
 * by the html server.
 */

class ValuePair : public PatEntry {
  public:
    char *value;
    int vlen;
    ValuePair(const char *s, const char *v);
    ~ValuePair();
};

/* Constructor for a value pair
 */

ValuePair::ValuePair(const char *s, const char *v)

{
    keylen = strlen(s);
    key = new byte[keylen];
    memcpy(key, s, keylen);
    vlen = strlen(v);
    value = new char[vlen+1];
    memcpy(value, v, vlen);
    value[vlen] = '\0';
}

/* Destructor for a value pair. Free the allocated
 * strings.
 */

ValuePair::~ValuePair()

{
    delete [] key;
    delete [] value;
}

/* Add a value pair to a given tree.
 * If the value pair already exists, just
 * substitute the new value.
 */

void addVP(PatTree *tree, const char *s, const char *v)

{
    ValuePair *entry;

    entry = (ValuePair *) tree->find(s);
    if (!entry) {
	entry = new ValuePair(s, v);
	tree->add(entry);
    }
    else {
        delete [] entry->value;
	entry->vlen = strlen(v);
	entry->value = new char[entry->vlen+1];
	memcpy(entry->value, v, entry->vlen);
	entry->value[entry->vlen] = '\0';
    }
}

/* The ospfd_html program is a CGI application
 * that establishes a TCP connection to the ospfd
 * routing daemon, and then based on information
 * received from the web server, returns the correct
 * pages of information.
 */

int main(int /* argc */, const char * /* argv*/[])

{
    int port;
    hostent *hostp;
    char *qs;
    char *vp;
    in_addr in;
    ValuePair *entry;
    bool attached=true;

    puts("Content-type: text/html\n\n");
    addVP(&pairs, "errno", "error unknown");
    addVP(&pairs, "cstate", "unattached");

    // Set host and port defaults
    if (gethostname(buffer, sizeof(buffer)) != 0) {
        display_error("gethostname");
	exit(0);
    }
    if (!(hostp = gethostbyname(buffer))) {
        display_html(page_header);
        addVP(&pairs, "operation", "gethostbyname");
	sprintf(buffer, "host error #%d", h_errno);
	addVP(&pairs, "errno", buffer);
	display_html(error_page);
	display_html(page_footer);
	exit(0);
    }

    in.s_addr = *(uns32 *)(hostp->h_addr);
    port = OSPFD_MON_PORT;

    // Parse the query string coming from the server
    qs = getenv("QUERY_STRING");
    vp = qs ? strtok(qs, "&") : 0;
    while (vp != 0) {
	char *value;
	value = strchr(vp, '=');
	*value = '\0';
	value++;
	addVP(&pairs, vp, value);
	vp = strtok(0, "&");
	}
	    
    if (!pairs.find("addr")) {
	attached = false;
	addVP(&pairs, "addr", inet_ntoa(in));
    }
    if (!pairs.find("port")) {
	attached = false;
	sprintf(buffer, "%d", port);
	addVP(&pairs, "port", buffer);
    }

    if (attached) {
        entry = (ValuePair *)pairs.find("addr");
	sprintf(buffer, "%s", entry->value);
	addVP(&pairs, "cstate", buffer);
    }

    display_html(page_header);
    header_printed = true;

    if (!attached) {
        display_html(query_attach);
    }
    else {
	sockaddr_in saddr;
	char *command=0;
	// Open monitoring connection
	if ((monfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    display_error("socket");
	    exit(0);
	}
	entry = (ValuePair *)pairs.find("addr");
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(entry->value);
	entry = (ValuePair *)pairs.find("port");
	saddr.sin_port = hton16(atoi(entry->value));
	if (connect(monfd, (sockaddr *) &saddr, sizeof(saddr)) < 0) {
	    display_error("connect");
	    exit(0);
	}
	monpkt = new TcpPkt(monfd);
	// Switch on command
	if ((entry = (ValuePair *)pairs.find("command")))
	    command = entry->value;
	if (!command || strncmp(command, "stat", 4) == 0)
	    get_statistics(true);
	else if (strncmp(command, "area", 4) == 0)
	    get_areas(true);
	else if (strncmp(command, "data", 4) == 0)
	    get_database(0);
	else if (strncmp(command, "as", 2) == 0)
	    get_database(LST_ASL);
	else if (strncmp(command, "int", 3) == 0)
	    get_interfaces();
	else if (strncmp(command, "nei", 3) == 0)
	    get_neighbors();
	else if (strncmp(command, "route", 5) == 0)
	    get_rttbl();
	else if (strncmp(command, "lsa", 3) == 0)
	    select_lsa();
	else if (strncmp(command, "adv", 3) == 0)
	    get_lsa();
	else if (strncmp(command, "opqs", 4) == 0)
	    get_opaques();
    }

    display_html(page_footer);
    exit(0);
}

/* Get global OSPF statistics
 */

void get_statistics(bool print)

{
    MonMsg req;
    int mlen;
    MonHdr *mhdr;
    MonMsg *m;
    uns16 type;
    uns16 subtype;
    StatRsp *s;
    in_addr in;

    req.hdr.version = OSPF_MON_VERSION;
    req.hdr.retcode = 0;
    req.hdr.exact = 0;
    mlen = sizeof(MonHdr);
    req.hdr.id = hton16(id++);
    if (!monpkt->sendpkt_suspend(&req, MonReq_Stat, 0, mlen)) {
        display_error("Send failed");
	exit(0);
    }
    if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
        display_error("Receive failed");
	exit(0);
    }

    m = (MonMsg *) mhdr;
    s = &m->body.statrsp;
    in.s_addr = s->router_id;
    addVP(&pairs, "router_id", inet_ntoa(in));
    sprintf(buffer, "%d", ntoh32(s->n_aselsas));
    addVP(&pairs, "n_ases", buffer);
    sprintf(buffer, "0x%x", ntoh32(s->asexsum));
    addVP(&pairs, "ase_xsum", buffer);
    sprintf(buffer, "%d", ntoh32(s->n_ase_import));
    addVP(&pairs, "n_ase_orig", buffer);
    sprintf(buffer, "%d", ntoh32(s->extdb_limit));
    addVP(&pairs, "ase_ceiling", buffer);
    sprintf(buffer, "%d", ntoh32(s->n_dijkstra));
    addVP(&pairs, "n_dijkstra", buffer);
    sprintf(buffer, "%d", ntoh16(s->n_area));
    addVP(&pairs, "n_areas", buffer);
    sprintf(buffer, "%d", ntoh16(s->n_dbx_nbrs));
    addVP(&pairs, "n_exchange", buffer);
    addVP(&pairs, "mospf", yesorno(s->mospf));
    addVP(&pairs, "ia_multicast", yesorno(s->inter_area_mc));
    addVP(&pairs, "as_multicast", yesorno(s->inter_AS_mc));
    addVP(&pairs, "overflow", yesorno(s->overflow_state));
    sprintf(buffer, "%d.%d", s->vmajor, s->vminor);
    addVP(&pairs, "sw_vers", buffer);
    if (print)
	display_html(statistics_page);
}

/* Print out a line for each attached area.
 */

int get_areas(bool print)

{
    MonMsg req;
    int mlen;
    uns32 a_id;
    int i;
    int n_areas=0;

    get_statistics(false);
    if (print)
	display_html(area_header);
    for (a_id = 0, i = 0; ; i++) {
	MonHdr *mhdr;
	MonMsg *m;
	in_addr in;
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
            display_error("Send failed");
	    exit(0);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
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
	sprintf(buffer, "area_id%d", n_areas);
	addVP(&pairs, buffer, inet_ntoa(in));
	n_areas++;
	addVP(&pairs, "area_id", inet_ntoa(in));
	sprintf(buffer, "%d", ntoh16(arearsp->n_ifcs));
	addVP(&pairs, "n_ifcs", buffer);
	sprintf(buffer, "%d", ntoh16(arearsp->n_routers));
	addVP(&pairs, "n_rtrs", buffer);
	n_lsas = ntoh16(arearsp->n_rtrlsas);
	n_lsas += ntoh16(arearsp->n_netlsas);
	n_lsas += ntoh16(arearsp->n_summlsas);
	n_lsas += ntoh16(arearsp->n_asbrlsas);
	n_lsas += ntoh16(arearsp->n_grplsas);
	sprintf(buffer, "%d", n_lsas);
	addVP(&pairs, "n_lsas", buffer);
	sprintf(buffer, "0x%x", ntoh32(arearsp->dbxsum));
	addVP(&pairs, "area_xsum", buffer);
	buffer[0] = '\0';
	if (arearsp->transit)
	    strcat(buffer, "transit ");
	if (arearsp->demand)
	    strcat(buffer, "demand-capable ");
	if (arearsp->stub)
	    strcat(buffer, "stub ");
	if (!arearsp->import_summ)
	    strcat(buffer, "no-import ");
	addVP(&pairs, "comments", buffer);
	if (print)
	    display_html(area_row);
    }
    if (print)
	display_html(area_footer);
    return(n_areas);
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
    ValuePair *entry;

    get_statistics(false);
    if ((entry = (ValuePair *)pairs.find("area_id"))) {
	a_id = ntoh32(inet_addr(entry->value));
	display_html(database_page_top);
    }
    else if (lstype != LST_ASL) {
        if (get_areas(false) > 1) {
	    display_html(select_area_top);
	    for (int i=0; ; i++) {
	        sprintf(buffer, "area_id%d", i);
		if (!(entry = (ValuePair *)pairs.find(buffer)))
		    break;
	        printf("<option>%s\n", entry->value);
	    }
	    display_html(select_area_bottom);
	    return;
	}
	else {
	    entry = (ValuePair *)pairs.find("area_id");
	    a_id = ntoh32(inet_addr(entry->value));
	    display_html(database_page_top);
	}
    }
    else {
	a_id = 0;
	display_html(ase_page_top);
    }

    ls_id = 0;
    adv_rtr = 0;
    new_lstype = lstype;

    while (1) {
	MonHdr *mhdr;
	MonMsg *m;
	LShdr *lshdr;
	in_addr in;
	uns16 type;
	uns16 subtype;
	age_t age;
	char *ptr;

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
            display_error("Send failed");
	    exit(0);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	    break;
	if (a_id != ntoh32(m->body.lsarq.area_id))
	    break;
	new_lstype = ntoh32(m->body.lsarq.ls_type);
	if (new_lstype != lstype) {
	    if (lstype == LST_ASL)
		break;
	    if (new_lstype == LST_ASL) {
	        new_lstype++;
		ls_id = 0;
		adv_rtr = 0;
		continue;
	    }
	}
	ls_id = ntoh32(m->body.lsarq.ls_id);
	adv_rtr = ntoh32(m->body.lsarq.adv_rtr);

	n_lsas++;
	lshdr = (LShdr *) (((char *) m) + mlen);
	xsum += ntoh16(lshdr->ls_xsum);
	sprintf(buffer, "%d", lshdr->ls_type);
	addVP(&pairs, "ls_typeno", buffer);
	// Print out Link state header
	switch (lshdr->ls_type) {
	case LST_RTR:
	    addVP(&pairs, "ls_type", "Router");
	    break;
	case LST_NET:
	    addVP(&pairs, "ls_type", "Network");
	    break;
	case LST_SUMM:
	    addVP(&pairs, "ls_type", "Summary");
	    break;
	case LST_ASBR:
	    addVP(&pairs, "ls_type", "ASBR-Summary");
	    break;
	case LST_ASL:
	    addVP(&pairs, "ls_type", "ASE");
	    break;
	case LST_GM:
	    addVP(&pairs, "ls_type", "Group member");
	    break;
	case LST_LINK_OPQ:
	    addVP(&pairs, "ls_type", "Link Opaque");
	    break;
	case LST_AREA_OPQ:
	    addVP(&pairs, "ls_type", "Area Opaque");
	    break;
	case LST_AS_OPQ:
	    addVP(&pairs, "ls_type", "AS Opaque");
	    break;
	}
	in = *((in_addr *) &lshdr->ls_id);
	addVP(&pairs, "ls_id", inet_ntoa(in));
	in = *((in_addr *) &lshdr->ls_org);
	addVP(&pairs, "adv_rtr", inet_ntoa(in));
	sprintf(buffer, "0x%08x", ntoh32(lshdr->ls_seqno));
	addVP(&pairs, "seqno", buffer);
	sprintf(buffer, "0x%04x", ntoh16(lshdr->ls_xsum));
	addVP(&pairs, "lsa_xsum", buffer);
	sprintf(buffer, "%d", ntoh16(lshdr->ls_length));
	addVP(&pairs, "lsa_len", buffer);
	ptr = buffer;
	age = ntoh16(lshdr->ls_age);
	if ((age & DoNotAge) != 0) {
	    age &= ~DoNotAge;
	    strcpy(buffer, "DNA+");
	    ptr += strlen(buffer);
	}
	sprintf(ptr, "%d", age);
	addVP(&pairs, "ls_age", buffer);
	display_html(database_row);
    }
    sprintf(buffer, "%d", n_lsas);
    addVP(&pairs, "n_lsas", buffer);
    sprintf(buffer, "0x%x", xsum);
    addVP(&pairs, "area_xsum", buffer);
    display_html(database_page_bottom);
}

/* Get the Opaque-LSAs, through the registration interface.
 */

void get_opaques()

{
    MonMsg req;
    int mlen;
    int n_lsas = 0;
    uns32 xsum = 0;

    display_html(opaque_page_top);
    req.hdr.version = OSPF_MON_VERSION;
    req.hdr.retcode = 0;
    req.hdr.exact = 0;
    mlen = sizeof(MonHdr);
    req.hdr.id = hton16(id++);
    if (!monpkt->sendpkt_suspend(&req, MonReq_OpqReg, 0, mlen)) {
        display_error("Send failed");
	exit(0);
    }

    while (1) {
	MonHdr *mhdr;
	MonMsg *m;
	LShdr *lshdr;
	in_addr in;
	uns16 type;
	uns16 subtype;
	age_t age;
	char *ptr;

	req.hdr.version = OSPF_MON_VERSION;
	req.hdr.retcode = 0;
	req.hdr.exact = 0;
	mlen = sizeof(MonHdr);
	req.hdr.id = hton16(id++);
	if (!monpkt->sendpkt_suspend(&req, MonReq_OpqNext, 0, mlen)) {
            display_error("Send failed");
	    exit(0);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	    break;
	addVP(&pairs, "phyint", "n/a");
	addVP(&pairs, "if_addr", "n/a");
	addVP(&pairs, "area_id", "n/a");

	n_lsas++;
	lshdr = (LShdr *) (((char *) m) + sizeof(MonHdr) + sizeof(OpqRsp));
	xsum += ntoh16(lshdr->ls_xsum);
	sprintf(buffer, "%d", lshdr->ls_type);
	addVP(&pairs, "ls_typeno", buffer);
	// Print out Link state header
	switch (lshdr->ls_type) {
	case LST_LINK_OPQ:
	    addVP(&pairs, "ls_type", "Link Opaque");
	    sprintf(buffer, "%d", ntoh32(m->body.opqrsp.phyint));
	    addVP(&pairs, "phyint", buffer);
	    in = *((in_addr *) &m->body.opqrsp.if_addr);
	    addVP(&pairs, "if_addr", inet_ntoa(in));
	    break;
	case LST_AREA_OPQ:
	    addVP(&pairs, "ls_type", "Area Opaque");
	    in = *((in_addr *) &m->body.opqrsp.a_id);
	    addVP(&pairs, "area_id", inet_ntoa(in));
	    break;
	case LST_AS_OPQ:
	    addVP(&pairs, "ls_type", "AS Opaque");
	    break;
	}
	in = *((in_addr *) &lshdr->ls_id);
	addVP(&pairs, "ls_id", inet_ntoa(in));
	in = *((in_addr *) &lshdr->ls_org);
	addVP(&pairs, "adv_rtr", inet_ntoa(in));
	sprintf(buffer, "0x%08x", ntoh32(lshdr->ls_seqno));
	addVP(&pairs, "seqno", buffer);
	sprintf(buffer, "0x%04x", ntoh16(lshdr->ls_xsum));
	addVP(&pairs, "lsa_xsum", buffer);
	sprintf(buffer, "%d", ntoh16(lshdr->ls_length));
	addVP(&pairs, "lsa_len", buffer);
	ptr = buffer;
	age = ntoh16(lshdr->ls_age);
	if ((age & DoNotAge) != 0) {
	    age &= ~DoNotAge;
	    strcpy(buffer, "DNA+");
	    ptr += strlen(buffer);
	}
	sprintf(ptr, "%d", age);
	addVP(&pairs, "ls_age", buffer);
	display_html(opaque_row);
    }
    sprintf(buffer, "%d", n_lsas);
    addVP(&pairs, "n_lsas", buffer);
    sprintf(buffer, "0x%x", xsum);
    addVP(&pairs, "area_xsum", buffer);
    display_html(database_page_bottom);
}

/* Ask the user to select a particular LSA to
 * expand.
 */

/* Get the link-state database of a given area, printing
 * one line for each LSA. If called with LS type of 5
 * dumps AS-external-LSAs instead.
 */

void select_lsa()

{
    ValuePair *entry;

    get_areas(false);
    display_html(select_lsa_top);
    for (int i=0; ; i++) {
	sprintf(buffer, "area_id%d", i);
	if (!(entry = (ValuePair *)pairs.find(buffer)))
	    break;
	printf("<option>%s\n", entry->value);
    }
    display_html(select_lsa_bottom);
}

/* Get a given LSA, and print it out in detail.
 * Area must be specified for all advertisements except
 * the AS-external-LSAs.
 */

void get_lsa()

{
    MonMsg req;
    int mlen;
    uns32 a_id=0;
    uns32 ls_id=0;
    uns32 adv_rtr=0;
    byte lstype=0;
    MonHdr *mhdr;
    MonMsg *m;
    LShdr *lshdr;
    uns16 type;
    uns16 subtype;
    ValuePair *entry;

    if ((entry = (ValuePair *)pairs.find("ls_type")))
        lstype = atoi(entry->value);
    if ((entry = (ValuePair *)pairs.find("ls_id")))
        ls_id = ntoh32(inet_addr(entry->value));
    if ((entry = (ValuePair *)pairs.find("adv_rtr")))
        adv_rtr = ntoh32(inet_addr(entry->value));
    if ((entry = (ValuePair *)pairs.find("area_id")))
	a_id = ntoh32(inet_addr(entry->value));

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
        display_error("Send failed");
	exit(0);
    }

    if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
        display_error("Receive failed");
	exit(0);
    }

    m = (MonMsg *) mhdr;
    if (m->hdr.retcode != 0) {
	printf("LSA not found\n");
	return;
    }
    // Print out LSA
    lshdr = (LShdr *) (((char *) m) + mlen);
    display_html(expand_lsa_top);
    print_lsa(lshdr);
    display_html(expand_lsa_bottom);
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

    get_statistics(false);
    display_html(interface_page_top);
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
            display_error("Send failed");
	    exit(0);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
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
            display_error("Send failed");
	    exit(0);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	taid = ntoh32(m->body.ifcrsp.transit_id);
	endpt = ntoh32(m->body.ifcrsp.endpt_id);
	print_interface(m);
   }
    display_html(interface_page_bottom);
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
    addVP(&pairs, "phyname", ifcrsp->phyname);
    in = *((in_addr *) &ifcrsp->if_addr);
    addVP(&pairs, "if_addr", inet_ntoa(in));
    in = *((in_addr *) &ifcrsp->area_id);
    addVP(&pairs, "if_area", inet_ntoa(in));
    addVP(&pairs, "if_type", ifcrsp->type);
    addVP(&pairs, "if_state", ifcrsp->if_state);
    sprintf(buffer, "%d", ifcrsp->if_nnbrs);
    addVP(&pairs, "if_nnbrs", buffer);
    sprintf(buffer, "%d", ifcrsp->if_nfull);
    addVP(&pairs, "if_nfull", buffer);
    sprintf(buffer, "%d", ntoh16(ifcrsp->if_cost));
    addVP(&pairs, "if_cost", buffer);
    display_html(interface_row);
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

    get_statistics(false);
    display_html(neighbor_page_top);
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
            display_error("Send failed");
	    exit(0);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
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
            display_error("Send failed");
	    exit(0);
	}
	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
            display_error("Receive failed");
	    exit(0);
	}

	m = (MonMsg *) mhdr;
	if (m->hdr.retcode != 0)
	        break;
	taid = ntoh32(m->body.nbrsp.transit_id);
	endpt = ntoh32(m->body.nbrsp.endpt_id);
	print_neighbor(m);
    }
    display_html(neighbor_page_bottom);
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
    addVP(&pairs, "phyname", nbrsp->phyname);
    in = *((in_addr *) &nbrsp->n_addr);
    addVP(&pairs, "n_addr", inet_ntoa(in));
    in = *((in_addr *) &nbrsp->n_id);
    addVP(&pairs, "n_id", inet_ntoa(in));
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, nbrsp->n_state, MON_STATELEN);
    addVP(&pairs, "n_state", buffer);
    sprintf(buffer, "%d", ntoh32(nbrsp->n_ddlst));
    addVP(&pairs, "n_ddlst", buffer);
    sprintf(buffer, "%d", ntoh32(nbrsp->n_rqlst));
    addVP(&pairs, "n_rqlst", buffer);
    sprintf(buffer, "%d", ntoh32(nbrsp->rxmt_count));
    addVP(&pairs, "rxmt_count", buffer);
    display_html(neighbor_row);
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

    get_statistics(false);
    display_html(rttbl_page_top);
    for (net = 0, mask = 0, i = 0; ; i++) {
	MonHdr *mhdr;
	MonMsg *m;
	in_addr in;
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
	    display_error("Send failed");
	    exit(0);
	}

	if (monpkt->rcv_suspend((void **)&mhdr, type, subtype) == -1) {
	    display_error("Receive failed");
	    exit(0);
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
	sprintf(buffer, "%s/%d", inet_ntoa(in), prefix_length);
	addVP(&pairs, "prefix", buffer);
	addVP(&pairs, "rt_type", rtersp->type);
	sprintf(buffer, "%d", ntoh32(rtersp->cost));
	addVP(&pairs, "rt_cost", buffer);
	n_paths = ntoh32(rtersp->npaths);
	sprintf(buffer, "%d", n_paths);
	addVP(&pairs, "rt_paths", buffer);
	if (n_paths == 0) {
	    addVP(&pairs, "rt_ifc", "n/a");
	    addVP(&pairs, "rt_nh", "n/a");
	}
	else {
	    addVP(&pairs, "rt_ifc", rtersp->hops[0].phyname);
	    in = *((in_addr *) &rtersp->hops[0].gw);
	    if (rtersp->hops[0].gw == 0)
	        addVP(&pairs, "rt_nh", "n/a");
	    else
	        addVP(&pairs, "rt_nh", inet_ntoa(in));
	}
	display_html(rttbl_row);
    }
    display_html(rttbl_page_bottom);
}

/* Display HTML, substituting for $variable$
 * in the string argument.
 */

void display_html(const char *s)

{
    const char *ptr;
    const char *next;
    const char *end;
 
    end = s + strlen(s);
    for (ptr = s; (next = strchr(ptr, '$')); ptr = next+1) {
	int size;
	ValuePair *entry;
	size = next - ptr;
	fwrite(ptr, size, 1, stdout);
	ptr = next+1;
	next = strchr(ptr, '$');
	size = next - ptr;
	entry = (ValuePair *) pairs.find((const byte *)ptr, size);
	if (entry)
	    fwrite(entry->value, entry->vlen, 1, stdout);
    }

    if (end > ptr)
        fwrite(ptr, end-ptr, 1, stdout);
}

/* HTML pages.
 */

/* The header that is printed with every
 * page.
 */

const char *page_header = "\
<html>\n\
<head>\n\
<title>OSPFD browser: $cstate$ $command$</title>\n\
<meta name=description content=\"A Web based monitor of the ospfd\n\
routing protocol daemon\">\n\
</head>\n\
\n\
<body bgcolor=white text=black>\n\
\n\
<center>\n\
<table>\n\
<tr>\n\
<td align=\"center\">\n\
\n\
<table>\n\
<tr align=\"center\">\n\
\n\
<!-- Left hand column -->\n\
<td>\n\
<table cellpadding=0 cellspacing=0 border=0 width=130>\n\
<tr bgcolor=\"#efefef\">\n\
<td>\n\
This is the statistics browser for the OSPFD routing daemon.\n\
Definitions of the various OSPF statistics can be\n\
found in the\n\
<a href=\"http://www.ietf.org/rfc/rfc1850.txt\">OSPF MIB.</a>\n\
Descriptions of configurable values can also\n\
be found in Appendix C of\n\
<a href=\"http://www.ietf.org/rfc/rfc2328.txt\">the OSPF specification.</a>\n\
</td>\n\
</tr>\n\
</table>\n\
</td>\n\
\n\
<!-- Right hand column -->\n\
<td>\n\n";

/* This one just asks for address and port to connect
 * to.
 */ 

const char *query_attach = "\
    <table cellpadding=0 cellspacing=0 border=0 width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    Enter the IP address and port to which you wish to connect:\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <tr>\n\
    <td align=\"right\">\n\
    IP Address:\n\
    </td>\n\
    <td align=\"left\">\n\
    <form action=\"/cgi-bin/ospfd_browser\">\n\
    <input name=\"command\" value=\"stat\" type=hidden>\n\
    <input name=\"addr\" value=\"$addr$\" size=16>\n\
    </td>\n\
    </tr>\n\
    <tr>\n\
    <td align=\"right\">\n\
    Port:\n\
    </td>\n\
    <td align=\"left\">\n\
    <input name=\"port\" value=\"$port$\" size=5>\n\
    </td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    <input type=\"submit\">\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </form>\n\
    </table>\n";

/* The statistics page.
 */

const char *statistics_page = "\
<table cellpadding=0 cellspacing=0 border=0>\n\
<tr>\n\
<td>Router ID</td>\n\
<td>$router_id$</td>\n\
</tr>\n\
<tr>\n\
<td># AS-external-LSAs</td>\n\
<td>$n_ases$</td>\n\
</tr>\n\
<tr>\n\
<td>ASE checksum</td>\n\
<td>$ase_xsum$</td>\n\
</tr>\n\
<tr>\n\
<td># ASEs originated</td>\n\
<td>$n_ase_orig$</td>\n\
</tr>\n\
<tr>\n\
<td>ASEs allowed</td>\n\
<td>$ase_ceiling$</td>\n\
</tr>\n\
<tr>\n\
<td># Dijkstras</td>\n\
<td>$n_dijkstra$</td>\n\
</tr>\n\
<tr>\n\
<td># Areas</td>\n\
<td>$n_areas$</td>\n\
</tr>\n\
<tr>\n\
<td># Nbrs in Exchange</td>\n\
<td>$n_exchange$</td>\n\
</tr>\n\
<tr>\n\
<td>MOSPF enabled</td>\n\
<td>$mospf$</td>\n\
</tr>\n\
<tr>\n\
<td>Inter-area multicast</td>\n\
<td>$ia_multicast$</td>\n\
</tr>\n\
<tr>\n\
<td>Inter-AS multicast</td>\n\
<td>$as_multicast$</td>\n\
</tr>\n\
<tr>\n\
<td>In overflow state</td>\n\
<td>$overflow$</td>\n\
</tr>\n\
<tr>\n\
<td>ospfd version</td>\n\
<td>$sw_vers$</td>\n\
</tr>\n\
</table>\n";

/* The areas page.
 */

const char *area_header ="\
<table border=1>\n\
<caption>Router $router_id$'s attached OSPF areas.</caption>\n\
<tr>\n\
<th>Area</th>\n\
<th>#Ifcs</th>\n\
<th>#Routers</th>\n\
<th>#LSAs</th>\n\
<th>Xsum</th>\n\
<th>Comments</th>\n\
</tr>\n";

const char *area_row ="\
<tr>\n\
<td>$area_id$</td>\n\
<td>$n_ifcs$</td>\n\
<td>$n_rtrs$</td>\n\
<td>$n_lsas$</td>\n\
<td>$area_xsum$</td>\n\
<td>$comments$</td>\n\
</tr>\n";

const char *area_footer ="\
</table>\n";

/* The select area page. First print the top, then the
 * list of areas in <opton> tags, and the the
 * bottom.
 */

const char *select_area_top = "\
    <table cellpadding=0 cellspacing=0 border=0 width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    Please select the area whose link-state database\n\
    you want displayed.\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <tr>\n\
    <td align=\"right\">\n\
    Area ID:\n\
    </td>\n\
    <td align=\"left\">\n\
    <form action=\"/cgi-bin/ospfd_browser\">\n\
    <input name=\"command\" value=\"database\" type=hidden>\n\
    <input name=\"addr\" value=\"$addr$\" type=hidden>\n\
    <input name=\"port\" value=\"$port$\" type=hidden>\n\
    <select name=\"area_id\">\n";

const char *select_area_bottom ="\
    </select>\n\
    </td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    <input type=\"submit\">\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </form>\n\
    </table>\n\
  </td>\n\
  </tr>\n\
  </table>\n";

/* The pages used to display an area's link-state database
 */

const char *database_page_top = "\
<table>\n\
<tr>\n\
<td width=200>\n\
Router $router_id$'s link-state database for OSPF\n\
Area $area_id$. AS-external-LSAs are not included.\n\
</td>\n\
</tr>\n\
<tr>\n\
<td>\n\
</center>\n\
<table border=1>\n\
<tr>\n\
<th>LSA Type</th>\n\
<th>LS ID</th>\n\
<th>Adv. Rtr.</th>\n\
<th>LS Seqno</th>\n\
<th>Xsum</th>\n\
<th>Length</th>\n\
<th>Age</th>\n\
</tr>\n";

const char *ase_page_top = "\
<table>\n\
<tr>\n\
<td width=200>\n\
Router $router_id$'s database of AS-external-LSAs.\n\
</td>\n\
</tr>\n\
<tr>\n\
<td>\n\
</center>\n\
<table border=1>\n\
<tr>\n\
<th>LSA Type</th>\n\
<th>LS ID</th>\n\
<th>Adv. Rtr.</th>\n\
<th>LS Seqno</th>\n\
<th>Xsum</th>\n\
<th>length</th>\n\
<th>Age</th>\n\
</tr>\n";

const char *database_row = "\
<tr>\n\
<td><a href=\"/cgi-bin/ospfd_browser?command=adv&addr=$addr$&port=$port$&area_id=$area_id$&ls_type=$ls_typeno$&ls_id=$ls_id$&adv_rtr=$adv_rtr$\">$ls_type$</a></td>\n\
<td>$ls_id$</td>\n\
<td>$adv_rtr$</td>\n\
<td>$seqno$</td>\n\
<td>$lsa_xsum$</td>\n\
<td>$lsa_len$</td>\n\
<td>$ls_age$</td>\n\
</tr>\n";

const char *database_page_bottom = "\
</table>\n\
</td>\n\
</tr>\n\
<tr>\n\
<td>\n\
<table border=0>\n\
<tr>\n\
<td><b>#LSAs:</b></td>\n\
<td>$n_lsas$</td>\n\
</tr>\n\
<tr>\n\
<td><b>Xsum:</b></td>\n\
<td>$area_xsum$</td>\n\
</tr>\n\
</table>\n\
</table>\n";

/* The pages used to display the Opaque-LSAs
 */

const char *opaque_page_top = "\
<table>\n\
<tr>\n\
<td width=200>\n\
Router $router_id$'s link-state database for OSPF\n\
Area $area_id$. AS-external-LSAs are not included.\n\
</td>\n\
</tr>\n\
<tr>\n\
<td>\n\
</center>\n\
<table border=1>\n\
<tr>\n\
<th>Phyint</th>\n\
<th>If Address</th>\n\
<th>Area</th>\n\
<th>LS type</th>\n\
<th>LS ID</th>\n\
<th>Adv. Rtr.</th>\n\
<th>LS Seqno</th>\n\
<th>Xsum</th>\n\
<th>Length</th>\n\
<th>Age</th>\n\
</tr>\n";

const char *opaque_row = "\
<tr>\n\
<td>$phyint$</a></td>\n\
<td>$if_addr$</a></td>\n\
<td>$area_id$</a></td>\n\
<td>$ls_type$</a></td>\n\
<td>$ls_id$</td>\n\
<td>$adv_rtr$</td>\n\
<td>$seqno$</td>\n\
<td>$lsa_xsum$</td>\n\
<td>$lsa_len$</td>\n\
<td>$ls_age$</td>\n\
</tr>\n";

/* Pages used to select a given LSA for display.
 * Top is followed by "<option>area" fields,
 * and then the bottom.
 */

const char *select_lsa_top = "\
    <table cellpadding=0 cellspacing=0 border=0 width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    Please select the link-state advertisement that\n\
    you want displayed.\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <form action=\"/cgi-bin/ospfd_browser\">\n\
    <input name=\"command\" value=\"adv\" type=hidden>\n\
    <input name=\"addr\" value=\"$addr$\" type=hidden>\n\
    <input name=\"port\" value=\"$port$\" type=hidden>\n\
    <tr>\n\
    <td align=right>Area ID:</td>\n\
    <td>\n\
    <select name=\"area_id\">\n";

const char *select_lsa_bottom = "\
    </select>\n\
    </td>\n\
    </tr>\n\
    <tr>\n\
    <td align=right>LS Type:</td>\n\
    <td>\n\
    <select name=\"ls_type\">\n\
    <option value=\"1\">router-LSA\n\
    <option value=\"2\">network-LSA\n\
    <option value=\"3\">summary-LSA\n\
    <option value=\"4\">ASBR-summary-LSA\n\
    <option value=\"5\">AS-external-LSA\n\
    <option value=\"6\">group-membership-LSA\n\
    </select>\n\
    </td>\n\
    </tr>\n\
    <tr>\n\
    <td align=right>LS ID:</td>\n\
    <td><input name=\"ls_id\" size=16></td>\n\
    </tr>\n\
    <tr>\n\
    <td align=right>Adv Rtr:</td>\n\
    <td><input name=\"adv_rtr\" size=16></td>\n\
    </tr>\n\
    </table>\n\
    <table width=400>\n\
    <tr>\n\
    <td>\n\
    <center>\n\
    <input type=\"submit\">\n\
    </center>\n\
    </td>\n\
    </tr>\n\
    </form>\n\
    </table>\n";

/* Pages used to expand a given LSA.
 */

const char *expand_lsa_top = "\
<table cellpadding=0 border=0>\n\
<tr>\n\
<td>\n\
The following LSA was found in the Area\n\
$area_id$ link-state database:\n\
<hr>\n\
<pre>\n";

const char *expand_lsa_bottom = "\
</pre>\n\
</td>\n\
</tr>\n\
</table>\n";

/* Pages used to display the list of interfaces.
 */

const char *interface_page_top = "\
<table border=1>\n\
<caption>Router $router_id$'s OSPF interfaces.</caption>\n\
<tr>\n\
<th>Phy</th>\n\
<th>Addr</th>\n\
<th>Area</th>\n\
<th>Type</th>\n\
<th>State</th>\n\
<th>#Nbr</th>\n\
<th>#Adj</th>\n\
<th>Cost</th>\n\
</tr>\n";

const char *interface_row = "\
<tr>\n\
<td>$phyname$</td>\n\
<td>$if_addr$</td>\n\
<td>$if_area$</td>\n\
<td>$if_type$</td>\n\
<td>$if_state$</td>\n\
<td>$if_nnbrs$</td>\n\
<td>$if_nfull$</td>\n\
<td>$if_cost$</td>\n\
</tr>\n";

const char *interface_page_bottom = "\
</table>\n";

/* Pages used to display the list of neighbors.
 */

const char *neighbor_page_top = "\
<table border=1>\n\
<caption>Router $router_id$'s OSPF neighbors.</caption>\n\
<tr>\n\
<th>Phy</th>\n\
<th>Addr</th>\n\
<th>ID</th>\n\
<th>State</th>\n\
<th>#DD</th>\n\
<th>#Req</th>\n\
<th>#Rxmt</th>\n\
</tr>\n";

const char *neighbor_row = "\
<tr>\n\
<td>$phyname$</td>\n\
<td>$n_addr$</td>\n\
<td>$n_id$</td>\n\
<td>$n_state$</td>\n\
<td>$n_ddlst$</td>\n\
<td>$n_rqlst$</td>\n\
<td>$rxmt_count$</td>\n\
</tr>\n";

const char *neighbor_page_bottom = "\
</table>\n";

/* Pages used to display the routing table.
 */

const char *rttbl_page_top = "\
<table border=1>\n\
<caption>Router $router_id$'s OSPF routing table.\n\
This is the OSPF protocol's internal routing table,\n\
and does not necessarily match the routing\n\
table in the kernel.</caption>\n\
<tr>\n\
<th>Prefix</th>\n\
<th>Type</th>\n\
<th>Cost</th>\n\
<th>Ifc</th>\n\
<th>Next-hop</th>\n\
<th>#Paths</th>\n\
</tr>\n";

const char *rttbl_row = "\
<tr>\n\
<td>$prefix$</td>\n\
<td>$rt_type$</td>\n\
<td>$rt_cost$</td>\n\
<td>$rt_ifc$</td>\n\
<td>$rt_nh$</td>\n\
<td>$rt_paths$</td>\n\
</tr>\n";

const char *rttbl_page_bottom = "\
</table>\n";

/* The footer that is printed with every page.
 */

const char *page_footer = "\
</td>\n\
</tr>\n\
</table>\n\
\n\
<!-- Bottom of the page -->\n\
<tr>\n\
<td>\n\
\n\
<center>\n\
<table cellpadding=0 cellspacing=0 border=0>\n\
<tr>\n\
<td width=420 valign=top>\n\
<font face=\"Geneva, Arial, Helvetica\" >\n\
<p><center>\n\
<a href=\"/cgi-bin/ospfd_browser?command=area&addr=$addr$&port=$port$\">Areas</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=database&addr=$addr$&port=$port$\">Database</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=interface&addr=$addr$&port=$port$\">Interfaces</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=neighbor&addr=$addr$&port=$port$\">Neighbors</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=statistics&addr=$addr$&port=$port$\">Statistics</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=lsa&addr=$addr$&port=$port$\">LSA expansion</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=ases&addr=$addr$&port=$port$\">AS externals</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=route&addr=$addr$&port=$port$\">Routing table</a> |\n\
<a href=\"/cgi-bin/ospfd_browser?command=opqs&addr=$addr$&port=$port$\">Opaque-LSAs</a>\n\
</center>\n\
</td>\n\
</tr>\n\
</table>\n\
</center>\n\
\n\
</td>\n\
</tr>\n\
</table>\n\
</center>\n\
\n\
</body>\n\
</html>";

/* The error page.
 */ 

const char *error_page = "\
    <table cellpadding=0 cellspacing=0 border=0 width=400>\n\
    <center>
    <tr>\n\
    <td align=\"center\">\n\
    Data connection error:\n\
    <br>
    $operation$: $errno$\n\
    </td>\n\
    </tr>\n\
    </center>\n\
    </table>\n";

/* Display the error page, converting errno to
 * a string first.
 */

void display_error(const char *operation)

{
    if (!header_printed) {
        display_html(page_header);
	header_printed = true;
    }
    addVP(&pairs, "operation", operation);
    addVP(&pairs, "errno", strerror(errno));
    display_html(error_page);
    display_html(page_footer);
}

/* Non-zero values signal enabled.
 */

const char *yesorno(byte val)

{
    return((val != 0) ? "yes" : "no");
}

