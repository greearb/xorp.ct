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

#include "mcache.h"

/* Class implementing system functions, such as time of day.
 */

class OspfSysCalls {
public:
    InPkt *getpkt(uns16 len);
    void freepkt(InPkt *pkt);
    OspfSysCalls();
    virtual ~OspfSysCalls();

    virtual void sendpkt(InPkt *pkt, int phyint, InAddr gw=0)=0;
    virtual void sendpkt(InPkt *pkt)=0;
    virtual bool phy_operational(int phyint)=0;
    virtual void phy_open(int phyint)=0;
    virtual void phy_close(int phyint)=0;
    virtual void join(InAddr group, int phyint)=0;
    virtual void leave(InAddr group, int phyint)=0;
    virtual void ip_forward(bool enabled)=0;
    virtual void set_multicast_routing(bool on)=0;
    virtual void set_multicast_routing(int phyint, bool on)=0;
    virtual void rtadd(InAddr, InMask, MPath *, MPath *, bool)=0; 
    virtual void rtdel(InAddr, InMask, MPath *ompp)=0;
    virtual void add_mcache(InAddr, InAddr, MCache *)=0;
    virtual void del_mcache(InAddr src, InAddr group)=0;
    virtual void upload_remnants()=0;
    virtual void monitor_response(struct MonMsg *, uns16, int, int)=0;
    virtual char *phyname(int phyint)=0;
    virtual void sys_spflog(int msgno, char *msgbuf)=0;
    virtual void store_hitless_parms(int, int, struct MD5Seq *) = 0;
    virtual void halt(int code, const char *string)=0;
};

/* Class used to indicate MD5 sequence numbers in use
 * on a particular interface.
 */

struct MD5Seq {
    int phyint;
    InAddr if_addr;
    uns32 seqno;
};

extern OspfSysCalls *sys;
extern SPFtime sys_etime;


