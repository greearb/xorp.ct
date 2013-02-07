/*
 * XORP Wrapper Client (LDWrapper)
 *
 * Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
 *                    Pascal Charest <pascal.charest@crc.gc.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation or - at your option - under
 * the terms of the GNU General Public Licence version 2 but can be
 * linked to any BSD-Licenced Software with public available sourcecode
 *
 */


#ifndef _LD_WRAPPER_H
#define _LD_WRAPPER_H

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stddef.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <sys/time.h>

#include <execinfo.h>
#include <queue>
#include <sys/ioctl.h>
#include <string>


#define MAX_SOCKET_ENTRY  40


#ifndef NDEBUG
#   define DEBUG a
#   define PDEBUG(fmt, args...) { \
    timeval time; \
    gettimeofday(&time, NULL); \
    fprintf(stderr, "%lu.%06lu DEBUG: " fmt, time.tv_sec, time.tv_usec, ## args); \
}
#else
#   define DEBUG
#   define PDEBUG(fmt, args...)
#endif
/*
#   define DEBUG
#   define PDEBUG(fmt, args...)
*/

struct def_record {
    const int id;
    const char *name;
};

#define DEF_START(a) const def_record a[] = {
#define DEF_ADD(a)  { a, #a },
#define DEF_END() { 0, NULL } };

const char *def_itoa(const def_record *records, int id) {
    while (records->name != NULL) {
        if (records->id == id) {
            return records->name;
        }
        ++records;
    }
    return "unknown";
}
DEF_START(SIOC)
    DEF_ADD(SIOCADDRT)
    DEF_ADD(SIOCADDRT)
    DEF_ADD(SIOCDELRT)
    DEF_ADD(SIOCGIFNAME)
    DEF_ADD(SIOCSIFLINK)
    DEF_ADD(SIOCGIFCONF)
    DEF_ADD(SIOCGIFFLAGS)
    DEF_ADD(SIOCSIFFLAGS)
    DEF_ADD(SIOCGIFADDR)
    DEF_ADD(SIOCSIFADDR)
    DEF_ADD(SIOCGIFDSTADDR)
    DEF_ADD(SIOCSIFDSTADDR)
    DEF_ADD(SIOCGIFBRDADDR)
    DEF_ADD(SIOCSIFBRDADDR)
    DEF_ADD(SIOCGIFNETMASK)
    DEF_ADD(SIOCSIFNETMASK)
    DEF_ADD(SIOCGIFMTU)
DEF_END();
DEF_START(SO)
    DEF_ADD(SO_DEBUG)
    DEF_ADD(SO_REUSEADDR)
    DEF_ADD(SO_TYPE)
    DEF_ADD(SO_ERROR)
    DEF_ADD(SO_DONTROUTE)
    DEF_ADD(SO_BROADCAST)
    DEF_ADD(SO_SNDBUF)
    DEF_ADD(SO_RCVBUF)
    DEF_ADD(SO_SNDBUFFORCE)
    DEF_ADD(SO_RCVBUFFORCE)
    DEF_ADD(SO_KEEPALIVE)
    DEF_ADD(SO_OOBINLINE)
    DEF_ADD(SO_NO_CHECK)
    DEF_ADD(SO_PRIORITY)
    DEF_ADD(SO_LINGER)
    DEF_ADD(SO_BSDCOMPAT)
#ifndef SO_REUSEPORT
#   define SO_REUSEPORT SO_BSDCOMPAT + 1
#endif
    DEF_ADD(SO_REUSEPORT)
DEF_END();

DEF_START(AF)
    DEF_ADD(AF_UNIX)
    DEF_ADD(AF_INET)
    DEF_ADD(AF_INET6)
    DEF_ADD(AF_IPX)
    DEF_ADD(AF_NETLINK)
    DEF_ADD(AF_X25)
    DEF_ADD(AF_AX25)
    DEF_ADD(AF_ATMPVC)
    DEF_ADD(AF_APPLETALK)
    DEF_ADD(AF_PACKET)
DEF_END();


DEF_START(SOCK)
    DEF_ADD(SOCK_STREAM)
    DEF_ADD(SOCK_DGRAM)
    DEF_ADD(SOCK_SEQPACKET)
    DEF_ADD(SOCK_RAW)
    DEF_ADD(SOCK_RDM)
    DEF_ADD(SOCK_PACKET)
DEF_END();

DEF_START(RTM)
    DEF_ADD(RTM_NEWLINK)
    DEF_ADD(RTM_DELLINK)
    DEF_ADD(RTM_GETLINK)
    DEF_ADD(RTM_NEWADDR)
    DEF_ADD(RTM_DELADDR)
    DEF_ADD(RTM_GETADDR)
    DEF_ADD(RTM_NEWROUTE)
    DEF_ADD(RTM_DELROUTE)
    DEF_ADD(RTM_GETROUTE)
    DEF_ADD(RTM_NEWNEIGH)
    DEF_ADD(RTM_DELNEIGH)
    DEF_ADD(RTM_GETNEIGH)
    DEF_ADD(RTM_NEWRULE)
    DEF_ADD(RTM_DELRULE)
    DEF_ADD(RTM_GETRULE)
    DEF_ADD(RTM_NEWQDISC)
    DEF_ADD(RTM_DELQDISC)
    DEF_ADD(RTM_GETQDISC)
    DEF_ADD(RTM_NEWTCLASS)
    DEF_ADD(RTM_DELTCLASS)
    DEF_ADD(RTM_GETTCLASS)
    DEF_ADD(RTM_NEWTFILTER)
    DEF_ADD(RTM_DELTFILTER)
    DEF_ADD(RTM_GETTFILTER)
DEF_END();

DEF_START(RTA)
    DEF_ADD(RTA_UNSPEC)
    DEF_ADD(RTA_DST)
    DEF_ADD(RTA_SRC)
    DEF_ADD(RTA_IIF)
    DEF_ADD(RTA_OIF)
    DEF_ADD(RTA_GATEWAY)
    DEF_ADD(RTA_PRIORITY)
    DEF_ADD(RTA_PREFSRC)
    DEF_ADD(RTA_METRICS)
    DEF_ADD(RTA_MULTIPATH)
    DEF_ADD(RTA_PROTOINFO)
    DEF_ADD(RTA_FLOW)
    DEF_ADD(RTA_CACHEINFO)
DEF_END();

DEF_START(RT_TABLE)
    DEF_ADD(RT_TABLE_UNSPEC)
    DEF_ADD(RT_TABLE_COMPAT)
    DEF_ADD(RT_TABLE_DEFAULT)
    DEF_ADD(RT_TABLE_MAIN)
    DEF_ADD(RT_TABLE_LOCAL)
DEF_END();


DEF_START(RT_SCOPE)
    DEF_ADD(RT_SCOPE_UNIVERSE)
    DEF_ADD(RT_SCOPE_SITE)
    DEF_ADD(RT_SCOPE_LINK)
    DEF_ADD(RT_SCOPE_HOST)
    DEF_ADD(RT_SCOPE_NOWHERE)
DEF_END();


DEF_START(RTN)
    DEF_ADD(RTN_UNICAST)
    DEF_ADD(RTN_LOCAL)
    DEF_ADD(RTN_BROADCAST)
    DEF_ADD(RTN_ANYCAST)
    DEF_ADD(RTN_MULTICAST)
    DEF_ADD(RTN_BLACKHOLE)
    DEF_ADD(RTN_UNREACHABLE)
    DEF_ADD(RTN_PROHIBIT)
    DEF_ADD(RTN_THROW)
    DEF_ADD(RTN_NAT)
    DEF_ADD(RTN_XRESOLVE)
DEF_END();

#include "wrapper_types.h"

struct portmapping {
   int used_port;
   int mapped_port;
};

struct rxData {
    std::string      src_add;
    uint32_t    src_port;
    std::string      remote_add;
    uint32_t    remote_port;
    std::vector<uint8_t> data;
};

struct tcp_udp_socket {
    bool        _used;
    int         sockId;
    bool	reuse;
    std::string	bTOd;
    std::string sock;
    int         type;
    int         domain;
    int         setup_state;
    int         protocol;
    bool        pair_socket;
    std::queue<rxData> q;
};

enum sockType
{
    TCP,
    UDP
};

enum status {
   INIT  = 1,
   ERROR = -1,
   OK = 0,
   } ;

enum sock_state {

   NO_STATE,
   CREATE,
   BIND,
   CONNECT,
   ACCEPT,
   LISTEN,
   CLOSE
   };

tcp_udp_socket   sockIf[MAX_SOCKET_ENTRY];

int number_socket_entry = 0;

portmapping wrapperports[MAX_SOCKET_ENTRY];
int number_port_entry = 0;

// ---------------------------------------------------------------------------
//
//
void initSock ()
{
    int i;
    for (i = 0; i < MAX_SOCKET_ENTRY; i ++) {
        sockIf[i]._used = false;
    }
}

// ---------------------------------------------------------------------------
//
//
int checkSock (int sockId)
{
    int i;
    for (i = 0; i < MAX_SOCKET_ENTRY; i ++) {
        if (sockIf[i]._used && sockId == sockIf[i].sockId) {// found
            return i;
        }
    }
    return -1;
}

int checkByName (std::string  sock)
{
    int i;
    for (i = 0; i < MAX_SOCKET_ENTRY; i ++) {
        if (sockIf[i]._used && sock == sockIf[i].sock) {// found
            return i;
        }
    }
    return -1;
}

int creatSock(int domain, int type, int protocol)
{
    int i;
    for (i = 0; i < MAX_SOCKET_ENTRY; i ++) {
        if (!(sockIf[i]._used)) {// found
            sockIf[i].domain = domain;
            sockIf[i].type   = type ;
            sockIf[i].protocol = protocol ;
            sockIf[i].setup_state = CREATE;
            sockIf[i].sock = "N/A";
	    sockIf[i].bTOd = "N/A";
            sockIf[i]._used = true;
            PDEBUG(">>> createSock ret: %i\n",i);
            return i;
        }
    }
    fprintf(stderr,"MAX_SOCKET_ENTRY reached\n");
    return -1;
}


#ifdef __cplusplus
extern "C" {
#endif


/* ***********************************************************************************************
                                         XORP XRL APIs
   *********************************************************************************************** */

int sendmsg_rt_funtion (int sockIdx, const struct msghdr *msg, int flag);
int xorp_xrl_recv_udp_packet_from (int sockIdx, void *buf, size_t len, struct sockaddr_in *src_addr, socklen_t *addrlen);

int xorp_xrl_udp_open_bind      (int sockIdx, const struct sockaddr * addr,int len);
int xorp_xrl_udp_enable_recv    (int sockIdx);
int xorp_xrl_send_udp_packet_to(std::string sock, std::string target_addr, uint32_t target_port, std::vector<uint8_t> payload);
int xorp_xrl_udp_open  (int domain, int type, int protocol);
int xorp_xrl_close          (int sockIdx);
int xorp_xrl_set_socket_option_to(int sockIdx, int level, const char* str, const void * devName, int length);
int xorp_xrl_set_socket_option(int sockIdx, int level, int optionName, const void * optValue, int length);

int xorp_xrl_udp_recvmsg   (int sockIdx,  struct msghdr *msg, int flag);
int recvFrom_udp_packet_function (int sockIdx, void *buf, size_t length, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

/* ***********************************************************************************************
                                         XORP XRL Kernal Routing table
   *********************************************************************************************** */
int xorp_xrl_rt_add_route         (char *dest_network,
                                   char *nexthop_addr,
                                   char *if_name,
                                   unsigned metric,
                                   unsigned priority);
int xorp_xrl_rt_delete_route       (char *dest_network,
                                   char *if_name);

int xrl_policy_route(char *msg);

/* ***********************************************************************************************
                                         Utilities
   *********************************************************************************************** */

#ifdef __cplusplus
}
#endif


#endif // _LD_WRAPPER_H

