/*
 *   OSPFD routing daemon
 *   Copyright (C) 1999 by John T. Moy
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

/* IGMP runs on a per-physical interface. For that
 * reason we include the IGMP timer classes
 * here.
 */

class StartupQueryTimer : public ITimer {
    class PhyInt *phyp;
    int startup_queries;// Initial set of queries remaining
  public:
    StartupQueryTimer(class PhyInt *);
    virtual void action();
    friend class PhyInt;
};

class IGMPQueryTimer : public ITimer {
    class PhyInt *phyp;
  public:
    IGMPQueryTimer(class PhyInt *);
    virtual void action();
};

class IGMPOtherQuerierTimer : public Timer {
    class PhyInt *phyp;
  public:
    IGMPOtherQuerierTimer(class PhyInt *);
    virtual void action();
};

/* Definitions for physical interfaces.
 * There may be one or more OSPF interfaces defined
 * over each physical interface; OSPF interfaces
 * are defined in spfifc.h.
 */

class PhyInt : public AVLitem {
    int ipmult;		// Whether IP multicast enabled
    bool operational;
    InAddr my_addr;	// Associated IP address
    SpfIfc *mospf_ifp;	// Associated MOSPF-enabled interface
    // IGMP parameters
    bool igmp_enabled;
    InAddr igmp_querier;// Current IGMP querier
    IGMPQueryTimer qrytim;
    StartupQueryTimer strqtim;
    IGMPOtherQuerierTimer oqtim;
    // IGMP configurable parameters
    int robustness_variable;
    int query_interval;
    int query_response_interval;// Tenths of seconds
    int group_membership_interval;
    int other_querier_present_interval;
    int startup_query_interval;
    int startup_query_count;
    int last_member_query_interval;// Tenths of seconds
    int last_member_query_count;
    // Dynamic data
    bool cached;	// Already installed in multicast entry?
  public:
    PhyInt(int phyint);
    inline bool IAmQuerier();
    void verify_igmp_capabilities();
    void start_query_duties();
    void stop_query_duties();
    void send_query(InAddr group);
    void received_igmp_query(InAddr src, InAddr group, byte max_response);
    void received_igmp_report(InAddr group, bool v1);
    void received_igmp_leave(InAddr group);
    friend class OSPF;
    friend class StartupQueryTimer;
    friend class IGMPOtherQuerierTimer;
    friend class GroupTimeoutTimer;
    friend class LeaveQueryTimer;
    friend class SpfArea;
    friend class SpfIfc;
};

// Are we the IGMP querier?
inline bool PhyInt::IAmQuerier()

{
    return(my_addr && mospf_ifp && (my_addr == igmp_querier));
}

/* Definitions for group membership entries.
 * Individual entries represent members of a specific
 * group on a specific physical interface.
 * Group specific timers are also included here.
 */

class LeaveQueryTimer : public ITimer {
    class GroupMembership *gentry;
    int queries_remaining;// Queries remaining
  public:
    LeaveQueryTimer(class GroupMembership *);
    friend class PhyInt;
    virtual void action();
};

class GroupTimeoutTimer : public Timer {
    class GroupMembership *gentry;
  public:
    GroupTimeoutTimer(class GroupMembership *);
    virtual void action();
};

class V1MemberTimer : public Timer {
    class GroupMembership *gentry;
  public:
    V1MemberTimer(class GroupMembership *);
    virtual void action();
};

/* Actual representation of a group membership.
 * All group members stored in a single AVL tree,
 * which can be indexed first by group, and then
 * by physical interface.
 * Group members associated with the router as a whole
 * (interface-less) are stored with phyint == -1.
 */

class GroupMembership : public AVLitem {
    LeaveQueryTimer leave_tim;
    GroupTimeoutTimer exp_tim;
    V1MemberTimer v1_tim;
    bool v1members;
  public:
    GroupMembership(InAddr group, int phyint);
    friend class PhyInt;
    friend class GroupTimeoutTimer;
    friend class LeaveQueryTimer;
    friend class V1MemberTimer;
};

const InAddr IGMPAllSystems = 0xe0000001;
const InAddr IGMPAllRouters = 0xe0000002;
