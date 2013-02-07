/*
 * XORP Wrapper Client (LDWrapper)
 *
 * Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation or - at your option - under
 * the terms of the GNU General Public Licence version 2 but can be
 * linked to any BSD-Licenced Software with public available sourcecode
 *
 */


#ifndef _TO_XORP_H
#define _TO_XORP_H

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include "wrapper_types.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFSIZE 65535

bool recvData(void * buf, int * len);
bool sendData(void * data, int len);
bool connTo(char * ipAddr, int port, void * callback);
void server_callback(wrapperData_t * wdata);

int xrl_check_policy_route();

int xrl_close(xrl_socket_id_t *sock);
int xrl_enable_recv(xrl_socket_id_t *sock);
int xrl_send_udp(xrl_send_udp_t *sendudp);
int xrl_socket_option_to(xrl_socket_option_t *sockoption);
int xrl_socket_option(xrl_socket_option_t *sockoption);
int xrl_udp_open_bind(xrl_socket_t *xrlsocket);
int xrl_udp_open(udp_open_t *udpopen);
int xrl_del_route(del_route_t *delroute);
int xrl_add_route(add_route_t *addroute);

#endif
