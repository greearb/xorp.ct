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

#include "toXorp.h"
#include <csignal>

//============== global variables ============
int _sock_id;
struct sockaddr_in _server_addr;
char s_buffer[BUFFSIZE];
char r_buffer[BUFFSIZE];
int  r_buf_left = 0;

bool data_ready;

pthread_t _thread_read;
int _thread_id;
pthread_mutex_t     mutex;
bool isQuit;

uint32_t xrl_code;
uint32_t xrl_data_len;
char 	 xrl_data[BUFFSIZE];

//============== global variables ============


void * readThread(void * prt)
{
    int recvedlen;
    char recvedbuf[BUFFSIZE];
    void (*callback)(wrapperData_t *);
    wrapperData_t *wdata;
    callback = (void (*)(wrapperData_t *))prt;
    while (!isQuit) {
	if (recvData(recvedbuf,&recvedlen)) {
	    // recv ok
	    wdata = (wrapperData_t *)recvedbuf;
	    wdata->data_len = recvedlen - sizeof(wrapperData_t);
	    wdata->data = recvedbuf+sizeof(wrapperData_t);
	    callback(wdata);
	}
	else {
	    // connection lost
	    isQuit = true;
	}
    }
    fprintf(stderr,"Lost connection to wrapper server!\n");
    kill(getpid(),SIGINT);
}


bool connTo(char * ipAddr, int port, void * callback)
{
    /* Create the TCP socket */
    if ((_sock_id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
       return false;
    }
    /* Construct the server sockaddr_in structure */
    memset(&_server_addr, 0, sizeof(_server_addr));    /* Clear struct */
    _server_addr.sin_family = AF_INET;                 /* Internet/IP */
    _server_addr.sin_addr.s_addr = inet_addr(ipAddr);  /* IP address */
    _server_addr.sin_port = htons(port);         /* server port */
    /* Establish connection */
    if (connect(_sock_id,
           (struct sockaddr *) &_server_addr,
            sizeof(_server_addr)) < 0) {
        return false;
    }
    isQuit = false;
    _thread_id = pthread_create( &_thread_read, NULL, readThread, callback);
    return true;
}

bool sendData(void * data, int len) 
{
    int tlen = len+sizeof(uint32_t);
    uint32_t nlen = htonl(len);
    int rc = 1;
    try {
    rc = pthread_mutex_lock(&mutex);
    if (rc == 0) {
        memcpy((void *)s_buffer,(void *)(&nlen),sizeof(uint32_t));
        memcpy((void *)(s_buffer+sizeof(uint32_t)),data,len);
        if (send(_sock_id, s_buffer, tlen, 0) != tlen) {
	    rc = pthread_mutex_unlock(&mutex);
            return false;
	}
	rc = pthread_mutex_unlock(&mutex);
    }
    else {
	fprintf(stderr,"cannot lock pthread mutex %d\n",rc);
    }
    } catch(...) {
	if (rc==0) rc = pthread_mutex_unlock(&mutex);
	return false;
    }
    return true;
}

bool recvData(void * buf, int * len)
{
    uint32_t *nlen = NULL;
    int received = 0; 
    int exp = 4;
    int bytes = 0;
    while (received < exp) {
	if (r_buf_left<=0) {
            if ((bytes = recv(_sock_id, r_buffer+received, BUFFSIZE-1-received, 0)) < 1) {
                return false;
            }
	}
	else {
	    bytes = r_buf_left;
	    r_buf_left = 0;
	}
        received += bytes;
	if (nlen==NULL && received>=sizeof(uint32_t)) {
	    nlen = (uint32_t *)r_buffer;
	    *len = ntohl(*nlen);
	    exp = *len + sizeof(uint32_t);
	}
    }
    memcpy(buf,(void *)(r_buffer+sizeof(uint32_t)),*len);
    r_buf_left = received-exp;
    if (received>exp) {
	for (int i=0;i<r_buf_left;i++) r_buffer[i] = r_buffer[i+exp];
    }
    return true;
}

bool recvwait(uint32_t maxTO) 
{
    uint32_t count = 0;
    struct timeval oneTry;
    while (count<maxTO) {
	oneTry.tv_sec = 0;
	oneTry.tv_usec = 10000;
	select(0,0,0,0,&oneTry);
	count += 10;
	// check flag
	if (data_ready) {
	    return true;
	}
    }
    return false;
}

//==============================================
// xorp_wrapper callback
void rece_from_udp(uint32_t xrlcode, uint32_t xrldata_len, char *xrldata);
void policyroute(uint32_t xrlcode, uint32_t xrldata_len, char *xrldata);
void server_callback(wrapperData_t * wdata) {
    //fprintf(stderr,"server_callback called. code: %d datalen: %d\n",wdata->code,wdata->data_len);
    switch (wdata->code) {
	// following code is not an ACK
	case LDWRAPPER_UDP_RECV:
	    rece_from_udp(wdata->code,wdata->data_len,(char *)wdata->data);
	    break;
	case LDWRAPPER_PUSH_ROUTE:
	    policyroute(wdata->code,wdata->data_len,(char *)wdata->data);
	    break;
	default:
	    xrl_code = wdata->code;
	    xrl_data_len = wdata->data_len;
	    memcpy((void *)xrl_data,(void *)wdata->data,xrl_data_len);
	    data_ready = true;
    }
}

int wait_xrl(char* sbuf, wrapperData_t * wdata, int timeout_ms) 
{
    data_ready = false;
    sendData((void *)sbuf,wdata->data_len+sizeof(wrapperData_t));
    // wait ack back (server_callback will be called)

    if (recvwait(timeout_ms)) {
    	// get back
        //fprintf(stderr,"exp: %d  ACK back for cmd %d  data len: %d\n",xrl_code, wdata->code, wdata->data_len);
	return xrl_code==(wdata->code+1);
    }
      else {
      	fprintf(stderr,"no ack back for cmd %d\n",wdata->code);
 	return 0;
    }
}

// ----------------------------------------------
int xrl_check_policy_route()
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_GET_POLICY_ROUTE;
    wdata.data_len = 0;
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
// add a route
int xrl_add_route(add_route_t *addroute)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_ADD_ROUTE;
    wdata.data_len = sizeof(add_route_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)addroute,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
// delete a route
int xrl_del_route(del_route_t *delroute)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_DEL_ROUTE;
    wdata.data_len = sizeof(del_route_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)delroute,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
//
int xrl_udp_open(udp_open_t *udpopen)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_OPEN;
    wdata.data_len = sizeof(udp_open_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)udpopen,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_udp_open_bind(xrl_socket_t *xrlsocket) 
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_OPENBIND;
    wdata.data_len = sizeof(xrl_socket_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)xrlsocket,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_socket_option(xrl_socket_option_t *sockoption) 
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_SET_OPTION;
    wdata.data_len = sizeof(xrl_socket_option_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)sockoption,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_socket_option_to(xrl_socket_option_t *sockoption)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_SET_OPTION_TO;
    wdata.data_len = sizeof(xrl_socket_option_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)sockoption,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_send_udp(xrl_send_udp_t *sendudp)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_SEND_MSG;
    int sulen = sizeof(xrl_send_udp_t);
    int wdlen = sizeof(wrapperData_t);
    wdata.data_len = sulen + sendudp->data_len;
    memcpy((void *)sbuf,(void *)(&wdata),wdlen);
    memcpy((void *)(sbuf+wdlen),(void *)sendudp,sulen);
    memcpy((void *)(sbuf+wdlen+sulen),(void *)(sendudp->data),sendudp->data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_enable_recv(xrl_socket_id_t *sock)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_UDP_ENABLE_RECV;
    wdata.data_len = sizeof(xrl_socket_id_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)sock,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}

// ----------------------------------------------
int xrl_close(xrl_socket_id_t *sock)
{
    char sbuf[BUFFSIZE];
    wrapperData_t wdata;
    wdata.code = LDWRAPPER_CLOSE;
    wdata.data_len = sizeof(xrl_socket_id_t);
    memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
    memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)sock,wdata.data_len);
    return wait_xrl(sbuf,&wdata,3000);
}











/*
void recved(wrapperData_t * wdata) {
//   fprintf(stderr,"code %d data len %d %s\n",wdata->code, wdata->data_len,wdata->data );
   fprintf(stderr,"code %d data len %d \n",wdata->code, wdata->data_len );
   data_ready = true;
}

int
main(int argc, char *argv[])
{
  wrapperData_t wdata;
  char buf[BUFFSIZE];
  char sbuf[BUFFSIZE];
  add_route_t addroute;

  fprintf(stderr,"start running\n");
  if (!connTo(argv[1],atoi(argv[2]),(void *)(recved))) {
    fprintf(stderr,"Conn error\n");
    exit(1);
  }
  // send a cmd
  fprintf(stderr,"send cmd \n");

  wdata.data = buf;

  wdata.code = 0;
  strcpy(buf,"this is a test\n");
  wdata.data_len = strlen(buf)+1;
  memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
  memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)buf,wdata.data_len);

//  wdata.code = LDWRAPPER_ADD_ROUTE;
//  wdata.data_len = sizeof(add_route_t);
//  strcpy(addroute.dst,"192.168.9.234/32");
//  strcpy(addroute.next_hop,"192.168.122.1");
//  strcpy(addroute.ifname,"virbr0");
//  strcpy(addroute.vifname,"virbr0");
//  addroute.unicast = true;
//  addroute.multicast = false;
//  addroute.metric = 30;
//  addroute.priority = 1;
//  memcpy((void *)sbuf,(void *)(&wdata),sizeof(wrapperData_t));
//  memcpy((void *)(sbuf+sizeof(wrapperData_t)),(void *)(&addroute),wdata.data_len);

  data_ready = false;
  fprintf(stderr,"send %d %d %s\n",wdata.code, wdata.data_len,wdata.data);
  sendData((void *)sbuf,wdata.data_len+sizeof(wrapperData_t));
  // wait echo back
  if (recvwait(1000)) {
    // get back
  }
  else {
    fprintf(stderr,"no echo back in 1s\n");
  }
  // close and exit
  close(_sock_id);
  exit(0);
}

*/
