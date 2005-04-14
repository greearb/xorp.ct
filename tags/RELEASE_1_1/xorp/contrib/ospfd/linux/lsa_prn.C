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
#include "machdep.h"
#include "spftype.h"
#include "ip.h"
#include "arch.h"
#include "lshdr.h"


/* Routines used to print out the contents
 * of LSAs
 */

void print_lsa(LShdr *hdr)

{
    byte *body;
    int	len;
    in_addr in;
    age_t age;
    bool dna;

    age = ntoh16(hdr->ls_age);
    if ((dna = ((age & DoNotAge) != 0)))
	age &= ~DoNotAge;
    printf("\tLS age:\t\t%s%d\n", (dna ? "DNA+" : ""), age);
    printf("\tLS Options:\t0x%x\n", (int) hdr->ls_opts);
    printf("\tLS Type:\t%d\n", (int) hdr->ls_type);
    in = *((in_addr *) &hdr->ls_id); 
    printf("\tLink State ID:\t%s\n", inet_ntoa(in));
    in = *((in_addr *) &hdr->ls_org);
    printf("\tAdvert. Rtr.:\t%s\n", inet_ntoa(in));
    printf("\tLS Seqno:\t0x%x\n", ntoh32(hdr->ls_seqno));
    printf("\tLS Xsum:\t0x%x\n", ntoh16(hdr->ls_xsum));
    printf("\tLS Length:\t%d\n", ntoh16(hdr->ls_length));

    body = (byte *) (hdr+1);
    len = ntoh16(hdr->ls_length);
    len -= sizeof(LShdr);

    switch (hdr->ls_type) {
	void print_rtr_lsa(byte *, int);
	void print_net_lsa(byte *, int);
	void print_summ_lsa(byte *, int);
	void print_asbr_lsa(byte *, int);
	void print_asex_lsa(byte *, int);
	void print_gm_lsa(byte *, int);
	void print_opq_lsa(byte *, int);
      case LST_RTR:
	print_rtr_lsa(body, len);
	break;
      case LST_NET:
	print_net_lsa(body, len);
	break;
      case LST_SUMM:
	print_summ_lsa(body, len);
	break;
      case LST_ASBR:
	print_asbr_lsa(body, len);
	break;
      case LST_ASL:
	print_asex_lsa(body, len);
	break;
      case LST_GM:
	print_gm_lsa(body, len);
	break;
      case LST_LINK_OPQ:
      case LST_AREA_OPQ:
      case LST_AS_OPQ:
	print_opq_lsa(body, len);
	break;
      default:
	break;
    }
}


/* Print out the body of a router-LSA.
 */

void print_rtr_lsa(byte *body, int len)

{
    RTRhdr *hdr;
    int	i, nl;
    RtrLink *link;
    byte *end;

    hdr = (RTRhdr *) body;
    nl = ntoh16(hdr->nlinks);
    end = body + len;

    printf("\t\t// Router-LSA body\n");
    printf("\t\tRouter type:\t0x%x\n", (int) hdr->rtype);
    printf("\t\t# links:\t%d\n", nl);
    link = (RtrLink *) (hdr+1);
    for (i = 0; i < nl; i++) {
	int n_tos;
	TOSmetric *mp;
	in_addr in;
	if (((byte *) (link+1)) > end)
	    break;
	n_tos = link->n_tos;
	printf("\t\t// Link #%d\n", i);
	in = *((in_addr *) &link->link_id);
	printf("\t\tLink ID:\t%s\n", inet_ntoa(in));
	in = *((in_addr *) &link->link_data);
	printf("\t\tLink Data:\t%s\n", inet_ntoa(in));
	printf("\t\tLink type:\t%d\n", (int) link->link_type);
	printf("\t\t# TOS metrics:\t%d\n", n_tos);
	printf("\t\tLink cost:\t%d\n", ntoh16(link->metric));
	mp = (TOSmetric *) (link+1);
	for (; n_tos; n_tos--, mp++) {
	    uns32 cost;
	    if (((byte *) (mp+1)) > end)
	        break;
	    printf("\t\t\tTOS%d metric:\t", mp->tos);
	    cost = mp->hibyte << 16;
	    cost += ntoh16(mp->metric);
	    printf("%d\n", cost);
	}
	link = (RtrLink *) mp;
    }
}

/* Print out the body of a network-LSA.
 */

void print_net_lsa(byte *body, int len)

{
    NetLShdr *hdr;
    byte *end;
    lsid_t *rtr;
    int	i;
    in_addr in;

    hdr = (NetLShdr *) body;
    end = body + len;
    rtr = (lsid_t *) (hdr+1);

    printf("\t\t// Network-LSA body\n");
    in = *((in_addr *) &hdr->netmask);
    printf("\t\tNetwork Mask:\t%s\n", inet_ntoa(in));
    for (i=0; (byte *) rtr < end; i++, rtr++) {
	if (((byte *) (rtr+1)) > end)
	    break;
	in = *((in_addr *) rtr);
	printf("\t\tAttached router #%d: %s\n", i, inet_ntoa(in));
    }
}

/* Print out the body of a summary-LSA.
 */

void print_summ_lsa(byte *body, int len)

{
    byte *end;
    SummHdr *summ;
    in_addr in;
    TOSmetric *mp;

    summ = (SummHdr *) body;
    end = body + len;

    printf("\t\t// Summary-LSA body\n");
    in = *((in_addr *) &summ->mask);
    printf("\t\tNetwork Mask:\t%s\n", inet_ntoa(in));
    printf("\t\tCost:\t\t%d\n", ntoh32(summ->metric));
    mp = (TOSmetric *) (summ+1);
    for (; ((byte *) mp) < end; mp++) {
	uns32 cost;
	if (((byte *) (mp+1)) > end)
	    break;
	printf("\t\t\tTOS%d metric:\t", mp->tos);
	cost = mp->hibyte << 16;
	cost += ntoh16(mp->metric);
	printf("%d\n", cost);
    }
}

/* Print out the body of an ASBR-summary-LSA.
 */

void print_asbr_lsa(byte *body, int len)

{
    byte *end;
    SummHdr *summ;
    TOSmetric *mp;

    summ = (SummHdr *) body;
    end = body + len;

    printf("\t\t// ASBR-summary-LSA body\n");
    printf("\t\tCost:\t\t%d\n", ntoh32(summ->metric));
    mp = (TOSmetric *) (summ+1);
    for (; ((byte *) mp) < end; mp++) {
	uns32 cost;
	printf("\t\t\tTOS%d metric:\t", mp->tos);
	cost = mp->hibyte << 16;
	cost += ntoh16(mp->metric);
	printf("%d\n", cost);
    }
}

/* Print out the body of an AS-external-LSA.
 */

void print_asex_lsa(byte *body, int len)

{
    byte *end;
    ASEhdr *ase;
    in_addr in;
    uns32 cost;
    ASEmetric *mp;

    ase = (ASEhdr *) body;
    end = body + len;

    printf("\t\t// AS-external-LSA body\n");
    in = *((in_addr *) &ase->mask);
    printf("\t\tNetwork Mask:\t%s\n", inet_ntoa(in));
    cost = ase->hibyte << 16;
    cost += ntoh16(ase->metric);
    printf("\t\tCost:\t\t%d\n", cost);
    in = *((in_addr *) &ase->faddr);
    printf("\t\tForwarding address: %s\n", inet_ntoa(in));
    printf("\t\tRoute tag:\t0x%x\n", ntoh32(ase->rtag));
    mp = (ASEmetric *) (ase+1);
    for (; ((byte *) mp) < end; mp++) {
	if (((byte *) (mp+1)) > end)
	    break;
	printf("\t\t\tTOS%d metric:\t", mp->tos);
	cost = mp->hibyte << 16;
	cost += ntoh16(mp->metric);
	printf("%d\n", cost);
	in = *((in_addr *) &mp->faddr);
	printf("\t\t\tForwarding address: %s\n", inet_ntoa(in));
	printf("\t\t\tRoute tag:\t0x%x\n", ntoh32(mp->rtag));
    }
}

/* Print out the body of a group-membership-LSA.
 */

void print_gm_lsa(byte *body, int len)

{
    GMref *ref;
    byte *end;
    int	i;

    ref = (GMref *) body;
    end = body + len;

    printf("\t\t// Group-membership-LSA body\n");
    for (i=0; (byte *) ref < end; i++, ref++) {
	in_addr in;
	if (((byte *) (ref+1)) > end)
	    break;
	printf("\t\t// Reference #%d\n", i);
	printf("\t\tLS type:\t%d\n", ntoh32(ref->ls_type));
	in = *((in_addr *) &ref->ls_id);
	printf("\t\tLink State ID:\t%s\n", inet_ntoa(in));
    }
}

/* Print out the body of an opaque-LSA.
 */

void print_opq_lsa(byte *body, int len)

{
    byte *end;
    int	i;

    end = body + len;

    printf("\t// Opaque-LSA body\n\t");
    for (i=0; i < len; i++) {
	if (i != 0 && (i % 16) == 0)
	    printf("\r\n\t");
	printf("%02x ", body[i]);
    }
}
