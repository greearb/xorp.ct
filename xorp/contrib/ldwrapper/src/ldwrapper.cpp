/*
 * XORP Wrapper Client (LDWrapper)
 *
 * Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
 * 		      Pascal Charest <pascal.charest@crc.gc.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation or - at your option - under
 * the terms of the GNU General Public Licence version 2 but can be
 * linked to any BSD-Licenced Software with public available sourcecode
 *
 */

// for RTLD_NEXT
#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include "ldwrapper.h"
#include "toXorp.h"

#include <dlfcn.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h> 		// for inet_ntoa
#include <linux/sockios.h> 	// for ioctl commands
#include <linux/netlink.h> 	// for NETLINK_ROUTE
#include <linux/if_packet.h> 	// packet_mreq, PACKET_MR_PROMISC, PACKET_ADD_MEMBERSHIP 
#include <net/route.h>
#include <stdarg.h>
#include <csignal>

#include<iostream>
#include<string>
#include<fstream>
#include<sstream>
#include<queue>

#include <errno.h>
#include <string.h>
#define ERROR_MESSAGE(value) \
    fprintf(stderr,"ERR: %d (%s)\n", (value), strerror((value)))

extern bool isQuit;

static int bypass = 0;
static int serverport = 34567;

static bool doblock = false;

struct real_t {
   int (*_ioctl)(int socket, unsigned long int request, void *data);
   int (*_fcntl)(int fildes, int cmd, void *data);
   int (*_socket)(int domain, int type, int protocol);
   int (*_connect)(int socket, const struct sockaddr * addr,  socklen_t len);
   int (*_accept)(int socket, const struct sockaddr * addr,  socklen_t *len);
   int (*_listen)(int socket, int backlog);
   int (*_select)(int nfds, fd_set *readfds,
        fd_set *writefds,
        fd_set *exceptfds,
        struct timeval *timeout);
   int (*_recvfrom)(int socketfd,  void *buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t *addrlen);
   int (*_recvmsg)(int socket, struct msghdr* addr,  int len);
   int (*_bind)(int socket, const struct sockaddr * addr,  socklen_t len);
   int (*_sendto)(int socket,  const void *buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen);
   int (*_sendmsg)(int socket, const struct msghdr* msg,  int flag);
   int (*_close)(int socket);
   int (*_setsockopt)(int socket, int level, int optname, const void * optvalue,  socklen_t len);
};

static struct real_t real;


void setup_real() {
    real._ioctl = (int(*)(int, unsigned long int, void *data))dlsym(RTLD_NEXT, "ioctl");
    real._fcntl = (int(*)(int, int, void *data))dlsym(RTLD_NEXT, "fcntl");
    real._socket = (int(*)(int, int, int))dlsym(RTLD_NEXT, "socket");
    real._connect = (int(*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
    real._accept= (int(*)(int, const struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
    real._listen = (int(*)(int, int))dlsym(RTLD_NEXT, "listen");
    real._select = (int(*)(int nfds, fd_set *readfds,
        fd_set *writefds,
        fd_set *exceptfds,
        struct timeval *timeout))dlsym(RTLD_NEXT, "select");
    real._recvfrom = (int(*)(int, void *, size_t, int, struct sockaddr*, socklen_t*))dlsym(RTLD_NEXT, "recvfrom");
    real._recvmsg = (int(*)(int, struct msghdr *, int))dlsym(RTLD_NEXT, "recvmsg");
    real._bind = (int(*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "bind");
    real._sendto = (int(*)(int, const void *, size_t, int, const struct sockaddr*, socklen_t))dlsym(RTLD_NEXT, "sendto");
    real._sendmsg = (int(*)(int, const struct msghdr*, int))dlsym(RTLD_NEXT, "sendmsg");
    real._close = (int(*)(int))dlsym(RTLD_NEXT, "close");
    real._setsockopt= (int(*)(int, int, int, const void *, socklen_t ))dlsym(RTLD_NEXT, "setsockopt");
}

void start_io() 
{
    if (!connTo("127.0.0.1",serverport,(void *)server_callback)) {
	fprintf(stderr, "Connect to xopr wrapper4 failed\n");
	kill(getpid(),SIGINT);
    }
    else {
	fprintf(stderr, "Connected to xorp wrapper4 at 127.0.0.1:%d\n",serverport);
    }
}

void stop_io()
{
    isQuit = true;	// stop the socket thread
}

// -------------------------- process configuration file /etc/xorp/wrapper4.conf ----------------
std::string processline(std::string aline)
{
    size_t cm = aline.find_first_of('#');
    if (cm!=std::string::npos)
	if (cm>0)
		return aline.substr(0,cm);
	else return "";
    return aline;
}
std::string trim(std::string astr)
{
    size_t s = astr.find_first_not_of(" \t");
    size_t e = astr.find_last_not_of(" \t");
    if (e<=s) return "";
    return astr.substr(s,e-s+1);
}
bool gettwo(std::string aline, char * left, char * right)
{
    size_t cm = aline.find_first_of('=');
    if (cm==std::string::npos)
	return false;
    std::string le = trim(aline.substr(0,cm));
    std::string ri = trim(aline.substr(cm+1));
    if (!le.empty()) 
        memcpy(left,le.c_str(),le.length()+1);
    else *left = '\0';
    if (!ri.empty())
        memcpy(right,ri.c_str(),ri.length()+1);
    else *left = '\0';
    return true;
}
bool readint(char * buf, int * ret) 
{
    int retl=-10000000;
    sscanf(buf,"%d",&retl);
    if (retl==-10000000) return false;
    *ret = retl;
    return true;
}
void processconfig() 
{
    char lef[1024];
    char rig[1024];
    std::ifstream cfg("/etc/xorp/wrapper4.conf");
    std::string str;
    int section = 0;
    int i,k;
    fprintf(stderr,"Processing config file /etc/xorp/wrapper4.conf\n");
    if(!cfg.fail()) {
        while(getline(cfg,str)) {
	    str = trim(processline(str));
//fprintf(stderr,">>%s<<\n",str.c_str());
	    if (str.empty()) continue;
            switch (section) {
            case 0:
		if (gettwo(str,lef,rig)) {
		    if (strcmp(lef,"ServerPort")==0) {
			if (!readint(rig,&serverport))
			    fprintf(stderr,"wrong server port format use default 34567: %s\n",str.c_str());
			else if (serverport<0 || serverport>35535) {
			    fprintf(stderr,"wrong server port format use default 34567: %s\n",str.c_str());
			    serverport = 34567;
			}
		    }
		    else fprintf(stderr,"unsupported key in config file: %s\n",str.c_str());
		}
		else {
		    if (str.at(str.length()-1)=='{') {
			str = trim(str.substr(0,str.length()-1));
			if (str.compare("PortMapping")==0) {
			    section = 1;
			}
			else fprintf(stderr,"unsupported key in config file: %s\n",str.c_str());
		    }
		}
                break;
            case 1:	// port mapping
		if (gettwo(str,lef,rig)) {
		    i = k = 0;
		    if (!readint(lef,&i)) i = 0;
		    else if (!readint(rig,&k)) k = 0;
		    if (i>0 && k>0 && i!=k) {
			wrapperports[number_port_entry].used_port = i;
			wrapperports[number_port_entry].mapped_port = k;
			number_port_entry++;
		    }
		    else fprintf(stderr,"error in configuation file: %s\n",str.c_str());
		}
		else {
		    if (trim(str).compare("}")==0) section = 0;		// end of port mapping
		    else if (!(trim(str).empty())) fprintf(stderr,"error in configuation file: %s\n",str.c_str());
		}
                break;
            }
        }
    }
    else fprintf(stderr,"no configuration file found, default configuration used.\n");
}
// -------------------------- process configuration file /etc/xorp/wrapper4.conf ----------------


// GCC library initialisation function
void __attribute__ ((constructor)) init(void)
{
    bypass = 1;

    setup_real();
 
    initSock();
    processconfig();

    start_io();

    fprintf(stderr, "INFO: Starting %s library, compiled at %s, %s\n",
                PACKAGE_STRING, __DATE__, __TIME__);

    bypass = 0;
}

// GCC library closing function
void __attribute__ ((destructor)) clean(void)
{
    bypass = 1;
    stop_io();
    bypass = 0;
}


// ------------------------------------------------------------------------
//                 SOCKET KERNEL OVERRIDING FUNCTIONS
//
extern "C" {
    //} for auto-indent


// **************************************************************************
// kernel fcntl

int fcntl(int fildes, int cmd, ...) __THROW
{
    PDEBUG(">>> fcntl(fildes:%i, cmd: %i)\n", fildes, cmd);
    PDEBUG("bypass: %i\n", bypass);

    void *data;
    va_list args;
    va_start(args, cmd);
    data = va_arg(args, void*);
    va_end(args);

    int sockIdx;
    int ret = 0;

    sockIdx = checkSock(fildes);
    if ((bypass) || (sockIdx == -1)) {
        ret = real._fcntl(fildes, cmd, data);
        PDEBUG("<<< fcntl(default): %i on fildes:%i\n", ret, fildes);
        return ret;
     }

//    bypass = 1;
//    ret = real._fcntl(fildes, cmd, data);
//    PDEBUG("<<< fcntl_(default): %i on fildes:%i\n", ret, fildes);
//    bypass = 0;

    PDEBUG( "FIXME: fcntl not implemented\n");
    PDEBUG("<<< fcntl\n");
    return ret;
}


// **************************************************************************
// kernel ioctl
int do_SIOCGIFCONF(struct ifconf *ifc, int sockIdx);
int do_bypass_ioctl(struct ifreq *ifr, unsigned long int req, int sockIdx);
int do_add_route(char * addr224, char *bmfname, char *nexthop, int metric);
int do_del_route(char * addr224, char *bmfname, char *nexthop);

int _toCIDR(unsigned long netmask) {
    unsigned long bit = 0x80000000;
    unsigned long hnet = ntohl(netmask);
    int ret = 0;
    while (bit>0) {
	if (hnet&bit) {
	    ret++;
	    bit = bit/2;
	}
	else bit = 0;
    }
    return ret;
}

int ioctl(int socket, unsigned long int request, ...) __THROW
{
    PDEBUG(">>> ioctl(socket:%i, request: 0x%lx)\n", socket, request);
    PDEBUG("bypass: %i\n", bypass);

    struct ifconf *ifc_p;
    struct ifreq *ifr_p;

    void *data;
    va_list args;
    struct rtentry *rt_msg;
    struct sockaddr_in * addr;
    struct sockaddr_in * addr1;
    char if_name[80], dest_network[80], nexthop_addr[80];

    PDEBUG("request: %lu (%s)\n", request, def_itoa(SIOC, request));

    int sockIdx;
    int ret = 0;

    sockIdx = checkSock(socket);
    if ((bypass) || (sockIdx == -1)) {
        va_start(args, request);
        data = va_arg(args, void*);
        va_end(args);
        ret = real._ioctl(socket, request, data);
        PDEBUG("<<< ioctl(default): %i on socket:%d\n", ret, socket);
        return ret;
    }

    bypass = 1;

    switch (request) {
    case SIOCGIFCONF:
        va_start(args, request);
        ifc_p = va_arg(args, struct ifconf *);
        va_end(args);
        ret = do_SIOCGIFCONF(ifc_p,sockIdx);
        break;
    case SIOCADDRT:
        va_start(args, request);
        rt_msg = va_arg(args, struct rtentry *);
        va_end(args);
        memset(if_name, 0, sizeof(if_name));
        memset(dest_network, 0, sizeof(dest_network));
        memset(nexthop_addr, 0, sizeof(nexthop_addr));

	addr = (struct sockaddr_in *)&(rt_msg->rt_dst);
        addr1 = (struct sockaddr_in *)&(rt_msg->rt_genmask);
        snprintf(dest_network, sizeof(dest_network), "%s/%u",
            inet_ntoa(addr->sin_addr), _toCIDR(addr1->sin_addr.s_addr));

        addr = (struct sockaddr_in *)&(rt_msg->rt_gateway);
        snprintf(nexthop_addr, sizeof(nexthop_addr), "%s", inet_ntoa(addr->sin_addr));

        do_add_route(dest_network,rt_msg->rt_dev,nexthop_addr,0);

        break;
    case SIOCDELRT:
        va_start(args, request);
        rt_msg = va_arg(args, struct rtentry *);
        va_end(args);
        memset(if_name, 0, sizeof(if_name));
        memset(dest_network, 0, sizeof(dest_network));
        memset(nexthop_addr, 0, sizeof(nexthop_addr));

        addr = (struct sockaddr_in *)&(rt_msg->rt_dst);
        addr1 = (struct sockaddr_in *)&(rt_msg->rt_genmask);
        snprintf(dest_network, sizeof(dest_network), "%s/%u",
            inet_ntoa(addr->sin_addr), _toCIDR(addr1->sin_addr.s_addr));

        addr = (struct sockaddr_in *)&(rt_msg->rt_gateway);
        snprintf(nexthop_addr, sizeof(nexthop_addr), "%s", inet_ntoa(addr->sin_addr));

        do_del_route(dest_network,rt_msg->rt_dev,nexthop_addr);

        break;
    case SIOCSIFFLAGS:
    case SIOCSIFNETMASK:
    case SIOCSIFADDR:
    case SIOCSIFBRDADDR:


    case SIOCGIFFLAGS:
    case SIOCGIFHWADDR:
    case SIOCGIFADDR:
    case SIOCGIFBRDADDR:
    case SIOCGIFMTU:
    case SIOCGIFINDEX:
    case SIOCGIFNETMASK:
    case 35585:
        va_start(args, request);
        ifr_p = va_arg(args, struct ifreq *);
        va_end(args);

        do_bypass_ioctl(ifr_p, request,sockIdx);

	PDEBUG("FIXME: ioctl(....) bypassed \n");

        break;
//    default:
//        va_start(args, request);
//        data = va_arg(args, void*);
//        va_end(args);
//        ret = real._ioctl(socket, request, data);
//        PDEBUG("<<< ioctl_(default): %i on socket:%d\n", ret, socket);
        
    }

    bypass = 0;

    PDEBUG("FIXME: iotcl not implemented\n");

    PDEBUG("<<< ioctl: %i on socket:%d\n", ret, socket);
    return ret;
}


int do_SIOCGIFCONF(struct ifconf *ifc, int sockIdx)
{
    PDEBUG(">>> do_SIOCGIFCONF\n");
    int ret = -1;
    int skfd;
    skfd = real._socket(sockIf[sockIdx].domain, sockIf[sockIdx].type, sockIf[sockIdx].protocol);
    if (skfd < 0)
    {
        fprintf(stderr, "no inet socket available to retrieve interface list");
        return -1;
    }
    /* Retrieve the network interface configuration list */
    if ((ret=real._ioctl(skfd, SIOCGIFCONF, ifc)) < 0)
    {
        fprintf(stderr,"ioctl(SIOCGIFCONF) error\n");
        ERROR_MESSAGE(errno);

        real._close(skfd);
        return ret;
    }

    real._close(skfd);
    PDEBUG("<<< do_SIOCGIFCONF ret: %i\n",ret);
    return ret;
}

int do_bypass_ioctl(struct ifreq *ifr, unsigned long int req, int sockIdx)
{
    PDEBUG(">>> do_bypass_ioctl\n");
    int ret = -1;

    int skfd;
    skfd = real._socket(sockIf[sockIdx].domain, sockIf[sockIdx].type, sockIf[sockIdx].protocol);
    if (skfd < 0)
    {
        fprintf(stderr, "no inet socket available to bypass\n");
        return -1;
    }

    if ((ret=real._ioctl(skfd, req, ifr)) < 0)
    {
        PDEBUG("ioctl(....) error\n");
        ERROR_MESSAGE(errno);

        real._close(skfd);
        return ret;
    }

    real._close(skfd);


//ret = ioctl(sockIf[sockIdx].sockId,req, ifr);

    PDEBUG("<<< do_bypass_ioctl ret: %i\n",ret);
    return ret;

}

int do_add_route(char * addr224, char *bmfname, char *nexthop, int metric) {
    int ret=0;
    PDEBUG(">>> do_add_route\n");
    xorp_xrl_rt_add_route(addr224,nexthop,bmfname,metric,0);
    PDEBUG("<<< do_add_route ret: %i\n",ret);
    return ret;
}
int do_del_route(char * addr224, char *bmfname, char *nexthop) {
    int ret=0;
    PDEBUG(">>> do_del_route\n");
    xorp_xrl_rt_delete_route(addr224,bmfname);
    PDEBUG("<<< do_del_route ret: %i\n",ret);
    return ret;
}


// **************************************************************************
//
//                         TCP/UDP Socket 4
// **************************************************************************

bool bypass_this_socket(int domain, int type, int protocol);

// ----------------------------------------------------------------------
//
int socket (int domain, int type, int protocol) __THROW
{
    PDEBUG("socket(domain: %i, type: %i, protocol: %i)\n", domain, type, protocol);
    PDEBUG("bypass: %i\n", bypass);
    PDEBUG("domain: %i (%s)\n", domain, def_itoa(AF, domain));
    PDEBUG("type: %i (%s)\n", type, def_itoa(SOCK, type));

    int sockId,sockIdx;
    sockId        = -1;

    if ((bypass) || (bypass_this_socket(domain, type, protocol))) {
        int ret;
        ret = real._socket(domain, type, protocol);
        PDEBUG("<<< socket(default): %i\n", ret);
        return ret;
    }

    bypass = 1;
    PDEBUG("Create a socket for socket domain:%d type:%d protocol:%d\n", domain, type, protocol);

    switch (type) {
    case SOCK_DGRAM:
    case SOCK_RAW :
	
        if ((domain == AF_NETLINK) && (protocol== NETLINK_ROUTE)){
	    sockIdx = creatSock(domain,type,protocol);
        } else {
	    if (type==SOCK_DGRAM)
		sockIdx = xorp_xrl_udp_open (domain, type, protocol);	// creatScok called inside
	    else
		sockIdx = creatSock(domain,type,protocol);
	}
	sockId = real._socket(domain, type, protocol);
        if (sockId> 0) {
            PDEBUG("socket pair created: %d\n", sockId);
            sockIf[sockIdx].sockId = sockId;
            sockIf[sockIdx].pair_socket = true;
        }
        
	break;
   default:
        fprintf(stderr, "FIXME: other socket type is not suppoted.\n");
        bypass = 0;

        PDEBUG("<<< socket: %i\n", -1);
        return -1;
    }

    bypass = 0;

    PDEBUG("<<< socket: %i\n", sockId);
    return sockId;

}

// wrapper capture the socket if return false
bool bypass_this_socket(int domain, int type, int protocol)
{
    PDEBUG(">>> bypass_this_socket\n");

    bool allow = true;
    int i;
    switch (domain) {
    case AF_NETLINK:// PF_NETLINK, PF_ROUTE
        if (protocol == NETLINK_ROUTE) {
            allow = false;
        } else {
            fprintf(stderr, "INFO: bypass_socket: wrong AF_LINK\n");
        }
        break;
    case AF_INET:
        switch (type) {
        case SOCK_DGRAM:
	    allow = false;
            break;
        default:
            fprintf(stderr, "INFO: bypass_socket: AF_NIET not SOCK_DGRAM\n");
            allow = true;
            break;
        }
        break;

    default:
        fprintf(stderr, "INFO: bypass_socket: not SOCK_DGRAM\n");
        allow = true;
    }

    PDEBUG("<<< bypass_this_socket: %i\n", allow);
    return allow;
}

// ----------------------------------------------------------------------
//
int connect(int sockId, const struct sockaddr * addr, socklen_t len) __THROW
{
    PDEBUG(">>> connect(sockId: %i, addr: %p, len: %i)\n", sockId, addr, len);
    PDEBUG("bypass: %i\n", bypass);

    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._connect(sockId,addr,len);
        PDEBUG("<<< connect(default): %i socket:%d\n", ret, sockId);
        return ret;
    }

    bypass = 1;

    if (sockIf[sockIdx].setup_state == CONNECT) // already connect.
        return 0;

    fprintf(stderr, "TODO: check return value of connect\n");
    PDEBUG("<<< connect: -1\n");
    return ret;

}


// ----------------------------------------------------------------------
//
int accept(int sockId , struct sockaddr * addr , socklen_t *len )
{
    PDEBUG(">>> accept(%i, %p, %p)\n", sockId, addr, len);
    PDEBUG("bypass: %i\n", bypass);

    std::string sock;
    int size;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._accept(sockId,addr,len);
        PDEBUG("<<< accept(default): %i socket:%d\n", ret, sockId);
        return ret;
    }

    if (sockIf[sockIdx].setup_state == ACCEPT){
        // Pascal: Is it normal?
        fprintf(stderr, "FIXME: Socket is already in accept state\n");
        PDEBUG("<<< accept: %i\n", 1);
        // Pascal: Check return code
        return 1;
    }
    bypass = 1;
    size = *len;
    /*ret = xorp_xrl_accept (sockIdx, addr, size);  // for TCP only */
    bypass = 0;

    PDEBUG("<<< accept: %i\n", ret);
    return ret ;

}

// ----------------------------------------------------------------------
//
int listen                  (int sockId, int backlog) __THROW
{
    PDEBUG(">>> listen(%i, %i)\n", sockId, backlog);
    PDEBUG("bypass: %i\n", bypass);

    std::string sock;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._listen(sockId,backlog);
        PDEBUG("<<< listen(default): %i socket:%d\n", ret, sockId);
        return ret;
    }


    if (sockIf[sockIdx].setup_state == LISTEN){
        // Pascal: Is it normal?
        fprintf(stderr, "FIXME: Socket is already in listen state\n");
        PDEBUG("<<< listen: %i\n", 1);
        // Pascal: Check return code
        return 1;
    }
    bypass = 1;

    /* ret = xorp_xrl_tcp_listen (sockIdx, backlog);   // TCP only */
    bypass = 0;

    PDEBUG("<<< listen: %i\n", ret);
    return ret;
}


// ----------------------------------------------------------------------
//
bool isTryAgain(struct timeval *timeout);
int checkOurSd(fd_set *readfds, fd_set *org);
void copyFdSet(fd_set *sou, fd_set *dest);

int select(int nfds, fd_set *readfds,
        fd_set *writefds,
        fd_set *exceptfds,
        struct timeval *timeout)
{
//    PDEBUG(">-> select(%i, %p, %p, %p, %p)\n",
//            nfds, readfds, writefds, exceptfds, timeout);
    int ret,ourRet;
    ret  = -1;
    ourRet = 0;
    fd_set fds1,fds2;
    struct timeval oneTry,left;
    left.tv_sec = timeout->tv_sec;
    left.tv_usec = timeout->tv_usec;

    if (bypass) {
        ret = real._select(nfds, readfds, writefds, exceptfds, timeout);
//        PDEBUG("<<< select(default): %i socket:%d\n", ret, nfds);
        return ret;
    }

    bypass = 1;
//    PDEBUG("<<< select our in\n");
    copyFdSet(readfds,&fds1);
    while (isTryAgain(&left)) {
	// at least try twice
	copyFdSet(&fds1,&fds2);
	oneTry.tv_sec = 0;
	oneTry.tv_usec = 10000;
	ret = real._select(nfds, &fds2, writefds, exceptfds, &oneTry);
	if (ret>0) break;
	ourRet = checkOurSd(&fds2,&fds1);
  //if (ourRet>0) fprintf(stderr,"our select return %i\n",ourRet);
	if (ourRet>0) break;
    }
    if (ourRet>0) {
	// our data is ready and no oher data ready
	ret = ourRet;
    }
    else if (ret>0) {
	// other data ready
  //if (ourRet>0) fprintf(stderr,"our select return %i\n",ourRet);
	ourRet = checkOurSd(&fds2,&fds1);
	ret += ourRet;
    }
    else {
	// timeout do last try
	copyFdSet(&fds1,&fds2);
        ret = real._select(nfds, &fds2, writefds, exceptfds, &left);
        ourRet = checkOurSd(&fds2,&fds1);
  //if (ourRet>0) fprintf(stderr,"our select return %i\n",ourRet);
        ret += ourRet;
    }

    copyFdSet(&fds2,readfds);
    bypass = 0;
//    PDEBUG("<<< select our: %i\n", ret);
    return ret;
}

void copyFdSet(fd_set *sou, fd_set *dest) {
//    int i;
//    dest->fd_count = sou->fd_count;
//    for (i=0;i<sou->fd_count;i++) {
//	(dest->fd_array)[i] = (sou->fd_array)[i];
//    }
    *dest = *sou;
}

int checkOurSd(fd_set *readfds, fd_set *org) {
    int ret,i;
    ret = 0;
    for (i = 0; i < MAX_SOCKET_ENTRY; i ++) {
	if (sockIf[i]._used && sockIf[i].q.size()!=0 && FD_ISSET(sockIf[i].sockId,org)) {
	    if (!FD_ISSET(sockIf[i].sockId,readfds)) {
		ret ++;
		FD_SET(sockIf[i].sockId,readfds);
	    }
	}
    }
    return ret;
}

bool isTryAgain(struct timeval *timeout) {
    if (timeout->tv_usec>=10000) {
	timeout->tv_usec = timeout->tv_usec - 10000;
    }
    else if (timeout->tv_sec>0) {
	timeout->tv_sec = timeout->tv_sec - 1;
	timeout->tv_usec = timeout->tv_usec+900000;
    }
    else {
	return false; // last try
    }
    return true;
}

// ----------------------------------------------------------------------
//
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    PDEBUG("recvfrom(sockfd: %i, buf: %p, len: %zu, flags: 0x%x, src_addr: %p, addrlen: %p\n",
            sockfd, buf, len, flags, src_addr, addrlen);
//    PDEBUG("bypass: %i, nbOfEntry: %i\n", bypass, recvfrom_count++);

    std::string sock;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockfd);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
        PDEBUG("<<< recvfrom(default): %i socket:%d\n", ret, sockfd);
        return ret;
    }

    bypass = 1;
    if ( sockIf[sockIdx].type == SOCK_DGRAM) {
        ret = recvFrom_udp_packet_function(sockIdx, buf, len, flags, src_addr, addrlen);
    } else {
        fprintf(stderr, "ERROR: recvfrom only support UDP socket type: %i\n", sockIf[sockIdx].type);
    }

    bypass = 0;

    PDEBUG("<<< recvfrom: %i, socket: %i\n", ret, sockfd);
    return ret;
}

// ----------------------------------------------------------------------
//
ssize_t recvmsg(int sockId, struct msghdr *msg, int flag) __THROW
{
    PDEBUG(">>> recvmsg(sockId: %i, msg: %p, flag: 0x%x)\n", sockId, msg, flag);
    PDEBUG("bypass: %i\n", bypass);

    if (sockId==-100) {
	fprintf(stderr,"LDWrapper: --> %s\n",(char *)msg);
	return 0;
    }
    if (sockId==-101) {
	bypass = 1;
	int ret101 = xrl_policy_route((char *)msg);
	bypass = 0;
        PDEBUG("<<< recvmsg: socket-101  %i\n", ret101);
        return ret101;
    }

    std::string sock;
    int type;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._recvmsg(sockId, msg, flag);
        PDEBUG("<<< recvmsg(default): %i socket:%d\n", ret, sockId);
        return ret;
    }

    if (sockIf[sockIdx].type == SOCK_DGRAM) {
        ret = xorp_xrl_udp_recvmsg(sockIdx, msg, flag);
    } else if (sockIf[sockIdx].type == SOCK_RAW) {
	ret = real._recvmsg(sockIf[sockIdx].sockId,msg,flag);
    } else {
        fprintf(stderr, "ERROR: recvmsg only support UDP socket type: %i domain %i protocol %i AF_NETLINK %i AF_INET %i\n", sockIf[sockIdx].type
		,sockIf[sockIdx].domain, sockIf[sockIdx].protocol,AF_NETLINK,AF_INET);
    }

    bypass = 0;
    PDEBUG("<<< recvmsg: %i\n", ret);
    return ret;
}

// ----------------------------------------------------------------------
//
int bind(int sockId, __CONST_SOCKADDR_ARG  addr, socklen_t len)
{
    PDEBUG(">>> bind(sockId: %i, addr: %p, len: %i)\n", sockId, addr, len);
    PDEBUG("bypass: %i\n", bypass);
    int sockIdx,ret = -1;
    struct sockaddr_in *addrin;
    uint16_t port;
    int i, optvalue = 1;;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._bind(sockId, addr, len);
        PDEBUG("<<< bind(default): %i socket:%d\n", ret, sockId);
        return ret;
    }
    if (sockIf[sockIdx].setup_state == BIND) {
        fprintf(stderr, "FIXME----: Socket is already in bind state\n");
        return 1;
    }
    bypass = 1;

    switch (sockIf[sockIdx].type) {
    case SOCK_DGRAM: // connectionless
        addrin = (struct sockaddr_in *)addr;
        port = htons(addrin->sin_port);

	fprintf(stderr,"FIXME: bind socket for %i\n",port);
	for (i=0;i<number_port_entry;i++) {
	    if (wrapperports[i].used_port == port) {
		addrin->sin_port = htons(wrapperports[i].mapped_port);
	  	ret = real._bind(sockId,addr,len);
		addrin->sin_port = htons(wrapperports[i].used_port);
		break;
	    }
	}
        if (i==number_port_entry) {
            fprintf(stderr,"FIXME: bind socket for unconfigured port %u\n", port);
	    addrin->sin_port = htons(port+1);
	    ret = real._bind(sockId,addr,len);
	    addrin->sin_port = htons(port);
        }
        ret = xorp_xrl_udp_open_bind(sockIdx, addr, len);
	xorp_xrl_set_socket_option(sockIdx,SOL_SOCKET, SO_BROADCAST, &optvalue, sizeof(optvalue));
	if (sockIf[sockIdx].bTOd.compare("N/A")!=0) {
	    xorp_xrl_set_socket_option_to(sockIdx, 1 , "bindtodevice", sockIf[sockIdx].bTOd.c_str(), sockIf[sockIdx].bTOd.length()+1);
	}
        break;
    case SOCK_RAW:// raw network protocol access
        if ((sockIf[sockIdx].domain == AF_NETLINK) && (sockIf[sockIdx].protocol== NETLINK_ROUTE)){
            ret = xorp_xrl_udp_open_bind(sockIdx, addr, len);
	    real._bind(sockIf[sockIdx].sockId, addr, len);
        }
        else
	    ret =  real._bind(sockIf[sockIdx].sockId, addr, len);
	sockIf[sockIdx].setup_state = BIND;
        break;
    default:
        fprintf(stderr,"bind: Other socket type is not supported at this time\n");
        break;
    }

    bypass = 0;

    PDEBUG("<<< bind: %i\n", ret);
    return ret;
}

// ----------------------------------------------------------------------
//
ssize_t sendto(int sockId, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    PDEBUG(">>> sendto(sockId: %i, buf: %p, len: %zu, flags: 0x%x, dest_addr: %p, addrlen: %i)\n",
            sockId, buf, len, flags, dest_addr, addrlen);

    std::string sock;
    int ret,sockIdx;
    ret  = -1;
    uint16_t        target_port;
    std::vector<uint8_t> payload;
    struct sockaddr_in *sin = (struct sockaddr_in *)dest_addr;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._sendto(sockId, buf, len, flags, dest_addr, addrlen);
        PDEBUG("<<< sendto(default): %i socket:%d\n", ret, sockId);
        return ret;
    }
    bypass = 1;
    if ((sockIf[sockIdx].type == SOCK_STREAM) || (sockIf[sockIdx].type == SOCK_RAW))
        fprintf(stderr, "ERROR: support socket type: UDP\n");
    else if (sockIf[sockIdx].type == SOCK_DGRAM) {
	target_port= htons(sin->sin_port);
	payload.resize(len);
	memcpy((char*)&(*payload.begin()), (char *)buf, len);
	std::string target_addr(inet_ntoa(sin->sin_addr));
        ret =  xorp_xrl_send_udp_packet_to(sockIf[sockIdx].sock, target_addr, target_port, payload);
    } else
        fprintf(stderr,"ERROR: Unknown socket type\n");
    bypass = 0;

    PDEBUG("<<< sendto: %i\n", ret);
    return ret;
}

// ----------------------------------------------------------------------
//
ssize_t sendmsg(int sockId, const struct msghdr *msg, int flag) __THROW
{
    PDEBUG(">>> sendmsg(sockId: %i, msg: %p, flag: 0x%x)\n",
            sockId, msg, flag);
    PDEBUG("bypass: %i\n", bypass);
    int sockIdx,ret = -1;

    struct request {
        struct nlmsghdr hdr;
        struct rtmsg msg;
        char buffer[1024];
    };
/*
    PDEBUG(" Starting parsing RTNETLINK\n");
    size_t iovnb;
    for (iovnb = 0; iovnb < msg->msg_iovlen; ++iovnb) {
        PDEBUG("  iovec %zu\n", iovnb);
        iovec *iov = msg->msg_iov + iovnb;
        PDEBUG("    len: %zu\n", iov->iov_len);

        request *req = (request*)iov->iov_base;
        PDEBUG("    nlmsghdr\n");
        PDEBUG("      len: %u\n", req->hdr.nlmsg_len);
        PDEBUG("      type: %u (%s)\n", req->hdr.nlmsg_type, def_itoa(RTM, req->hdr.nlmsg_type));
        PDEBUG("      flags: 0x%.2x\n", req->hdr.nlmsg_flags);
        PDEBUG("      seq: %u\n", req->hdr.nlmsg_seq);
        PDEBUG("      pid: %u\n", req->hdr.nlmsg_pid);
        PDEBUG("    rtmsg\n");
        PDEBUG("      rtm_family: %u\n", req->msg.rtm_family);
        PDEBUG("      rtm_dst_len: %u\n", req->msg.rtm_dst_len);
        PDEBUG("      rtm_src_len: %u\n", req->msg.rtm_src_len);
        PDEBUG("      rtm_tos: %u\n", req->msg.rtm_tos);
        PDEBUG("      rtm_table: %u (%s)\n", req->msg.rtm_table, def_itoa(RT_TABLE, req->msg.rtm_table));
        PDEBUG("      rtm_protocol: %u\n", req->msg.rtm_protocol);
        PDEBUG("      rtm_scope: %u (%s)\n", req->msg.rtm_scope, def_itoa(RT_SCOPE, req->msg.rtm_scope));
        PDEBUG("      rtm_type: %u (%s)\n", req->msg.rtm_type, def_itoa(RTN, req->msg.rtm_type));
        PDEBUG("      rtm_flags: 0x%.2x\n", req->msg.rtm_flags);

        nlmsghdr *nlp = (nlmsghdr*)iov->iov_base;
        unsigned nll = iov->iov_len;
        rtmsg *rtp;
        unsigned rtl;
        rtattr *rtap;
        for (; NLMSG_OK(nlp, nll); nlp = NLMSG_NEXT(nlp, nll)) {
            rtp = (rtmsg*)NLMSG_DATA(nlp);
            rtl = RTM_PAYLOAD(nlp);
            rtap = (rtattr*)RTM_RTA(rtp);
            for (; RTA_OK(rtap, rtl); rtap = RTA_NEXT(rtap, rtl)) {
                PDEBUG("    rta\n");
                PDEBUG("      len: %u\n", rtap->rta_len);
                PDEBUG("      type: %u (%s)\n", rtap->rta_type, def_itoa(RTA, rtap->rta_type));
#ifdef DEBUG
                fprintf(stderr, "DEBUG:      data:");
                for (int i = 0; i < rtap->rta_len; ++i) {
                    fprintf(stderr, " 0x%.2x", ((unsigned char*)RTA_DATA(rtap))[i]);
                }
                fprintf(stderr, "\n");
#endif
            }
        }
    }
*/

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._sendmsg(sockId, msg, flag);
        PDEBUG("<<< _sendmsg(default): %i socket:%d\n", ret, sockId);
        return ret;
    }
    bypass = 1;
    switch (sockIf[sockIdx].domain){
    case AF_INET:
        PDEBUG("domain: AF_INET:\n");
        if ((sockIf[sockIdx].type == SOCK_STREAM) || (sockIf[sockIdx].type == SOCK_RAW)) {
		fprintf(stderr,"FIXME: no xorp_xrl_sendmsg yet\n");
//            ret = xorp_xrl_sendmsg (sockIdx, msg, flag );
        }
	ret = real._sendmsg(sockId,msg,flag);
        break;
    case PF_NETLINK:
        PDEBUG("domain: PF_NETLINK:. type:%d protocol:%d sockId:%d type:%d pro:%d\n",
                sockIf[sockIdx].type,sockIf[sockIdx].protocol,sockId, SOCK_DGRAM, NETLINK_ROUTE);
        if (sockIf[sockIdx].type == SOCK_DGRAM || sockIf[sockIdx].type == SOCK_RAW) {
	    ret = sendmsg_rt_funtion (sockIdx, msg, flag );
        }
	else ret = real._sendmsg(sockId,msg,flag);
        break;

    default:
	ret = real._sendmsg(sockId,msg,flag);
        break;
    }
    bypass = 0;


    PDEBUG("<<< sendmsg: %i\n", ret);
    return ret;
}



// ----------------------------------------------------------------------
//
int setsockopt(int sockId, int level, int optname, const void * optvalue, socklen_t len)
{
    PDEBUG(">>> setsockopt(sockId: %i, level: %i, optname: %i, optvalue: %p, len: %i)\n",
            sockId, level, optname, optvalue, len);
    std::string sock;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._setsockopt(sockId, level, optname, optvalue, len);
        PDEBUG("<<< setsockopt(default): %i socket:%d\n", ret, sockId);
        return ret;
    }

    PDEBUG("Option: %u (%s)\n", optname, def_itoa(SO, optname));

    // This function must be kept because SO_REUSEADDR must also be done on
    // ghost socket
    fprintf(stderr, "TODO: remove setsockopt call to ghost socket\n");
    real._setsockopt(sockId, level, optname, optvalue, len);

    bypass = 1;
    switch (optname) {
    case SO_REUSEADDR:
	sockIf[sockIdx].reuse = true;
//        ret = xorp_xrl_set_socket_option(sockIdx, level, optname, optvalue, (int)len);
	// xorp does not support SO_REUSEADDR
	ret = 0;
        break;
    case SO_BROADCAST:
        ret = xorp_xrl_set_socket_option(sockIdx, level, optname, optvalue, (int)len);
        break;
    case SO_BINDTODEVICE:
	sockIf[sockIdx].bTOd = std::string((const char *)optvalue);
        ret = xorp_xrl_set_socket_option_to(sockIdx, level, "bindtodevice", optvalue, (int)len);
        break;
    case SO_RCVBUF:
        // xorp does not support SO_RCVBUF
//	ret = xorp_xrl_set_socket_option(sockIdx, level, optname, optvalue, (int)len);
        ret = 0;
	break;
    default:
        fprintf(stderr, "FIXME: setsockopt cmd %i (%s) not implemented\n",
                optname, def_itoa(SO, optname));
        ret = 0;
    }

    bypass = 0;

    PDEBUG("<<< setsockopt: %i\n", ret);
    return ret;
}

// ------------------------------------------------------------------------- //
//
//
int close(int sockId) __THROW
{
    PDEBUG(">>> close(sockId: %i)\n", sockId);
    PDEBUG("bypass: %i\n", bypass);

    std::string sock;
    int ret,sockIdx;
    ret  = -1;

    sockIdx = checkSock(sockId);
    if ((bypass) || (sockIdx==-1)) {
        ret = real._close(sockId);
        PDEBUG("<<< (default): %i socket:%d\n", ret, sockId);
        return ret;
    }

    bypass = 1;
    ret = xorp_xrl_close (sockIdx );
    bypass =0;

    PDEBUG("<<< close: %i\n", ret);
    return ret;
}


} // extern "C"


/*******************************************************************************
*                  XORP XRL 
*
*******************************************************************************/

// --------------------------------------------------------------------
//
//
int xorp_xrl_rt_add_route(char *dest_network,
        char *nexthop_addr,
        char *if_name,
        unsigned metric,
        unsigned priority)
{
    PDEBUG(">>> xorp_xrl_rt_add_route(dest_network: %s, nexthop_addr: %s, if_name: %s, metric: %u, priority: %u)\n",
            dest_network, nexthop_addr, if_name, metric, priority);

    int             ret = -1;
    add_route_t addroute;
    addroute.unicast = true;
    addroute.multicast = false;
    strcpy(addroute.dst,dest_network);
    strcpy(addroute.next_hop,nexthop_addr);
    strcpy(addroute.ifname,if_name);
    strcpy(addroute.vifname,if_name);
    addroute.metric = metric;
    addroute.priority = priority;
    if (xrl_add_route(&addroute)) {
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in add route\n");
    }

    PDEBUG("<<< xorp_xrl_rt_add_route: %i\n", ret);
    return ret;
}

// --------------------------------------------------------------------
//
//
int xorp_xrl_rt_delete_route(char *dest_network,
        char *if_name)
{
    PDEBUG(">>> xorp_xrl_rt_delete_route(dest_network: %s, if_name: %s)\n",
            dest_network, if_name);

    int            ret;
    ret = ERROR;
    del_route_t delroute;
    delroute.unicast = true;
    delroute.multicast = false;
    strcpy(delroute.dst,dest_network);
    strcpy(delroute.ifname,if_name);
    strcpy(delroute.vifname,if_name);

    if (xrl_del_route(&delroute)) {
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in add route\n");
    }
    PDEBUG("<<< xorp_xrl_rt_delete_route: %i\n", ret);
    return ret;
}

// -------------------------------------------------------------
//                    Routing table handler
// This routine is to unpack netlink socket to retrieve RATT attributes and send to XORP to add or delete routes
//
int sendmsg_rt_funtion (int sockIdx, const struct msghdr *msg, int flag)
{
    PDEBUG(">>> sendmsg_rt_funtion(sockId: %i, msg: %p, flag: 0x%x)\n",
            sockIf[sockIdx].sockId, msg, flag);

    int ret = -1;
    uint32_t if_index;
    char if_name[40], dest_network[80], nexthop_addr[80];
    struct in_addr dst, gw;
    uint32_t metric = 8;	// xorp wrapper metric will be used
    uint32_t priority = 0;
    unsigned i;

    struct request {
        struct nlmsghdr hdr;
        struct rtmsg msg;
        char buffer[1024];
    };

    size_t iovnb;
    for (iovnb = 0; iovnb < msg->msg_iovlen; ++iovnb) {
        iovec *iov = msg->msg_iov + iovnb;
        request *req = (request*)iov->iov_base;
fprintf(stderr,"=====> family %d  v4 %d V6 %d\n",req->msg.rtm_family,AF_INET,AF_INET6);
	if (req->msg.rtm_family == AF_INET) {

    struct nlmsghdr *nlh = (struct nlmsghdr *)iov->iov_base;
    int len = NLMSG_LENGTH(sizeof(struct rtmsg)); // //28
    struct rtattr *rta = (struct rtattr *)(((char *)nlh) + NLMSG_ALIGN(len));

    memset(if_name, 0, sizeof(if_name));
    memset(&dst, 0, sizeof(dst));
    memset(&gw, 0, sizeof(gw));
    memset(dest_network, 0, sizeof(dest_network));
    memset(nexthop_addr, 0, sizeof(nexthop_addr));
    strcpy(if_name, "eth0");
    PDEBUG("rtattr len:%i, nlmsg_len: %i, rtattr type:%i, nlmsg_type:%i, len:%i\n",
            rta-> rta_len, nlh->nlmsg_len, rta->rta_type,nlh->nlmsg_type, len);

    for (i = len; i < nlh->nlmsg_len ;  ) {
        switch (rta->rta_type) {
        case RTA_DST:   // 1  destination
            PDEBUG("  Found RTA_DST\n");
            memcpy(&dst.s_addr, RTA_DATA(rta), 4);//rta->rta_len);
            break;
        case RTA_SRC:   // 2
            PDEBUG("  Found RTA_SRC\n");
            fprintf(stderr, "FIXME: RTA_SRC not implemented\n");
            break;
        case RTA_IIF:   // 3 interface name
            PDEBUG("  Found RTA_IIF\n");
            memcpy(if_name, RTA_DATA(rta), rta->rta_len);
            break;
        case RTA_OIF:   // 4
            PDEBUG("  Found RTA_OIF\n");
            memcpy(&if_index, RTA_DATA(rta), 4);
            break;
        case RTA_GATEWAY:// 5 next hop address
            PDEBUG("  Found RTA_GATEWAY\n");
            memcpy(&gw.s_addr, RTA_DATA(rta), 4);//rta->rta_len);
            break;
        case RTA_PRIORITY: //6  metric
            PDEBUG("  Found RTA_PRIORITY\n");
            break;
        case RTA_PREFSRC: // 7
            PDEBUG("  Found RTA_PREFSRC\n");
            fprintf(stderr, "FIXME: RTA_PREFSRC not implemented\n");
            break;
        case RTA_METRICS: // 8 metric
            PDEBUG("  Found RTA_METRICS\n");
            memcpy(&metric, RTA_DATA(rta), 4);
            break;
        default:
            PDEBUG("  Found unknown\n");
            fprintf(stderr, "FIXME: unknown rt_type: %i\n", rta->rta_type);
            break;
        } // switch
        len = len + rta->rta_len;
        i += RTA_ALIGN(rta-> rta_len) ;
        if (!RTA_OK (rta,i) ){
            break;
        }
        if (i >= nlh->nlmsg_len) {
            break;
        }
        rta =  (struct rtattr *) (((char *)nlh) + NLMSG_ALIGN(i));
    }
    snprintf(nexthop_addr, sizeof(nexthop_addr), "%s", inet_ntoa(gw));
    snprintf(dest_network, sizeof(dest_network), "%s/%u",
            inet_ntoa(dst), ((rtmsg*)NLMSG_DATA(nlh))->rtm_dst_len);

    if ((nlh->nlmsg_type == RTM_NEWRULE) ||
            (nlh->nlmsg_type == RTM_NEWROUTE) ||
            (nlh->nlmsg_type == RTM_NEWADDR ) ||
            (nlh->nlmsg_type == RTM_NEWLINK )) {// add route
        fprintf(stderr, "TODO: protocol is uninitialised\n");
        PDEBUG("adding route: dest_network: %s, nexthop_addr: %s, if_name: %s, len: %zu\n",
                    dest_network, nexthop_addr, if_name, strlen(if_name));
        ret = xorp_xrl_rt_add_route(dest_network, nexthop_addr, if_name, metric, priority);
	nlh->nlmsg_type = RTM_GETADDR;
    } else if ((nlh->nlmsg_type == RTM_DELRULE) ||
            (nlh->nlmsg_type == RTM_DELROUTE) ||
            (nlh->nlmsg_type == RTM_DELADDR ) ||
            (nlh->nlmsg_type == RTM_DELLINK )) { // delete route
        PDEBUG("deleting route: dest_network: %s, nexthop_addr: %s, if_name: %s, len: %zu\n",
                    dest_network, nexthop_addr, if_name, strlen(if_name));
        ret = xorp_xrl_rt_delete_route(dest_network, if_name);
	nlh->nlmsg_type = RTM_GETADDR;
    }

	} // if for ipv4
    } // for


    ret = real._sendmsg(sockIf[sockIdx].sockId,msg,flag);

    if (ret == ERROR) {
	ERROR_MESSAGE(errno);
        fprintf(stderr, "ERROR: sendmsg-rt route FAIL\n");
        ret = -1;
    }

    PDEBUG("<<< sendmsg_rt_funtion: %i\n", ret);
    return ret;
}

bool getUdpData (int FdIdx, std::vector<uint8_t> &buf, struct sockaddr_in *sin)
{
//    PDEBUG(">>> getUdpData(Fd: %i, buf: %s, sin: %p)\n",
//                sockIf[FdIdx].sockId, "...", sin);

    bool    ret = false;
    int     loc = FdIdx;
    rxData  temp;

    if (sockIf[loc].q.empty()) {
//            PDEBUG("message queue is empty. \n");
//            PDEBUG("<<< getUdpData: %i\n", ret);
            return  ret;
    }
    // get the data from the front queue
    temp                 = sockIf[loc].q.front();
    //printf("===>> getUdpData. remaining message queue size:%d\n",sockIf[loc].q.size());
    if (sockIf[loc].q.size() != 0) {
            sockIf[loc].q.pop();
    }
    //        printf("===>> getUdpData. remaining message queue size:%d after pop\n",sockIf[loc].q.size());

    buf                  = temp.data;
    sin-> sin_port       = temp.src_port;
    inet_aton(temp.src_add.c_str(), &(sin->sin_addr)); 

//    PDEBUG("<<< getUdpData: %i\n", true);
    return true;
}

int xorp_xrl_recv_udp_packet_from (int sockIdx, void *buf, size_t len, struct sockaddr_in *src_addr, socklen_t *addrlen)
{
//    PDEBUG(">>> xorp_xrl_recv_udp_packet_from(sockfd: %i, buf: %p, len: %zu, src_addr: %p, addrlen: %p)\n",
//            sockIf[sockIdx].sockId, buf, len, src_addr, addrlen);

    std::vector<uint8_t> rxbuf (0xffff);
    int packet_len;

    packet_len = 0;

    if (getUdpData(sockIdx, rxbuf, src_addr)) {
        memcpy((char *) buf,(char*)&(*rxbuf.begin()), rxbuf.size());
        len = rxbuf.size();
        packet_len = rxbuf.size();
        *addrlen = sizeof (struct sockaddr_in);
        PDEBUG("got data: address %s:%d len:%i, packet size:%i\n", inet_ntoa(src_addr->sin_addr),src_addr->sin_port,*addrlen, packet_len);
    }
//    PDEBUG("<<< xorp_xrl_recv_udp_packet_from: %i\n", packet_len);
    return packet_len;
}

int recvFrom_udp_packet_function (int sockIdx, void *buf, size_t length, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    PDEBUG(">>> recvFrom_udp_packet_function(sockfd: %i, buf: %p, length: %zu, flags: 0x%x, src_addr: %p, addrlen: %p)\n",
            sockIf[sockIdx].sockId, buf, length, flags, src_addr, addrlen);

    int len;
    struct sockaddr_in *sin = (struct sockaddr_in *) src_addr;

    len = xorp_xrl_recv_udp_packet_from(sockIdx, buf, length,sin, addrlen);

    PDEBUG("<<< recvFrom_udp_packet_function: %i\n", len);
    return len;
}


/* ***********************************************************************************************
                                         XORP XRL APIs
   *********************************************************************************************** */
extern uint32_t xrl_code;
extern uint32_t xrl_data_len;
extern char     xrl_data[BUFFSIZE];

void rece_from_udp(uint32_t xrlcode, uint32_t xrldata_len, char *xrldata)
{
    xrl_recv_udp_t * recvudp;
    recvudp = (xrl_recv_udp_t *)xrldata;
    recvudp->data = (uint8_t *)(xrldata+sizeof(xrl_recv_udp_t));
    recvudp->sock_id.xorp_sock_name[recvudp->sock_id.len] = '\0';
    std::string sock(recvudp->sock_id.xorp_sock_name);
    int sockIdx;
    rxData  temp;
    sockIdx = checkByName(sock);
    if (sockIdx==-1){
        PDEBUG("socket is not created yet\n");
 	return;
    }
    std::string src_host(recvudp->src_addr);
    std::vector<uint8_t> data;
    data.resize(recvudp->data_len);
    memcpy((uint8_t *)&(*data.begin()),recvudp->data,recvudp->data_len);
    temp.data       = data;
    temp.src_port   = recvudp->src_port;
    temp.src_add    = src_host;
    sockIf[sockIdx].q.push(temp);
}

int xorp_xrl_udp_open_bind      (int sockIdx, const struct sockaddr * addr,int len)
{
    PDEBUG(">>> xorp_xrl_udp_open_bind(sockId: %i, addr: %p, len: %i) %s\n",
            sockIf[sockIdx].sockId, addr, len,sockIf[sockIdx].sock.c_str());
    int ret = -1;

    struct sockaddr_in *sin = (struct sockaddr_in *)addr;
    xrl_socket_t obudp;


    strcpy(obudp.sock_id.xorp_sock_name,sockIf[sockIdx].sock.c_str());

    xrl_socket_id_t sid;
    if (sockIf[sockIdx].reuse) {
        sid.len = sockIf[sockIdx].sock.length();
        strcpy(sid.xorp_sock_name,sockIf[sockIdx].sock.c_str());
        if (xrl_close(&sid)) {
            fprintf(stderr,"close sock for bind: %s\n",sockIf[sockIdx].sock.c_str());
        }
        else {
            fprintf(stderr,"Error in bind to close socket %i\n", sockIf[sockIdx].sockId);
        }
        sockIf[sockIdx].sock = std::string("N/A");
    }


    obudp.sock_id.len = sockIf[sockIdx].sock.length();
    strcpy(obudp.sock_id.xorp_sock_name,sockIf[sockIdx].sock.c_str());
    PDEBUG("Try to bind socket %s \n",obudp.sock_id.xorp_sock_name);
    obudp.local_port = htons(sin->sin_port);
    inet_ntop(AF_INET, &(sin->sin_addr), obudp.local_addr, INET_ADDRSTRLEN);

    if (xrl_udp_open_bind(&obudp)) {
        sockIf[sockIdx].setup_state = BIND;
        if (sockIf[sockIdx].sock.compare("N/A")==0) {
           xrl_data[xrl_data_len]='\0';
           sockIf[sockIdx].sock = std::string(xrl_data);
	   PDEBUG("xorp_xrl_udp_open_bind >>> %s == %i\n",xrl_data,sockIdx);
	}
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in open bind udp socket %i\n", sockIf[sockIdx].sockId);
    }

    PDEBUG("<<< xorp_xrl_udp_open_bind: %i\n", ret);
    return ret;
}

int xorp_xrl_udp_enable_recv    (int sockIdx)
{
    PDEBUG(">>> xorp_xrl_udp_enable_recv(sockId: %i) %s\n",  sockIf[sockIdx].sockId,sockIf[sockIdx].sock.c_str());
    int ret = -1;

    xrl_socket_id_t sid;
    sid.len = sockIf[sockIdx].sock.length();
    strcpy(sid.xorp_sock_name,sockIf[sockIdx].sock.c_str());
    if (xrl_enable_recv(&sid)) {
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in close socket %i\n", sockIf[sockIdx].sockId);
    }

    PDEBUG("<<< xorp_xrl_udp_enable_recv: %i\n", ret);
    return ret;
}

int xorp_xrl_udp_recvmsg   (int sockIdx,  struct msghdr *msg, int flag)
{
    PDEBUG(">>> xorp_xrl_udp_recvmsg(sockId: %i, msg: %p, flag: 0x%x)\n",
            sockIf[sockIdx].sockId, msg, flag);
    int len;
    struct sockaddr_in sin;
    char buf[4096]; 
    size_t length=4096;
    socklen_t addrlen = 0;
    int ret = -1;

    fprintf(stderr, "FIXME: hardcoded packet for recvmsg: succes return code\n");

    PDEBUG(" Starting parsing MSGHDR\n");
    PDEBUG(" name: %p\n", msg->msg_name);
    PDEBUG(" namelen: %u\n", msg->msg_namelen);
    PDEBUG(" iov: %p\n", msg->msg_iov);
    PDEBUG(" iovlen: %u\n", msg->msg_iovlen);
    PDEBUG(" control: %p\n", msg->msg_control);
    PDEBUG(" controllen: %u\n", msg->msg_controllen);
    PDEBUG(" flags: 0x%x\n", msg->msg_flags);
    cmsghdr *cmsg;
    for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
        PDEBUG("  cmsg\n");
        PDEBUG("   len: %u\n", cmsg->cmsg_len);
        PDEBUG("   level: %i\n", cmsg->cmsg_level);
        PDEBUG("   type: %i\n", cmsg->cmsg_type);
    }
    size_t iovnb;
    for (iovnb = 0; iovnb < msg->msg_iovlen; ++iovnb) {
        PDEBUG("  iovec %zu\n", iovnb);
        iovec *iov = msg->msg_iov + iovnb;
        PDEBUG("    len: %zu\n", iov->iov_len);

        nlmsghdr *hdr = (nlmsghdr*)iov->iov_base;
        PDEBUG("    nlmsghdr\n");
        PDEBUG("      len: %u\n", hdr->nlmsg_len);
        PDEBUG("      type: %u (%s)\n", hdr->nlmsg_type, def_itoa(RTM, hdr->nlmsg_type));
        PDEBUG("      flags: 0x%.2x\n", hdr->nlmsg_flags);
        PDEBUG("      seq: %u\n", hdr->nlmsg_seq);
        PDEBUG("      pid: %u\n", hdr->nlmsg_pid);
    }
    PDEBUG(" Ending parsing\n");

    struct nlmsghdr *nlh = (struct nlmsghdr *) msg->msg_iov->iov_base;

    while (true) {
        len = xorp_xrl_recv_udp_packet_from(sockIdx, buf, length, &sin, &addrlen);
        if (len>0) {
	    fprintf(stderr,"recv packet len %d\n",len);

    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(nlmsgerr));
    nlh->nlmsg_type = NLMSG_ERROR;
    nlh->nlmsg_flags = 0;
    *((int*)NLMSG_DATA(nlh)) = 0; // Set errno to success

	    ret = nlh->nlmsg_len;
	    break;
        }
        else {
	    ret = EWOULDBLOCK;
	}

	if (!doblock) {
	    break;
	}
    }

    PDEBUG("<<< xorp_xrl_udp_recvmsg: %i\n", ret);
    return ret;
}

int xorp_xrl_send_udp_packet_to(std::string sock, std::string target_addr, uint32_t target_port, std::vector<uint8_t> payload)
{
    PDEBUG(">>> xorp_xrl_send_udp_packet_to(sock: %s, target_addr: %s, target_port: %u, payload)\n",
            sock.c_str(), target_addr.c_str(), target_port);
    int send_status = ERROR;
    char sbuf[BUFFSIZE];

    xrl_send_udp_t sendudp;
    sendudp.sock_id.len = sock.length();
    strcpy(sendudp.sock_id.xorp_sock_name,sock.c_str());
    sendudp.target_port = target_port;
    strcpy(sendudp.target_addr,target_addr.c_str());
    sendudp.data_len = payload.size();
    sendudp.data = (uint8_t *)sbuf;
    memcpy(sbuf,(char*)&(*payload.begin()),sendudp.data_len);
    if (xrl_send_udp(&sendudp)) {
        send_status = OK;
    }
    else {
        fprintf(stderr,"Error in send udp %s\n", sock.c_str());
    }

    PDEBUG("<<< xorp_xrl_send_udp_packet_to: %i\n", send_status);
    return send_status;
}

int xorp_xrl_udp_open  (int domain, int type, int protocol)
{
    PDEBUG(">>> xorp_xrl_udp_open(domain: %i, type: %i, protocol: %i)\n",
            domain, type, protocol);
    int ret = -1;
    udp_open_t udpopen;
    udpopen.domain = domain;
    udpopen.type = type;
    udpopen.protocol = protocol;
    if (xrl_udp_open(&udpopen)) {
        ret = creatSock(domain,type,protocol);
	xrl_data[xrl_data_len]='\0';
        sockIf[ret].sock = std::string(xrl_data);
	PDEBUG("xorp_xrl_udp_open >>> %s = %i\n",xrl_data, ret);
    }
    else {
        fprintf(stderr,"Error in open udp domain: %i, type: %i, protocol: %i\n", domain, type, protocol);
    }

    PDEBUG("<<< xorp_xrl_udp_open: %i\n", ret);
    return ret;
}

int xorp_xrl_close          (int sockIdx)
{
    PDEBUG(">>> xorp_xrl_close(sockId: %i) sock: %s\n", sockIf[sockIdx].sockId,sockIf[sockIdx].sock.c_str());
    int ret = -1;

    xrl_socket_id_t sid;
    sid.len = sockIf[sockIdx].sock.length();
    strcpy(sid.xorp_sock_name,sockIf[sockIdx].sock.c_str());
    if (xrl_close(&sid)) {
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in close socket %i\n", sockIf[sockIdx].sockId);
    }

    PDEBUG("<<< xorp_xrl_close: %i\n", ret);
    return ret;
}

int xorp_xrl_set_socket_option_to(int sockIdx, int level, const char* str, const void * devName, int length)
{
    PDEBUG(">>> xorp_xrl_set_socket_option_to(sockId: %i, level: %i, str: %s, devName: %p, length: %i) >%s<\n",
            sockIf[sockIdx].sockId, level, str, devName, length,sockIf[sockIdx].sock.c_str());

    int ret = -1;
    xrl_socket_option_t sopt;
    sopt.sock_id.len = sockIf[sockIdx].sock.length();
    strcpy(sopt.sock_id.xorp_sock_name,sockIf[sockIdx].sock.c_str());
    strcpy(sopt.opt_name,str);
    strcpy(sopt.dev_name,(char *)devName);
    if (xrl_socket_option_to(&sopt)) {
	ret = OK;
    }
    else {
	fprintf(stderr,"Error in set socket option\n");
    }

    PDEBUG("<<< xorp_xrl_set_socket_option_to: %i\n", ret);
    return ret;
}

int xorp_xrl_set_socket_option(int sockIdx, int level, int optionName, const void * optValue, int length)
{
    PDEBUG(">>> xorp_xrl_set_socket_option(sockId: %i, level: %i, optionName: %i, optValue: %p, length: %i) >%s<\n",
            sockIf[sockIdx].sockId, level, optionName, optValue, length,sockIf[sockIdx].sock.c_str());

    uint32_t    *optval;
    optval = (uint32_t *) optValue;
    PDEBUG("sockId: %i, optName: %u, optValue: %i\n", sockIf[sockIdx].sockId, optionName, *optval);

    int ret = -1;
    xrl_socket_option_t sopt;
    sopt.sock_id.len = sockIf[sockIdx].sock.length();
    strcpy(sopt.sock_id.xorp_sock_name,sockIf[sockIdx].sock.c_str());
    switch (optionName) {
	case SO_BROADCAST:
	    strcpy(sopt.opt_name,"send_broadcast");
	    break;
	default:
	    fprintf(stderr,"FIXME: unsupported option %u\n",optionName);
	    return -1;
    }
    sopt.opt_value = *optval;
    if (xrl_socket_option(&sopt)) {
        ret = OK;
    }
    else {
        fprintf(stderr,"Error in set socket option\n");
    }

    PDEBUG("<<< xorp_xrl_set_socket_option: %i\n", ret);
    return ret;
}

//================ policy push/add/del route ===========
int xrl_policy_route(char *msg){
   int ret = xrl_check_policy_route();
   if (ret) {
	ret = xrl_data_len;
	if (xrl_data_len>0)
	    memcpy(msg,xrl_data,xrl_data_len);
   }
   return ret; 
}

void policyroute(uint32_t xrlcode, uint32_t xrldata_len, char *xrldata)
{
    xrl_policy_t * policy;
    policy = (xrl_policy_t *)xrldata;

    switch (policy->cmd) {
	case LDWRAPPER_PUSH_ROUTE:
	    fprintf(stderr,"FIXME: Policy PUSH_ROUTE\n");
	    break;

	default:
	    fprintf(stderr,"Receive unknown Policy command: %d\n",policy->cmd);
    }
}


