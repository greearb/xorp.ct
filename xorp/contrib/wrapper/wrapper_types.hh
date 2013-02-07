// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// ===========================================================================

#ifndef __WRAPPER_TYPES_HH__
#define __WRAPPER_TYPES_HH__
#include "stdint.h"

#define  POLICY_ADD_ROUTE	100
#define  POLICY_DEL_ROUTE       101

#define  LDWRAPPER_ERROR        2
#define  LDWRAPPER_OK           1
#define  LDWRAPPER_NOOP         0

#define  LDWRAPPER_ADD_ROUTE    10
#define  LDWRAPPER_ADD_ROUTE_OK 11
#define  LDWRAPPER_DEL_ROUTE    12
#define  LDWRAPPER_DEL_ROUTE_OK 13

#define  LDWRAPPER_CLOSE        20
#define  LDWRAPPER_CLOSE_OK     21
#define  LDWRAPPER_UDP_OPEN     22
#define  LDWRAPPER_UDP_OPEN_OK  23
#define  LDWRAPPER_UDP_OPENBIND 24
#define  LDWRAPPER_UDP_OPENBIND_OK      25

#define  LDWRAPPER_UDP_RECV             26
#define  LDWRAPPER_PUSH_ROUTE           27

#define  LDWRAPPER_UDP_ENABLE_RECV      28
#define  LDWRAPPER_UDP_ENABLE_RECV_OK   29
#define  LDWRAPPER_UDP_SEND_MSG         30
#define  LDWRAPPER_UDP_SEND_MSG_OK      31
#define  LDWRAPPER_UDP_SET_OPTION       32
#define  LDWRAPPER_UDP_SET_OPTION_OK    33
#define  LDWRAPPER_UDP_SET_OPTION_TO    34
#define  LDWRAPPER_UDP_SET_OPTION_TO_OK 35

#define  LDWRAPPER_GET_POLICY_ROUTE     36
#define  LDWRAPPER_GET_POLICY_ROUTE_OK  37


typedef struct wrapper_Data {
    uint32_t code;
    uint32_t data_len;
    void * data;
} wrapperData_t;

typedef struct add_route_st {
    bool unicast;
    bool multicast;
    char dst[40];
    char next_hop[20];
    char ifname[128];
    char vifname[128];
    uint32_t metric;
    uint32_t priority;
} add_route_t;

typedef struct del_route_st {
    bool unicast;
    bool multicast;
    char dst[40];
    char ifname[128];
    char vifname[128];
} del_route_t;

typedef struct udp_open_st {
    uint32_t domain;
    uint32_t type;
    uint32_t protocol;
} udp_open_t;

typedef struct xrl_socket_id_st {
    uint32_t len;
    char xorp_sock_name[128];
} xrl_socket_id_t;

typedef struct xrl_socket_st {
    xrl_socket_id_t sock_id;
    char        local_addr[40];
    uint32_t    local_port;
} xrl_socket_t;

typedef struct xrl_socket_option_st {
    xrl_socket_id_t sock_id;
    uint32_t    level;
    char        opt_name[64];
    uint32_t    opt_value;
    char        dev_name[128];
    uint32_t    length;
} xrl_socket_option_t;

typedef struct xrl_send_udp_st {
    xrl_socket_id_t sock_id;
    char        target_addr[40];
    uint32_t    target_port;
    uint32_t    data_len;
    uint8_t     *data;
} xrl_send_udp_t;

typedef struct xrl_recv_udp_st {
    xrl_socket_id_t sock_id;
    char ifname[128];
    char vifname[128];
    char        src_addr[40];
    uint32_t    src_port;
    uint32_t    data_len;
    const unsigned char *data;
} xrl_recv_udp_t;

typedef struct xrl_policy_st {
    uint32_t cmd;
    bool unicast;
    bool multicast;
    char network[40];
    char next_hop[20];
    uint32_t metric;

} xrl_policy_t;

#endif //__WRAPPER_TYPES_HH__
